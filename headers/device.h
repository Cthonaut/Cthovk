#pragma once

#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace Cthovk
{
class Device
{
  public:
    Device(bool enableVL, std::vector<const char *> vl, std::vector<const char *> windowExt);
    ~Device();

  private:
    VkInstance instance;

    void initInstance(bool enableValidationLayers, std::vector<const char *> validationLayers,
                      std::vector<const char *> windowApiExtensions);
};
} // namespace Cthovk
