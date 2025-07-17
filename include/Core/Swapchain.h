#ifndef SWAPCHAIN_H
#define SWAPCHAIN_H

#include "Utils/config.h"
#include "Utils/cstm_types.h"
#include "Utils/MemoryUtils.h"

#include "Core/VulkanInstance.h"
#include "Core/VulkanDevices.h"

class ShaderLoader; 
class BufferManager;
class Image; 

struct RenderTarget {
	std::shared_ptr<Image> depthImage = nullptr;
	//std::shared_ptr<Image> colorImages; -> instead of the lines below
	std::vector<VkImage> images;
	std::vector<VkImageView> imageViews;
	std::vector<VkDeviceMemory> imageMemories; //COLOR ATTACHMENT IMAGES SHOULD ALSO USE <Image> class instead, refactor and clean up a bit [TODO]
	VkExtent2D extent;
	VkFormat format;
	bool isSwapchain;
	std::vector<VkFramebuffer> offscreenFramebuffers; // offscreen pass framebuffers
	std::vector<VkFramebuffer> mainFramebuffers; //Swapchain/main pass framebuffers
	std::vector<VkImageLayout> currentLayouts; // used for manual transitions of offscreen target
	uint32_t imageIndex = 0; // Only used if manually cycling offscreen target
};

class RenderTargeter {
public:
	RenderTargeter(std::shared_ptr<VulkanInstance> instance, std::shared_ptr<Devices> devices)
		: swpch_instance(instance), swpch_devices(devices) {
		std::cout << "RenderTargeter constructor start" << std::endl;

		if (!swpch_instance) throw std::runtime_error("Instance is null in RenderTargeter");
		if (!swpch_devices) throw std::runtime_error("Devices is null in RenderTargeter");

		std::cout << "RenderTargeter constructor end" << std::endl;
	};

	// ==MAIN FUNCTIONS==
	void createDepthImage();

	void getFramebufferDetails() {
		std::cout << "[RenderTargeter::getFramebufferDetails] entered" << std::endl;

		VkDevice logicalDevice = swpch_devices->getLogicalDevice();
		VkPhysicalDevice physicalDevice = swpch_devices->getPhysicalDevice();
		GLFWwindow* window = swpch_instance->getWindowPtr();
		VkSurfaceKHR surface = swpch_instance->getSurface();

		SwapchainSupportDetails supportDetails = querySwapchainSupport(physicalDevice, surface);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(supportDetails.formats);
		VkExtent2D extent = chooseSwapExtent(supportDetails.capabilities, window);

		renderTarget.extent = extent;
		renderTarget.format = surfaceFormat.format;

		std::cout << "[RemderTargeter::getFramebufferDetails] exiting: \n"
			<< "[RemderTargeter::getFramebufferDetails] EXTENT : [h:" << renderTarget.extent.height << " w: " << renderTarget.extent.width << "] \n"
			<< "[RemderTargeter::getFramebufferDetails] SURFACE FORMAT: " << renderTarget.format << std::endl;
	}
	void createSwapchainResources(); // if using offscreen bascially duplicate this function to make offscreen images and image views
	void createFramebuffers(VkRenderPass renderpass, bool usingOffscreenPass);

	//Renderpass creation
	void createMainRenderpass(bool isSwapchain) {
		isSwapchain ? std::cout << "creating MAIN renderpass WITH swapchain" << std::endl : std::cout << "creating renderpass WITHOUT swapchain" << std::endl;

		//Define variables to create render pass
		VkDevice logicalDevice = swpch_devices->getLogicalDevice();

		// == COLOR ATTACHMENT == 
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = renderTarget.format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		//No stencil buffer currently, dont store data
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		//Initial and final layout of the swapchain image
		// -> in this case final layout is VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0; //refers the the index of this attachment
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// == DEPTH ATTACHMENT == 
		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat(swpch_devices->getPhysicalDevice());
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &mainPass) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create render pass");
		};
	};

	void createOffscreenRenderpass() {
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = renderTarget.format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat(swpch_devices->getPhysicalDevice());
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		std::array<VkSubpassDependency, 2> dependencies{};

		// External -> Subpass (ensure correct layout before rendering)
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// Subpass -> External (for reading as texture later)
		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		if (vkCreateRenderPass(swpch_devices->getLogicalDevice(), &renderPassInfo, nullptr, &offscreenPass) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create offscreen render pass");
		}
	}

	//Per-Frame Transition helper function
	void transitionTargetImageLayout(
		VkCommandBuffer cmdBuffer,
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		VkImageAspectFlags aspectMask,
		uint32_t mipLevels = 1,
		uint32_t baseMipLevel = 0,
		uint32_t baseArrayLayer = 0,
		uint32_t layerCount = 1
	);

	//GET RID OF SURFACE CLEAN UP AND ADD TO INSTANCE <<<----
	void cleanup(bool isLastCleanup);

	//Getter functions
	//For the swapchain variables
	VkSwapchainKHR& getSwapchain() { return swapchain; };
	VkSampler getOffscreenSampler() { return sampler; };
	RenderTarget& getRenderTarget() { return renderTarget; };
	VkRenderPass& getMainPass() { return mainPass; };
	VkRenderPass& getOffscreenPass() { return offscreenPass; };

private:
	//Inject Vulkan Core Components
	std::shared_ptr<VulkanInstance> swpch_instance = nullptr; 
	std::shared_ptr<Devices> swpch_devices = nullptr; 

	RenderTarget renderTarget; 

	//Swapchain -> only used if rendering to the entire screen in game mode
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	//Sampler -> only used if offscreen rendering
	VkSampler sampler;

	//Renderpasses
	VkRenderPass mainPass; 
	VkRenderPass offscreenPass; 

	// Swapchain helpers
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilties, GLFWwindow* window);

};

#endif