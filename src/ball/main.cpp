#include <windowsVulkanArch.h>
#include <shaderVulkanArch.h>
#include <cameraHelper.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <buffer.h>
#include <chrono>

/*
 * This sence will draw a light move around a ball
 */
static vkGraphicsDevice * vulkanDevice = new vkGraphicsDevice(1280, 720);
vulkanShader* vs = new vulkanShader("../src/ball/meta/vs.spv", VK_SHADER_STAGE_VERTEX_BIT);
vulkanShader* ps = new vulkanShader("../src/ball/meta/ps.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

const int MAX_FRAMES_IN_FLIGHT = 2;

#define PIPELINE_CREATE_RELATED_CODE
#define FRAMEBUFFER_IMGAE_FINAL_CONTAINER_CODE
#define COMMAND_BUFFER_RELATED_CODE
#define RECORD_COMMAND_INTO_COMMAND_BUFFER_CODE
#define MAIN_LOOP_CODE

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
 * add vertex buffer and indices
 */
struct Vertex {
    glm::vec3 position;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription(){

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0; // specifies the index of the binding in the arrary of bindings,now only one
        bindingDescription.stride = sizeof(Vertex);//specifies the numver of bytes from one entry to the next,now is size of Vertex
        /*
         * VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex
         * VK_VERTEX_INPUT_RATE_INSTANCE: Move to the next data entry after each instance ,about instanced rendering
         */
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription; 
    }

    /*
     * 2: attribute descriptions
     * we hve two attribute ,so we need two vertexInputAttributeDescription pos and color
     * an attribute description struct describes how to extract a vertex attribute from chunk of vertex data 
     * originating from a binding description.
     * 
     * at one word, descript vertex buffer format , how does data Gather together as one binary
     */
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0; //which binding the per-vertex data comes
        attributeDescriptions[0].location = 0;//which location of the input in the vertex shader , now is 0
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;//data format in vertex buffer 
        attributeDescriptions[0].offset = offsetof(Vertex, position);//offset of pos in vertex

        attributeDescriptions[1].binding = 0; //which binding the per-vertex data comes
        attributeDescriptions[1].location = 1;//which location of the input in the vertex shader , now is 0
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;//data format in vertex buffer 
        attributeDescriptions[1].offset = offsetof(Vertex, color);//offset of pos in vertex

        return attributeDescriptions;
    }


};
/*
 * in this case we want to draw a ball
 * so we should cal ball vertex data
 */
//draw a ball
const int Y_SEGMENTS = 50;
const int X_SEGMENTS = 50;
const GLfloat  PI = 3.14159265358979323846f;
std::vector<Vertex> vertices;

void createBallVertex(void)
{

	// 生成球的顶点
	for (int y = 0; y <= Y_SEGMENTS; y++)
	{
		for (int x = 0; x <= X_SEGMENTS; x++)
		{
			float xSegment = (float)x / (float)X_SEGMENTS;
			float ySegment = (float)y / (float)Y_SEGMENTS;
			float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
			float yPos = std::cos(ySegment * PI);
			float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

            struct Vertex v;
            v.position = glm::vec3(xPos,yPos,zPos);
            v.color =glm::vec3(0.0,0.0,1.0);

			vertices.push_back(v);
		}
	}
}
std::vector<uint16_t> indices;
void createBallVertexInd()
{
	// 生成球的Indices
	for (int i = 0; i < Y_SEGMENTS; i++)
	{
		for (int j = 0; j < X_SEGMENTS; j++)
		{
 
			indices.push_back(i * (X_SEGMENTS+1) + j);
			indices.push_back((i + 1) * (X_SEGMENTS + 1) + j);
			indices.push_back((i + 1) * (X_SEGMENTS + 1) + j + 1);
 
			indices.push_back(i * (X_SEGMENTS + 1) + j);
			indices.push_back((i + 1) * (X_SEGMENTS + 1) + j + 1);
			indices.push_back(i * (X_SEGMENTS + 1) + j + 1);
		}
	}
}

