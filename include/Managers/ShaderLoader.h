#ifndef SHADER_LOADER_H
#define SHADER_LOADER_H

#include "Utils/config.h"

class ShaderLoader {
public:
	//Main functions
	std::vector<char> readShaderFile(const std::string& filename);
	VkShaderModule createShaderModule(VkDevice logicalDevice, const std::vector<char>& code);
	
	//Getter functions

	//Logger functions

	//Variables


private:

};

#endif