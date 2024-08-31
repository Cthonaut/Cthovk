#include "../headers/device.h"

namespace Cthovk
{

void vkCheck(bool result, const char *error)
{
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(error);
    }
};

Device::Device(bool enableVL, std::vector<const char *> vl, std::vector<const char *> windowExt)
{
    initInstance(enableVL, vl, windowExt);
};

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

Device::~Device()
{
    vkDestroyInstance(instance, nullptr);
};

} // namespace Cthovk
