#include <windowsVulkanArch.h>
#include <shaderVulkanArch.h>

static vkGraphicsDevice * vulkanDevice = new vkGraphicsDevice(800, 600);
vulkanShader* vs = new vulkanShader("../src/firstTriangle/meta/vs.spv", VK_SHADER_STAGE_VERTEX_BIT);
vulkanShader* ps = new vulkanShader("../src/firstTriangle/meta/ps.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

const int MAX_FRAMES_IN_FLIGHT = 2;

#define PIPELINE_CREATE_RELATED_CODE
#define FRAMEBUFFER_IMGAE_FINAL_CONTAINER_CODE
#define COMMAND_BUFFER_RELATED_CODE
#define RECORD_COMMAND_INTO_COMMAND_BUFFER_CODE
#define MAIN_LOOP_CODE

#ifdef PIPELINE_CREATE_RELATED_CODE
VkRenderPass renderPass;
void createRenderPass() {
    /*
     * first we will create attachement descripition ,
     * and tell renderpass how many attachement we will used and what the configuration it is.
     */
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = vulkanDevice->swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    /*
     * then we create subpass , and point out which attchemnt index the subpass will use
     * by VkAttachmentReference
     */
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    /*
     * point out dependency with different subpass
     * in this case , we only use single subpass , so this option is ignore.
     */
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    /*
     * use follow inoformation to bake a renderpass
     * 1. attachement format stored in colorAttachment
     * 2. subpass info(which include this subpass's attachement index in colorAttachement array)
     * 3. dependency between subpass
     *
     * Note : subpass final attchament object is selected in framebuffer
     * framebufferInfo.pAttachments;
     * in this case it is vkImageView which warp form vkImage by swap chain extension
     */
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;  //attachment info 
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;//subpass(include subpass)
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    if (vkCreateRenderPass(vulkanDevice->device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
    printf("create Renderpass Success\r\n");
}

VkPipeline graphicsPipeline;
VkPipelineLayout pipelineLayout;
void crateGraphicsPipeline() {

    VkPipelineShaderStageCreateInfo vsShaderStageInfo = vs->getShaderStageInfo(vulkanDevice->device);
    VkPipelineShaderStageCreateInfo psShaderStageInfo = ps->getShaderStageInfo(vulkanDevice->device);

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vsShaderStageInfo,
        psShaderStageInfo,
    };

    /*vertex input related*/
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;

    /*input assembly related*/
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    /*viewport and scissor related default used as dynamic state*/
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    /*rasterizer related*/
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    /*multisampling*/
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    /*blending related*/
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    /*dynamic state related*/
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    /*pipeline layout related*/
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional

    if (vkCreatePipelineLayout(vulkanDevice->device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    /*sum all informat bake to pipelineInfo*/
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    /*
     * finally,shader is instert into pipeline object in form of VkPipelineShaderStageCreateInfo array
     * 1. compile GLSI/HLSI source code into byte-code
     * 2. warp byte-code into VkShaderModule
     * 3. warp VkShaderModule into VkPipelineShaderStageCreateInfo(this step pointout shader type)
     * 4. warp all shader createinfo into array and finally bake into pipeline object shaderStages
     */
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    /*
     * if we want use vertex buffer object as vertex shader input
     * some vertex buffer data format configuration should be point out here
     */
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    /*
     * point out primitive type
     * and primitiveRestartEnable mode , if we want to use element buffer
     */
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    /*
     * viewport and sisscor configuration
     * if use dynamic state , we do not pointout viewport and sicssor configuration here ,but number of viewport and sicssor should be selected
     * else we should pointout viewport and sicssor configuration
     */
    pipelineInfo.pViewportState = &viewportState;
    /*
     * point out shader mode , fill or edge
     * cullMode etc rasterizer configuration
     */
    pipelineInfo.pRasterizationState = &rasterizer;
    /*mutisampling*/
    pipelineInfo.pMultisampleState = &multisampling;
    /*blending*/
    pipelineInfo.pColorBlendState = &colorBlending;
    /*dynamic state*/
    pipelineInfo.pDynamicState = &dynamicState;
    /*uniform related configuration*/
    pipelineInfo.layout = pipelineLayout;
    /*render pass*/
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    //final create pipeline object
    if (vkCreateGraphicsPipelines(vulkanDevice->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    printf("create pipeline Success\r\n");
}
#endif

#ifdef FRAMEBUFFER_IMGAE_FINAL_CONTAINER_CODE
/*
 * image create flow
 * 1, get vkImage from swap chain extension
 *      vkGetSwapchainImagesKHR(vulkanDevice->device, swapChain, &imageCount, nullptr);
 *      swapChainImages.resize(imageCount);
 *      vkGetSwapchainImagesKHR(vulkanDevice->device, swapChain, &imageCount, swapChainImages.data());
 * 2, warp vkImage into vkImageView
 *      createImageViews()
 * 3, warp ImageViews into framebuffer
 */
std::vector<VkFramebuffer> swapChainFramebuffers;
void createFramebuffer() {
    /*resize swapChainFramebuffers by vkImageView*/
    swapChainFramebuffers.resize(vulkanDevice->swapChainImageViews.size());

    for (size_t i = 0; i < vulkanDevice->swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {
            vulkanDevice->swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;//select renderpass we create before
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = vulkanDevice->swapChainExtent.width;
        framebufferInfo.height = vulkanDevice->swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(vulkanDevice->device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}
#endif

/*
 * Commands in Vulkan, like drawing operations and memory transfers,
 * are not executed directly using function calls.
 * You have to record all of the operations you want to perform in command buffer objects
 *
 * first will should create CommandPool and then alloc a command buffer for recording command
 * second , alloc command buffer from command buffer pool
 */
#ifdef COMMAND_BUFFER_RELATED_CODE
VkCommandPool commandPool;
std::vector<VkCommandBuffer> commandBuffers;
void createCommandPool() {
    QueueFamilyIndices queueFamilyIndices = vulkanDevice->findQueueFamilies(vulkanDevice->physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    //get graphics queue index in findQueueFamilies
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(vulkanDevice->device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

/*secondary we can alloc a turely command buffer from commandPool we create before */
void createCommandBuffer() {

    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(vulkanDevice->device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}
#endif

/*
 * record command flow
 * 1. begin command buffer
 * 2. begin renderpass
 * 3. record draw command
 *    3.1 record bind pipeline command
 *    3.2 if enable dynamic pipeline state ,
 *           dynamic pipeline state info should be recorded here
 *    3.3 actually draw command
 * 4. end renderpass
 * 5. end command buffer
 */
#ifdef RECORD_COMMAND_INTO_COMMAND_BUFFER_CODE
void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];

    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = vulkanDevice->swapChainExtent;

    VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // bind pipeline 
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    //dynamic state cmd , in this case we enable dynamic cmd
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(vulkanDevice->swapChainExtent.width);
    viewport.height = static_cast<float>(vulkanDevice->swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = vulkanDevice->swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    //draw command
    /*
     *vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
     *instanceCount: Used for instanced rendering, use 1 if you're not doing that.
     *firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
     *firstInstance: Used as an offset for instanced rendering, defines the lowest value of
     */
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}
#endif

/*
 * rendering a frame in vulkan consists of a common set of steps
 * 1. wait for the previous frame to finish
 * 2. acquire an image from the swap chain
 * 3. record a command buffer which draws the scene onto that image
 * 4. submit the recorded command buffer
 * 5. present the swap chain image
 *
 * for Synchronization , we will use two object
 * 1, semaphores , A semaphore is used to add order between queue operations(such as present queue and graphics queue)
 * 2, fench , sync command recorded to command buffer with CPU host
 */
std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinishedSemaphores;
std::vector<VkFence> inFlightFences;
#ifdef MAIN_LOOP_CODE
void createSyncObjects() {

    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // for passing first frame wait 

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(vulkanDevice->device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(vulkanDevice->device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(vulkanDevice->device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}
uint32_t currentFrame = 0;
void drawFrame() {

    //At the start of the frame, we want to wait until the previous frame has finished, 
    //so that the command buffer and semaphores are available to use
    vkWaitForFences(vulkanDevice->device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    //we should manually reset the fench to unsignaled state
    vkResetFences(vulkanDevice->device, 1, &inFlightFences[currentFrame]);
    /*
     * logic vulkanDevice->device handle
     * swapchain
     * timeout
     * imageAvailableSemaphore specify synchronization objects that are to be signaled when the presentation engine is finished using the image
     * imageIndex : which swapchain image is available
     */
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(vulkanDevice->device, vulkanDevice->swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    // select which framebuffer shouled be used into renderpass ,when begain renderpass.
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    //submit command buffer(which record lots of command) into queue
    /*
     * wait imageAvailableSemaphore to be signaled
     * the pipeline will wait in state of VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT until imageAvailableSemaphore be signaled
     *
     * We want to wait with writing colors to the image until it's available,
     * so we're specifying the stage of the graphics pipeline that writes to the color attachment.
     * That means that theoretically the implementation can already start executing our vertex shader and such while the image is not yet available.
     * Each entry in the waitStages array corresponds to the semaphore with the same index in pWaitSemaphores.
     *
     * pipelien stop at VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT ,
     * lots of pipeline work has been done before VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
     *
     */
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;// which semaphores to wait on before execution begins 
    submitInfo.pWaitDstStageMask = waitStages;// in which stage(s) of the pipeline to wait

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    //when command all work done it will singal signalSemaphores to inform other queue(present queue in this case)
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vulkanDevice->graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = { vulkanDevice->swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(vulkanDevice->presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vulkanDevice->framebufferResized) {
        vulkanDevice->framebufferResized = false;
        vulkanDevice->recreateSwapChain();
    }
    else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}
#endif

int main() {
    createRenderPass();
    crateGraphicsPipeline();
    createFramebuffer();

    createCommandPool();
    createCommandBuffer();

    createSyncObjects();//loop sync object

    vulkanDevice->drawFrame = drawFrame;
    vulkanDevice->vkGraphicsDeviceHandle();

}

