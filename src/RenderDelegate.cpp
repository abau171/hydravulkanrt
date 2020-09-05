#include <Common.h>

#include <RenderPass.h>
#include <Mesh.h>
#include <Material.h>

#include <RenderDelegate.h>


const pxr::TfTokenVector HVRTRenderDelegate::SUPPORTED_RPRIM_TYPES =
{
    pxr::HdPrimTypeTokens->mesh,
};

const pxr::TfTokenVector HVRTRenderDelegate::SUPPORTED_SPRIM_TYPES =
{
    pxr::HdPrimTypeTokens->camera,
    pxr::HdPrimTypeTokens->extComputation,
    pxr::HdPrimTypeTokens->material,
};

const pxr::TfTokenVector HVRTRenderDelegate::SUPPORTED_BPRIM_TYPES =
{
};


HVRTRenderDelegate::HVRTRenderDelegate()
    : pxr::HdRenderDelegate()
{
    init();
}

HVRTRenderDelegate::HVRTRenderDelegate(pxr::HdRenderSettingsMap const& settingsMap)
    : pxr::HdRenderDelegate(settingsMap)
{
    init();
}

void HVRTRenderDelegate::init() {
    _resourceRegistry = std::make_shared<pxr::HdResourceRegistry>();
    vulkanInit();
}

const pxr::TfTokenVector& HVRTRenderDelegate::GetSupportedRprimTypes() const {
    return SUPPORTED_RPRIM_TYPES;
}

const pxr::TfTokenVector& HVRTRenderDelegate::GetSupportedSprimTypes() const {
    return SUPPORTED_SPRIM_TYPES;
}

const pxr::TfTokenVector& HVRTRenderDelegate::GetSupportedBprimTypes() const {
    return SUPPORTED_BPRIM_TYPES;
}

pxr::HdResourceRegistrySharedPtr HVRTRenderDelegate::GetResourceRegistry() const {
    return _resourceRegistry;
}

pxr::HdRenderPassSharedPtr HVRTRenderDelegate::CreateRenderPass(
    pxr::HdRenderIndex* index,
    pxr::HdRprimCollection const& collection)
{
    return std::make_shared<HVRTRenderPass>(index, collection, _vkbi, _meshes);
}

pxr::HdInstancer* HVRTRenderDelegate::CreateInstancer(
    pxr::HdSceneDelegate* delegate,
    pxr::SdfPath const& id,
    pxr::SdfPath const& instancerId)
{
    (void) delegate;
    (void) id;
    (void) instancerId;
    return nullptr;
}

void HVRTRenderDelegate::DestroyInstancer(pxr::HdInstancer* instancer) {
    (void) instancer;
}

pxr::HdRprim* HVRTRenderDelegate::CreateRprim(
    pxr::TfToken const& typeId,
    pxr::SdfPath const& rprimId,
    pxr::SdfPath const& instancerId)
{
    if (typeId == pxr::HdPrimTypeTokens->mesh) {
        HVRTMesh* mesh = new HVRTMesh(rprimId, instancerId, _vkbi, _materials);
        _meshes.insert(mesh);
        return mesh;
    }
    return nullptr;
}

void HVRTRenderDelegate::DestroyRprim(pxr::HdRprim* rPrim) {

    // If rPrim is a mesh, remove it from _meshes.
    _meshes.erase(reinterpret_cast<HVRTMesh*>(rPrim));

    delete rPrim;
}

pxr::HdSprim* HVRTRenderDelegate::CreateSprim(pxr::TfToken const& typeId, pxr::SdfPath const& sprimId) {
    if (typeId == pxr::HdPrimTypeTokens->camera) {
        return new pxr::HdCamera(sprimId);
    } else if (typeId == pxr::HdPrimTypeTokens->extComputation) {
        return new pxr::HdExtComputation(sprimId);
    } else if (typeId == pxr::HdPrimTypeTokens->material) {
        HVRTMaterial* material = new HVRTMaterial(sprimId);
        _materials[sprimId.GetString()] = material;
        return material;
    }
    return nullptr;
}

