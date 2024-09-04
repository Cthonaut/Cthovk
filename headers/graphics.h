#pragma once

#include <algorithm>
#include <fstream>
#include <functional>
#include <set>
#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace Cthovk
{

struct SwapChainObj
{
    VkSwapchainKHR SwapChain;
    VkFormat format;
    VkExtent2D extent;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    VkDevice logDevice;

    SwapChainObj(VkDevice logDevice, VkPhysicalDevice phyDevice, VkSurfaceKHR surface,
                 std::function<void(uint32_t &width, uint32_t &height)> getFrameBufferSize);
    void initSwapChain(VkPhysicalDevice phyDevice, VkSurfaceKHR surface,
                       std::function<void(uint32_t &width, uint32_t &height)> getFrameBufferSize);
    void initImageViews();
    ~SwapChainObj();
};

struct ShaderObj
{
    VkPipelineShaderStageCreateInfo stageInfo;
    VkShaderModule module;
    VkDevice logDevice;

    ShaderObj(VkDevice logDevice, const char *shaderLocation, VkShaderStageFlagBits shaderStageBit);
    ~ShaderObj();
};

class Graphics
{
  public:
    Graphics(VkDevice logDevice, VkPhysicalDevice phyDevice, VkSurfaceKHR surface,
             std::function<void(uint32_t &width, uint32_t &height)> getFrameBufferSize, const char *vertShaderLocation,
             const char *fragShaderLocation);

  private:
    SwapChainObj sc;
    ShaderObj shaders[2];
    VkFormat depthformat;
};

} // namespace Cthovk
