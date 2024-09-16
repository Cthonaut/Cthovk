#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "../headers/application.h"

namespace std
{
template <> struct hash<Cthovk::Vertex>
{
    size_t operator()(Cthovk::Vertex const &vertex) const
    {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1);
    }
};
} // namespace std

void loadModel(std::vector<Cthovk::Vertex> *vertices, std::vector<uint32_t> *indices, std::string path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
        throw std::runtime_error(warn + err);

    std::unordered_map<Cthovk::Vertex, uint32_t> uniqueVertices{};
    for (uint32_t i{0}; i < shapes.size(); i++)
    {
        for (uint32_t j{0}; j < shapes[i].mesh.indices.size(); j++)
        {
            Cthovk::Vertex vertex{
                .pos =
                    {
                        attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index],
                        attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 1],
                        attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 2],
                    },
                .color =
                    {
                        // random coloring function
                        std::abs(attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 1]) *
                                (std::abs(attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 0]) +
                                 std::abs(attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 2])) +
                            0.2f,
                        0.0f,
                        std::abs(attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 1]) *
                                (std::abs(attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 2]) +
                                 std::abs(attrib.vertices[3 * shapes[i].mesh.indices[j].vertex_index + 0])) +
                            0.2f,
                    },
            };

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices->size());
                vertices->push_back(vertex);
            }
            if (indices != nullptr)
                indices->push_back(uniqueVertices[vertex]);
        }
    }
}

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

    std::function<bool()> terminateCheck = [&]() {
        if (glfwWindowShouldClose(window))
            return true;
        glfwPollEvents();
        return false;
    };
};

int main()
{
    Cthovk::Model torus;
    loadModel(&torus.verticesData, &torus.indicesData, "models/torus.obj");

    Cthovk::Model torusZ(torus);
    torusZ.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    torusZ.updateUBO = [&](Cthovk::UniformBufferObject &ubo, Cthovk::SwapChainObj &sc) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), sc.extent.width / (float)sc.extent.height, 0.01f, 100.0f);
        ubo.proj[1][1] *= -1;
    };

    Cthovk::Model torusY(torus);
    torusY.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    torusY.updateUBO = [&](Cthovk::UniformBufferObject &ubo, Cthovk::SwapChainObj &sc) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), sc.extent.width / (float)sc.extent.height, 0.01f, 100.0f);
        ubo.proj[1][1] *= -1;
    };

    Cthovk::Model torusX(torus);
    torusX.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
    torusX.updateUBO = [&](Cthovk::UniformBufferObject &ubo, Cthovk::SwapChainObj &sc) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.proj = glm::perspective(glm::radians(45.0f), sc.extent.width / (float)sc.extent.height, 0.01f, 100.0f);
        ubo.proj[1][1] *= -1;
    };

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
        .multiSampleCount = VK_SAMPLE_COUNT_16_BIT,
        .framesInFlight = 2,
        .models = {torusX, torusY, torusZ},
        .clearValue = {{{0.02f, 0.0f, 0.03f}}},
    };
    Cthovk::Application app(deviceInfo, graphicsInfo, glfw.terminateCheck);

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
