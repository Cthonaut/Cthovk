#include "../headers/device.h"

namespace Cthovk
{

void vkCheck(bool result, const char *error)
{
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(error);
    }
}

Device::Device(DeviceInfo inf)
{
    initInstance(inf.enableVL, inf.vl, inf.windowExt);
    if (inf.enableVL)
        initValidationLayers();
    inf.initSurface(instance, &surface);
    selectGPU(inf.deviceExt);
    initLogDevice(inf.enableVL, inf.deviceExt, inf.vl);
}

VkFormat Device::findDepthFormat()
{
    VkFormat candidates[3] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    for (uint32_t i{0}; i < 3; i++)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(phyDevice, candidates[i], &props);
        if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) ==
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            return candidates[i];
    }
    throw std::runtime_error("failed to find supported depth format");
}

void Device::initInstance(bool enableValidationLayers, std::vector<const char *> validationLayers,
                          std::vector<const char *> windowApiExtensions)
{
    // check for validation Layers
    if (enableValidationLayers)
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        uint8_t commonLayer{0};
        for (uint8_t i{0}; i < validationLayers.size(); ++i)
        {
            for (uint16_t j{0}; j < layerCount; ++j)
            {
                if (std::strcmp(validationLayers[i], availableLayers[j].layerName) == 0)
                    commonLayer++;
            }
        }
        if (commonLayer != validationLayers.size())
            throw std::runtime_error("validationLayers not found");

        windowApiExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // required for validation layers
    }

    VkInstanceCreateInfo instanceInfo{

        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .enabledLayerCount = enableValidationLayers ? static_cast<uint32_t>(validationLayers.size()) : 0,
        .ppEnabledLayerNames = enableValidationLayers ? validationLayers.data() : nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(windowApiExtensions.size()),
        .ppEnabledExtensionNames = windowApiExtensions.data(),
    };
    vkCheck(vkCreateInstance(&instanceInfo, nullptr, &instance), "failed to create instance");
}

void Device::initValidationLayers()
{
    // get the functions for Validation Layers
    vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (vkCreateDebugUtilsMessengerEXT == nullptr)
    {
        throw std::runtime_error("failed to get validation layers function: vkCreateDebugUtilsMessengerEXT");
    }
    if (vkDestroyDebugUtilsMessengerEXT == nullptr)
    {
        throw std::runtime_error("failed to get validation layers function : vkDestroyDebugUtilsMessengerEXT");
    }

    PFN_vkDebugUtilsMessengerCallbackEXT callback =
        [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
           const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) -> VkBool32 {
        std::string prefix;
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            prefix = "VL_ERROR: ";
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            prefix = "VL_WARNING: ";
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
            prefix = "VL_INFO: ";
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        {
            prefix = "VL_DEBUG: ";
        }
        std::cerr << prefix << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    };

    // create callback
    VkDebugUtilsMessengerCreateInfoEXT callbackCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = callback,
    };
    vkCheck(vkCreateDebugUtilsMessengerEXT(instance, &callbackCreateInfo, nullptr, &messenger),
            "failed to setup callback");
}

uint32_t Device::rateGPU(VkPhysicalDevice device, std::vector<const char *> deviceExt)
{
    // check queueFamilies
    uint32_t queueFamilyCount{0};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamiliesList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamiliesList.data());

    bool foundGrFamily{false};
    bool foundPrFamily{false};
    bool foundCoFamily{false};
    for (uint32_t i{0}; i < queueFamilyCount; ++i)
    {
        if (queueFamiliesList[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            foundGrFamily = true;
        VkBool32 presentSupport{false};
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
            foundPrFamily = true;
        if (queueFamiliesList[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
            foundCoFamily = true;
    }

    if (!foundCoFamily || !foundPrFamily || !foundGrFamily)
        return 0;

    // check extensions
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExt(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExt.data());

    uint8_t commonExtensions{0};
    for (uint8_t i{0}; i < deviceExt.size(); ++i)
    {
        for (uint16_t j{0}; j < extensionCount; ++j)
        {
            if (std::strcmp(deviceExt[i], availableExt[j].extensionName) == 0)
                commonExtensions++;
        }
    }
    if (commonExtensions != deviceExt.size())
        return 0;

    // check formats
    uint32_t availableFormatsCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &availableFormatsCount, nullptr);
    if (availableFormatsCount == 0)
        return 0;

    uint32_t availablePresentModesCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &availablePresentModesCount, nullptr);
    if (availablePresentModesCount == 0)
        return 0;

    // basic for now may change later
    int32_t rating{1};
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        rating += 1;
    }
    return rating;
}

