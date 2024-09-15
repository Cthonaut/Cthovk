#include "../headers/application.h"

namespace Cthovk
{

Application::Application(DeviceInfo deviceInfo, GraphicsInfo graphicsInfo, std::function<bool()> terminateCheck)
    : device(deviceInfo), graphics(device.logDevice, device.phyDevice, device.surface, device.findDepthFormat(),
                                   device.queues.graphics, graphicsInfo),
      terminateCheck(terminateCheck), grInfo(graphicsInfo)
{
}

void Application::run()
{
    while (!terminateCheck())
    {
        graphics.draw(device.phyDevice, device.surface, grInfo, device.queues.graphics, device.queues.present);
    }
}

Application::~Application()
{
}

} // namespace Cthovk