//draw a simple light box
void crateBoxVertex(void){
    std::vector<glm::vec3> boxVertexs={
        {-0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f, -0.5f},
        {-0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f},
        { 0.5f,  0.5f,  0.5f},
        { 0.5f,  0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f},
        {-0.5f, -0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f},
        {-0.5f,  0.5f, -0.5f},
        {-0.5f, -0.5f, -0.5f},
        {-0.5f, -0.5f, -0.5f},
        {-0.5f, -0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f},
        { 0.5f,  0.5f,  0.5f},
        { 0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},
        { 0.5f,  0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f},
        {-0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f,  0.5f},
        { 0.5f, -0.5f,  0.5f},
        {-0.5f, -0.5f,  0.5f},
        {-0.5f, -0.5f, -0.5f},
        {-0.5f,  0.5f, -0.5f},
        { 0.5f,  0.5f, -0.5f},
        { 0.5f,  0.5f,  0.5f},
        { 0.5f,  0.5f,  0.5f},
        {-0.5f,  0.5f,  0.5f},
        {-0.5f,  0.5f, -0.5f},
    };
    struct Vertex v;

	for (auto boxVertex : boxVertexs)
	{
        v.position = glm::vec3(boxVertex[0],boxVertex[1],boxVertex[2]);
        v.color = glm::vec3(1.0,1.0,1.0);

        vertices.push_back(v); 
	}

}

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
void createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    //create staging buffer
    printf("vertex buffer size = %d\r\n",bufferSize);
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer( 
        vulkanDevice->device,  /*logic device*/
        vulkanDevice->physicalDevice, /*physical device*/
        bufferSize,  /*buffer size*/
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  /*this is a transfer source buffer*/
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,  /*find memory section in physical device condition , cpu visable and non-cacheable*/
        stagingBuffer,  /*return  handle*/
        stagingBufferMemory  /*return Memory handle*/
    );
    
    //copy vertex data into staging buffer
    void* data;
    vkMapMemory(vulkanDevice->device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(vulkanDevice->device, stagingBufferMemory);

    //create vertex buffer
    createBuffer( 
        vulkanDevice->device,  /*logic device*/
        vulkanDevice->physicalDevice, /*physical device*/
        bufferSize,  /*buffer size*/
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,  /*this is a transfer dest buffer and of course it is vertex buffer*/
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,  /*we can use gpu local memory which cpu can not touch , and it can not be map*/
        vertexBuffer,  /*return vertexBuffer handle*/
        vertexBufferMemory  /*return vertexBufferMemory handle*/
    );

    copyBuffer(stagingBuffer, vertexBuffer, bufferSize, vulkanDevice->device,vulkanDevice->transferQueue,commandPool);

    vkDestroyBuffer(vulkanDevice->device, stagingBuffer, nullptr);
    vkFreeMemory(vulkanDevice->device, stagingBufferMemory, nullptr);
}
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;
void createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size(); //cal vertex buffer size
    
    //create a stagingBuffer, this buffer can visable by cpu and stored vertex data
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer( 
        vulkanDevice->device,  /*logic device*/
        vulkanDevice->physicalDevice, /*physical device*/
        bufferSize,  /*buffer size*/
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  /*this is a transfer source buffer*/
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,  /*find memory section in physical device condition , cpu visable and non-cacheable*/
        stagingBuffer,  /*return  handle*/
        stagingBufferMemory  /*return Memory handle*/
    );

    /*initial data into staging buffer*/
    void* data;
    vkMapMemory(vulkanDevice->device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(vulkanDevice->device, stagingBufferMemory);

    //now we do not copy data to vetex buffer , copy opertion will happend by GPU
    //gpu will transfer stagingbuffer data to vertex buffer
    
    createBuffer( 
        vulkanDevice->device,  /*logic device*/
        vulkanDevice->physicalDevice, /*physical device*/
        bufferSize,  /*buffer size*/
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,  /*this is a transfer dest buffer and of course it is index buffer*/
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,  /*we can use gpu local memory which cpu can not touch , and it can not be map*/
        indexBuffer,  /*return vertexBuffer handle*/
        indexBufferMemory  /*return vertexBufferMemory handle*/
    );

    //we can copy stagingbuffer to vertex buffer use command buffer and queue commit
    copyBuffer(stagingBuffer, indexBuffer, bufferSize, vulkanDevice->device,vulkanDevice->transferQueue,commandPool);

    vkDestroyBuffer(vulkanDevice->device, stagingBuffer, nullptr);
    vkFreeMemory(vulkanDevice->device, stagingBufferMemory, nullptr);
}

