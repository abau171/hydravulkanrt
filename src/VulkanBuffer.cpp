#include <Common.h>

#include <VulkanBuffer.h>


VulkanBuffer::VulkanBuffer(const VulkanBasicInfo& vkbi) : _vkbi(vkbi)
{
}

void VulkanBuffer::allocate(uint64_t size, bool host, vk::BufferUsageFlags usage) {

    _memory.reset();
    _buffer.reset();

    _size = size;

    uint32_t queueFamilyIndices[] = {
        _vkbi.graphicsQueueFamilyIndex,
        _vkbi.transferQueueFamilyIndex,
        _vkbi.computeQueueFamilyIndex
    };
    _buffer = _vkbi.device.createBufferUnique({
        .flags = {},
        .size = _size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eConcurrent,
        .queueFamilyIndexCount = 3,
        .pQueueFamilyIndices = queueFamilyIndices,
    });

    vk::MemoryRequirements memoryRequirements =
        _vkbi.device.getBufferMemoryRequirements(_buffer.get());
    vk::MemoryPropertyFlags memoryPropertyFlags =
        host
        ? (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
        : vk::MemoryPropertyFlagBits::eDeviceLocal;
    vk::MemoryAllocateInfo memoryAllocateInfo(
        memoryRequirements.size,
        vulkanFindMemoryType(
            _vkbi.physicalDevice,
            memoryRequirements.memoryTypeBits,
            memoryPropertyFlags));
    vk::MemoryAllocateFlagsInfo memoryAllocateFlagsInfo = {};
    if (usage & vk::BufferUsageFlagBits::eShaderDeviceAddress) {
        memoryAllocateFlagsInfo.flags = vk::MemoryAllocateFlagBits::eDeviceAddress;
        memoryAllocateInfo.setPNext(&memoryAllocateFlagsInfo);
    }
    _memory = _vkbi.device.allocateMemoryUnique(memoryAllocateInfo, nullptr);

    _vkbi.device.bindBufferMemory(_buffer.get(), _memory.get(), 0);

    if (host) {
        _mappedPtr = _vkbi.device.mapMemory(
            _memory.get(),
            0,
            memoryRequirements.size,
            {});
    }
}
