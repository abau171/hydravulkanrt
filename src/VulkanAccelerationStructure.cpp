#include <Common.h>

#include <VulkanAccelerationStructure.h>


const vk::BuildAccelerationStructureFlagsKHR TOP_LEVEL_BUILD_FLAGS =
    vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;

const vk::BuildAccelerationStructureFlagsKHR BOTTOM_LEVEL_BUILD_FLAGS =
    vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild
    // | vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction // TODO
    | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;


VulkanAccelerationStructure::VulkanAccelerationStructure(const VulkanBasicInfo& vkbi)
    : _vkbi(vkbi),
      _buffer(_vkbi)
{
}

uint64_t VulkanAccelerationStructure::_getRequiredMemorySize(
    vk::AccelerationStructureMemoryRequirementsTypeKHR type)
{
    vk::MemoryRequirements memoryRequirements =
        _vkbi.device.getAccelerationStructureMemoryRequirementsKHR(
            {
                .type = type,
                .buildType = vk::AccelerationStructureBuildTypeKHR::eDevice,
                .accelerationStructure = _as.get(),
            },
            _vkbi.dispatchLoader).memoryRequirements;
    return memoryRequirements.size;
}

void VulkanAccelerationStructure::_allocate(
    const vk::AccelerationStructureCreateGeometryTypeInfoKHR& asCreateGeometryTypeInfo,
    vk::AccelerationStructureTypeKHR asType,
    vk::BuildAccelerationStructureFlagsKHR asFlags)
{
    _as.reset();

    _as = _vkbi.device.createAccelerationStructureKHRUnique(
        {
            .compactedSize = 0,
            .type = asType,
            .flags = asFlags,
            .maxGeometryCount = 1,
            .pGeometryInfos = &asCreateGeometryTypeInfo,
            .deviceAddress = 0,
        },
        nullptr,
        _vkbi.dispatchLoader);

    uint64_t scratchBuildMemorySize =
        _getRequiredMemorySize(vk::AccelerationStructureMemoryRequirementsTypeKHR::eBuildScratch);
    uint64_t scratchUpdateMemorySize =
        _getRequiredMemorySize(vk::AccelerationStructureMemoryRequirementsTypeKHR::eUpdateScratch);

    // We take the max of the two size requirements here to satisfy the requirements, but in
    // practice update scratch size can be much smaller than build scratch size (if eAllowUpdate is
    // not used). This doesn't matter for us because we never try to shrink our scratch buffers.
    _scratchMemorySize = std::max(
        scratchBuildMemorySize,
        scratchUpdateMemorySize);

    // TODO: Don't reallocate if size is sufficient.
    _buffer.allocate(
        _getRequiredMemorySize(vk::AccelerationStructureMemoryRequirementsTypeKHR::eObject),
        false,
        vk::BufferUsageFlagBits::eRayTracingKHR);

    vk::BindAccelerationStructureMemoryInfoKHR bindASMemoryInfo = {
        .accelerationStructure = _as.get(),
        .memory = _buffer.getMemory(),
        .memoryOffset = 0,
        .deviceIndexCount = 0,
        .pDeviceIndices = nullptr,
    };
    _vkbi.device.bindAccelerationStructureMemoryKHR(1, &bindASMemoryInfo, _vkbi.dispatchLoader);
}

void VulkanAccelerationStructure::allocateTopLevel(uint32_t maxInstances) {

    _allocate(
        {
            .geometryType = vk::GeometryTypeKHR::eInstances,
            .maxPrimitiveCount = maxInstances,
            .indexType = vk::IndexType::eNoneKHR,
            .maxVertexCount = 0,
            .vertexFormat = vk::Format::eUndefined,
            .allowsTransforms = true,
        },
        vk::AccelerationStructureTypeKHR::eTopLevel,
        TOP_LEVEL_BUILD_FLAGS);
}

void VulkanAccelerationStructure::allocateBottomLevel(uint32_t maxVertices, uint32_t maxTriangles) {

    _allocate(
        {
            .geometryType = vk::GeometryTypeKHR::eTriangles,
            .maxPrimitiveCount = maxTriangles,
            .indexType = vk::IndexType::eUint32,
            .maxVertexCount = maxVertices,
            .vertexFormat = vk::Format::eR32G32B32Sfloat,
            .allowsTransforms = false,
        },
        vk::AccelerationStructureTypeKHR::eBottomLevel,
        BOTTOM_LEVEL_BUILD_FLAGS);
}

