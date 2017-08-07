#include "Renderer.h"
#include "Mesh.h"
#include "os_support.h"

using namespace Primitive;
using namespace vkh;

namespace Renderer
{
	AppRenderData appRenderData;

	void createDescriptorSetLayout(AppRenderData& rs);
	void createDescriptorSet(AppRenderData& rs);
	void createPipelines(AppRenderData& rs);

	void initializeRendering(HINSTANCE Instance, HWND wndHdl, const char* applicationName)
	{
		CreateWin32Context(GContext, OS::getScreenW(), OS::getScreenH(), Instance, wndHdl, applicationName);
		CreateColorOnlyRenderPass(appRenderData.renderPass, GContext.swapChain, GContext.lDevice.device);
		CreateFramebuffers(appRenderData.swapChainFramebuffers, GContext.swapChain, appRenderData.renderPass, GContext.lDevice.device);
	
		createDescriptorSetLayout(appRenderData);
		//createDescriptorSet(appRenderData);
		createPipelines(appRenderData);
		handleScreenResize(appRenderData);

	}

	void createDescriptorSetLayout(AppRenderData& rs)
	{
		//we only need a single binding, since we're passing both our params in a single UBO 
		VkDescriptorSetLayoutBinding mvpLayoutBinding = {};
		mvpLayoutBinding.binding = 0;
		mvpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		mvpLayoutBinding.descriptorCount = 1;
		mvpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		mvpLayoutBinding.pImmutableSamplers = nullptr; // Optional

													   //different bindings NEED different uniform buffers. 
		VkDescriptorSetLayoutBinding bindings[] = { mvpLayoutBinding };

		//a resource descriptor is a way for shaders to freely access resources like buffers and images
		//to use our uniform data, we need to tell Vulkan about our descriptor

		VkDescriptorSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = bindings;

		VkResult res = vkCreateDescriptorSetLayout(GContext.lDevice.device, &layoutInfo, nullptr, &rs.descriptorSetLayout);
		assert(res == VK_SUCCESS);
	}

	void Renderer::createDescriptorSet(AppRenderData& rs)
	{
		VkResult res;
	
		VkDescriptorSetLayout layouts[] = { rs.descriptorSetLayout };
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = GContext.descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = layouts;
	
		res = vkAllocateDescriptorSets(GContext.lDevice.device, &allocInfo, &rs.descriptorSet);
		

		VkBufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		createInfo.size = sizeof(Primitive::PrimitiveUniformObject);
		createInfo.usage = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;

		VkBuffer emptyBuffer;
		vkCreateBuffer(GContext.lDevice.device, &createInfo, nullptr, &emptyBuffer);

		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(Primitive::PrimitiveUniformObject);
		bufferInfo.buffer = emptyBuffer;

		//The configuration of descriptors is updated using the vkUpdateDescriptorSets function, which takes an array of VkWriteDescriptorSet structs as parameter.
		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = rs.descriptorSet;
		descriptorWrite.dstBinding = 0; //refers to binding in shader
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr; // Optional
		descriptorWrite.pTexelBufferView = nullptr; // Optional
	 //
		vkUpdateDescriptorSets(GContext.lDevice.device, 1, &descriptorWrite, 0, nullptr);
	}

	void createPipelines(AppRenderData& rs)
	{
		//create paddle pipeline
		VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.pName = "main";
		CreateShaderModule(vertShaderStageInfo.module, "./shaders/vertColorPassthrough.vert.spv", GContext.lDevice.device);
	
		VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.pName = "main";
		CreateShaderModule(fragShaderStageInfo.module, "./shaders/fragFlatColor.frag.spv", GContext.lDevice.device);
	
		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
	
		//now we need to set up all the fixed function parts of the pipeline
		VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	
		auto bindingDescription = getVertexBindingDescription();
		auto attributeDescriptions = getVertexAttributeDescriptions();
	
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // Optional
		vertexInputInfo.vertexAttributeDescriptionCount = vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // Optional
	
	
		VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;
	
	
		VkViewport viewport;
		CreateDefaultViewportForSwapChain(viewport, GContext.swapChain);
	
	
		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = GContext.swapChain.extent;
	
		VkPipelineViewportStateCreateInfo viewportState = {};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;
	
		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		CreateDefaultPipelineRasterizationStateCreateInfo(rasterizer);
	
		VkPipelineMultisampleStateCreateInfo multisampling = {};
		CreateMultisampleStateCreateInfo(multisampling, 1);
	
		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		CreateOpaqueColorBlendAttachState(colorBlendAttachment);
	
		VkPipelineColorBlendStateCreateInfo colorBlending = {};
		CreateDefaultColorBlendStateCreateInfo(colorBlending, colorBlendAttachment);
	
		VkPushConstantRange pushConstantRange = {};
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(Primitive::PrimitiveUniformObject);
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1; // Optional
		pipelineLayoutInfo.pSetLayouts = &rs.descriptorSetLayout; // Optional
		pipelineLayoutInfo.pushConstantRangeCount = 1; // Optional
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange; // Optional
	
		VkResult res = vkCreatePipelineLayout(GContext.lDevice.device, &pipelineLayoutInfo, nullptr, &rs.blockMaterial.pipelineLayout);
		assert(res == VK_SUCCESS);
	
	
		VkGraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; // Optional
		pipelineInfo.layout = rs.blockMaterial.pipelineLayout;
		pipelineInfo.renderPass = rs.renderPass;
		pipelineInfo.subpass = 0;
	
		//can use this to create new pipelines by deriving from old ones
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional
	
		res = vkCreateGraphicsPipelines(GContext.lDevice.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &rs.blockMaterial.gfxPipeline);
		assert(res == VK_SUCCESS);
	
		vkDestroyShaderModule(GContext.lDevice.device, vertShaderStageInfo.module, nullptr);
		vkDestroyShaderModule(GContext.lDevice.device, fragShaderStageInfo.module, nullptr);
	}