void Device::selectGPU(std::vector<const char *> deviceExt)
{
    uint32_t deviceCount{0};
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
        throw std::runtime_error("found no GPUs");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // select suitable GPUs
    std::vector<uint32_t> ratings;
    uint32_t maxRater{0};
    for (uint8_t i{0}; i < deviceCount; i++)
    {
        ratings.push_back(rateGPU(devices[i], deviceExt));
        if (ratings[i] > ratings[maxRater])
        {
            maxRater = i;
        }
    }

    if (ratings[maxRater] == 0)
        throw std::runtime_error("no suitable GPUs found");

    phyDevice = devices[0];
}

void Device::initLogDevice(bool useVL, std::vector<const char *> deviceExt, std::vector<const char *> validationLayers)
{
    uint32_t queueFamilyCount{0};
    vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamiliesList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &queueFamilyCount, queueFamiliesList.data());

    std::vector<uint32_t> queueFamilies(3, UINT32_MAX);
    bool foundGrFamily{false};
    bool foundPrFamily{false};
    bool foundCoFamily{false};
    for (uint32_t i{0}; i < queueFamilyCount; ++i)
    {
        if (queueFamiliesList[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && !foundGrFamily)
        {
            queueFamilies[0] = i;
            foundGrFamily = true;
        }

        if (!foundPrFamily)
        {
            VkBool32 presentSupport{false};
            vkGetPhysicalDeviceSurfaceSupportKHR(phyDevice, i, surface, &presentSupport);
            if (presentSupport)
            {
                queueFamilies[1] = i;
                foundPrFamily = true;
            }
        }

        if (queueFamiliesList[i].queueFlags & VK_QUEUE_COMPUTE_BIT && !foundCoFamily)
        {
            queueFamilies[2] = i;
            foundCoFamily = true;
        }
    }
    std::sort(queueFamilies.begin(), queueFamilies.end());
    queueFamilies.erase(std::unique(queueFamilies.begin(), queueFamilies.end()), queueFamilies.end());

    VkDeviceQueueCreateInfo queueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueCount = 1,
        .pQueuePriorities = new float(1.0f),
    };
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(queueFamilies.size());
    for (uint8_t i{0}; i < queueFamilies.size(); ++i)
    {
        queueCreateInfos[i] = queueCreateInfo;
        queueCreateInfos[i].queueFamilyIndex = *(std::next(queueFamilies.begin(), i));
    }

    VkDeviceCreateInfo logDeviceInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledLayerCount = useVL ? static_cast<uint32_t>(validationLayers.size()) : 0,
        .ppEnabledLayerNames = useVL ? validationLayers.data() : nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExt.size()),
        .ppEnabledExtensionNames = deviceExt.data(),
        .pEnabledFeatures = nullptr,
    };
    vkCheck(vkCreateDevice(phyDevice, &logDeviceInfo, nullptr, &logDevice), "failed to initialize logic device");

    // retrieve queue handle
    vkGetDeviceQueue(logDevice, queueFamilies[0], 0, &queues.graphics);
    queues.present = queues.graphics;
    queues.compute = queues.graphics;
    if (queueFamilies[1] != queueFamilies[0])
    {
        vkGetDeviceQueue(logDevice, queueFamilies[1], 0, &queues.present);
    }
    if (queueFamilies[2] != queueFamilies[0] && queueFamilies[2] != queueFamilies[1])
    {
        vkGetDeviceQueue(logDevice, queueFamilies[2], 0, &queues.compute);
    }
    else if (queueFamilies[2] != queueFamilies[0])
    {
        queues.compute = queues.present;
    }
}

Device::~Device()
{
    vkDestroyDevice(logDevice, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    if (messenger != nullptr)
    {
        vkDestroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
}

} // namespace Cthovk