pxr::HdSprim* HVRTRenderDelegate::CreateFallbackSprim(pxr::TfToken const& typeId) {
    return CreateSprim(typeId, pxr::SdfPath::EmptyPath());
}

void HVRTRenderDelegate::DestroySprim(pxr::HdSprim* sprim) {

    // If sprim is a material, remove it from _materials.
    _materials.erase(sprim->GetId().GetString());

    delete sprim;
}

pxr::HdBprim* HVRTRenderDelegate::CreateBprim(pxr::TfToken const& typeId, pxr::SdfPath const& bprimId) {
    (void) typeId;
    (void) bprimId;
    return nullptr;
}

pxr::HdBprim* HVRTRenderDelegate::CreateFallbackBprim(pxr::TfToken const& typeId) {
    (void) typeId;
    return nullptr;
}

void HVRTRenderDelegate::DestroyBprim(pxr::HdBprim* bprim) {
    (void) bprim;
}

void HVRTRenderDelegate::CommitResources(pxr::HdChangeTracker* tracker) {
    (void) tracker;

    for (HVRTMesh* mesh : _meshes) {
        mesh->commitResources();
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    (void) pUserData;

    std::cerr << "Vulkan";
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        std::cerr << " ERROR";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << " WARNING";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        std::cerr << " VERBOSE";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        std::cerr << " INFO";
    } else {
        std::cerr << " UNKNOWN";
    }
    if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
        std::cerr << " GENERAL";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
        std::cerr << " VALIDATION";
    } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
        std::cerr << " PERFORMANCE";
    } else {
        std::cerr << " UNKNOWN";
    }
    std::cerr << ": " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

static uint32_t vulkanFindQueueFamily(
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties,
    vk::QueueFlags requiredFlags,
    vk::QueueFlags disqualifyingFlags = {})
{
    for (uint32_t i = 0; i < queueFamilyProperties.size(); i++) {
        vk::QueueFamilyProperties& properties = queueFamilyProperties[i];
        if ((properties.queueFlags & requiredFlags) == requiredFlags
            && !(properties.queueFlags & disqualifyingFlags))
        {
            return i;
        }
    }

    throw std::runtime_error("Could not find requested queue family.");
}

void HVRTRenderDelegate::vulkanInit() {

    // Create Vulkan instance. Add validation layers if in debug mode.

    vk::ApplicationInfo appInfo(
        "HydraVulkanRT",
        VK_MAKE_VERSION(1, 0, 0),
        "HydraVulkanRT",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_2);

    std::vector<const char*> extensions = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
    };

    std::vector<const char*> layers;

#if !NDEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    layers.push_back("VK_LAYER_KHRONOS_validation");
#endif

    _instance = vk::createInstanceUnique(vk::InstanceCreateInfo(
        {},
        &appInfo,
        layers,
        extensions));
    _vkbi.instance = _instance.get();
    _vkbi.dispatchLoader = vk::DispatchLoaderDynamic(_vkbi.instance, vkGetInstanceProcAddr);

#if !NDEBUG
    _debugMessenger = _vkbi.instance.createDebugUtilsMessengerEXTUnique(
        vk::DebugUtilsMessengerCreateInfoEXT(
            {},
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
            // | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
            | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo,
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
            vulkanDebugCallback),
        nullptr,
        _vkbi.dispatchLoader);
