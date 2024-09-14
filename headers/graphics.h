#pragma once

#include <algorithm>
#include <cstring>
#include <fstream>
#include <functional>
#include <set>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace Cthovk
{

struct ShaderObj
{
    VkPipelineShaderStageCreateInfo stageInfo;
    VkShaderModule module;
    VkDevice logDevice;

    ShaderObj(VkDevice logDevice, std::string shaderLocation, VkShaderStageFlagBits shaderStageBit);
    ~ShaderObj();
};

struct SwapChainObj
{
    VkSwapchainKHR SwapChain;
    VkFormat format;
    VkExtent2D extent;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    VkDevice logDevice;

    SwapChainObj(VkDevice logDevice, VkPhysicalDevice phyDevice, VkSurfaceKHR surface,
                 std::function<void(uint32_t &width, uint32_t &height)> getFrameBufferSize);
    void initSwapChain(VkPhysicalDevice phyDevice, VkSurfaceKHR surface,
                       std::function<void(uint32_t &width, uint32_t &height)> getFrameBufferSize);
    void initImageViews();
    ~SwapChainObj();
};

struct CommandObj
{
    VkCommandPool Pool;
    std::vector<VkCommandBuffer> Buffers;
    VkClearValue clearValue = {{{0.005f, 0.0f, 0.01f, 1.0f}}};
    VkDevice logDevice;

    CommandObj(VkDevice logDevice, VkPhysicalDevice phyDevice, uint32_t framesInFlight);
    ~CommandObj();

    void record();
};

struct BufferObj
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDevice logDevice;

    BufferObj(VkDevice logDevice, VkPhysicalDevice phyDevice, VkDeviceSize size, VkBufferUsageFlags usage,
              VkMemoryPropertyFlags properties);
    ~BufferObj();

    static BufferObj optimizeForGPU(VkDevice logDevice, VkPhysicalDevice phyDevice, VkDeviceSize size,
                                    const void *inputData, VkBufferUsageFlagBits usageBit, CommandObj &command,
                                    VkQueue graphicsQueue);
};

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
};

struct UniformBufferObject
{
};

struct GraphicsInfo
{
    std::function<void(uint32_t &width, uint32_t &height)> getFrameBufferSize;
    std::string vertShaderLocation;
    std::string fragShaderLocation;
    VkSampleCountFlagBits multiSampleCount;
    uint32_t framesInFlight;
    std::vector<Vertex> verticesDatas;
    std::vector<uint32_t> indicesDatas;
};

class Graphics
{
  public:
    Graphics(VkDevice logDevice, VkPhysicalDevice phyDevice, VkSurfaceKHR surface, VkFormat depthFormat,
             VkQueue graphicsQueue, GraphicsInfo inf);
    ~Graphics();

  private:
    VkDevice logDevice;
    SwapChainObj sc;
    std::vector<ShaderObj> shaders;
    VkFormat depthformat;
    VkRenderPass renderPass;
    CommandObj command;
    BufferObj vertex;
    BufferObj index;
    std::vector<BufferObj *> pUniforms;
    std::vector<void *> uniformMemoryPointers;

    void initRenderPass(VkDevice logDevice, SwapChainObj &sc, VkSampleCountFlagBits multiSampleCount,
                        VkFormat depthFormat);
};

} // namespace Cthovk
