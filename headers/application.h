#pragma once

#include "device.h"
#include "graphics.h"

namespace Cthovk
{
class Application
{
  public:
    void run() {};

    Application(bool useVL, std::vector<const char *> vl, std::vector<const char *> windowExt,
                std::function<void(VkInstance instance, VkSurfaceKHR *surface)> initSurface,
                std::function<void(uint32_t &, uint32_t &)> getFrameBufferSize,
                std::vector<const char *> deviceExtensions, const char *vertShaderLocation,
                const char *fragShaderLocation);
    ~Application();

  private:
    Device device;
    Graphics graphics;
};
} // namespace Cthovk
