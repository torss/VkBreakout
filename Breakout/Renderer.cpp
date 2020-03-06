#include "Renderer.h"
#include "Mesh.h"
#include "os_support.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>

using namespace Primitive;
using namespace vkh;

namespace Renderer
{
	AppRenderData appRenderData;
	void* udata = nullptr;

	void createDescriptorSetLayout(AppRenderData& rs);
	void createDescriptorSet(AppRenderData& rs);
	void createPipelines(AppRenderData& rs);
	void createQueryPool(AppRenderData& rs);

	void initializeRendering(HINSTANCE Instance, HWND wndHdl, const char* applicationName)
	{
		CreateWin32Context(GContext, OS::getScreenW(), OS::getScreenH(), Instance, wndHdl, applicationName);
		CreateColorOnlyRenderPass(appRenderData.renderPass, GContext.swapChain, GContext.lDevice.device);
		CreateFramebuffers(appRenderData.swapChainFramebuffers, GContext.swapChain, appRenderData.renderPass, GContext.lDevice.device);
	
		createDescriptorSetLayout(appRenderData);
		createDescriptorSet(appRenderData);
		createPipelines(appRenderData);

#if ENABLE_VK_TIMESTAMP
		createQueryPool(appRenderData);
#endif
		handleScreenResize(appRenderData);
	}

	void createQueryPool(AppRenderData& rs)
	{
		VkQueryPoolCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		createInfo.queryCount = 2;
	
		VkResult res = vkCreateQueryPool(GContext.lDevice.device, &createInfo, nullptr, &rs.queryPool);
		assert(res == VK_SUCCESS);
	}

