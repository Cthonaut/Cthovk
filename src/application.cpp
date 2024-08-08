#include "../headers/application.h"

namespace Cthovk
{

void Application::run()
{
    initVulkan();
    loop();
    cleanup();
}

void Application::initVulkan()
{
    device.initInstance();
    if (device.enableValidationLayers)
        device.initValidationLayers();
    device.initSurface();
}

void Application::loop()
{
}

void Application::cleanup()
{
    device.cleanup();
}

} // namespace Cthovk
