#pragma once

#include <Common.h>


const pxr::GfMatrix4f VK_TO_GL_DEPTH_CORRECTION_MATRIX(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.5f, 1.0f);

struct VulkanBasicInfo {

    struct QueueInfo {
        uint32_t queueFamilyIndex;
        vk::Queue queue;
    };

    vk::Instance instance;
    vk::DispatchLoaderDynamic dispatchLoader;
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    uint32_t graphicsQueueFamilyIndex;
    uint32_t transferQueueFamilyIndex;
    uint32_t computeQueueFamilyIndex;
    vk::Queue graphicsQueue;
    vk::Queue transferQueue;
    vk::Queue computeQueues[3];

};

uint32_t vulkanFindMemoryType(
    const vk::PhysicalDevice& physicalDevice,
    uint32_t typeBits,
    vk::MemoryPropertyFlags propertyFlags);

vk::UniqueShaderModule loadShaderModule(const VulkanBasicInfo& vkbi, const std::string& fileName);

vk::UniqueSemaphore createSemaphore(const VulkanBasicInfo& vkbi);

vk::UniqueSemaphore createExternalSemaphore(const VulkanBasicInfo& vkbi, int* externalHandle);

vk::UniqueFence createFence(const VulkanBasicInfo& vkbi, bool signalled = false);
