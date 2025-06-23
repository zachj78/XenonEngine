#pragma once 
#ifndef MATERIAL_H
#define MATERIAL_H

#include "Utils/config.h"
#include "Builders/DescriptorBuilder.h"

class Material {
public:
    Material(std::string name,
        std::shared_ptr<Image> textureImage,
        VkDescriptorSetLayout descriptorSetLayout,
        VkDevice logicalDevice
    )
        : name(name), textureImage(textureImage), logicalDevice(logicalDevice) {
        std::cout << "Created material : " << name << std::endl;
    }

    void setDescriptorSets(std::vector<VkDescriptorSet> sets) {
        std::cout << "Setting mat descriptor set : [" << name << "]" << std::endl;
        descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        descriptorSets = sets;
    }

    std::shared_ptr<Image> getTextureImage() const { return textureImage; };
    const std::string getName() const { return name; };
    std::vector<VkDescriptorSet> getDescriptorSets() { return descriptorSets; };

private:

    //Injected device
    VkDevice logicalDevice;

    std::string name;
    std::shared_ptr<Image> textureImage;

    //ONLY USED IF BINDLESS TEXTURES ARE NOT SUPPORTED
    std::vector<VkDescriptorSet> descriptorSets;
};

#endif