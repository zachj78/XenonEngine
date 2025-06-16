#include "../include/System_Components/GUI.h"
#include "../include/Core/GraphicsPipeline.h"

void GUI::linkToApp(
	bool usingSwapchain,
	GLFWwindow* window,
	VkInstance instance,
	VkDevice logicalDevice,
	VkPhysicalDevice physicalDevice,
	uint32_t graphicsQueueIndex,
	VkQueue graphicsQueue,
	uint32_t minImageCount,
	uint32_t imageCount,
	std::vector<VkImageView> targetImageViews,
	VkRenderPass renderPass,
	VkSampler targetSampler
	) {

	// ================================
	//   CREATE GUI DESCRIPTOR POOL
	// ================================
	std::cout << "Creating gui descriptor pool" << std::endl;

	//2 sets for each descriptor resource
	uint32_t totalSets = 1000;

	std::vector<VkDescriptorPoolSize> poolSizes = {
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000}
	};

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = totalSets;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool");
	}

	if (!usingSwapchain) {
		// ===========================
		// CREATE OFFSCREEN SET LAYOUT
		// ===========================

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 0;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &samplerLayoutBinding;

		if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &offscreenLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout!");
		}
	}

	//Set up ImGui backend for GLFW
	ImGui_ImplGlfw_InitForVulkan(window, true);

	ImGui_ImplVulkan_InitInfo init_info = {};
	//FROM INSTANCES
	init_info.Instance = instance;
	//FROM DEVICES
	init_info.PhysicalDevice = physicalDevice;
	init_info.Device = logicalDevice;
	init_info.QueueFamily = graphicsQueueIndex;
	init_info.Queue = graphicsQueue;
	init_info.PipelineCache = VK_NULL_HANDLE;
	//FROM DESCRIPTOR MANAGER
	init_info.DescriptorPool = descriptorPool;
	//FROM SWAPCHAIN
	init_info.MinImageCount = minImageCount;
	init_info.ImageCount = imageCount;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.RenderPass = renderPass;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = nullptr;

	ImGui_ImplVulkan_Init(&init_info);
	ImGui_ImplVulkan_CreateFontsTexture();

	//DON'T SAMPLE IMAGES IF USING SWAPCHAIN

	if (!usingSwapchain) {
		offscreenTextureIDs.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			offscreenTextureIDs[i] = reinterpret_cast<ImTextureID>(ImGui_ImplVulkan_AddTexture(
				targetSampler,
				targetImageViews[i],
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			));
		}
	}
}

//ACTUAL GUI LOGIC GOES HERE
void GUI::beginFrame(
	uint32_t currentFrame
	//VkSampler sampler, 
	//VkImageView imageView
) {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	
	//Get viewport dimensions
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 work_pos = viewport->WorkPos;
	ImVec2 work_size = viewport->WorkSize;

	// ==========================
	//	     TOP BAR MENU
	// ==========================

	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("View")) {
			ImGui::MenuItem("Show ECS Hierarchy", nullptr, &showLeftPanel);
			ImGui::MenuItem("Show Asset Manager", nullptr, &showAssetManager); // Asset manager should be a menu on the bottom where materials and meshes are made/imported
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	// =================================
	//		WINDOW 1 - ECS HIERARCHY
	// =================================

	//Side left sidebar width 
	float hierarchyWidth = 300.0f;
	float infoWidth = 350.0f;

	if (showLeftPanel) {
		ImVec2 window_pos = work_pos;
		ImVec2 window_size = ImVec2(hierarchyWidth, work_size.y);

		ImGui::SetNextWindowPos(window_pos);
		ImGui::SetNextWindowSize(window_size);
		ImGui::SetNextWindowViewport(viewport->ID);

		// LEFT SIDE BAR FLAGSs
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoBringToFrontOnFocus
			| ImGuiWindowFlags_NoNavFocus
			| ImGuiWindowFlags_NoDocking;

		//Window 1 -> ECS HIERARCHY AND (EVENTUAL) FILE MANAGER
		ImGui::Begin("Left Tab", nullptr, window_flags);
		ImGui::Text("ECS Hierarchy");
		ImGui::End();
	}

	// ============================
	//		WINDOW 2 - INFO VIEWER
	// ============================ 

	ImGuiWindowFlags rightWindow_flags = ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoDocking;

	ImVec2 rightWindow_pos = ImVec2(work_pos.x + work_size.x - infoWidth, work_pos.y);
	ImVec2 rightWindow_size = ImVec2(infoWidth, work_size.y);

	ImGui::SetNextWindowPos(rightWindow_pos);
	ImGui::SetNextWindowSize(rightWindow_size);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGui::Begin("Info Viewer", nullptr, rightWindow_flags);
	ImGui::End();

	// ===========================
	//  WINDOW 3 - ASSET MANAGER
	// =========================== 
	float assetManagerHeight = showAssetManager ? 200.0f : 0.0f;

	if (showAssetManager) {
		float assetManagerWidth = 800.0f; 

		ImGuiWindowFlags assetManager_flags = ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoBringToFrontOnFocus
			| ImGuiWindowFlags_NoDocking;

		ImVec2 assetManager_pos = ImVec2(work_pos.x, work_pos.y + work_size.y - assetManagerHeight);
		ImVec2 assetManager_size = ImVec2(assetManagerWidth, assetManagerHeight);

		ImGui::SetNextWindowPos(assetManager_pos);
		ImGui::SetNextWindowSize(assetManager_size);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::Begin("Asset Manager", nullptr, assetManager_flags);
		ImGui::End();
	}

	// Compute central viewport area
	//ImVec2 centerWindowPos = ImVec2(work_pos.x + hierarchyWidth, work_pos.y);
	//ImVec2 centerWindowSize = ImVec2(
	//	work_size.x - hierarchyWidth - infoWidth,
	//	work_size.y - assetManagerHeight
	//);

	//ImGuiWindowFlags centerWindowFlags = ImGuiWindowFlags_NoResize
	//	| ImGuiWindowFlags_NoMove
	//	| ImGuiWindowFlags_NoCollapse
	//	| ImGuiWindowFlags_NoBringToFrontOnFocus
	//	| ImGuiWindowFlags_NoDocking;

	//ImGui::SetNextWindowPos(centerWindowPos);
	//ImGui::SetNextWindowSize(centerWindowSize);
	//ImGui::SetNextWindowViewport(viewport->ID);

	//ImGui::Begin("Scene Preview", nullptr, centerWindowFlags);

	//// Example: render the offscreen image
	//ImVec2 previewSize = ImGui::GetContentRegionAvail();

	//ImGui::Image(offscreenTextureIDs[currentFrame], previewSize);

	/*ImGui::End();*/
}

void GUI::endFrame() {
	ImGui::Render();
}

void GUI::record(VkCommandBuffer commandBuffer) {
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}