#endif

    // Select a physical device.

    std::vector<vk::PhysicalDevice> physicalDevices = _vkbi.instance.enumeratePhysicalDevices();
    vk::PhysicalDevice* selectedPhysicalDevice = nullptr;
    vk::PhysicalDeviceProperties2 selectedPhysicalDeviceProperties;
    vk::PhysicalDeviceRayTracingPropertiesKHR selectedPhysicalDeviceRTProperties;
    vk::PhysicalDeviceFeatures selectedPhysicalDeviceFeatures;
    for (vk::PhysicalDevice& physicalDevice : physicalDevices) {

        vk::PhysicalDeviceProperties2 properties;
        vk::PhysicalDeviceRayTracingPropertiesKHR rtProperties;
        properties.pNext = &rtProperties;
        physicalDevice.getProperties2(&properties);
        vk::PhysicalDeviceFeatures features = physicalDevice.getFeatures();

        if (features.geometryShader
            && (selectedPhysicalDevice == nullptr
                || properties.properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu))
        {
            selectedPhysicalDevice = &physicalDevice;
            selectedPhysicalDeviceProperties = properties;
            selectedPhysicalDeviceRTProperties = rtProperties;
            selectedPhysicalDeviceFeatures = features;
        }
    }
    if (selectedPhysicalDevice == nullptr) {
        throw std::runtime_error("No suitable Vulkan physical device found.");
    }
    _vkbi.physicalDevice = *selectedPhysicalDevice;

    // Select queues and create logical device.

    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        _vkbi.physicalDevice.getQueueFamilyProperties();
    _vkbi.graphicsQueueFamilyIndex = vulkanFindQueueFamily(
        queueFamilyProperties,
        vk::QueueFlagBits::eGraphics);

    try {
        _vkbi.transferQueueFamilyIndex = vulkanFindQueueFamily(
            queueFamilyProperties,
            vk::QueueFlagBits::eTransfer,
            vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute);
    } catch (std::runtime_error& e) {
        _vkbi.transferQueueFamilyIndex = _vkbi.graphicsQueueFamilyIndex;
    }

    try {
        _vkbi.computeQueueFamilyIndex = vulkanFindQueueFamily(
            queueFamilyProperties,
            vk::QueueFlagBits::eCompute,
            vk::QueueFlagBits::eGraphics);
    } catch (std::runtime_error& e) {
        _vkbi.computeQueueFamilyIndex = _vkbi.graphicsQueueFamilyIndex;
    }

    float queuePriorities[3] = { 1.0f, 1.0f, 1.0f };
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos = {
        { vk::DeviceQueueCreateFlags(), _vkbi.graphicsQueueFamilyIndex, 1, queuePriorities },
        { vk::DeviceQueueCreateFlags(), _vkbi.transferQueueFamilyIndex, 1, queuePriorities },
        { vk::DeviceQueueCreateFlags(), _vkbi.computeQueueFamilyIndex, 3, queuePriorities },
    };

    std::vector<const char*> deviceExtensions = {
        VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
        VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_EXTENSION_NAME,
    };

    vk::PhysicalDeviceFeatures2* features = nullptr;

    vk::PhysicalDeviceRayTracingFeaturesKHR rtFeatures = {};
    rtFeatures.rayTracing = true;
    rtFeatures.setPNext(features);
    features = reinterpret_cast<vk::PhysicalDeviceFeatures2*>(&rtFeatures);

    vk::PhysicalDeviceVulkan12Features vk12Features = {};
    vk12Features.descriptorIndexing = true;
    vk12Features.bufferDeviceAddress = true;
    vk12Features.setPNext(features);
    features = reinterpret_cast<vk::PhysicalDeviceFeatures2*>(&vk12Features);

    vk::DeviceCreateInfo deviceCreateInfo(
        {},
        queueCreateInfos,
        layers, // Deprecated, but good practice to include for compatibility.
        deviceExtensions,
        nullptr);
    deviceCreateInfo.setPNext(features);
    _device = _vkbi.physicalDevice.createDeviceUnique(deviceCreateInfo);
    _vkbi.device = _device.get();

    _vkbi.graphicsQueue = _vkbi.device.getQueue(_vkbi.graphicsQueueFamilyIndex, 0);
    _vkbi.transferQueue = _vkbi.device.getQueue(_vkbi.transferQueueFamilyIndex, 0);
    for (int i = 0; i < 3; i++) {
        _vkbi.computeQueues[i] = _vkbi.device.getQueue(_vkbi.computeQueueFamilyIndex, i);
    }
}
