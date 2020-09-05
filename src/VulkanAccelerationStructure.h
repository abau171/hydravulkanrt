#pragma once

#include <Common.h>

#include <VulkanUtils.h>
#include <VulkanBuffer.h>


class VulkanAccelerationStructure {

public:

    VulkanAccelerationStructure(const VulkanBasicInfo& vkbi);

    const vk::AccelerationStructureKHR& getAccelerationStructure() {
        return _as.get();
    }

    uint64_t getScratchMemorySize() {
        return _scratchMemorySize;
    }

    void allocateTopLevel(uint32_t maxInstances);

    void allocateBottomLevel(uint32_t maxVertices, uint32_t maxTriangles);

    void buildTopLevel(
        vk::CommandBuffer& commandBuffer,
        VulkanBuffer& scratchBuffer,
        uint32_t numInstances,
        VulkanBuffer& instanceBuffer);

    void buildBottomLevel(
        vk::CommandBuffer& commandBuffer,
        VulkanBuffer& scratchBuffer,
        uint32_t numTriangles,
        VulkanBuffer& vertexBuffer,
        VulkanBuffer& indexBuffer,
        bool update = false);

private:

    uint64_t _getRequiredMemorySize(vk::AccelerationStructureMemoryRequirementsTypeKHR type);

    void _allocate(
        const vk::AccelerationStructureCreateGeometryTypeInfoKHR& asCreateGeometryTypeInfo,
        vk::AccelerationStructureTypeKHR asType,
        vk::BuildAccelerationStructureFlagsKHR asFlags);

    void _build(
        vk::CommandBuffer& commandBuffer,
        VulkanBuffer& scratchBuffer,
        vk::AccelerationStructureGeometryKHR& asGeometry,
        vk::AccelerationStructureTypeKHR asType,
        vk::BuildAccelerationStructureFlagsKHR asFlags,
        uint32_t numPrimitives,
        bool update = false);

    const VulkanBasicInfo& _vkbi;

    vk::UniqueHandle<vk::AccelerationStructureKHR, vk::DispatchLoaderDynamic> _as;
    VulkanBuffer _buffer;
    uint64_t _scratchMemorySize;

};
