#include <Common.h>

#include <Blitter.h>

#include <RenderPass.h>


HVRTRenderPass::HVRTRenderPass(
    pxr::HdRenderIndex* index,
    pxr::HdRprimCollection const& collection,
    const VulkanBasicInfo& vkbi,
    std::unordered_set<HVRTMesh*>& meshes)
    : pxr::HdRenderPass(index, collection),
      _vkbi(vkbi),
      _meshes(meshes),
      _outputColorImg(_vkbi),
      _outputDepthImg(_vkbi),
      _albedoImg(_vkbi),
      _worldPositionImg(_vkbi),
      _worldNormalImg(_vkbi),
      _lightBuffer(_vkbi),
      _instanceBuffer(_vkbi),
      _tlas(_vkbi),
      _scratchBuffers{ {_vkbi}, {_vkbi}, {_vkbi} },
      _sbtBuffer(_vkbi)
{
    vulkanInit();
    _blitter.importSemaphores(_renderDoneSemaphoreExternalHandle, _blitDoneSemaphoreExternalHandle);
}

HVRTRenderPass::~HVRTRenderPass() {
    _vkbi.device.waitIdle();
}

void HVRTRenderPass::_Sync() {
    // It's possible for meshes to free memory during their _Sync() call, so we must wait until the
    // last draw command buffer is finished so we don't free resources in use. We can do this here
    // since HdRenderPass's _Sync() happens before and blocks the mesh _Sync() calls.
    _vkbi.device.waitForFences(1, &_renderDoneFence.get(), true, std::numeric_limits<uint64_t>::max());
}

void HVRTRenderPass::_Execute(
    pxr::HdRenderPassStateSharedPtr const& renderPassState,
    pxr::TfTokenVector const& renderTags)
{
    (void) renderTags;

    pxr::GfVec4f viewport = renderPassState->GetViewport();
    if (viewport != _viewport) {
        _viewport = viewport;
        _viewportExtent = vk::Extent2D(_viewport[2] - _viewport[0], _viewport[3] - _viewport[1]);

        // Delete old memory object from GL if one exists.
        _blitter.deleteTexture();

        vulkanCreateFramebuffer();

        // Import memory object into GL.
        _blitter.createTexture(
            _viewportExtent.width,
            _viewportExtent.height,
            _outputColorImg.getExternalHandle(),
            _outputColorImg.getMemorySize(),
            _outputDepthImg.getExternalHandle(),
            _outputDepthImg.getMemorySize());

        _accumulateFrame = 0;
    }

    pxr::GfMatrix4d worldToView = renderPassState->GetWorldToViewMatrix();
    pxr::GfMatrix4d viewToNdc = renderPassState->GetProjectionMatrix();
    if (worldToView != _worldToView || viewToNdc != _viewToNdc) {

        _worldToView = worldToView;
        _viewToNdc = viewToNdc;

        _accumulateFrame = 0;
    }

    vulkanDraw();
    _blitter.blit();
}