void VulkanAccelerationStructure::_build(
    vk::CommandBuffer& commandBuffer,
    VulkanBuffer& scratchBuffer,
    vk::AccelerationStructureGeometryKHR& asGeometry,
    vk::AccelerationStructureTypeKHR asType,
    vk::BuildAccelerationStructureFlagsKHR asFlags,
    uint32_t numPrimitives,
    bool update)
{
    vk::AccelerationStructureGeometryKHR* asGeometries = &asGeometry;
    vk::AccelerationStructureBuildGeometryInfoKHR asBuildGeometryInfo = {
        .type = asType,
        .flags = asFlags,
        .update = update,
        .srcAccelerationStructure = _as.get(),
        .dstAccelerationStructure = _as.get(),
        .geometryArrayOfPointers = false,
        .geometryCount = 1,
        .ppGeometries = &asGeometries,
        .scratchData =
            _vkbi.device.getBufferAddressKHR( // Plain getBufferAddress fails for some reason...
                vk::BufferDeviceAddressInfo(scratchBuffer.getBuffer()),
                _vkbi.dispatchLoader),
    };
    vk::AccelerationStructureBuildOffsetInfoKHR offset = {
        .primitiveCount = numPrimitives,
        .primitiveOffset = 0,
        .firstVertex = 0,
        .transformOffset = 0,
    };
    vk::AccelerationStructureBuildOffsetInfoKHR* offsets = &offset;
    commandBuffer.buildAccelerationStructureKHR(1, &asBuildGeometryInfo, &offsets, _vkbi.dispatchLoader);
}

void VulkanAccelerationStructure::buildTopLevel(
    vk::CommandBuffer& commandBuffer,
    VulkanBuffer& scratchBuffer,
    uint32_t numInstances,
    VulkanBuffer& instanceBuffer)
{
    vk::AccelerationStructureGeometryKHR asGeometry = {
        .geometryType = vk::GeometryTypeKHR::eInstances,
        .geometry = {
            vk::AccelerationStructureGeometryInstancesDataKHR(
                false,
                _vkbi.device.getBufferAddressKHR(
                    vk::BufferDeviceAddressInfo(instanceBuffer.getBuffer()),
                    _vkbi.dispatchLoader)),
        },
        .flags = vk::GeometryFlagBitsKHR::eOpaque,
    };
    _build(
        commandBuffer,
        scratchBuffer,
        asGeometry,
        vk::AccelerationStructureTypeKHR::eTopLevel,
        TOP_LEVEL_BUILD_FLAGS,
        numInstances);
}

void VulkanAccelerationStructure::buildBottomLevel(
    vk::CommandBuffer& commandBuffer,
    VulkanBuffer& scratchBuffer,
    uint32_t numTriangles,
    VulkanBuffer& vertexBuffer,
    VulkanBuffer& indexBuffer,
    bool update)
{
    vk::AccelerationStructureGeometryKHR asGeometry = {
        .geometryType = vk::GeometryTypeKHR::eTriangles,
        .geometry = {
            vk::AccelerationStructureGeometryTrianglesDataKHR(
                vk::Format::eR32G32B32Sfloat,
                _vkbi.device.getBufferAddressKHR(
                    vk::BufferDeviceAddressInfo(vertexBuffer.getBuffer()),
                    _vkbi.dispatchLoader),
                sizeof(pxr::GfVec3f),
                vk::IndexType::eUint32,
                _vkbi.device.getBufferAddressKHR(
                    vk::BufferDeviceAddressInfo(indexBuffer.getBuffer()),
                    _vkbi.dispatchLoader),
                nullptr)
        },
        .flags = vk::GeometryFlagBitsKHR::eOpaque,
    };
    _build(
        commandBuffer,
        scratchBuffer,
        asGeometry,
        vk::AccelerationStructureTypeKHR::eBottomLevel,
        BOTTOM_LEVEL_BUILD_FLAGS,
        numTriangles,
        update);
}
