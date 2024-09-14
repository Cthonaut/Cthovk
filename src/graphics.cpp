#include "../headers/graphics.h"
#include <strings.h>

namespace Cthovk
{

static void vkCheck(bool result, const char *error)
{
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(error);
    }
}

Graphics::Graphics(VkDevice logDevice, VkPhysicalDevice phyDevice, VkSurfaceKHR surface, VkFormat depthFormat,
                   VkQueue graphicsQueue, GraphicsInfo inf)
    : logDevice(logDevice), sc(logDevice, phyDevice, surface, inf.getFrameBufferSize),
      command(logDevice, phyDevice, inf.framesInFlight),
      vertex(BufferObj::optimizeForGPU(logDevice, phyDevice, sizeof(inf.verticesDatas[0]) * inf.verticesDatas.size(),
                                       inf.verticesDatas.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, command,
                                       graphicsQueue)),
      index(BufferObj::optimizeForGPU(logDevice, phyDevice, sizeof(inf.indicesDatas[0]) * inf.indicesDatas.size(),
                                      inf.indicesDatas.data(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, command,
                                      graphicsQueue))
{
    ShaderObj vertexShader(logDevice, inf.vertShaderLocation, VK_SHADER_STAGE_VERTEX_BIT);
    ShaderObj fragmentShader(logDevice, inf.fragShaderLocation, VK_SHADER_STAGE_FRAGMENT_BIT);
    initRenderPass(logDevice, sc, inf.multiSampleCount, depthFormat);
    pUniforms.resize(inf.framesInFlight);
    uniformMemoryPointers.resize(inf.framesInFlight);
    for (uint8_t i{0}; i < inf.framesInFlight; i++)
    {
        pUniforms[i] =
            new BufferObj(logDevice, phyDevice, sizeof(UniformBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vkMapMemory(logDevice, pUniforms[i]->memory, 0, sizeof(UniformBufferObject), 0, &uniformMemoryPointers[i]);
    }
}

void Graphics::initRenderPass(VkDevice logDevice, SwapChainObj &sc, VkSampleCountFlagBits multiSampleCount,
                              VkFormat depthFormat)
{
    VkAttachmentDescription color{
        .format = sc.format,
        .samples = multiSampleCount,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkAttachmentReference colorRef{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkAttachmentDescription depth{
        .format = depthFormat,
        .samples = multiSampleCount,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };
    VkAttachmentReference depthRef{
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    VkAttachmentDescription colorResolve{
        .format = sc.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    VkAttachmentReference colorRefResolve{
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    VkSubpassDescription subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorRef,
        .pResolveAttachments = multiSampleCount != VK_SAMPLE_COUNT_1_BIT ? &colorRefResolve : nullptr,
        .pDepthStencilAttachment = &depthRef,
    };
    VkSubpassDependency dependency{
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    };

    std::vector<VkAttachmentDescription> attachments = {color, depth};
    if (multiSampleCount != VK_SAMPLE_COUNT_1_BIT)
        attachments.push_back(colorResolve);

    VkRenderPassCreateInfo renderPassInfo{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };
    vkCheck(vkCreateRenderPass(logDevice, &renderPassInfo, nullptr, &renderPass), "failed to create RenderPass");
}

Graphics::~Graphics()
{
    vkDestroyRenderPass(logDevice, renderPass, nullptr);
    for (uint8_t i{0}; i < pUniforms.size(); i++)
    {
        delete pUniforms[i];
    }
}

BufferObj::BufferObj(VkDevice logDevice, VkPhysicalDevice phyDevice, VkDeviceSize size, VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags properties)
    : logDevice(logDevice)
{
    VkBufferCreateInfo bInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    vkCheck(vkCreateBuffer(logDevice, &bInfo, nullptr, &buffer), "failed to create vertex buffer");

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(logDevice, buffer, &memRequirements);

    // find memory type
    uint32_t vbMemoryType;
    bool vbMemoryTypeFound{false};
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(phyDevice, &memProperties);
    for (uint32_t i{0}; i < memProperties.memoryTypeCount; i++)
    {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            vbMemoryType = i;
            vbMemoryTypeFound = true;
        }
    }
    if (!vbMemoryTypeFound)
        throw std::runtime_error("failed to find memory type for buffer");

    VkMemoryAllocateInfo memInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = vbMemoryType,
    };
    vkCheck(vkAllocateMemory(logDevice, &memInfo, nullptr, &memory), "failed to allocate vertex buffer memory!");

    vkBindBufferMemory(logDevice, buffer, memory, 0);
}

BufferObj BufferObj::optimizeForGPU(VkDevice logDevice, VkPhysicalDevice phyDevice, VkDeviceSize size,
                                    const void *inputData, VkBufferUsageFlagBits usageBit, CommandObj &command,
                                    VkQueue graphicsQueue)
{
    // create buffer for storing
    BufferObj staging(logDevice, phyDevice, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *data;
    vkMapMemory(logDevice, staging.memory, 0, size, 0, &data);
    memcpy(data, inputData, (size_t)size);
    vkUnmapMemory(logDevice, staging.memory);

    BufferObj result(logDevice, phyDevice, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usageBit,
                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // copy buffers
    VkCommandBufferAllocateInfo tempCBInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command.Pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VkCommandBuffer tempCB;
    vkAllocateCommandBuffers(logDevice, &tempCBInfo, &tempCB);

    VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(tempCB, &beginInfo);

    VkBufferCopy copyRegion{
        .size = size,
    };
    vkCmdCopyBuffer(tempCB, staging.buffer, result.buffer, 1, &copyRegion);
    vkEndCommandBuffer(tempCB);

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &tempCB,
    };
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(logDevice, command.Pool, 1, &tempCB);

    return result;
}

BufferObj::~BufferObj()
{
    vkDestroyBuffer(logDevice, buffer, nullptr);
    vkFreeMemory(logDevice, memory, nullptr);
}

CommandObj::CommandObj(VkDevice logDevice, VkPhysicalDevice phyDevice, uint32_t framesInFlight) : logDevice(logDevice)
{
    // get graphics family
    uint32_t queueFamilyCount{0};
    vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamiliesList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &queueFamilyCount, queueFamiliesList.data());

    uint32_t graphicsFamily;
    bool foundGrFamily{false};
    for (uint32_t i{0}; i < queueFamilyCount && !foundGrFamily; ++i)
    {
        if (queueFamiliesList[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsFamily = i;
            foundGrFamily = true;
        }
    }
    if (!foundGrFamily)
        throw std::runtime_error("failed to find graphics family for command pool");

    VkCommandPoolCreateInfo poolInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphicsFamily,
    };
    vkCheck(vkCreateCommandPool(logDevice, &poolInfo, nullptr, &Pool), "failed to create command pool");

    Buffers.resize(framesInFlight);
    VkCommandBufferAllocateInfo cbInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = Pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = framesInFlight,
    };
    vkCheck(vkAllocateCommandBuffers(logDevice, &cbInfo, Buffers.data()), "failed to create command buffers");
}

void CommandObj::record()
{
}

CommandObj::~CommandObj()
{
    vkDestroyCommandPool(logDevice, Pool, nullptr);
}

SwapChainObj::SwapChainObj(VkDevice logDevice, VkPhysicalDevice phyDevice, VkSurfaceKHR surface,
                           std::function<void(uint32_t &width, uint32_t &height)> getFrameBufferSize)
    : logDevice(logDevice)
{
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

ShaderObj::ShaderObj(VkDevice logDevice, std::string shaderLocation, VkShaderStageFlagBits shaderStageBit)
    : logDevice(logDevice), module(VK_NULL_HANDLE)
{
    std::ifstream file(shaderLocation, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("failed to open shader file: " + shaderLocation);
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
