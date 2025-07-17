#include "../include/Managers/DescriptorManager.h"
#include "../include/Managers/BufferManager.h"
#include "../include/Managers/Buffer.h"
#include "../include/System_Components/Camera.h"

//EACH MESH SHOULD CREATE ITS OWN UNIFORM BUFFER
void DescriptorManager::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(UBO);
	std::cout << "Creating uniform buffers : [" << bufferSize << "]" << std::endl;

	ubufInfo.resize(MAX_FRAMES_IN_FLIGHT);

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
		void* mappedPtr = nullptr;

		VkResult mapResult = vkMapMemory(descManager_logicalDevice, ubuf->getMemory(), 0, bufferSize, 0, &mappedPtr);
		if (mapResult != VK_SUCCESS) {
			throw std::runtime_error("Ubuf `vkMapMemory` operation failed");
		} 
		
		ubufInfo[i] = { mappedPtr, ubuf };

		std::cout << "Mapped [" << ubufName << "] to [ptr:" << i << "]- result: [" << mapResult << "]";
	}
}

void DescriptorManager::createDescriptorPool(int meshCount, int materialCount) {
	std::cout << "Creating descriptor pool" << std::endl;
	std::cout << " with " << meshCount << " meshes and " << materialCount << " materials" << std::endl;

	uint32_t totalMaterials = materialCount;

	uint32_t ssboSpace = 10; 
	uint32_t materialDescriptorCount = totalMaterials * MAX_FRAMES_IN_FLIGHT;

	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,        static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,        static_cast<uint32_t>(ssboSpace) },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, materialDescriptorCount }
	};

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = static_cast<uint32_t>(
		MAX_FRAMES_IN_FLIGHT * (1 + ssboSpace + totalMaterials)
		);


	if (vkCreateDescriptorPool(descManager_logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool");
	}
}

void DescriptorManager::createPerFrameDescriptors() {
	std::cout << "Creating descriptor sets" << std::endl;

	bool layoutBuilt = false; 

	descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		DescriptorBuilder builder = DescriptorBuilder::begin(descManager_logicalDevice);
		
		std::string ubufName = "ubuf" + std::to_string(i);

		std::cout << "---> BINDING BUFFER: [" << ubufName << "]" << std::endl;

		std::shared_ptr<Buffer> ubuf = descManager_bufferManager->getBuffer(ubufName);
		VkBuffer ubufHandle = ubuf->getHandle();

		if (!ubuf || ubuf->getHandle() == VK_NULL_HANDLE) {
			throw std::runtime_error("Failed to retrieve valid uniform buffer: " + ubufName);
		}

		//Write the descriptor info to the buffer
		builder.bindBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, ubufHandle, sizeof(UBO));

		if (!layoutBuilt) {
			builder.buildLayout(descriptorSetLayout, false);
			layoutBuilt = true;
		};

		builder.buildSet(descriptorSetLayout, descriptorSets[i], descriptorPool, false);
	};
};

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

	std::cout << "Camera Pos: " << descManager_camera->position.x << ", "
		<< descManager_camera->position.y << ", "
		<< descManager_camera->position.z << std::endl;

	std::cout << "Front Dir: " << descManager_camera->front.x << ", "
		<< descManager_camera->front.y << ", "
		<< descManager_camera->front.z << std::endl;

	//ptrs should be map, perhaps even grouped into a struct `UBOInfo` *** 
	void* & ptr = ubufInfo[currentImage].mappedPtr;

	memcpy(ptr, &ubo, sizeof(ubo));
};

void DescriptorManager::cleanup() {
	std::cout << "    Destroying `UniformBufferManager` " << std::endl;
	
	if (descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(descManager_logicalDevice, descriptorPool, nullptr);
		descriptorPool = VK_NULL_HANDLE;
	}

	vkDestroyDescriptorSetLayout(descManager_logicalDevice, descriptorSetLayout, nullptr);
};