#pragma once

#include "device.h"
#include "graphics.h"

namespace Cthovk
{
class Application
{
  public:
    void run() {};

    Application(DeviceInfo deviceInfo, GraphicsInfo graphicsInfo);
    ~Application();

  private:
    Device device;
    Graphics graphics;
};
} // namespace Cthovk
