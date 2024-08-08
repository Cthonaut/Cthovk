#include "../headers/device.h"
#include <vulkan/vulkan_core.h>

namespace Cthovk
{

void Device::initInstance()
{
    if (enableValidationLayers)
    {
        // sort Validation Layers for comparing
        std::sort(validationLayers.begin(), validationLayers.end(), standardCstringComp);
        uint8_t vlCount = validationLayers.size();

        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        std::vector<const char *> availableLayersNames(layerCount);
        for (uint8_t i{0}; i < layerCount; i++)
        {
            availableLayersNames[i] = availableLayers[i].layerName;
        }
        std::sort(availableLayersNames.begin(), availableLayersNames.end(),
                  standardCstringComp); // sorting for comparing

        // comparing
        uint8_t layersMatching{0};
        for (uint8_t i{0}; i < layerCount && layersMatching < vlCount; i++)
        {
            uint32_t vlNameLength = strlen(validationLayers[layersMatching]);
            if (strlen(availableLayersNames[i]) != vlNameLength)
                continue;

            bool skip{false};
            for (uint8_t j{0}; j < vlNameLength; j++)
            {
                if (validationLayers[layersMatching][j] != availableLayersNames[i][j])
                {
                    skip = true;
                    break;
                }
            }
            if (skip)
                continue;
            layersMatching++;
        }
        if (layersMatching != vlCount || vlCount > layerCount)
            throw std::runtime_error("validation layers not available");

        windowApiExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME); // required for validation layers
    }

    VkInstanceCreateInfo instanceInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .enabledLayerCount = 0,
        .enabledExtensionCount = static_cast<uint32_t>(windowApiExtensions.size()),
        .ppEnabledExtensionNames = windowApiExtensions.data(),
    };
    if (enableValidationLayers)
    {
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceInfo.ppEnabledLayerNames = validationLayers.data();
    }
    vkCheck(vkCreateInstance(&instanceInfo, nullptr, &instance), "failed to create instance");
}

void Device::initValidationLayers()
{
    // get the functions for Validation Layers
    vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
    vkDebugReportMessageEXT =
        reinterpret_cast<PFN_vkDebugReportMessageEXT>(vkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT"));
    vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
    if (vkCreateDebugReportCallbackEXT == nullptr || vkDebugReportMessageEXT == nullptr ||
        vkDestroyDebugReportCallbackEXT == nullptr)
    {
        throw std::runtime_error("failed to get report callback functions");
    }

    // create callback
    VkDebugReportCallbackCreateInfoEXT callbackCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                 VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
        .pfnCallback = &callbackMessage,
        .pUserData = nullptr,
    };
    vkCheck(vkCreateDebugReportCallbackEXT(instance, &callbackCreateInfo, nullptr, &callback),
            "failed to setup callback");
}

void Device::selectGPU()
{
    phyDevice = VK_NULL_HANDLE;
    std::sort(deviceExt.begin(), deviceExt.end(), standardCstringComp);
    uint8_t devExtCount = deviceExt.size();

    uint32_t deviceCount{0};
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0)
        throw std::runtime_error("found no GPUs");
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // select suitable GPUs
    for (uint8_t i{0}; i < deviceCount; i++)
    {
        if (!getQueueFamilies(devices[i], surface, nullptr))
            continue;

        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(devices[i], nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExt(extensionCount);
        vkEnumerateDeviceExtensionProperties(devices[i], nullptr, &extensionCount, availableExt.data());
        std::vector<const char *> availableExtNames(extensionCount);
        for (uint8_t j{0}; j < extensionCount; j++)
        {
            availableExtNames[j] = availableExt[j].extensionName;
        }
        std::sort(availableExtNames.begin(), availableExtNames.end(),
                  standardCstringComp); // sort for comparing

        uint8_t extMatching{0};
        for (uint8_t k{0}; k < extensionCount && extMatching < devExtCount; k++)
        {
            uint8_t extensionNameLength = strlen(deviceExt[extMatching]);
            if (strlen(availableExtNames[k]) != extensionNameLength)
                continue;
            bool skip{false};
            for (uint8_t j{0}; j < extensionNameLength; j++)
            {
                if (deviceExt[extMatching][j] != availableExtNames[k][j])
                {
                    skip = true;
                    break;
                }
            }
            if (skip)
                continue;
            extMatching++;
        }
        if (extMatching != devExtCount || devExtCount > extensionCount)
            continue;

        uint32_t availableFormatsCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(devices[i], surface, &availableFormatsCount, nullptr);
        if (availableFormatsCount == 0)
            continue;

        uint32_t availablePresentModesCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(devices[i], surface, &availablePresentModesCount, nullptr);
        if (availablePresentModesCount == 0)
            continue;

        phyDevice = devices[i];
        break;
    }

    if (phyDevice == VK_NULL_HANDLE)
        throw std::runtime_error("sutable GPU not found");

    // get multisampling cababilities
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(phyDevice, &physicalDeviceProperties);
    multiCounts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                  physicalDeviceProperties.limits.framebufferDepthSampleCounts;
}

