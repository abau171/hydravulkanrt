#include <Common.h>

#include <VulkanImage.h>


VulkanImage::VulkanImage(const VulkanBasicInfo& vkbi) : _vkbi(vkbi)
{
}

void VulkanImage::allocate(
    vk::Format format,
    vk::Extent2D extent,
    vk::ImageUsageFlags usage,
    vk::ImageAspectFlags aspects,
    bool externalUse)
{
    _imageView.reset();
    _image.reset();
    _memory.reset();

    uint32_t queueFamilyIndices[] = {
        _vkbi.graphicsQueueFamilyIndex,
        _vkbi.transferQueueFamilyIndex,
        _vkbi.computeQueueFamilyIndex
    };
    vk::ImageCreateInfo imageCreateInfo(
        {},
        vk::ImageType::e2D,
        format,
        vk::Extent3D(extent.width, extent.height, 1),
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        usage,
        vk::SharingMode::eConcurrent,
        3,
        queueFamilyIndices,
        vk::ImageLayout::eUndefined);
    vk::ExternalMemoryImageCreateInfo exportMemoryImageCreateInfo;
    if (externalUse) {
        exportMemoryImageCreateInfo = { vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd };
        imageCreateInfo.setPNext(&exportMemoryImageCreateInfo);
    }
    _image = _vkbi.device.createImageUnique(imageCreateInfo);

    // Allocate and bind memory for the output color image.

    vk::MemoryRequirements memoryRequirements = _vkbi.device.getImageMemoryRequirements(_image.get());
    vk::MemoryAllocateInfo memoryAllocateInfo(
        memoryRequirements.size,
        vulkanFindMemoryType(
            _vkbi.physicalDevice,
            memoryRequirements.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal));
    vk::ExportMemoryAllocateInfo exportMemoryAllocateInfo;
    if (externalUse) {
        exportMemoryAllocateInfo = { vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd };
        memoryAllocateInfo.setPNext(&exportMemoryAllocateInfo);
    }
    _memory = _vkbi.device.allocateMemoryUnique(memoryAllocateInfo, nullptr);
    _memorySize = memoryRequirements.size;
    if (externalUse) {
        _externalHandle = _vkbi.device.getMemoryFdKHR(
            vk::MemoryGetFdInfoKHR(_memory.get(), vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd),
            _vkbi.dispatchLoader);
    } else {
        _externalHandle = -1;
    }

    _vkbi.device.bindImageMemory(_image.get(), _memory.get(), 0);

    _imageView = _vkbi.device.createImageViewUnique(vk::ImageViewCreateInfo(
        {},
        _image.get(),
        vk::ImageViewType::e2D,
        format,
        vk::ComponentMapping(),
        vk::ImageSubresourceRange(aspects, 0, 1, 0, 1)));
}
