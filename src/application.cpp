#include "../headers/application.h"

namespace Cthovk
{

Application::Application(bool useVL, std::vector<const char *> vl, std::vector<const char *> windowExt)
    : device(Device(useVL, vl, windowExt))
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
