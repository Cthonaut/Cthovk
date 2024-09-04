#pragma once

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace Cthovk
{
class Device
{
  public:
    VkSurfaceKHR surface;
    VkPhysicalDevice phyDevice{VK_NULL_HANDLE};
    VkDevice logDevice;

    VkFormat findDepthFormat();

    Device(bool enableVL, std::vector<const char *> vl, std::vector<const char *> windowExt,
           std::function<void(VkInstance instance, VkSurfaceKHR *surface)> initSurface,
           std::vector<const char *> deviceExt);
    ~Device();

  private:
    VkInstance instance;

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
    VkDebugUtilsMessengerEXT messenger{nullptr};

    void initInstance(bool enableValidationLayers, std::vector<const char *> validationLayers,
                      std::vector<const char *> windowApiExtensions);
    void initValidationLayers();
    uint32_t rateGPU(VkPhysicalDevice device, std::vector<const char *> deviceExt);
    void selectGPU(std::vector<const char *> deviceExt);
    void initLogDevice(bool useVL, std::vector<const char *> deviceExt, std::vector<const char *> validationLayers);
};
} // namespace Cthovk
