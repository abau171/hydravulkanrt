#pragma once

#include <Common.h>

#include <VulkanUtils.h>


class VulkanImage {

public:

    VulkanImage(const VulkanBasicInfo& vkbi);

    void allocate(
        vk::Format format,
        vk::Extent2D extent,
        vk::ImageUsageFlags usage,
        vk::ImageAspectFlags aspects,
        bool externalUse = false);

    const vk::DeviceMemory& getMemory() {
        return _memory.get();
    }

    const vk::Image& getImage() {
        return _image.get();
    }

    const vk::ImageView& getImageView() {
        return _imageView.get();
    }

    int getExternalHandle() {
        return _externalHandle;
    }

    uint64_t getMemorySize() {
        return _memorySize;
    }

private:

    const VulkanBasicInfo& _vkbi;

    vk::UniqueDeviceMemory _memory;
    vk::UniqueImage _image;
    vk::UniqueImageView _imageView;

    int _externalHandle;
    uint64_t _memorySize;

};
