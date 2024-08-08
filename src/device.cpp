#include "../headers/device.h"

namespace Cthovk
{

void Device::initInstance()
{
    if (enableValidationLayers)
    {
        // sort Validation Layers for comparing
        std::sort(validationLayers.begin(), validationLayers.end(), standardCstringComp);
        uint32_t vlCount = validationLayers.size();

        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        std::vector<const char *> availableLayersNames(layerCount);
        for (uint32_t i{0}; i < layerCount; i++)
        {
            availableLayersNames[i] = availableLayers[i].layerName;
        }
        std::sort(availableLayersNames.begin(), availableLayersNames.end(),
                  standardCstringComp); // sorting for comparing

        // comparing
        uint32_t layersMatching{0};
        for (uint32_t i{0}; i < layerCount && layersMatching < vlCount; i++)
        {
            uint32_t vlNameLength = strlen(validationLayers[layersMatching]);
            if (strlen(availableLayersNames[i]) != vlNameLength)
                continue;

            bool skip = false;
            for (uint32_t j{0}; j < vlNameLength; j++)
            {
                if (validationLayers[layersMatching][j] != availableLayersNames[i][j])
                {
                    skip = true;
                    break;
                }
            }
            if (skip)
                continue;
            layersMatching++;
        }
        if (layersMatching != vlCount || vlCount > layerCount)
            throw std::runtime_error("validation layers not available");

        windowApiExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME); // required for validation layers
    }

    VkInstanceCreateInfo instanceInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .enabledLayerCount = 0,
        .enabledExtensionCount = static_cast<uint32_t>(windowApiExtensions.size()),
        .ppEnabledExtensionNames = windowApiExtensions.data(),
    };
    if (enableValidationLayers)
    {
        instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceInfo.ppEnabledLayerNames = validationLayers.data();
    }
    vkCheck(vkCreateInstance(&instanceInfo, nullptr, &instance), "failed to create instance");
}

void vkCheck(bool result, const char *error)
{
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error(error);
    }
};

} // namespace Cthovk
