#pragma once

#include <Common.h>

#include <Blackboard.h>
#include <VulkanUtils.h>
#include <VulkanBuffer.h>
#include <VulkanImage.h>
#include <VulkanAccelerationStructure.h>
#include <Blitter.h>
#include <Mesh.h>


class HVRTRenderPass : public pxr::HdRenderPass {

public:

    HVRTRenderPass(
        pxr::HdRenderIndex* index,
        pxr::HdRprimCollection const& collection,
        const VulkanBasicInfo& vkbi,
        std::unordered_set<HVRTMesh*>& meshes);

    virtual ~HVRTRenderPass();

    virtual bool IsConverged() const override {
        return getInt("converge", 0) == 0;
    }

protected:

    void _Sync() override;

    void _Execute(
        pxr::HdRenderPassStateSharedPtr const& renderPassState,
        pxr::TfTokenVector const& renderTags) override;

private:

    struct RTPushConstants {
        pxr::GfMatrix4f ndcToWorld;
        pxr::GfVec3f cameraOrigin;
        uint32_t accumulateFrame;
        int32_t aoRaysPerFrame;
    };

    struct LightData {

        pxr::GfVec4i numLights_padding;
        pxr::GfVec4f ambientLightIntensity_maxDistance;

        struct Light {

            pxr::GfVec4f v_directional;
            pxr::GfVec4f intensity_raytraced;

            Light() {}

            Light(pxr::GfVec3f v, bool directional, pxr::GfVec3f intensity, bool raytraced)
                : v_directional(v[0], v[1], v[2], directional ? 1.0f : 0.0f),
                  intensity_raytraced(intensity[0], intensity[1], intensity[2], raytraced ? 1.0f : 0.0f)
            {
            }

        } lights[10];
    };

    void vulkanInit();

    void vulkanCreateFramebuffer();

    void vulkanDraw();

    pxr::GfVec4f _viewport;
    vk::Extent2D _viewportExtent;
    pxr::GfMatrix4d _worldToView;
    pxr::GfMatrix4d _viewToNdc;
    uint32_t _accumulateFrame;

    Blitter _blitter;

    const VulkanBasicInfo& _vkbi;

    std::unordered_set<HVRTMesh*>& _meshes;

    bool _firstRender;
    bool _mustTransitionOutputColor;
    vk::UniqueCommandPool _graphicsCommandPool;
    vk::UniqueCommandPool _computeCommandPool;
    vk::UniqueCommandBuffer _blasBuildCommandBuffers[3];
    vk::UniqueCommandBuffer _tlasBuildCommandBuffer;
    vk::UniqueCommandBuffer _rasterizeCommandBuffer;
    vk::UniqueCommandBuffer _raytraceCommandBuffer;
    vk::UniqueRenderPass _renderPass;

    vk::UniqueSemaphore _blitDoneSemaphore;
    int _blitDoneSemaphoreExternalHandle;
    vk::UniqueSemaphore _rasterDoneSemaphore;
    vk::UniqueSemaphore _blasBuildDoneSemaphores[3];
    vk::UniqueSemaphore _tlasBuildDoneSemaphore;
    vk::UniqueSemaphore _renderDoneSemaphore;
    int _renderDoneSemaphoreExternalHandle;
    vk::UniqueFence _renderDoneFence;

    vk::UniqueShaderModule _vertexShaderModule;
    vk::UniqueShaderModule _fragmentShaderModule;

    VulkanImage _outputColorImg;
    VulkanImage _outputDepthImg;
    VulkanImage _albedoImg;
    VulkanImage _worldPositionImg;
    VulkanImage _worldNormalImg;

    vk::UniqueFramebuffer _outputFramebuffer;
    vk::UniquePipelineLayout _pipelineLayout;
    vk::UniquePipeline _pipeline;
    LightData _lightData;
    VulkanBuffer _lightBuffer;

    size_t _maxTlasInstances;
    VulkanBuffer _instanceBuffer;
    VulkanAccelerationStructure _tlas;
    size_t _maxScratchMemorySize;
    VulkanBuffer _scratchBuffers[3];
    vk::UniqueDescriptorPool _descriptorPool;
    vk::UniqueDescriptorSetLayout _rtDescriptorSetLayout;
    vk::UniqueDescriptorSet _rtDescriptorSet;
    vk::UniqueShaderModule _raygenShaderModule;
    vk::UniqueShaderModule _missShaderModule;
    vk::UniqueShaderModule _closestHitShaderModule;
    vk::UniquePipelineLayout _rtPipelineLayout;
    vk::UniqueHandle<vk::Pipeline, vk::DispatchLoaderDynamic> _rtPipeline;
    VulkanBuffer _sbtBuffer;

};
