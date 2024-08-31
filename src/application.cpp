#include "../headers/application.h"

namespace Cthovk
{

Application::Application(bool useVL, std::vector<const char *> vl, std::vector<const char *> windowExt,
                         std::function<void(VkInstance instance, VkSurfaceKHR *surface)> initSurface)
    : device(Device(useVL, vl, windowExt, initSurface))
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