void HVRTRenderPass::vulkanInit() {

    _firstRender = true;
    _mustTransitionOutputColor = false;
    _maxTlasInstances = 0;

    // Just create a single command pool here for now.

    _graphicsCommandPool = _vkbi.device.createCommandPoolUnique(vk::CommandPoolCreateInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        _vkbi.graphicsQueueFamilyIndex));

    std::vector<vk::UniqueCommandBuffer> graphicsCommandBuffers =
        _vkbi.device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
            _graphicsCommandPool.get(),
            vk::CommandBufferLevel::ePrimary,
            2));
    _rasterizeCommandBuffer = std::move(graphicsCommandBuffers[0]);
    _raytraceCommandBuffer = std::move(graphicsCommandBuffers[1]);

    _computeCommandPool = _vkbi.device.createCommandPoolUnique(vk::CommandPoolCreateInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        _vkbi.computeQueueFamilyIndex));

    std::vector<vk::UniqueCommandBuffer> computeCommandBuffers =
        _vkbi.device.allocateCommandBuffersUnique(vk::CommandBufferAllocateInfo(
            _computeCommandPool.get(),
            vk::CommandBufferLevel::ePrimary,
            4));
    _blasBuildCommandBuffers[0] = std::move(computeCommandBuffers[0]);
    _blasBuildCommandBuffers[1] = std::move(computeCommandBuffers[1]);
    _blasBuildCommandBuffers[2] = std::move(computeCommandBuffers[2]);
    _tlasBuildCommandBuffer = std::move(computeCommandBuffers[3]);

    // Create render pass for drawing to interop image.

    vk::AttachmentDescription attachments[] = {
        {
            {},
            vk::Format::eR16G16B16A16Sfloat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral
        },
        {
            {},
            vk::Format::eR32G32B32A32Sfloat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral
        },
        {
            {},
            vk::Format::eR16G16B16A16Sfloat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eGeneral
        },
        {
            {},
            vk::Format::eD32Sfloat,
            vk::SampleCountFlagBits::e1,
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eDontCare,
            vk::AttachmentStoreOp::eDontCare,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        },
    };
    vk::AttachmentReference colorAttachmentReferences[] = {
        {0, vk::ImageLayout::eColorAttachmentOptimal},
        {1, vk::ImageLayout::eColorAttachmentOptimal},
        {2, vk::ImageLayout::eColorAttachmentOptimal},
    };
    vk::AttachmentReference depthAttachmentReference =
        {3, vk::ImageLayout::eDepthStencilAttachmentOptimal};
    vk::SubpassDescription subpass(
        {},
        vk::PipelineBindPoint::eGraphics,
        0,
        nullptr,
        3,
        colorAttachmentReferences,
        nullptr,
        &depthAttachmentReference,
        0,
        nullptr);
    vk::SubpassDependency subpassDependency(
        VK_SUBPASS_EXTERNAL,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::AccessFlags(),
        vk::AccessFlagBits::eColorAttachmentWrite,
        vk::DependencyFlags());
    _renderPass = _vkbi.device.createRenderPassUnique(vk::RenderPassCreateInfo(
        {},
        4,
        attachments,
        1,
        &subpass,
        1,
        &subpassDependency));

    // Create synchronization primitives for interop.

    _blitDoneSemaphore = createExternalSemaphore(_vkbi, &_blitDoneSemaphoreExternalHandle);
    _rasterDoneSemaphore = createSemaphore(_vkbi);
    for (int i = 0; i < 3; i++) {
        _blasBuildDoneSemaphores[i] = createSemaphore(_vkbi);
    }
    _tlasBuildDoneSemaphore = createSemaphore(_vkbi);
    _renderDoneSemaphore = createExternalSemaphore(_vkbi, &_renderDoneSemaphoreExternalHandle);
    _renderDoneFence = createFence(_vkbi, true);

    // Create shader modules.

    _vertexShaderModule = loadShaderModule(_vkbi, "main.vert");
    _fragmentShaderModule = loadShaderModule(_vkbi, "main.frag");
    _raygenShaderModule = loadShaderModule(_vkbi, "main.rgen");
    _missShaderModule = loadShaderModule(_vkbi, "main.rmiss");
    _closestHitShaderModule = loadShaderModule(_vkbi, "main.rchit");

    // Create RT descriptor set.

    vk::DescriptorPoolSize descriptorPoolSizes[] = {
        {
            .type = vk::DescriptorType::eAccelerationStructureKHR,
            .descriptorCount = 1,
        },
        {
            .type = vk::DescriptorType::eStorageImage,
            .descriptorCount = 4,
        },
        {
            .type = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
        },
    };
    _descriptorPool = _vkbi.device.createDescriptorPoolUnique({
        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        1,
        2,
        descriptorPoolSizes,
    });

    vk::DescriptorSetLayoutBinding bindings[] = {
        {
            0,
            vk::DescriptorType::eAccelerationStructureKHR,
            1,
            vk::ShaderStageFlagBits::eRaygenKHR,
        },
        {
            1,
            vk::DescriptorType::eStorageImage,
            1,
            vk::ShaderStageFlagBits::eRaygenKHR,
        },
        {
            2,
            vk::DescriptorType::eStorageImage,
            1,
            vk::ShaderStageFlagBits::eRaygenKHR,
        },
        {
            3,
            vk::DescriptorType::eStorageImage,
            1,
            vk::ShaderStageFlagBits::eRaygenKHR,
        },
        {
            4,
            vk::DescriptorType::eStorageImage,
            1,
            vk::ShaderStageFlagBits::eRaygenKHR,
        },
        {
            5,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eRaygenKHR,
        },
    };
    _rtDescriptorSetLayout = _vkbi.device.createDescriptorSetLayoutUnique({ {}, 6, bindings });

    std::vector<vk::UniqueDescriptorSet> descriptorSets = _vkbi.device.allocateDescriptorSetsUnique({
        _descriptorPool.get(),
        1,
        &_rtDescriptorSetLayout.get()
    });
    _rtDescriptorSet = std::move(descriptorSets[0]);

    // Create RT pipeline.

    vk::PipelineShaderStageCreateInfo rtStages[] = {
        { {}, vk::ShaderStageFlagBits::eRaygenKHR, _raygenShaderModule.get(), "main" },
        { {}, vk::ShaderStageFlagBits::eMissKHR, _missShaderModule.get(), "main" },
        { {}, vk::ShaderStageFlagBits::eClosestHitKHR, _closestHitShaderModule.get(), "main" },
    };

    vk::RayTracingShaderGroupCreateInfoKHR rtGroups[] = {
        {
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 0,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        },
        {
            .type = vk::RayTracingShaderGroupTypeKHR::eGeneral,
            .generalShader = 1,
            .closestHitShader = VK_SHADER_UNUSED_KHR,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        },
        {
            .type = vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,
            .generalShader = VK_SHADER_UNUSED_KHR,
            .closestHitShader = 2,
            .anyHitShader = VK_SHADER_UNUSED_KHR,
            .intersectionShader = VK_SHADER_UNUSED_KHR,
        },
    };

    std::vector<vk::DescriptorSetLayout> rtDescriptorSetLayouts = { _rtDescriptorSetLayout.get() };
    std::vector<vk::PushConstantRange> rtPushConstantRanges = {
        {vk::ShaderStageFlagBits::eRaygenKHR, 0, sizeof(RTPushConstants)},
    };
    _rtPipelineLayout = _vkbi.device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo(
        {},
        rtDescriptorSetLayouts,
        rtPushConstantRanges));

    vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderDynamic> rtPipeline =
        _vkbi.device.createRayTracingPipelineKHRUnique(
            {},
            {
                .flags = vk::PipelineCreateFlags(),
                .stageCount = 3,
                .stages = rtStages,
                .groupCount = 3,
                .groups = rtGroups,
                .maxRecursionDepth = 1,
                .libraries = {},
                .pLibraryInterface = nullptr,
                .layout = _rtPipelineLayout.get(),
            },
            nullptr,
            _vkbi.dispatchLoader);
    _rtPipeline = std::move(rtPipeline);

    // Create RT shader binding table.

    vk::PhysicalDeviceProperties2 properties;
    vk::PhysicalDeviceRayTracingPropertiesKHR rtProperties;
    properties.pNext = &rtProperties;
    _vkbi.physicalDevice.getProperties2(&properties);

    std::vector<uint8_t> shaderGroupHandles(3 * rtProperties.shaderGroupHandleSize);
    _vkbi.device.getRayTracingShaderGroupHandlesKHR(
        _rtPipeline.get(),
        0,
        3,
        shaderGroupHandles.size() * sizeof(uint8_t),
        shaderGroupHandles.data(),
        _vkbi.dispatchLoader);

    _sbtBuffer.allocate(
        3 * rtProperties.shaderGroupBaseAlignment,
        true,
        vk::BufferUsageFlagBits::eRayTracingKHR);
    for (uint32_t i = 0; i < 3; i++) {
        std::memcpy(
            reinterpret_cast<uint8_t*>(_sbtBuffer.data()) + i * rtProperties.shaderGroupBaseAlignment,
            shaderGroupHandles.data() + i * rtProperties.shaderGroupHandleSize,
            rtProperties.shaderGroupHandleSize);
    }

    // Allocate light buffer.

    _lightBuffer.allocate(sizeof(LightData), true, vk::BufferUsageFlagBits::eUniformBuffer);
    vk::DescriptorBufferInfo _lightBufferInfo = {
        _lightBuffer.getBuffer(),
        0,
        VK_WHOLE_SIZE,
    };
    vk::WriteDescriptorSet writeDescriptorSets[] = {
        {
            _rtDescriptorSet.get(),
            5,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            nullptr,
            &_lightBufferInfo,
            nullptr,
        },
    };
    _vkbi.device.updateDescriptorSets(1, writeDescriptorSets, 0, nullptr);
}