	void createDescriptorSetLayout(AppRenderData& rs)
	{
		//we only need a single binding, since we're passing both our params in a single UBO 
		VkDescriptorSetLayoutBinding mvpLayoutBinding = {};
		mvpLayoutBinding.binding = 0;
		mvpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
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
	
		//now we need to configure the descriptor inside the set.
	
		//our descriptor points to a uniform buffer, so it's configured with
		//a VkDescriptorBufferInfo
	
		size_t uboAlignment = GContext.gpu.deviceProps.limits.minUniformBufferOffsetAlignment;
		size_t dynamicAlignment = (sizeof(Primitive::PrimitiveUniformObject) / uboAlignment) * uboAlignment + ((sizeof(Primitive::PrimitiveUniformObject) % uboAlignment) > 0 ? uboAlignment : 0);
	
		
		VkDeviceSize bufferSize = dynamicAlignment * 6000;

#if DEVICE_LOCAL_MEMORY
		CreateBuffer(rs.stagingBuffer,
			rs.stagingBufferMemory,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			GContext.lDevice.device,
			GContext.gpu.device);

		CreateBuffer(rs.uniformBuffer,
			rs.uniformBufferMemory,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			GContext.lDevice.device,
			GContext.gpu.device);
#else
		CreateBuffer(rs.uniformBuffer,
			rs.uniformBufferMemory,
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			GContext.lDevice.device,
			GContext.gpu.device);

#endif
	
	
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = rs.uniformBuffer;
		bufferInfo.offset = 0;
		bufferInfo.range = dynamicAlignment;
	
		//The configuration of descriptors is updated using the vkUpdateDescriptorSets function, which takes an array of VkWriteDescriptorSet structs as parameter.
		VkWriteDescriptorSet descriptorWrite = {};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = rs.descriptorSet;
		descriptorWrite.dstBinding = 0; //refers to binding in shader
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = nullptr; // Optional
		descriptorWrite.pTexelBufferView = nullptr; // Optional
	
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
	
		VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1; // Optional
		pipelineLayoutInfo.pSetLayouts = &rs.descriptorSetLayout; // Optional
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = 0; // Optional
	
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

	void* mapBufferPtr(int maxPrimMeshes)
	{
		size_t uboAlignment = GContext.gpu.deviceProps.limits.minUniformBufferOffsetAlignment;
		size_t dynamicAlignment = (sizeof(Primitive::PrimitiveUniformObject) / uboAlignment) * uboAlignment + ((sizeof(Primitive::PrimitiveUniformObject) % uboAlignment) > 0 ? uboAlignment : 0);

		if (!udata)
		{
#if DEVICE_LOCAL_MEMORY
			vkMapMemory(GContext.lDevice.device, appRenderData.stagingBufferMemory, 0, dynamicAlignment * maxPrimMeshes, 0, &udata);
#else
			vkMapMemory(GContext.lDevice.device, appRenderData.uniformBufferMemory, 0, dynamicAlignment * maxPrimMeshes, 0, &udata);
#endif
		}

		return udata;
	}

	void unmapBufferPtr()
	{
#if DEVICE_LOCAL_MEMORY
		vkUnmapMemory(GContext.lDevice.device, appRenderData.stagingBufferMemory);
#else
		vkUnmapMemory(GContext.lDevice.device, appRenderData.uniformBufferMemory);
#endif 
		udata = nullptr;
	}

	void draw(const struct PrimitiveUniformObject* uniformData, const std::vector<int> primMeshes)
	{	
		size_t uboAlignment = GContext.gpu.deviceProps.limits.minUniformBufferOffsetAlignment;
		size_t dynamicAlignment = (sizeof(Primitive::PrimitiveUniformObject) / uboAlignment) * uboAlignment + ((sizeof(Primitive::PrimitiveUniformObject) % uboAlignment) > 0 ? uboAlignment : 0);
	
		VkResult res;
	
		//acquire an image from the swap chain
		uint32_t imageIndex;
	
		//using uint64 max for timeout disables it
		res = vkAcquireNextImageKHR(GContext.lDevice.device, GContext.swapChain.swapChain, UINT64_MAX, GContext.imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		
		if (GContext.frameFences[imageIndex])
		{
			// Fence should be unsignaled
			if (vkGetFenceStatus(GContext.lDevice.device, GContext.frameFences[imageIndex]) == VK_SUCCESS)
			{
				vkWaitForFences(GContext.lDevice.device, 1, &GContext.frameFences[imageIndex], true, 0);
			}
		}
		vkResetFences(GContext.lDevice.device, 1, &GContext.frameFences[imageIndex]);
	
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
		vkResetCommandBuffer(GContext.commandBuffers[imageIndex], VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		res = vkBeginCommandBuffer(GContext.commandBuffers[imageIndex], &beginInfo);

		assert(res == VK_SUCCESS);

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = dynamicAlignment * primMeshes.size();

#if DEVICE_LOCAL_MEMORY
		if (activeFrame) vkCmdCopyBuffer(GContext.commandBuffers[imageIndex], appRenderData.stagingBuffer, appRenderData.uniformBuffer, 1, &copyRegion);
#endif

#if ENABLE_VK_TIMESTAMP
		vkCmdResetQueryPool(GContext.commandBuffers[imageIndex], appRenderData.queryPool, 0, 2);
#endif

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = appRenderData.renderPass;
		renderPassInfo.framebuffer = appRenderData.swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = GContext.swapChain.extent;
	
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		vkCmdBeginRenderPass(GContext.commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(GContext.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, appRenderData.blockMaterial.gfxPipeline);

#if ENABLE_VK_TIMESTAMP
		vkCmdWriteTimestamp(GContext.commandBuffers[imageIndex], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, appRenderData.queryPool, 0);
#endif
		for (int i = 0; i < primMeshes.size(); ++i)
		{
			Mesh* mesh = GetMeshData(primMeshes[i]);
			uint32_t dynamicOffset = i * static_cast<uint32_t>(dynamicAlignment);
			VkBuffer vertexBuffers[] = { mesh->vBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindDescriptorSets(GContext.commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, appRenderData.blockMaterial.pipelineLayout, 0, 1, &appRenderData.descriptorSet, 1, &dynamicOffset);
	
			vkCmdBindVertexBuffers(GContext.commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(GContext.commandBuffers[imageIndex], mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT16);
			vkCmdDrawIndexed(GContext.commandBuffers[imageIndex], static_cast<uint32_t>(mesh->indexCount), 1, 0, 0, 0);
		}

#if ENABLE_VK_TIMESTAMP
		vkCmdWriteTimestamp(GContext.commandBuffers[imageIndex], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, appRenderData.queryPool, 1);
#endif
		vkCmdEndRenderPass(GContext.commandBuffers[imageIndex]);
		res = vkEndCommandBuffer(GContext.commandBuffers[imageIndex]);
		assert(res == VK_SUCCESS);
	
		//wait on writing colours to the buffer until the semaphore says the buffer is available
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
		VkSemaphore waitSemaphores[] = { GContext.imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
	
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &GContext.commandBuffers[imageIndex];
	
		VkSemaphore signalSemaphores[] = { GContext.renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;
	
		res = vkQueueSubmit(GContext.lDevice.graphicsQueue, 1, &submitInfo, GContext.frameFences[imageIndex]);
		assert(res == VK_SUCCESS);
	
	
		//finally, present this thing on screen
	
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
	
		VkSwapchainKHR swapChains[] = { GContext.swapChain.swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional
		res = vkQueuePresentKHR(GContext.lDevice.transferQueue, &presentInfo);
	
		if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
		{
			handleScreenResize(appRenderData);  
		}

#if ENABLE_VK_TIMESTAMP
		//log performance data:

		struct TimestampResultSet {
			uint32_t begin = 0;
			uint32_t end = 0;
		};

		constexpr int sampleCountReportThreshold = 1000;
		static std::string resultFilePath;
		static int frameCount = 0;
		static float sampleSum = 0.f;
		static std::vector<float> samples;

		float timestampPeriod = GContext.gpu.deviceProps.limits.timestampPeriod;

		if (frameCount == 0) {
			{
				std::stringstream strstr;
				strstr << "../local/results[DLM-" << DEVICE_LOCAL_MEMORY << "][SF-" << SKIP_FRAMES << "].ubjson";
				resultFilePath = strstr.str();
			}

			printf("\nVK timestamp benchmark settings\n  timestampPeriod: %f ns\n  DEVICE_LOCAL_MEMORY: %u\n  SKIP_FRAMES: %u\n\n",
				timestampPeriod, DEVICE_LOCAL_MEMORY, SKIP_FRAMES);

			std::ifstream ifs(resultFilePath, std::ios::binary);
			if (ifs.good()) {
				nlohmann::json json = nlohmann::json::from_ubjson(ifs);
				samples = json.get<std::vector<float>>();
				for (float&sample : samples)
					sampleSum += sample;
				printf("Loaded %u samples from file.\n", (unsigned int)samples.size());
			}
		}

		TimestampResultSet timestampResultSet;
		vkGetQueryPoolResults(GContext.lDevice.device, appRenderData.queryPool, 0, 2, sizeof(TimestampResultSet), &timestampResultSet, offsetof(TimestampResultSet, end), VK_QUERY_RESULT_WAIT_BIT);
		float diff = timestampPeriod * (timestampResultSet.end - timestampResultSet.begin);
		float diff_ms = diff / 1e6f;
		samples.push_back(diff_ms);
		sampleSum += diff_ms;

		++frameCount;
		if (samples.size() % sampleCountReportThreshold == 0) {
			float sampleMean = sampleSum / (float)samples.size();
			float correctedSampleStandardDeviation = 0.f;
			{
				// Compute the correctedSampleStandardDeviation
				for (float&sample : samples) {
					float diff = sample - sampleMean;
					correctedSampleStandardDeviation += diff * diff;
				}
				correctedSampleStandardDeviation /= (float)(samples.size() - 1);
				correctedSampleStandardDeviation = std::sqrt(correctedSampleStandardDeviation);
			}
			printf("VK timestamp benchmark results\n  Samples: %u\n  Mean: %f ms\n  StdDev: %f ms\n  Sum: %f ms\n\n",
				(unsigned int)samples.size(), sampleMean, correctedSampleStandardDeviation, sampleSum);

			std::ofstream ofs(resultFilePath, std::ios::binary);
			if (ofs.good()) {
				nlohmann::json json = samples;
				std::vector<uint8_t> data = nlohmann::json::to_ubjson(json);
				ofs.write((const char *)data.data(), data.size());
			} else {
				printf("Failed to write samples to file!\n");
			}

			// samples.clear();
			// sampleSum = 0;
		}
#endif

	}
}
