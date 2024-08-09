#include "../headers/graphicspl.h"

namespace Cthovk
{
ShaderObj::ShaderObj(VkDevice logDevice, const char *shaderLocation, VkShaderStageFlagBits shaderStageBit)
{
    std::ifstream file(shaderLocation, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("failed to open shader file");
    uint32_t fileSize = (uint32_t)file.tellg();
    std::vector<char> shaderCode(fileSize);
    file.seekg(0);
    file.read(shaderCode.data(), fileSize);
    file.close();

    VkShaderModuleCreateInfo shaderModuleInfo{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = fileSize,
        .pCode = reinterpret_cast<const uint32_t *>(shaderCode.data()),
    };
    vkCheck(vkCreateShaderModule(logDevice, &shaderModuleInfo, nullptr, &module), "failed to create shaders");

    VkPipelineShaderStageCreateInfo shaderStageInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = shaderStageBit,
        .module = module,
        .pName = "main",
    };
    stageInfo = shaderStageInfo;
}

void ShaderObj::cleanup(VkDevice logDevice)
{
    vkDestroyShaderModule(logDevice, module, nullptr);
}
} // namespace Cthovk
