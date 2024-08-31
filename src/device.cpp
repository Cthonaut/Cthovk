#include "../headers/device.h"

namespace Cthovk
{

void vkCheck(bool result, const char *error)
{
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(error);
    }
}

Device::Device(bool enableVL, std::vector<const char *> vl, std::vector<const char *> windowExt)
{
    initInstance(enableVL, vl, windowExt);
    if (enableVL)
        initValidationLayers();
}

void Device::initInstance(bool enableValidationLayers, std::vector<const char *> validationLayers,
                          std::vector<const char *> windowApiExtensions)
{
    // check for validation Layers
    if (enableValidationLayers)
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        uint8_t commonLayer{0};
        for (uint8_t i{0}; i < validationLayers.size(); i++)
        {
            for (uint16_t j{0}; j < layerCount; j++)
            {
                if (std::strcmp(validationLayers[i], availableLayers[j].layerName) == 0)
                    commonLayer++;
            }
        }
        if (commonLayer != validationLayers.size())
            throw std::runtime_error("validationLayers not found");

        windowApiExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // required for validation layers
    }

    VkInstanceCreateInfo instanceInfo{

        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .enabledLayerCount = enableValidationLayers ? static_cast<uint32_t>(validationLayers.size()) : 0,
        .ppEnabledLayerNames = enableValidationLayers ? validationLayers.data() : nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(windowApiExtensions.size()),
        .ppEnabledExtensionNames = windowApiExtensions.data(),
    };
    vkCheck(vkCreateInstance(&instanceInfo, nullptr, &instance), "failed to create instance");
}

void Device::initValidationLayers()
{
    // get the functions for Validation Layers
    vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    if (vkCreateDebugUtilsMessengerEXT == nullptr)
    {
        throw std::runtime_error("failed to get validation layers function: vkCreateDebugUtilsMessengerEXT");
    }
    if (vkDestroyDebugUtilsMessengerEXT == nullptr)
    {
        throw std::runtime_error("failed to get validation layers function : vkDestroyDebugUtilsMessengerEXT");
    }

    PFN_vkDebugUtilsMessengerCallbackEXT callback =
        [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
           const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) -> VkBool32 {
        std::string prefix;
        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            prefix = "VL_ERROR: ";
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            prefix = "VL_WARNING: ";
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        {
            prefix = "VL_INFO: ";
        }
        else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        {
            prefix = "VL_DEBUG: ";
        }
        std::cerr << prefix << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    };

    // create callback
    VkDebugUtilsMessengerCreateInfoEXT callbackCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = callback,
    };
    vkCheck(vkCreateDebugUtilsMessengerEXT(instance, &callbackCreateInfo, nullptr, &messenger),
            "failed to setup callback");
}

Device::~Device()
{
    if (messenger != nullptr)
    {
        vkDestroyDebugUtilsMessengerEXT(instance, messenger, nullptr);
    }
    vkDestroyInstance(instance, nullptr);
}

} // namespace Cthovk
