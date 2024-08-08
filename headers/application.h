#pragma once

#include "device.h"

namespace Cthovk
{

class Application
{
  private:
    void initVulkan();
    void loop();
    void cleanup();

  public:
    Device device;

    void run();
};

} // namespace Cthovk
