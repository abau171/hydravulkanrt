#pragma once

#include <Common.h>

#include <VulkanUtils.h>


class VulkanBuffer {

public:

    VulkanBuffer(const VulkanBasicInfo& vkbi);

    void allocate(uint64_t size, bool host, vk::BufferUsageFlags usage);

    const vk::DeviceMemory& getMemory() {
        return _memory.get();
    }

    const vk::Buffer& getBuffer() {
        return _buffer.get();
    }

    uint64_t size() {
        return _size;
    }

    void* data() {
        return _mappedPtr;
    }

private:

    const VulkanBasicInfo& _vkbi;

    vk::UniqueBuffer _buffer;
    vk::UniqueDeviceMemory _memory;
    uint64_t _size = 0;
    void* _mappedPtr = nullptr;

};
