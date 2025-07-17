#include "../include/Builders/DescriptorBuilder.h"

DescriptorBuilder DescriptorBuilder::begin(VkDevice device) {
	DescriptorBuilder builder; 
	builder.device = device; 
	return builder; 
}

DescriptorBuilder& DescriptorBuilder::bindBuffer(uint32_t binding,
	VkDescriptorType type,
	VkShaderStageFlags stageFlags,
	VkBuffer buffer,
	VkDeviceSize range
) {
	//Binding
	VkDescriptorSetLayoutBinding layoutBinding{};
	layoutBinding.binding = binding;
	layoutBinding.descriptorType = type;
	layoutBinding.descriptorCount = 1;
	layoutBinding.stageFlags = stageFlags;

	bindings.push_back(layoutBinding);

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = range;

	bufferInfos.push_back(bufferInfo);

	VkWriteDescriptorSet write{};
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstBinding = binding;
	write.descriptorCount = 1;
	write.descriptorType = type;
	write.pBufferInfo = &bufferInfos.back();

	writes.push_back(write);

	return *this;
}

//Binds an image
DescriptorBuilder& DescriptorBuilder::bindImage(uint32_t binding, 
    VkDescriptorType type, 
    VkShaderStageFlags stageFlags, 
    VkImageView imageView, 
    VkSampler sampler
) {
    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = type;
    layoutBinding.descriptorCount = 1;
    layoutBinding.stageFlags = stageFlags;

    bindings.push_back(layoutBinding);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = imageView;
    imageInfo.sampler = sampler;

    traditionalImageInfos.push_back(imageInfo);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = &traditionalImageInfos.back();

    writes.push_back(write);

    //[DEBUG]
    std::cout << "[DescriptorBuilder] bindImage for binding " << binding
        << " imageView: " << imageView << ", sampler: " << sampler << std::endl;

    return *this;
}

DescriptorBuilder& DescriptorBuilder::bindImageArray(
    uint32_t binding,
    VkDescriptorType type,
    VkShaderStageFlags stageFlags,
    const std::vector<VkImageView> imageViews,
    const std::vector<VkSampler> samplers,
    VkDescriptorBindingFlags flags
) {
    if (imageViews.size() != samplers.size()) {
        throw std::runtime_error("Image views and sampler count mismatched in DescriptorBuilder::bindImageArray");
    }

    VkDescriptorSetLayoutBinding layoutBinding{};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = type;
    layoutBinding.descriptorCount = static_cast<uint32_t>(imageViews.size());
    layoutBinding.stageFlags = stageFlags;
    layoutBinding.pImmutableSamplers = nullptr; 

    bindings.push_back(layoutBinding);
    bindingFlags.push_back(flags);

    std::vector<VkDescriptorImageInfo> infos{};
    infos.reserve(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); ++i) {
        infos.push_back({
            samplers[i],
            imageViews[i],
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        });
    };

    imageInfos.push_back(std::move(infos));

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = binding;
    write.descriptorCount = layoutBinding.descriptorCount;
    write.descriptorType = type;
    write.pImageInfo = imageInfos.back().data();

    writes.push_back(write);

    return *this;
};

//Creates a descriptor set layout
bool DescriptorBuilder::buildLayout(VkDescriptorSetLayout& layout, bool isImageArrayLayout = false) {
    // Create Layout
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (isImageArrayLayout) {
        VkDescriptorSetLayoutBindingFlagsCreateInfoEXT extendedInfo{};
        extendedInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
        extendedInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
        extendedInfo.pBindingFlags = bindingFlags.data();

        layoutInfo.pNext = &extendedInfo;
    }

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout) != VK_SUCCESS) {
        return false;
    }

    return true; 
}

bool DescriptorBuilder::buildSet(VkDescriptorSetLayout& layout, VkDescriptorSet& set, VkDescriptorPool pool, bool isImageArraySet = false) {
    // Allocate Set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;
    
    //Image array binding flag management
    bool hasVariableDescriptor = false;
    uint32_t variableCount = 0;
    VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountAllocInfo{};

    if (isImageArraySet) {
        if (!bindingFlags.empty()) {
            const auto& lastFlag = bindingFlags.back();
            if (lastFlag & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT) {
                variableCount = bindings.back().descriptorCount;
                variableCountAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
                variableCountAllocInfo.descriptorSetCount = 1;
                variableCountAllocInfo.pDescriptorCounts = &variableCount;

                allocInfo.pNext = &variableCountAllocInfo;
            }
        }
    }

    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &set);
    if (result != VK_SUCCESS) {
        std::cerr << "[ERROR] vkAllocateDescriptorSets failed with error code: " << result << std::endl;
        set = VK_NULL_HANDLE;
        return false;
    }

    // Update Set
    for (auto& write : writes) {
        write.dstSet = set;
    }

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    return true;
}