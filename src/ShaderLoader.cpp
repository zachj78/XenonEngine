#include "../include/ShaderLoader.h"

std::vector<char> ShaderLoader::readShaderFile(const std::string& filename) {
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) != nullptr) {
		std::cout << "Current working directory: " << cwd << std::endl;
	}

	std::cout << "Opening shader file: " << filename << std::endl;
	std::ifstream file(filename, std::ios::ate | std::ios::binary);



	if (!file.is_open()) {
		throw std::runtime_error("Failed to open .spv file");
	} else {
		std::cout << "Opening shader file: " << filename << std::endl;
	};

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

VkShaderModule ShaderLoader::createShaderModule(VkDevice logicalDevice, const std::vector<char>& code) {
	VkShaderModuleCreateInfo shaderInfo{};
	shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderInfo.codeSize = code.size();
	shaderInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	printf("Creating shader module of size: " + code.size());

	VkShaderModule shaderModule; 
	if (vkCreateShaderModule(logicalDevice, &shaderInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module");
	} else {
		std::printf("Successfully created shader module");
	}

	return shaderModule; 
};