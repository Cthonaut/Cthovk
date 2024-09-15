#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <set>
#include <vector>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

// Forward Declaration for CommandObj
struct PipelineObj;
struct BufferObj;

struct CommandObj
{
    VkCommandPool Pool;
    std::vector<VkCommandBuffer> Buffers;
    VkClearValue clearValue = {{{0.005f, 0.0f, 0.01f, 1.0f}}};
    VkDevice logDevice;

    CommandObj(VkDevice logDevice, VkPhysicalDevice phyDevice, uint32_t framesInFlight);
    ~CommandObj();

    void record(PipelineObj *pipeline, VkDescriptorSet descriptorSet, BufferObj &vertexBuf, BufferObj &indexBuf,
                VkRenderPass renderPass, VkFramebuffer frameBuffer, SwapChainObj &sc, uint32_t currentcb);
};

struct BufferObj
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    uint32_t Count; // optinal really
    VkDevice logDevice;

    BufferObj(VkDevice logDevice, VkPhysicalDevice phyDevice, VkDeviceSize size, VkBufferUsageFlags usage,
              VkMemoryPropertyFlags properties, uint32_t count = 0);
    ~BufferObj();

    static BufferObj optimizeForGPU(VkDevice logDevice, VkPhysicalDevice phyDevice, VkDeviceSize size,
                                    const void *inputData, VkBufferUsageFlagBits usageBit, CommandObj &command,
                                    VkQueue graphicsQueue, uint32_t count = 0);
};

struct ImageObj
{
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkDevice logDevice;

    ImageObj(VkDevice logDevice, VkPhysicalDevice phyDevice, VkExtent2D extent, VkSampleCountFlagBits samples,
             VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags imageAspectFlag);
    ~ImageObj();
};

struct DescriptorPoolObj
{
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout descriptorlayout;
    VkDevice logDevice;

    DescriptorPoolObj(VkDevice logDevice, uint32_t fIF);
    ~DescriptorPoolObj();
};

struct PipelineObj
{
    VkPipelineLayout layout;
    VkPipeline pl;
    VkPrimitiveTopology topology;
    VkDevice logDevice;

    PipelineObj(VkDevice logDevice, VkRenderPass renderPass, DescriptorPoolObj &pool, SwapChainObj &sc,
                std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos, VkSampleCountFlagBits multi,
                VkPrimitiveTopology topology);

    ~PipelineObj();
};

struct SyncObj
{
    std::vector<VkSemaphore> imageSemaphores;
    std::vector<VkSemaphore> renderSemaphores;
    std::vector<VkFence> processFences;
    VkDevice logDevice;

    SyncObj(VkDevice logDevice, uint32_t fIF);
    ~SyncObj();

    void resize(uint32_t newSize)
    {
        imageSemaphores.resize(newSize);
        renderSemaphores.resize(newSize);
        processFences.resize(newSize);
    }
};

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;

    static std::vector<VkVertexInputAttributeDescription> getAttributes()
    {
        std::vector<VkVertexInputAttributeDescription> vertexInputAttributes(2);
        VkVertexInputAttributeDescription temp{
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
        };
        vertexInputAttributes[0] = temp;
        vertexInputAttributes[0].location = 0;
        vertexInputAttributes[0].offset = offsetof(Vertex, pos);
        vertexInputAttributes[1] = temp;
        vertexInputAttributes[1].location = 1;
        vertexInputAttributes[1].offset = offsetof(Vertex, color);
        return vertexInputAttributes;
    };

    bool operator==(const Vertex &other) const
    {
        return pos == other.pos && color == other.color;
    }
};

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

struct GraphicsInfo
{
    std::function<void(uint32_t &width, uint32_t &height)> getFrameBufferSize;
    std::string vertShaderLocation;
    std::string fragShaderLocation;
    VkSampleCountFlagBits multiSampleCount;
    uint32_t framesInFlight;
    std::vector<Vertex> verticesData;
    std::vector<uint32_t> indicesData;
};

class Graphics
{
  public:
    Graphics(VkDevice logDevice, VkPhysicalDevice phyDevice, VkSurfaceKHR surface, VkFormat depthFormat,
             VkQueue graphicsQueue, GraphicsInfo inf);
    ~Graphics();

    void draw(VkPhysicalDevice phyDevice, VkSurfaceKHR surface, GraphicsInfo inf, VkQueue graphicsQueue,
              VkQueue presentQueue);

  private:
    VkDevice logDevice;
    SwapChainObj sc;
    std::vector<ShaderObj> shaders;
    VkRenderPass renderPass;
    CommandObj command;
    BufferObj vertex;
    BufferObj index;
    std::vector<BufferObj *> pUniforms;
    std::vector<void *> uniformMemoryPointers;
    ImageObj depth;
    ImageObj color;
    DescriptorPoolObj pool;
    PipelineObj *pipeline;
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkFramebuffer> frameBuffers;
    SyncObj sync;
    uint32_t currentFrame;
    bool reinitSC{false};
    VkFormat depthFormat;

    void initRenderPass(VkDevice logDevice, VkSampleCountFlagBits multiSampleCount, VkFormat depthFormat);
    void initDescriptorSets(VkDevice logDevice, uint32_t fIF);
    void initFrameBuffers(VkDevice logDevice, VkSampleCountFlagBits multiSampleCount);
    void reinitSwapChain(VkPhysicalDevice phyDevice, VkSurfaceKHR surface, GraphicsInfo inf);
};

} // namespace Cthovk
