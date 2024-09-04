#include "../headers/application.h"

namespace Cthovk
{

Application::Application(bool useVL, std::vector<const char *> vl, std::vector<const char *> windowExt,
                         std::function<void(VkInstance instance, VkSurfaceKHR *surface)> initSurface,
                         std::function<void(uint32_t &, uint32_t &)> getFrameBufferSize,
                         std::vector<const char *> deviceExtensions, const char *vertShaderLocation,
                         const char *fragShaderLocation)
    : device(useVL, vl, windowExt, initSurface, deviceExtensions),
      graphics(device.logDevice, device.phyDevice, device.surface, getFrameBufferSize, vertShaderLocation,
               fragShaderLocation)
{
}

Application::~Application()
{
}

} // namespace Cthovk
