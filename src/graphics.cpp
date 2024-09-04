#include "../headers/graphics.h"

namespace Cthovk
{

static void vkCheck(bool result, const char *error)
{
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(error);
    }
}

Graphics::Graphics(VkDevice logDevice, VkPhysicalDevice phyDevice, VkSurfaceKHR surface,
                   std::function<void(uint32_t &width, uint32_t &height)> getFrameBufferSize,
                   const char *vertShaderLocation, const char *fragShaderLocation)
    : sc(logDevice, phyDevice, surface, getFrameBufferSize),
      shaders{
          ShaderObj(logDevice, vertShaderLocation, VK_SHADER_STAGE_VERTEX_BIT),
          ShaderObj(logDevice, fragShaderLocation, VK_SHADER_STAGE_FRAGMENT_BIT),
      }
{
}

SwapChainObj::SwapChainObj(VkDevice logDevice, VkPhysicalDevice phyDevice, VkSurfaceKHR surface,
                           std::function<void(uint32_t &width, uint32_t &height)> getFrameBufferSize)
{
    SwapChainObj::logDevice = logDevice;
    initSwapChain(phyDevice, surface, getFrameBufferSize);
    initImageViews();
}

void SwapChainObj::initSwapChain(VkPhysicalDevice phyDevice, VkSurfaceKHR surface,
                                 std::function<void(uint32_t &width, uint32_t &height)> getFrameBufferSize)
{
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phyDevice, surface, &capabilities);
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
        imageCount = capabilities.maxImageCount;

    if (capabilities.currentExtent.width != UINT32_MAX)
        extent = capabilities.currentExtent;
    else
    {
        uint32_t width, height;
        getFrameBufferSize(width, height);
        extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    }

    VkColorSpaceKHR colorSpace;
    uint32_t availableFormatsCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevice, surface, &availableFormatsCount, nullptr);
    std::vector<VkSurfaceFormatKHR> availableFormats(availableFormatsCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevice, surface, &availableFormatsCount, availableFormats.data());
    format = availableFormats[0].format;
    colorSpace = availableFormats[0].colorSpace;
    for (uint8_t i{0}; i < availableFormatsCount; ++i)
    {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            format = availableFormats[i].format;
            colorSpace = availableFormats[i].colorSpace;
        }
    }

    // get Queue Families
    uint32_t queueFamilyCount{0};
    vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamiliesList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &queueFamilyCount, queueFamiliesList.data());

    std::set<uint32_t> queueFamilies;
    bool foundGrFamily{false};
    bool foundPrFamily{false};
    bool foundCoFamily{false};
    for (uint32_t i{0}; i < queueFamilyCount; ++i)
    {
        if (queueFamiliesList[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && !foundGrFamily)
        {
            queueFamilies.insert(i);
            foundGrFamily = true;
        }

        if (!foundPrFamily)
        {
            VkBool32 presentSupport{false};
            vkGetPhysicalDeviceSurfaceSupportKHR(phyDevice, i, surface, &presentSupport);
            if (presentSupport)
            {
                queueFamilies.insert(i);
                foundPrFamily = true;
            }
        }

        if (queueFamiliesList[i].queueFlags & VK_QUEUE_COMPUTE_BIT && !foundCoFamily)
        {
            queueFamilies.insert(i);
            foundCoFamily = true;
        }
    }
    std::vector<uint32_t> _queueFamilies(queueFamilies.begin(), queueFamilies.end());

    VkPresentModeKHR presentMode{VK_PRESENT_MODE_FIFO_KHR};
    uint32_t presentModesCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phyDevice, surface, &presentModesCount, nullptr);
    std::vector<VkPresentModeKHR> availablePresentModes(presentModesCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(phyDevice, surface, &presentModesCount, availablePresentModes.data());
    bool foundPresentMode{false};
    for (uint8_t i{0}; i < presentModesCount; ++i)
    {
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = availablePresentModes[i];
        }
    }

    // create swap chain
    VkSwapchainCreateInfoKHR schInfo{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = format,
        .imageColorSpace = colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = queueFamilies.size() > 2 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = queueFamilies.size() > 2 ? static_cast<uint32_t>(queueFamilies.size()) : 0,
        .pQueueFamilyIndices = queueFamilies.size() > 2 ? _queueFamilies.data() : nullptr,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };
    vkCheck(vkCreateSwapchainKHR(logDevice, &schInfo, nullptr, &SwapChain), "failed to create swap chain");

    vkGetSwapchainImagesKHR(logDevice, SwapChain, &imageCount, nullptr);
    images.resize(imageCount);
    vkGetSwapchainImagesKHR(logDevice, SwapChain, &imageCount, images.data());
}

void SwapChainObj::initImageViews()
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
    for (uint8_t i{0}; i < images.size(); ++i)
    {
        viewInfo.image = images[i];
        vkCheck(vkCreateImageView(logDevice, &viewInfo, nullptr, imageViews.data() + i),
                "failed to create images for Swap Chain");
    }
}

SwapChainObj::~SwapChainObj()
{
    for (uint16_t i{0}; i < imageViews.size(); ++i)
    {
        vkDestroyImageView(logDevice, imageViews[i], nullptr);
    }
    vkDestroySwapchainKHR(logDevice, SwapChain, nullptr);
}

ShaderObj::ShaderObj(VkDevice logDevice, const char *shaderLocation, VkShaderStageFlagBits shaderStageBit)
{
    ShaderObj::logDevice = logDevice;
    std::ifstream file(shaderLocation, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("failed to open shader file");
    uint32_t fileSize = (uint32_t)file.tellg();
    std::vector<char> shaderCode(fileSize);
    file.seekg(0);
    file.read(shaderCode.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo shaderModuleInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = fileSize,
        .pCode = reinterpret_cast<const uint32_t *>(shaderCode.data()),
    };
    vkCheck(vkCreateShaderModule(logDevice, &shaderModuleInfo, nullptr, &module), "failed to create shaders");

    VkPipelineShaderStageCreateInfo shaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = shaderStageBit,
        .module = module,
        .pName = "main",
    };
    stageInfo = shaderStageInfo;
}

ShaderObj::~ShaderObj()
{
    vkDestroyShaderModule(logDevice, module, nullptr);
}

} // namespace Cthovk
