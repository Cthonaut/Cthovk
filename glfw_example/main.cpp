#include <iostream>

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
};

int main()
{
    GLFW glfw(800, 800);
    Cthovk::Application app(true, {"VK_LAYER_KHRONOS_validation"}, glfw.getExtensions());

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
