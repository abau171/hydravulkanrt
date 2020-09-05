#include <Common.h>

#include <VulkanUtils.h>


uint32_t vulkanFindMemoryType(
    const vk::PhysicalDevice& physicalDevice,
    uint32_t typeBits,
    vk::MemoryPropertyFlags propertyFlags)
{
    vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

    uint32_t memoryTypeIndex;
    for (memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; memoryTypeIndex++) {
        if ((typeBits & (1u << memoryTypeIndex))
            && ((memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & propertyFlags)
                == propertyFlags))
        {
            break;
        }
    }

    if (memoryTypeIndex == memoryProperties.memoryTypeCount) {
        throw std::runtime_error("Could not find required memory type.");
    }

    return memoryTypeIndex;
}

vk::UniqueShaderModule loadShaderModule(const VulkanBasicInfo& vkbi, const std::string& fileName) {

    const char* resourcePath = getenv("HVRT_RESOURCE_PATH");
    if (resourcePath == nullptr) {
        throw std::runtime_error("HVRT_RESOURCE_PATH environment variable not configured.");
    }

    std::filesystem::path spvFilePath =
        std::filesystem::path(resourcePath) / "shaders" / (fileName + ".spv");
    std::ifstream file(spvFilePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error(
            "Failed to open file '" + spvFilePath.string() + "'. "
            "Is HVRT_RESOURCE_PATH environment variable configured properly?");
    }

    size_t fileSize = file.tellg();
    std::vector<uint32_t> spvBinary((fileSize + 3) / 4);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(spvBinary.data()), fileSize);

    file.close();

    return vkbi.device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, spvBinary));
}

vk::UniqueSemaphore createSemaphore(const VulkanBasicInfo& vkbi) {
    return vkbi.device.createSemaphoreUnique(vk::SemaphoreCreateInfo());
}

vk::UniqueSemaphore createExternalSemaphore(const VulkanBasicInfo& vkbi, int* externalHandle) {

    vk::ExportSemaphoreCreateInfo externalSemaphoreCreateInfo(
        vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd);
    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    semaphoreCreateInfo.setPNext(&externalSemaphoreCreateInfo);
    vk::UniqueSemaphore semaphore = vkbi.device.createSemaphoreUnique(semaphoreCreateInfo);

    *externalHandle = vkbi.device.getSemaphoreFdKHR(
        vk::SemaphoreGetFdInfoKHR(
            semaphore.get(),
            vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd),
        vkbi.dispatchLoader);

    return semaphore;
}

vk::UniqueFence createFence(const VulkanBasicInfo& vkbi, bool signalled) {
    vk::FenceCreateFlags flags =
        signalled
        ? vk::FenceCreateFlagBits::eSignaled
        : vk::FenceCreateFlags();
    return vkbi.device.createFenceUnique({ flags });
}
