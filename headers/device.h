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

  public:
    VkInstance instance;

    bool enableValidationLayers = false;
    std::vector<const char *> validationLayers;
    std::vector<const char *> windowApiExtensions;

    void initInstance();
};

void vkCheck(bool result, const char *error = "");

} // namespace Cthovk
