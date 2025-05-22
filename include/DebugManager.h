#pragma once
#ifndef DEBUG_MANAGER_H
#define DEBUG_MANAGER_H

#include "config.h"
#include "../include/helperFuncs.h"

class DebugManager {
public: 
    //Allows a user to set a memory objects name for validation messaging
    inline void setObjectName(VkDevice device, uint64_t handle, VkObjectType type, const char* name) {
        static PFN_vkSetDebugUtilsObjectNameEXT func =
            reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT")); //THIS EXTENSION NEEDS TO BE ENABLED IN REQUIRED EXTENSIONS STRUCT IN DEVICES

        if (func) {
            VkDebugUtilsObjectNameInfoEXT info{};
            info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            info.objectType = type;
            info.objectHandle = handle;
            info.pObjectName = name;
            func(device, &info);
        }
    }

    void setupDebugMessenger(VkInstance instance) {
        // If we are not in debug -> messenger will not be created
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create debug messenger");
        }
    };

    //Called before creating an instance to ensure validation layers are supported
    bool checkValidationLayerSupport() {
        //Get all possible VkInstance layer count
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        //Creates vec availableLayers of size layerCount 
        std::vector<VkLayerProperties> availableLayers(layerCount);
        //Assign instance layers to availableLayers
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        // For each layer in our validation layer struct, 
        // it will be checked that it exists in availableLayers
        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            };

            if (!layerFound) {
                return false;
            }
        };

        return true;
    };

    //Populates a createInfo struct for a debug utility messenger
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    void cleanup(VkInstance instance) {
        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
        };
    }

    const std::vector<const char*>& getValidationLayers() const { return validationLayers; }

private: 
    VkDebugUtilsMessengerEXT debugMessenger;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    // == EXTENSION LOADER FUNCTIONS == 
    //This function loads and executes the vkCreateDebugUtilsMessengerEXT function
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkDebugUtilsMessengerEXT* pDebugMessenger
    );

    //This function loads and executes the vkDestroyDebugUtilsMessengerEXT function
    void DestroyDebugUtilsMessengerEXT(VkInstance instance,
        VkDebugUtilsMessengerEXT debugMessenger,
        const VkAllocationCallbacks* pAllocator
    );
};

#endif