	void handleScreenResize(AppRenderData& rd)
	{
		int w = OS::getScreenW();
		int h = OS::getScreenH();
		rd.screenW = w;
		float aspect = (float)w/ (float)h;
		float invAspect = (float)h / (float)w;
		float screenDim = 100.0f * aspect;
		float iscreenDim = 100.0f* invAspect;
		rd.screenW = (int)screenDim;
		rd.screenH = 100;
		rd.VIEW_PROJECTION = glm::ortho(-(float)screenDim, (float)screenDim, -(float)100, (float)100, -1.0f, 1.0f);
	
	}

	void draw(const struct PrimitiveUniformObject* uniformData, const std::vector<int> primMeshes)
	{
		//max size of buffer we allocated
		assert(primMeshes.size() < 256);
	
		VkhContext& Gctxt = GContext;
		VkResult res;
	
		//acquire an image from the swap chain
		uint32_t imageIndex;
	
		//using uint64 max for timeout disables it
		res = vkAcquireNextImageKHR(Gctxt.lDevice.device, Gctxt.swapChain.swapChain, UINT64_MAX, Gctxt.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		
		if (Gctxt.frameFences[imageIndex])
		{
			// Fence should be unsignaled
			if (vkGetFenceStatus(Gctxt.lDevice.device, Gctxt.frameFences[imageIndex]) == VK_SUCCESS)
			{
				vkWaitForFences(Gctxt.lDevice.device, 1, &Gctxt.frameFences[imageIndex], true, 0);
			}
		}
		vkResetFences(Gctxt.lDevice.device, 1, &Gctxt.frameFences[imageIndex]);
	
		if (res == VK_ERROR_OUT_OF_DATE_KHR)
		{
			handleScreenResize(appRenderData);
			return;
		}
		else
		{
			assert(res == VK_SUCCESS);
		}
	
	
		//record drawing commands for cmd buffer
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional
		vkResetCommandBuffer(Gctxt.commandBuffers[imageIndex], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		res = vkBeginCommandBuffer(Gctxt.commandBuffers[imageIndex], &beginInfo);
		assert(res == VK_SUCCESS);
	
		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = appRenderData.renderPass;
		renderPassInfo.framebuffer = appRenderData.swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = Gctxt.swapChain.extent;
	
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
	
		vkCmdBeginRenderPass(Gctxt.commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	
		vkCmdBindPipeline(Gctxt.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, appRenderData.blockMaterial.gfxPipeline);
		for (int i = 0; i < primMeshes.size(); ++i)
		{
			Mesh* mesh = GetMeshData(primMeshes[i]);
			VkBuffer vertexBuffers[] = { mesh->vBuffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdPushConstants(
				Gctxt.commandBuffers[imageIndex],
				appRenderData.blockMaterial.pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT,
				0,
				sizeof(Primitive::PrimitiveUniformObject),
				&uniformData[i]);


		//	vkCmdBindDescriptorSets(Gctxt.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, appRenderData.blockMaterial.pipelineLayout, 0, 1, &appRenderData.descriptorSet, 1, NULL);
	
			vkCmdBindVertexBuffers(Gctxt.commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(Gctxt.commandBuffers[imageIndex], mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
			vkCmdDrawIndexed(Gctxt.commandBuffers[imageIndex], static_cast<uint32_t>(mesh->indexCount), 1, 0, 0, 0);
		}
	
		vkCmdEndRenderPass(Gctxt.commandBuffers[imageIndex]);
	
		res = vkEndCommandBuffer(Gctxt.commandBuffers[imageIndex]);
		assert(res == VK_SUCCESS);
	
		//wait on writing colours to the buffer until the semaphore says the buffer is available
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
		VkSemaphore waitSemaphores[] = { Gctxt.imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
	
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &Gctxt.commandBuffers[imageIndex];
	
		VkSemaphore signalSemaphores[] = { Gctxt.renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;
	
		res = vkQueueSubmit(Gctxt.lDevice.graphicsQueue, 1, &submitInfo, Gctxt.frameFences[imageIndex]);
		assert(res == VK_SUCCESS);
	
	
		//finally, present this thing on screen
	
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
	
		VkSwapchainKHR swapChains[] = { Gctxt.swapChain.swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional
		res = vkQueuePresentKHR(Gctxt.lDevice.transferQueue, &presentInfo);
	
		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
		{
			handleScreenResize(appRenderData);  
		}
	}
}
