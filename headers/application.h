#pragma once

#include "device.h"
#include "graphics.h"

namespace Cthovk
{
class Application
{
  public:
    void run();

    Application(DeviceInfo deviceInfo, GraphicsInfo graphicsInfo, std::function<bool()> terminateCheck);
    ~Application();

  private:
    std::function<bool()> terminateCheck;
    Device device;
    Graphics graphics;
    GraphicsInfo grInfo;
};
} // namespace Cthovk
