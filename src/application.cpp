#include "../headers/application.h"
#include <vulkan/vulkan_core.h>

namespace Cthovk
{

void SwapChainObj::initSwapChain(Device *pDevice)
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

void SwapChainObj::initImageViews(VkDevice logDevice)
{
    imageViews.resize(images.size());
    VkImageViewCreateInfo viewInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };
    for (uint8_t i{0}; i < images.size(); i++)
    {
        viewInfo.image = images[i];
        vkCheck(vkCreateImageView(logDevice, &viewInfo, nullptr, imageViews.data() + i),
                "failed to create images for Swap Chain");
    }
}

void Application::run()
{
    initVulkan();
    loop();
    cleanup();
}

void Application::initVulkan()
{
    device.initInstance();
    if (device.enableValidationLayers)
        device.initValidationLayers();
    device.initSurface(device.instance, &device.surface);
    device.selectGPU();
    device.initLogDevice();

    scResources.initSwapChain(&device);
    scResources.initImageViews(device.logDevice);
}

void Application::loop()
{
}

void Application::cleanup()
{
    scResources.cleanup(device.logDevice);
    device.cleanup();
}

} // namespace Cthovk
