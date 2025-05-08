#include "../include/UniformBufferManager.h"

void UniformBufferManager::createDescriptorSetLayout() {
	std::cout << "Creating descriptor set layout" << std::endl;

	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;

	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	VkResult result = vkCreateDescriptorSetLayout(descManager_logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout");
	} else {
		std::cout << "Created descriptor set layout successfully : [" << result << "]" << std::endl;
	};
};

void UniformBufferManager::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	std::cout << "Creating uniform buffers : [" << bufferSize << "]" << std::endl;

	uniformBuffer_ptrs.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		std::string ubufName = "ubuf" + std::to_string(i);

		std::shared_ptr<BaseBuffer> ubuf = BufferUtils::createUniformBuffer(
			BufferType::UNIFORM,
			ubufName,
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			descManager_logicalDevice, descManager_physicalDevice
		);

		std::cout << "Created ubuf :[" << ubuf->getHandle() << "] : [" << ubufName << "]" << std::endl;

		VkResult mapResult = vkMapMemory(descManager_logicalDevice, ubuf->buf_memory, 0, bufferSize, 0, &uniformBuffer_ptrs[i]);
		if (mapResult != VK_SUCCESS) {
			throw std::runtime_error("Ubuf `vkMapMemory` operation failed");
		} else {
			std::cout << "Mapped [" << ubufName << "] to [ptr:" << i << "]- result: [" << mapResult << "]";
		};

		//After memory is mapped, put buffer into map of uniform buffers
		// -> if moved before program will silently crash
		uniformBuffers[ubufName] = std::move(ubuf);
	}
}

//HAS BEEN DEBUGGED AND WORKS
void UniformBufferManager::updateUniformBuffer(uint32_t currentImage, VkExtent2D swapchainExtent) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.proj = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	std::cout << "UBO time: " << time << "\nModel[0][0]: " << ubo.model[0][0] << std::endl;
	std::cout << "Mapped ptr for frame [" << currentImage << "] is: " << uniformBuffer_ptrs[currentImage] << std::endl;

	//ptrs should be map, perhaps even grouped into a struct `UBOInfo` *** 
	memcpy(uniformBuffer_ptrs[currentImage], &ubo, sizeof(ubo));
};

void UniformBufferManager::createDescriptorPool() {
	std::cout << "Creating descriptor pool" << std::endl;

	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(descManager_logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool");
	};
};

void UniformBufferManager::createDescriptorSet() {
	std::cout << "Creating descriptor sets" << std::endl;

	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
	if (vkAllocateDescriptorSets(descManager_logicalDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets");
	};

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		std::string ubufName = "ubuf" + std::to_string(i);
		VkBuffer ubuf_handle = nullptr; 

		auto it = uniformBuffers.find(ubufName);
		if (it != uniformBuffers.end()) {
			BaseBuffer* ubuf = it->second.get();
			ubuf_handle = ubuf->getHandle();
			std::cout << "Found uniform buffer [" << ubuf->getMemory() << "]: " << ubufName << std::endl;;
		} else {
			throw std::runtime_error("Failed write descriptor sets for each uniform buffer -> could not retreive [" + ubufName + "] successfully");
		}

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = ubuf_handle;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		std::cout << "Writing descriptor for set[" << i << "] buffer: " << bufferInfo.buffer << ", range: " << bufferInfo.range << std::endl;

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;

		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		//Used for descriptors that refer to image data 
		descriptorWrite.pImageInfo = nullptr;
		//Used for descriptors that refer to buffer views
		descriptorWrite.pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(descManager_logicalDevice, 1, &descriptorWrite, 0, nullptr);
	};
};

void UniformBufferManager::cleanup() {
	for (const auto& pair : uniformBuffers) {
		BaseBuffer* ubuf = pair.second.get();

		vkDestroyBuffer(descManager_logicalDevice, ubuf->getHandle(), nullptr);
		vkFreeMemory(descManager_logicalDevice, ubuf->getMemory(), nullptr);
	};

	vkDestroyDescriptorPool(descManager_logicalDevice, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(descManager_logicalDevice, descriptorSetLayout, nullptr);
};