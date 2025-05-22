#include "../include/UniformBufferManager.h"
#include "../include/BufferManager.h"
#include "../include/Buffer.h"
#include "../include/Camera.h"


void DescriptorManager::createPerFrameDescriptorSetLayout(){
	std::cout << "Creating per frame descriptor set layout" << std::endl;

	//Define descriptors to be used in this set
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
	}
	else {
		std::cout << "Created descriptor set layout successfully : [" << result << "]" << std::endl;
	};
}

//EACH MESH SHOULD CREATE ITS OWN UNIFORM BUFFER
void DescriptorManager::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UBO);
	std::cout << "Creating uniform buffers : [" << bufferSize << "]" << std::endl;

	uniformBuffer_ptrs.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		std::string ubufName = "ubuf" + std::to_string(i);

		descManager_bufferManager->createBuffer(
			BufferType::UNIFORM,
			ubufName,
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			std::nullopt,
			std::nullopt
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
//THIS SHOULD BE UPDATECAMERA, AND ONLY UPDATE VIEW AND PROJECTION MATRICES, WHILE EACH MESH SHOULD GET ITS OWN MODEL MATRIX AND UPDATE IT THROUGH ITS OWN MODEL MATRIX
void DescriptorManager::updateUniformBuffer(uint32_t currentImage, VkExtent2D swapchainExtent) {
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	float aspectRatio = (float)swapchainExtent.width / (float)swapchainExtent.height; 

	UBO ubo{};
	//Apply transformations to MVP matrix: 
	ubo.view = descManager_camera->getViewMatrix();
	ubo.proj = descManager_camera->getProjectionMatrix(aspectRatio);

	//Apply lighting
	ubo.lightPos = glm::vec3(0.0f, 1.0f, 5.0f);
	ubo.lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
	ubo.cameraPos = descManager_camera->getPosition();

	ubo.proj[1][1] *= -1;

	/*std::cout << "Camera Pos: " << descManager_camera->position.x << ", "
		<< descManager_camera->position.y << ", "
		<< descManager_camera->position.z << std::endl;

	std::cout << "Front Dir: " << descManager_camera->front.x << ", "
		<< descManager_camera->front.y << ", "
		<< descManager_camera->front.z << std::endl;

	std::cout << "Model[3]: " << ubo.model[3][0] << ", " << ubo.model[3][1] << ", " << ubo.model[3][2] << std::endl;*/

	//ptrs should be map, perhaps even grouped into a struct `UBOInfo` *** 
	memcpy(uniformBuffer_ptrs[currentImage], &ubo, sizeof(ubo));
};

void DescriptorManager::createDescriptorPool(int meshCount, int materialCount) {
	std::cout << "Creating descriptor pool" << std::endl;

	uint32_t totalSets = MAX_FRAMES_IN_FLIGHT + meshCount * MAX_FRAMES_IN_FLIGHT + materialCount;

	std::vector<VkDescriptorPoolSize> poolSizes = {
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(meshCount * MAX_FRAMES_IN_FLIGHT)},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(materialCount)}
	};

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = totalSets;

	if (vkCreateDescriptorPool(descManager_logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool");
	}
}

void DescriptorManager::createPerFrameDescriptorSet() {
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
		bufferInfo.range = sizeof(UBO);

		std::cout << "Writing descriptor for set[" << i << "] buffer: " << bufferInfo.buffer << ", range: " << bufferInfo.range << std::endl;

		//Write the descriptor info to the buffer

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(descManager_logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	};
};

void DescriptorManager::cleanup() {
	std::cout << "    Destroying `UniformBufferManager` " << std::endl;
	
	if (descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(descManager_logicalDevice, descriptorPool, nullptr);
		descriptorPool = VK_NULL_HANDLE;
	}

	vkDestroyDescriptorSetLayout(descManager_logicalDevice, descriptorSetLayout, nullptr);
};