void HVRTRenderPass::vulkanCreateFramebuffer() {

    // Make sure we don't try to recreate the framebuffer during rendering.

    _vkbi.device.waitForFences(1, &_renderDoneFence.get(), true, std::numeric_limits<uint64_t>::max());

    // Create all images.

    _outputColorImg.allocate(
        vk::Format::eR32G32B32A32Sfloat,
        _viewportExtent,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage,
        vk::ImageAspectFlagBits::eColor,
        true);
    _mustTransitionOutputColor = true;

    _outputDepthImg.allocate(
        vk::Format::eD32Sfloat,
        _viewportExtent,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::ImageAspectFlagBits::eDepth,
        true);

    _albedoImg.allocate(
        vk::Format::eR16G16B16A16Sfloat,
        _viewportExtent,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage,
        vk::ImageAspectFlagBits::eColor);

    _worldPositionImg.allocate(
        vk::Format::eR32G32B32A32Sfloat,
        _viewportExtent,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage,
        vk::ImageAspectFlagBits::eColor);

    _worldNormalImg.allocate(
        vk::Format::eR16G16B16A16Sfloat,
        _viewportExtent,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage,
        vk::ImageAspectFlagBits::eColor);

    // Create the output framebuffer.

    vk::ImageView attachments[] = {
        _albedoImg.getImageView(),
        _worldPositionImg.getImageView(),
        _worldNormalImg.getImageView(),
        _outputDepthImg.getImageView()
    };
    _outputFramebuffer = _vkbi.device.createFramebufferUnique(vk::FramebufferCreateInfo(
        {},
        _renderPass.get(),
        4,
        attachments,
        _viewportExtent.width,
        _viewportExtent.height,
        1));

    // Create pipeline.

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCreateInfos = {
        {{}, vk::ShaderStageFlagBits::eVertex, _vertexShaderModule.get(), "main"},
        {{}, vk::ShaderStageFlagBits::eFragment, _fragmentShaderModule.get(), "main"}
    };

    std::vector<vk::VertexInputBindingDescription> vertexInputBindingDescriptions = {
        { .binding = 0, .stride = sizeof(pxr::GfVec3f), .inputRate = vk::VertexInputRate::eVertex },
        { .binding = 1, .stride = sizeof(pxr::GfVec3f), .inputRate = vk::VertexInputRate::eVertex },
    };
    std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions = {
        { .binding = 0, .location = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = 0 },
        { .binding = 1, .location = 1, .format = vk::Format::eR32G32B32Sfloat, .offset = 0 },
    };
    vk::PipelineVertexInputStateCreateInfo vertexInputState(
        {},
        vertexInputBindingDescriptions,
        vertexInputAttributeDescriptions);

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState(
        {},
        vk::PrimitiveTopology::eTriangleList,
        false);

    vk::Viewport viewport(0.0f, 0.0f, _viewportExtent.width, _viewportExtent.height, 0.0f, 1.0f);
    vk::Rect2D scissor({ 0, 0 }, _viewportExtent);
    vk::PipelineViewportStateCreateInfo viewportState({}, 1, &viewport, 1, &scissor);

    vk::PipelineRasterizationStateCreateInfo rasterizationState(
        {},
        false,
        false,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eClockwise,
        false,
        0.0f,
        0.0f,
        0.0f,
        1.0f);

    vk::PipelineMultisampleStateCreateInfo multisampleState(
        {},
        vk::SampleCountFlagBits::e1,
        false,
        1.0f,
        nullptr,
        false,
        false);

    vk::PipelineDepthStencilStateCreateInfo depthStencilState(
        {},
        true,
        true,
        vk::CompareOp::eLess,
        false,
        false,
        vk::StencilOpState(),
        vk::StencilOpState(),
        0.0f,
        1.0f);

    vk::PipelineColorBlendAttachmentState colorBlendAttachmentStates[] = {
        {
            false,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::ColorComponentFlagBits::eR
            | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB
            | vk::ColorComponentFlagBits::eA
        },
        {
            false,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::ColorComponentFlagBits::eR
            | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB
            | vk::ColorComponentFlagBits::eA
        },
        {
            false,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::BlendFactor::eOne,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::ColorComponentFlagBits::eR
            | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB
            | vk::ColorComponentFlagBits::eA
        },
    };
    vk::PipelineColorBlendStateCreateInfo colorBlendState(
        {},
        false,
        vk::LogicOp::eCopy,
        3,
        colorBlendAttachmentStates,
        { 0.0f, 0.0f, 0.0f, 0.0f});

    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
    std::vector<vk::PushConstantRange> pushConstantRanges = {
        {vk::ShaderStageFlagBits::eVertex, 0, sizeof(HVRTMesh::PushConstants)},
    };
    _pipelineLayout = _vkbi.device.createPipelineLayoutUnique(vk::PipelineLayoutCreateInfo(
        {},
        descriptorSetLayouts,
        pushConstantRanges));

    vk::UniquePipeline pipeline = _vkbi.device.createGraphicsPipelineUnique(
        {},
        vk::GraphicsPipelineCreateInfo(
            {},
            shaderStageCreateInfos,
            &vertexInputState,
            &inputAssemblyState,
            nullptr,
            &viewportState,
            &rasterizationState,
            &multisampleState,
            &depthStencilState,
            &colorBlendState,
            nullptr,
            *_pipelineLayout,
            *_renderPass,
            0,
            nullptr,
            -1));
    _pipeline = std::move(pipeline);

    // Update ray tracing phase's image descriptors.

    vk::DescriptorImageInfo outputColorDescriptorImageInfo = {
        {},
        _outputColorImg.getImageView(),
        vk::ImageLayout::eGeneral,
    };
    vk::DescriptorImageInfo albedoDescriptorImageInfo = {
        {},
        _albedoImg.getImageView(),
        vk::ImageLayout::eGeneral,
    };
    vk::DescriptorImageInfo worldPositionDescriptorImageInfo = {
        {},
        _worldPositionImg.getImageView(),
        vk::ImageLayout::eGeneral,
    };
    vk::DescriptorImageInfo worldNormalDescriptorImageInfo = {
        {},
        _worldNormalImg.getImageView(),
        vk::ImageLayout::eGeneral,
    };
    vk::WriteDescriptorSet writeDescriptorSets[] = {
        {
            _rtDescriptorSet.get(),
            1,
            0,
            1,
            vk::DescriptorType::eStorageImage,
            &outputColorDescriptorImageInfo,
            nullptr,
            nullptr,
        },
        {
            _rtDescriptorSet.get(),
            2,
            0,
            1,
            vk::DescriptorType::eStorageImage,
            &albedoDescriptorImageInfo,
            nullptr,
            nullptr,
        },
        {
            _rtDescriptorSet.get(),
            3,
            0,
            1,
            vk::DescriptorType::eStorageImage,
            &worldPositionDescriptorImageInfo,
            nullptr,
            nullptr,
        },
        {
            _rtDescriptorSet.get(),
            4,
            0,
            1,
            vk::DescriptorType::eStorageImage,
            &worldNormalDescriptorImageInfo,
            nullptr,
            nullptr,
        },
    };
    _vkbi.device.updateDescriptorSets(4, writeDescriptorSets, 0, nullptr);
}

