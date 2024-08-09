#pragma once

#include "graphicspl.h"

namespace Cthovk
{

struct SwapChainObj
{
    VkSwapchainKHR SwapChain;
    VkFormat format;
    VkExtent2D extent;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;

    void initSwapChain(Device *pDevice);
    void initImageViews(VkDevice logDevice);

    void cleanup(VkDevice logDevice)
    {
        for (size_t i{0}; i < imageViews.size(); i++)
        {
            vkDestroyImageView(logDevice, imageViews[i], nullptr);
        }
        vkDestroySwapchainKHR(logDevice, SwapChain, nullptr);
    }
};

class Application
{
  private:
    SwapChainObj scResources;
    ShaderObj *vertexShader;
    ShaderObj *fragmentShader;

    void initVulkan();
    void loop();
    void cleanup();

  public:
    Device device;
    GraphicsPipeline graphics;

    std::string vertShaderLocation;
    std::string fragShaderLocation;

    void run();
};

} // namespace Cthovk
