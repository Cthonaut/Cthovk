#pragma once

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace Cthovk
{
class Device
{
  public:
    Device(bool enableVL, std::vector<const char *> vl, std::vector<const char *> windowExt,
           std::function<void(VkInstance instance, VkSurfaceKHR *surface)> initSurface);
    ~Device();

  private:
    VkInstance instance;
    VkSurfaceKHR surface;

    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
    VkDebugUtilsMessengerEXT messenger{nullptr};

    void initInstance(bool enableValidationLayers, std::vector<const char *> validationLayers,
                      std::vector<const char *> windowApiExtensions);
    void initValidationLayers();
};
} // namespace Cthovk
