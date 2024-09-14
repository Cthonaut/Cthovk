#include "../headers/application.h"

namespace Cthovk
{

Application::Application(DeviceInfo deviceInfo, GraphicsInfo graphicsInfo)
    : device(deviceInfo), graphics(device.logDevice, device.phyDevice, device.surface, device.findDepthFormat(),
                                   device.queues.graphics, graphicsInfo)
{
}

Application::~Application()
{
}

} // namespace Cthovk
