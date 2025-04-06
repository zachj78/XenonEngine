#include"debug_funcs.h"

void DBG_checkAllLayers(const std::vector<VkLayerProperties> &availableLayers) {
	std::cout << "Available Layers : \n" << std::endl;

	for (const auto& layer : availableLayers) {
		std::cout << "\t" << layer.layerName << std::endl;
	}
}