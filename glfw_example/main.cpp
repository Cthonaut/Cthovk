#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "../headers/application.h"

class GLFW
{
  public:
    GLFWwindow *window;

    GLFW(int width, int height)
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        window = glfwCreateWindow(width, height, "Cthovk_glfw", NULL, NULL);
    }

    ~GLFW()
    {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    std::vector<const char *> getExtensions()
    {
        uint32_t glfwExtensionCount{0};
        const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        return extensions;
    }

    std::function<void(VkInstance instance, VkSurfaceKHR *surface)> initSurface = [&](VkInstance instance,
                                                                                      VkSurfaceKHR *surface) {
        glfwCreateWindowSurface(instance, window, nullptr, surface);
    };

    std::function<void(uint32_t &, uint32_t &)> getFrameBufferSize = [&](uint32_t &width, uint32_t &height) {
        int widthS, heightS;
        glfwGetFramebufferSize(window, &widthS, &heightS);
        width = static_cast<uint32_t>(widthS);
        height = static_cast<uint32_t>(heightS);
    };
};

int main()
{
    GLFW glfw(800, 800);
    Cthovk::DeviceInfo deviceInfo{
        .enableVL = true,
        .vl = {"VK_LAYER_KHRONOS_validation"},
        .deviceExt = {VK_KHR_SWAPCHAIN_EXTENSION_NAME},
        .windowExt = glfw.getExtensions(),
        .initSurface = glfw.initSurface,
    };
    Cthovk::GraphicsInfo graphicsInfo{
        .getFrameBufferSize = glfw.getFrameBufferSize,
        .vertShaderLocation = "shaders/shader.vert.spv",
        .fragShaderLocation = "shaders/shader.frag.spv",
        .multiSampleCount = VK_SAMPLE_COUNT_1_BIT,
        .framesInFlight = 1,
        .verticesDatas = {{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                          {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},
                          {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}},
                          {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}}},
        .indicesDatas = {0, 1, 2, 2, 3, 0},
    };
    Cthovk::Application app(deviceInfo, graphicsInfo);

    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
