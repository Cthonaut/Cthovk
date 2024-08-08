#pragma once

#include <algorithm>
#include <cstring>
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

    bool enableValidationLayers = false;
    std::vector<const char *> validationLayers;
    std::vector<const char *> windowApiExtensions;

    void initInstance();
    void initValidationLayers();
    void cleanup();
};

void vkCheck(bool result, const char *error = "");

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
