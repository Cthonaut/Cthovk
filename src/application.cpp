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
}

void Application::loop()
{
}

void Application::cleanup()
{
}

} // namespace Cthovk
