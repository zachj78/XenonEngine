#include "../include/UniformBufferManager.h"
#include "../include/BufferManager.h"
#include "../include/Buffer.h"

void UniformBufferManager::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	std::cout << "Creating uniform buffers : [" << bufferSize << "]" << std::endl;

	uniformBuffer_ptrs.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		std::string ubufName = "ubuf" + std::to_string(i);

		descManager_bufferManager->createBuffer(
			BufferType::UNIFORM,
			ubufName,
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		);

		//FIX VERTEX AND INDEX BUFFER SETUP FOR NEW BUFFER AND BUFFERMANAGER
		std::shared_ptr<Buffer> ubuf = descManager_bufferManager->getBuffer(ubufName);

		VkResult mapResult = vkMapMemory(descManager_logicalDevice, ubuf->getMemory(), 0, bufferSize, 0, &uniformBuffer_ptrs[i]);
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

	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	if (vkCreateDescriptorPool(descManager_logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool");
	};
};

void UniformBufferManager::createDescriptorSets(VkImageView imageView, VkSampler sampler) {
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

		std::shared_ptr<Buffer> ubuf = descManager_bufferManager->getBuffer(ubufName);
		VkBuffer ubufHandle = ubuf->getHandle();

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = ubufHandle;
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(UniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = imageView;
		imageInfo.sampler = sampler;

		std::cout << "Writing descriptor for set[" << i << "] buffer: " << bufferInfo.buffer << ", range: " << bufferInfo.range << std::endl;

		//Write the descriptor info to the buffer

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(descManager_logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	};
};

void UniformBufferManager::cleanup() {
	for (const auto& pair : uniformBuffers) {
		std::string name = pair.first;
		std::shared_ptr<Buffer> ubuf = pair.second;

		vkDestroyBuffer(descManager_logicalDevice, ubuf->getHandle(), nullptr);
		vkFreeMemory(descManager_logicalDevice, ubuf->getMemory(), nullptr);
	};

	vkDestroyDescriptorPool(descManager_logicalDevice, descriptorPool, nullptr);
};