void Device::initLogDevice()
{
    std::vector<uint32_t> queueFamilies;
    getQueueFamilies(phyDevice, surface, &queueFamilies, true);
    VkDeviceQueueCreateInfo queueCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueCount = 1,
        .pQueuePriorities = new float(1.0f),
    };
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos(queueFamilies.size());
    for (uint8_t i{0}; i < queueFamilies.size(); i++)
    {
        queueCreateInfos[i] = queueCreateInfo;
        queueCreateInfos[i].queueFamilyIndex = queueFamilies[i];
    }

    VkDeviceCreateInfo logDeviceInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledLayerCount = 0,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExt.size()),
        .ppEnabledExtensionNames = deviceExt.data(),
        .pEnabledFeatures = new VkPhysicalDeviceFeatures(),
    };
    if (enableValidationLayers)
    {
        logDeviceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        logDeviceInfo.ppEnabledLayerNames = validationLayers.data();
    }
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

void Device::SwapChainObj::initSwapChain(Device *pDevice)
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pDevice->phyDevice, pDevice->surface, &capabilities);
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    {
        imageCount = capabilities.maxImageCount;
    }

    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        extent = capabilities.currentExtent;
    }
    else
    {
        uint32_t width, height;
        pDevice->getFrameBufferSize(&width, &height);
        extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    VkColorSpaceKHR colorSpace;
    uint32_t availableFormatsCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice->phyDevice, pDevice->surface, &availableFormatsCount, nullptr);
    std::vector<VkSurfaceFormatKHR> availableFormats(availableFormatsCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(pDevice->phyDevice, pDevice->surface, &availableFormatsCount,
                                         availableFormats.data());
    format = availableFormats[0].format;
    colorSpace = availableFormats[0].colorSpace;
    for (uint8_t i{0}; i < availableFormatsCount; i++)
    {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            format = availableFormats[i].format;
            colorSpace = availableFormats[i].colorSpace;
        }
    }

    std::vector<uint32_t> queueFamilies;
    getQueueFamilies(pDevice->phyDevice, pDevice->surface, &queueFamilies, true);

    VkPresentModeKHR presentMode{VK_PRESENT_MODE_FIFO_KHR};
    uint32_t presentModesCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pDevice->phyDevice, pDevice->surface, &presentModesCount, nullptr);
    std::vector<VkPresentModeKHR> availablePresentModes(presentModesCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(pDevice->phyDevice, pDevice->surface, &presentModesCount,
                                              availablePresentModes.data());
    bool foundPresentMode{false};
    for (uint8_t i{0}; i < presentModesCount; i++)
    {
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = availablePresentModes[i];
        }
    }

    // create swap chain
    VkSwapchainCreateInfoKHR schInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = pDevice->surface,
        .minImageCount = imageCount,
        .imageFormat = format,
        .imageColorSpace = colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };
    if (queueFamilies.size() > 2)
    {
        schInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        schInfo.queueFamilyIndexCount = queueFamilies.size();
        schInfo.pQueueFamilyIndices = queueFamilies.data();
    }
    vkCheck(vkCreateSwapchainKHR(pDevice->logDevice, &schInfo, nullptr, &SwapChain), "failed to create swap chain");

    vkGetSwapchainImagesKHR(pDevice->logDevice, SwapChain, &imageCount, nullptr);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(pDevice->logDevice, SwapChain, &imageCount, images.data());
}

void Device::cleanup()
{
    if (enableValidationLayers)
    {
        vkDestroyDebugReportCallbackEXT(instance, callback, nullptr);
    }
    scResources.cleanup(logDevice);
    vkDestroyDevice(logDevice, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

bool getQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, std::vector<uint32_t> *queueFamilies,
                      bool noDuplicates)
{
    uint32_t queueFamilyCount{0};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamiliesList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamiliesList.data());

    std::vector<uint32_t> *qf = new std::vector<uint32_t>();
    if (queueFamilies != nullptr)
        qf = queueFamilies;

    // find graphics family
    for (uint32_t i{0}; i < queueFamilyCount; i++)
    {
        if (queueFamiliesList[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            qf->push_back(i);
            break;
        };
    }

    // find present family
    for (uint32_t i{0}; i < queueFamilyCount; i++)
    {
        VkBool32 presentSupport{false};
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
        {
            qf->push_back(i);
            break;
        }
    }

    // find compute family
    for (uint32_t i{0}; i < queueFamilyCount; i++)
    {
        if (queueFamiliesList[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
        {
            qf->push_back(i);
            break;
        }
    }

    if (noDuplicates)
    {
        std::sort(queueFamilies->begin(), queueFamilies->end());
        queueFamilies->erase(unique(queueFamilies->begin(), queueFamilies->end()), queueFamilies->end());
    }

    // return if all found
    if (qf->size() < 3)
        return false;
    return true;
}

void vkCheck(bool result, const char *error)
{
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(error);
    }
};

} // namespace Cthovk
