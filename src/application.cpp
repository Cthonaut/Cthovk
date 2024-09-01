#include "../headers/application.h"

namespace Cthovk
{

Application::Application(bool useVL, std::vector<const char *> vl, std::vector<const char *> windowExt,
                         std::function<void(VkInstance instance, VkSurfaceKHR *surface)> initSurface,
                         std::vector<const char *> deviceExtensions)
    : device(Device(useVL, vl, windowExt, initSurface, deviceExtensions))
{
}

void Application::initDevice()
{
}

void Application::initGraphics()
{
}

Application::~Application()
{
}

} // namespace Cthovk
