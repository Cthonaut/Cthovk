#pragma once

#include <fstream>

#include "device.h"

namespace Cthovk
{

struct ShaderObj
{
    VkPipelineShaderStageCreateInfo stageInfo;
    VkShaderModule module;

    ShaderObj(VkDevice logDevice, const char *shaderLocation, VkShaderStageFlagBits shaderStageBit);
    void cleanup(VkDevice logDevice);
};

class GraphicsPipeline
{
  public:
    Device *pDevice;

  private:
};

} // namespace Cthovk
