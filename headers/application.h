#pragma once

#include "device.h"

namespace Cthovk
{
class Application
{
  public:
    void run()
    {
        initDevice();
        initGraphics();
    };

    Application(bool useVL, std::vector<const char *> vl, std::vector<const char *> windowExt);
    ~Application();

  private:
    Device device;

    void initDevice();
    void initGraphics();
};
} // namespace Cthovk
