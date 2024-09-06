#include "../headers/application.h"

namespace Cthovk
{

Application::Application(bool useVL, std::vector<const char *> vl, std::vector<const char *> deviceExtensions,
                         std::vector<const char *> windowExt,
                         std::function<void(VkInstance instance, VkSurfaceKHR *surface)> initSurface,
                         std::function<void(uint32_t &, uint32_t &)> getFrameBufferSize, const char *vertShaderLocation,
                         const char *fragShaderLocation, VkSampleCountFlagBits multi)
    : device(useVL, vl, windowExt, initSurface, deviceExtensions),
      graphics(device.logDevice, device.phyDevice, device.surface, getFrameBufferSize, vertShaderLocation,
               fragShaderLocation, multi, device.findDepthFormat())
{
}

Application::~Application()
{
}

} // namespace Cthovk