void HVRTRenderPass::vulkanDraw() {

    // Wait on any remaining work.

    _vkbi.device.waitForFences(1, &_renderDoneFence.get(), true, std::numeric_limits<uint64_t>::max());
    _vkbi.device.resetFences(1, &_renderDoneFence.get());

    // Prepare for AS rebuilds: reallocate TLAS, resize scratch buffer, resize instance buffer, etc..

    std::vector<vk::AccelerationStructureInstanceKHR> instances(_meshes.size());
    bool asChanged = false;
    uint64_t minScratchMemorySize = 0;
    size_t i = 0;
    for (HVRTMesh* mesh : _meshes) {

        bool meshInstanceChanged;
        uint64_t meshScratchMemorySize;
        mesh->getASBuildInfo(&meshInstanceChanged, &instances[i], &meshScratchMemorySize);

        asChanged |= meshInstanceChanged;
        minScratchMemorySize = std::max(minScratchMemorySize, meshScratchMemorySize);

        i++;
    }

    if (_maxTlasInstances < instances.size()) {
        _maxTlasInstances = instances.size();

        // Re-allocate the TLAS and update the descriptor.
        _tlas.allocateTopLevel(_maxTlasInstances);
        vk::WriteDescriptorSet writeDescriptorSets[] = {
            {
                _rtDescriptorSet.get(),
                0,
                0,
                1,
                vk::DescriptorType::eAccelerationStructureKHR,
                nullptr,
                nullptr,
                nullptr,
            },
        };
        vk::WriteDescriptorSetAccelerationStructureKHR writeDescriptorSetAS =
            { 1, &_tlas.getAccelerationStructure() };
        writeDescriptorSets[0].setPNext(&writeDescriptorSetAS);
        _vkbi.device.updateDescriptorSets(1, writeDescriptorSets, 0, nullptr);

        // Re-allocate the instance buffer.
        _instanceBuffer.allocate(
            _maxTlasInstances * sizeof(vk::AccelerationStructureInstanceKHR),
            true,
            vk::BufferUsageFlagBits::eRayTracingKHR
            | vk::BufferUsageFlagBits::eShaderDeviceAddress);
    }
    minScratchMemorySize = std::max(
        minScratchMemorySize,
        _tlas.getScratchMemorySize());

    if (_scratchBuffers[0].size() < minScratchMemorySize) {
        for (int i = 0; i < 3; i++) {
            _scratchBuffers[i].allocate(
                minScratchMemorySize,
                false,
                vk::BufferUsageFlagBits::eRayTracingKHR
                | vk::BufferUsageFlagBits::eShaderDeviceAddress);
        }
    }

    // Build RT acceleration structure.

    if (asChanged) {

        _accumulateFrame = 0;

        // Build bottom-level AS.

        for (int i = 0; i < 3; i++) {
            _blasBuildCommandBuffers[i]->begin(
                { vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr });
        }

        {
            int i = 0;
            for (HVRTMesh* mesh : _meshes) {
                int queueI = i % 3;
                mesh->buildAS(_blasBuildCommandBuffers[queueI].get(), _scratchBuffers[queueI]);
                vk::MemoryBarrier barrier = {
                    .srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR,
                    .dstAccessMask = vk::AccessFlagBits::eAccelerationStructureReadKHR,
                };
                _blasBuildCommandBuffers[queueI]->pipelineBarrier(
                    vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                    vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                    vk::DependencyFlags(),
                    1, &barrier,
                    0, nullptr,
                    0, nullptr);
                i++;
            }
        }

        for (int i = 0; i < 3; i++) {

            _blasBuildCommandBuffers[i]->end();

            vk::SubmitInfo submitInfo(
                0, nullptr, nullptr,
                1, &_blasBuildCommandBuffers[i].get(),
                1, &_blasBuildDoneSemaphores[i].get());
            _vkbi.computeQueues[i].submit(1, &submitInfo, vk::Fence());
        }

        // Build top-level AS.

        std::memcpy(
            _instanceBuffer.data(),
            instances.data(),
            instances.size() * sizeof(vk::AccelerationStructureInstanceKHR));

        _tlasBuildCommandBuffer->begin(
            vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr));
        _tlas.buildTopLevel(
            _tlasBuildCommandBuffer.get(),
            _scratchBuffers[0],
            instances.size(),
            _instanceBuffer);
        _tlasBuildCommandBuffer->end();

        {
            vk::PipelineStageFlags waitStages[] = {
                vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
            };
            vk::Semaphore waitSemaphores[] = {
                _blasBuildDoneSemaphores[0].get(),
                _blasBuildDoneSemaphores[1].get(),
                _blasBuildDoneSemaphores[2].get(),
            };
            vk::SubmitInfo submitInfo(
                3, waitSemaphores, waitStages,
                1, &_tlasBuildCommandBuffer.get(),
                1, &_tlasBuildDoneSemaphore.get());
            _vkbi.computeQueues[0].submit(1, &submitInfo, vk::Fence());
        }
    }

    // Rasterization pass.

    {
        _rasterizeCommandBuffer->begin(
            vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr));

        vk::ClearValue clearValues[] = {
            vk::ClearColorValue(std::array<float, 4>{ 0.1f, 0.1f, 0.1f, 1.0f }),
            vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }),
            vk::ClearColorValue(std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f }),
            vk::ClearDepthStencilValue(1.0f, 0.0f)
        };
        _rasterizeCommandBuffer->beginRenderPass(
            vk::RenderPassBeginInfo(
                _renderPass.get(),
                _outputFramebuffer.get(),
                vk::Rect2D({0, 0}, _viewportExtent),
                4,
                clearValues),
            vk::SubpassContents::eInline);

        _rasterizeCommandBuffer->bindPipeline(vk::PipelineBindPoint::eGraphics, _pipeline.get());
        pxr::GfMatrix4f worldToNdc =
            pxr::GfMatrix4f(_worldToView * _viewToNdc)
            * VK_TO_GL_DEPTH_CORRECTION_MATRIX;
        for (HVRTMesh* mesh : _meshes) {
            mesh->draw(_rasterizeCommandBuffer, _pipelineLayout, worldToNdc);
        }

        _rasterizeCommandBuffer->endRenderPass();

        _rasterizeCommandBuffer->end();

        // Submit the draw work.

        vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        vk::SubmitInfo submitInfo(
            _firstRender ? 0 : 1, &_blitDoneSemaphore.get(), &waitStage,
            1, &_rasterizeCommandBuffer.get(),
            1, &_rasterDoneSemaphore.get());
        _vkbi.graphicsQueue.submit(1, &submitInfo, vk::Fence());
    }

    // Ray tracing pass.

    {

        // Update light buffer.
        _lightData.numLights_padding = pxr::GfVec4i(getInt("numLights", 0), 0, 0, 0);
        _lightData.ambientLightIntensity_maxDistance = pxr::GfVec4f(
            0.3f,
            0.4f,
            0.7f,
            getFloat("ao_maxdist", 100.0f));
        for (int i = 0; i < 10; i++) {
            std::string iStr = std::to_string(i);
            _lightData.lights[i] = LightData::Light(
                getVec3("light_v_" + iStr, pxr::GfVec3f(0.0f, 0.0f, 1.0f)).GetNormalized(),
                true, // directional
                getVec3("light_intensity_" + iStr, pxr::GfVec3f(1.0f)),
                (bool) getInt("light_raytraced_" + iStr, 1));
        }
        std::memcpy(
            _lightBuffer.data(),
            &_lightData,
            sizeof(LightData));

        _raytraceCommandBuffer->begin(
            vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit, nullptr));

        if (_mustTransitionOutputColor) {

            // Transition output color image from eUndefined to eGeneral so we can store to it.
            vk::ImageMemoryBarrier barrier = {
                .srcAccessMask = vk::AccessFlags(),
                .dstAccessMask = vk::AccessFlagBits::eShaderWrite,
                .oldLayout = vk::ImageLayout::eUndefined,
                .newLayout = vk::ImageLayout::eGeneral,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .image = _outputColorImg.getImage(),
                .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 },
            };
            _raytraceCommandBuffer->pipelineBarrier(
                vk::PipelineStageFlagBits::eTopOfPipe,
                vk::PipelineStageFlagBits::eRayTracingShaderKHR,
                vk::DependencyFlags(),
                0, nullptr,
                0, nullptr,
                1, &barrier);

            _mustTransitionOutputColor = false;
        }

        _raytraceCommandBuffer->bindPipeline(
            vk::PipelineBindPoint::eRayTracingKHR,
            _rtPipeline.get());
        _raytraceCommandBuffer->bindDescriptorSets(
            vk::PipelineBindPoint::eRayTracingKHR,
            _rtPipelineLayout.get(),
            0,
            { _rtDescriptorSet.get() },
            {});

        if (getInt("converge", 0) == 0) _accumulateFrame = 0;
        RTPushConstants rtPushConstants = {
            pxr::GfMatrix4f((_worldToView * _viewToNdc).GetInverse()),
            pxr::GfVec3f(_worldToView.GetInverse().Transform(pxr::GfVec3d(0.0f))),
            _accumulateFrame,
            getInt("aoRaysPerFrame", 1),
        };
        _accumulateFrame++;
        _raytraceCommandBuffer->pushConstants(
            _rtPipelineLayout.get(),
            vk::ShaderStageFlagBits::eRaygenKHR,
            0,
            sizeof(RTPushConstants),
            reinterpret_cast<void*>(&rtPushConstants));

        vk::PhysicalDeviceProperties2 properties;
        vk::PhysicalDeviceRayTracingPropertiesKHR rtProperties;
        properties.pNext = &rtProperties;
        _vkbi.physicalDevice.getProperties2(&properties);
        vk::DeviceSize programSize = rtProperties.shaderGroupBaseAlignment;

        _raytraceCommandBuffer->traceRaysKHR(
            { _sbtBuffer.getBuffer(), 0 * programSize, programSize, programSize },
            { _sbtBuffer.getBuffer(), 1 * programSize, programSize, programSize },
            { _sbtBuffer.getBuffer(), 2 * programSize, programSize, programSize },
            { _sbtBuffer.getBuffer(), 0, 0, 0 },
            _viewportExtent.width,
            _viewportExtent.height,
            1,
            _vkbi.dispatchLoader);

        _raytraceCommandBuffer->end();

        // Submit the draw work.

        vk::Semaphore waitSemaphores[2];
        vk::PipelineStageFlags waitStages[2];

        waitSemaphores[0] = _rasterDoneSemaphore.get();
        waitStages[0] = vk::PipelineStageFlagBits::eRayTracingShaderKHR;

        waitSemaphores[1] = _tlasBuildDoneSemaphore.get();
        waitStages[1] = vk::PipelineStageFlagBits::eRayTracingShaderKHR;

        int numWaitSemaphores = asChanged ? 2 : 1;

        vk::SubmitInfo submitInfo(
            numWaitSemaphores, waitSemaphores, waitStages,
            1, &_raytraceCommandBuffer.get(),
            1, &_renderDoneSemaphore.get());
        _vkbi.graphicsQueue.submit(1, &submitInfo, _renderDoneFence.get());
    }

    _firstRender = false;
}