/*uniform MPV object format*/
struct UniformBufferObject{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

/*this function create a uniform buffer , and map it*/
std::vector<VkBuffer> uniformBuffers;
std::vector<VkDeviceMemory> uniformBuffersMemory;
std::vector<void*> uniformBuffersMapped;
void createUniformBuffers(){
    VkDeviceSize bufferSize = sizeof(UniformBufferObject) * 2;

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            
        createBuffer( 
            vulkanDevice->device,  /*logic device*/
            vulkanDevice->physicalDevice, /*physical device*/
            bufferSize,  /*buffer size*/
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,  /*this is a uniform buffer*/
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,  /*find memory section in physical device condition , cpu visable and non-cacheable*/
            uniformBuffers[i],  /*return  handle*/
            uniformBuffersMemory[i]  /*return Memory handle*/
        );
        /*uniform buffer is persistent mapping*/
        vkMapMemory(vulkanDevice->device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}
float now_radians = 0;
void updateUniformBuffer(uint32_t currentImage) {

    UniformBufferObject ubo[2];
    ubo[0].model = glm::mat4(1.0);
    ubo[0].view = vulkanDevice->camera.matrices.view;
    ubo[0].proj = vulkanDevice->camera.matrices.perspective;

    now_radians += 0.00360;
    if(now_radians >= 360.0){
        now_radians = 0; 
    }

    ubo[1].model = glm::translate(glm::mat4(1.0),glm::vec3(std::sin(glm::radians(now_radians)) *1.5 ,std::cos(glm::radians(now_radians)) * 1.5,0.0));
    ubo[1].model = glm::scale(ubo[1].model,glm::vec3(0.1,0.1,0.1));
    ubo[1].view = vulkanDevice->camera.matrices.view;
    ubo[1].proj = vulkanDevice->camera.matrices.perspective;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

VkDescriptorPool descriptorPool;
void createDescriptorPool()
{
    /* 
     * we should konw how descriptor size and type
     * now we only want to have only one uniform type descript
     */
    std::array<VkDescriptorPoolSize, 1> poolSizes{};

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //each frame have corresbinding descriptor 
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

    //create descriptor pool
    if (vkCreateDescriptorPool(vulkanDevice->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}


/*we will create a descriptor set layout(blue print of descriptor)*/
VkDescriptorSetLayout descriptorSetLayout;
void createDescriptorSetLayout() {

    /*
     * there are many discriptor type ,uniform is one of the descriptor type
     * when we create a descriptor set we should point out which descriptor type it is , just like buffer type
     * it can support create many descript use count.
     */
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    /*
     * we also need to specify in which shader stages the descriptor is going to be referenced
     * in this case , only vertex shader will access uniform
     */
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    /*
     * the pImmutableSamplers filed is only relevant for image sampling related descriptors.
     */
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional


    /*
     * create a acutally descriptor Set layout
     * now we have two binding point
     */
    std::array<VkDescriptorSetLayoutBinding, 1> bindings = {uboLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(vulkanDevice->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

std::vector<VkDescriptorSet> descriptorSets;
void createDescriptorSets(){
    //create a array that has same value descriptorSetLayout
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT * 2, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};

    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;
    allocInfo.pSetLayouts = layouts.data(); //layout has uniform format  information(descriptorSetLayout)

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT * 2);

    if (vkAllocateDescriptorSets(vulkanDevice->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    //above , first create a descriptor set from descriptor pool with bluprint descriptorLayout

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        //this struct link descript set we created before in this function and uniform buffer created before 
        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i]; //get descriptor set object
        descriptorWrites[0].dstBinding = 0;//uniform buffer , now it is set as 0 in shader
        descriptorWrites[0].dstArrayElement = 0;//descriptor can be array so we specify 0
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;//descriptor type is uniform 
        descriptorWrites[0].descriptorCount = 1;//
        descriptorWrites[0].pBufferInfo = &bufferInfo;//link uniform buffer

        //link descript set(include descript layout (uniform data format)) and uniform buffer
        vkUpdateDescriptorSets(vulkanDevice->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = sizeof(UniformBufferObject);
        bufferInfo.range = sizeof(UniformBufferObject);

        //this struct link descript set we created before in this function and uniform buffer created before 
        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i+2]; //get descriptor set object
        descriptorWrites[0].dstBinding = 0;//uniform buffer , now it is set as 0 in shader
        descriptorWrites[0].dstArrayElement = 0;//descriptor can be array so we specify 0
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;//descriptor type is uniform 
        descriptorWrites[0].descriptorCount = 1;//
        descriptorWrites[0].pBufferInfo = &bufferInfo;//link uniform buffer

        //link descript set(include descript layout (uniform data format)) and uniform buffer
        vkUpdateDescriptorSets(vulkanDevice->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
    //above , second updeate actually resource point into descriptor
}


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

    //record binding index and format betweent each vertex data in vertex data array
    auto bindingDescription = Vertex::getBindingDescription();
    //record vertex buffer interal attribute , now we have two attribute in each vertex(pos and color)
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    /*vertex input related*/
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   //bake vertex description(such as vertex array format and vertex inner format information) into pipeline object
    // we just only use one description,refer to binding index in attributeDescriptions and bindingDescription
    //vertexBindingDescriptionCount is not 1 , is special usage
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    //tell vulkan how many attribute are there in the one vertex , now is two (pos ,color)
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;//bake to pipeline objecty
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();//bake to pipeline objecty
    //from now on ,we tell vulkan the format of the vertex buffer , then we should tell a initaial vertex data to vulkan

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
    pipelineLayoutInfo.setLayoutCount = 1; // descriptor related
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // descriptor related

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
uint32_t currentFrame = 0;
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

    /*update uniform data*/
    updateUniformBuffer(currentFrame);

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

    uint32_t uniformOffset[]={0,0,0,0};
    //bind descriptor set and layout
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame+2], 0, nullptr);

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {51 * 51 * 6 * sizeof(float)};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    //draw box
    vkCmdDraw(commandBuffer, 36, 1, 0, 0);
    
    // bind pipeline 
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    //dynamic state cmd , in this case we enable dynamic cmd

    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(vulkanDevice->swapChainExtent.width);
    viewport.height = static_cast<float>(vulkanDevice->swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    scissor.offset = { 0, 0 };
    scissor.extent = vulkanDevice->swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    //bind descriptor set and layout
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

    offsets[0] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    //bind index buffer
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    //draw ball
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    
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

    /*
     * camera init,
     * register key handle, mouse handle 
     */
    cameraDeviceInit(vulkanDevice);

    createBallVertex();
    createBallVertexInd();

    crateBoxVertex();
    /*
     * now we should crate a uniform buffer and update initial uniform data into it
     * when we want we can update uniform buffer data by updateUniformBuffer
     */
    createUniformBuffers();

    /* use uniform to tansfer camera data into GPU MPV Matrix
     * we need descript & descript set & descript layout & descript pool
     */

    /*first we need descriptor pool*/
    createDescriptorPool();
    /*
     * second we should create descriptor layout (descriptor blueprint)
     * this layout will bake to pipeline object
     * in follow function ,we will create a blue print of descriptor set descriptorSetLayout,and
     * it will bake into pipeline object pipelineLayoutInfo.pSetLayouts and use vkCreatePipelineLayout
     * to make it effect
     */
    createDescriptorSetLayout();

    /* 
     * then we will create descriptor set from descriptor pool with blueprint of descriptor layout and descriptor pool
     * This steup will actual define uniform resource, if sampler, you should point out vkImage as texture
     * if uniform , you should select uniform buffer as resource
     * in this case , we will update MPV uniform data into uniformBuffers ,
     * when we create descriptor set use createDescriptorSets this function will linke uniformBuffers
     * with descriptor set.
     */
    createDescriptorSets();

    createRenderPass();
    crateGraphicsPipeline();
    createFramebuffer();

    createCommandPool();
    createCommandBuffer();

    createVertexBuffer();
    createIndexBuffer();

    createSyncObjects();//loop sync object
    
    vulkanDevice->drawFrame = drawFrame;
    vulkanDevice->vkGraphicsDeviceHandle();

}

