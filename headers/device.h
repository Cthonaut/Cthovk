#pragma once

#include <algorithm>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdint.h>
#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace Cthovk
{

class Device
{
  private:
    std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
    std::vector<const char *> deviceExt;

    static bool standardCstringComp(const char *a, const char *b)
    {
        return std::strcmp(a, b) < 0;
    };

    VkDebugReportCallbackEXT callback;

    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
    PFN_vkDebugReportMessageEXT vkDebugReportMessageEXT;
    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;

  public:
    VkInstance instance;
    VkSurfaceKHR surface;
    VkPhysicalDevice phyDevice;

    VkSampleCountFlags multiCounts;
    bool enableValidationLayers = false;
    std::vector<const char *> windowApiExtensions;

    void initInstance();
    void initValidationLayers();
    std::function<void()> initSurface;
    void selectGPU();
    void cleanup();
};

void vkCheck(bool result, const char *error = "");
bool getQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface, std::vector<uint32_t> *queueFamilies,
                      bool noDuplicates = false);

static VKAPI_ATTR VkBool32 VKAPI_CALL callbackMessage(VkDebugReportFlagsEXT flags,
                                                      VkDebugReportObjectTypeEXT objectType, uint64_t object,
                                                      size_t location, int32_t messageCode, const char *pLayerPrefix,
                                                      const char *pMessage, void *pUserData)
{
    std::string prefix;
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
    {
        prefix = "VL_ERROR: ";
    }
    else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
    {
        prefix = "VL_WARNING: ";
    }
    else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
    {
        prefix = "VL_INFO: ";
    }
    else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
    {
        prefix = "VL_DEBUG: ";
    }

    std::cerr << prefix << pMessage << std::endl;
    return VK_FALSE;
}

} // namespace Cthovk
