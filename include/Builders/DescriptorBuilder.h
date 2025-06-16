#pragma once 
#ifndef DESCRIPTOR_BUILDER_H
#define DESCRIPTOR_BUILDER_H

#include "Utils/config.h"
#include "Managers/Buffer.h"

struct UniformBufferInfo {
	void* mappedPtr = nullptr;
	std::shared_ptr<Buffer> buffer;
};

class DescriptorBuilder {
public:
	// Move constructor & assignment
	DescriptorBuilder(DescriptorBuilder&&) = default;
	DescriptorBuilder& operator=(DescriptorBuilder&&) = default;

	// Deleted copy constructor & assignment to prevent unsafe usage
	DescriptorBuilder(const DescriptorBuilder&) = delete;
	DescriptorBuilder& operator=(const DescriptorBuilder&) = delete;

	static DescriptorBuilder begin(VkDevice device);

	DescriptorBuilder& bindBuffer(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags, VkBuffer buffer, VkDeviceSize range = VK_WHOLE_SIZE);
	DescriptorBuilder& bindImage(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags, VkImageView imageView, VkSampler sampler);
	DescriptorBuilder& bindImageArray(uint32_t binding,
		VkDescriptorType type,
		VkShaderStageFlags stageFlags,
		const std::vector<VkImageView> imageViews,
		const std::vector<VkSampler> samplers,
		VkDescriptorBindingFlags flags);


	DescriptorBuilder& setLayoutFlags(VkDescriptorSetLayoutCreateFlags flags = 0);

	bool buildLayout(VkDescriptorSetLayout& layout);
	bool buildSet(VkDescriptorSetLayout& layout, VkDescriptorSet& set, VkDescriptorPool pool);

private: 
	DescriptorBuilder() = default;

	VkDevice device; 
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	std::vector<VkWriteDescriptorSet> writes;
	std::vector<VkDescriptorBufferInfo> bufferInfos;
	std::vector<std::vector<VkDescriptorImageInfo>> imageInfos;

	//FOR PER-MATERIAL BINDING
	std::vector<VkDescriptorImageInfo> traditionalImageInfos;

};

#endif