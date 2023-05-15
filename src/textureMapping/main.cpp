#include <stdio.h>
#include <windowsVulkanArch.h>
#include <shaderVulkanArch.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <buffer.h>
#include <image.h>
/*
 * how to use uniform buffer
 * 1 create descript layout whcih include descript layout info  --- createDescriptorSetLayout
 * 2 create a pipeline layout object(use pSetLayouts ,not push const now) related to descript layout in crateGraphicsPipeline , 
 *   then we get a pipelineLayout
 * 3 create a uniform buffer , size is depend on uniform data size --- createUniformBuffers
 * 4 create a descript set which alloc from a descript pool -- createDescriptorPool and createDescriptorSets
 * 5 link descript set and uniform buffer by vkUpdateDescriptorSets -- createDescriptorSets
 * 6 finally record a bind descriptor command into command buffer link pipelineLayout in step 2 and descriptorset in step 4&5
 *   vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
 * in ap we can update uniform buffer by uniform buffer memory anytime to sync data with gpu. --  updateUniformBuffer
 * 
 * 
 * 
 * how to sampler texture in shadedr
 * 
 * 1 load texture from file and copy the image into a staging buffer(which will copy its data into a gpu local buffer by gpu commander)
 * 2 crate a vkImage object and alloc a memory for it(its usage is VK_IMAGE_USAGE_TRANSFER_DST_BIT for transfering buffer to image)
 *   set vkImage's layout is IMAGE_LAYOUT_UNDEFINE 
 * 3 transfer vkImage layout from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
 * 4 copy staging buffer texture data into vkImage layout(which is in VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL layout)
 * 5 transfer vkImage layout from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
 *   (createTextureImage)--from now on , we create a vkImage which store texture pixel data and layout is VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
 * 6 vulkan can not directly access vkImage ,so we should warp vkImage into vkImageView ---createTextureImageView
 * 7 (createTextureSampler)create sampler object and configure sampler attribute
 * 
 * from now on , we create a vkImageView and sampler , then we should input a texCoord form uniform
 * 
 * 8 add texCoord vertex data getBindingDescription tell vulkan how each vertex in vertex array layout ,point out stride
 *   between vertex in vertex array
 * 9 getAttributeDescriptions tell vulkan how many uniform data and what's the format and offset it is
 *   in this case , we have three uniform data pos color and texCoord , select its format and offset and loaction in shader
 *   when we want input data into vertex shader by vertex buffer , we should touch VkPipelineVertexInputStateCreateInfo->pVertexBindingDescriptions
 *   and pVertexAttributeDescriptions
 * 10 steup bindingDescription and attributeDescriptions into vertexInputyInfo when create pipeline object
 * 11 (createDescriptorSetLayout) add sampler descriptor layout when create descriptorSetLayout object then create a 
 *    pipeline layout by descriptorSetLayout when create pipeline object. in this case we add VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
 *    descript and it's binding = 1 into pipeline layout.
 * 12 create a descriptorPool and create descript set for two type of descriptor VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER and VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
 *    in createDescriptorSets flow , link sampler object and vkImageview finally call vkUpdateDescriptorSets to make descriptset effect
 * 
 * 13 modify shader code to sampler texture form texture we binding
 * 
 */

static vkGraphicsDevice * vulkanDevice = new vkGraphicsDevice(800, 600);
vulkanShader* vs = new vulkanShader("../src/textureMapping/meta/vs.spv", VK_SHADER_STAGE_VERTEX_BIT);
vulkanShader* ps = new vulkanShader("../src/textureMapping/meta/ps.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    //there are tow types of structures to describe vertex buffer

    /*
     * 1: binding description
     * describe at which rate to load data from memory throughout the vertices.
     * specifies 
     * 1 . number of bytes between data entries
     * 2 . whether to move to the next data entry after each vertex or after each instance
     * 
     */
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
     * 
     * here we add a new uniform as coordinates 
     */
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0; //which binding the per-vertex data comes
        attributeDescriptions[0].location = 0;//which location of the input in the vertex shader , now is 0
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;//data format in vertex buffer 
        attributeDescriptions[0].offset = offsetof(Vertex, pos);//offset of pos in vertex

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }

};
const std::vector<Vertex> vertices = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

//index data is follow
const std::vector<uint16_t> indices = {
    0, 1, 2, 2, 3, 0
};

struct UniformBufferObject{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

const int MAX_FRAMES_IN_FLIGHT = 2;
uint32_t currentFrame = 0;


#define PIPELINE_CREATE_RELATED_CODE
#define FRAMEBUFFER_IMGAE_FINAL_CONTAINER_CODE
#define COMMAND_BUFFER_RELATED_CODE
#define RECORD_COMMAND_INTO_COMMAND_BUFFER_CODE
#define VERTEX_BUFFER_CREATE_CODE
#define DESCRIPTOR_SET_LAYOUT_UNIFORM_CODE
#define SAMPLE_TEXTURE_RELATED_CODE
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


#ifdef SAMPLE_TEXTURE_RELATED_CODE

VkImage textureImage;
VkDeviceMemory textureImageMemory;
/*
 * This function load a texture from image file 
 * then create a stagingBuffer to store texture load from image file
 * 
 * create a vkImage and vkImage related
 */
void createTextureImage() {
    /*loading a picture from file*/
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load("../src/textureMapping/texture/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer( 
        vulkanDevice->device,  /*logic device*/
        vulkanDevice->physicalDevice, /*physical device*/
        imageSize,  /*buffer size*/
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,  /*this is a transfer source buffer*/
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,  /*find memory section in physical device condition , cpu visable and non-cacheable*/
        stagingBuffer,  /*return  handle*/
        stagingBufferMemory  /*return Memory handle*/
    );

    /*copy texture pixel data into staging buffer*/
    void* data;
    vkMapMemory(vulkanDevice->device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(vulkanDevice->device, stagingBufferMemory);

    stbi_image_free(pixels);

    //then we should create vkImage and related vkImage memory
    //this helper function crate vkImage inital layout is VK_IMAGE_LAYOUT_UNDEFINED
    createImage(vulkanDevice->device,
                vulkanDevice->physicalDevice,
                texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,  //vkImage attribitue
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, //vkImage related memory attribute
                textureImage, textureImageMemory);

    /*
     * trans image layout from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
     * because we will copy vkbuffer into vkImage(vkImage is the des in copy flow), 
     * so vkImage layout should be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
     * this function will use pipeline barrier to make sure image layout transition is done
     */
    /*
     * when transfer vkImage layout from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
     * pipeline barrier configuration is 
     * source stage : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
     * des stage : VK_PIPELINE_STAGE_TRANSFER_BIT
     * 
     * barriers.srcAccessMask = 0
     * barriers.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT
     * 
     * that means 
     * 1. do not need to wait any pipeline stage because source stage is VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
     * 2. opertion after barriers , will blocked its pipeline stage as VK_PIPELINE_STAGE_TRANSFER_BIT because dst stage = VK_PIPELINE_STAGE_TRANSFER_BIT
     * 
     * vulkan driver will make sure when vkImage layout transition complete , it will visible to VK_PIPELINE_STAGE_TRANSFER_BIT 
     * in the other wold , cache invalide will occurs , to make sure pipeline stage read up-to-date data which store in L2 chache
     */
    transitionImageLayout(textureImage, //vkImage we create before
                         VK_FORMAT_R8G8B8A8_SRGB,//vkImage format
                         VK_IMAGE_LAYOUT_UNDEFINED,//old image layout , do not care
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,//new image layout
                         //gpu commit comotent
                         vulkanDevice->device,
                         vulkanDevice->transferQueue,
                         commandPool);

    /*copy stagingBuffer vkBuffer(store texture form file) into vkImage(now vkImage have VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) */
    copyBufferToImage(stagingBuffer, //vkBuffer which store image
    textureImage, //vkImage which have VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL layout
    static_cast<uint32_t>(texWidth), //copy size
    static_cast<uint32_t>(texHeight), //copy size
    //gpu commit component
    vulkanDevice->device,
    vulkanDevice->transferQueue,
    commandPool);

    //change vkImage layout 
    transitionImageLayout(textureImage, //vkImage we create before
                         VK_FORMAT_R8G8B8A8_SRGB,//vkImage format
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,//old image layout , now is transfer dst layout
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,//new image layout
                         //gpu commit comotent
                         vulkanDevice->device,
                         vulkanDevice->transferQueue,
                         commandPool);

    /*
     * note : vkImage's layout change two times
     * 1, VK_IMAGE_LAYOUT_UNDEFINED -> VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL (for transfer copy as dst)
     * 2, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL -> VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL (for shader read)
     * 
     * two transition have order that 
     * shader reads should wait on transfer writes, 
     * specifically the shader reads in the fragment shader, because that's where we're going to use the texture
     */

}
/*
 * note: images are accessed through image views rather than directly
 */
VkImageView textureImageView;
void createTextureImageView() {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = textureImage;//vkImage which should be warped into vkImageView
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(vulkanDevice->device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }

}
/*
 * create texture sampler object
 */
VkSampler textureSampler;
void createTextureSampler(){
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;//Magnification concerns the oversampling problem
    samplerInfo.minFilter = VK_FILTER_LINEAR;//minification concerns undersampling

    //transform function in sampler
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    //anisotropic configure

    //first we inqure if physical device is support anisotropic
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(vulkanDevice->physicalDevice, &properties);

    //now we enable anisotropy function for better rendering result
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    //boarder color we transform as boarder
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

    //we always use [0,1] range on all axes
    samplerInfo.unnormalizedCoordinates = VK_FALSE;

    /*
     * if a comparison function is enable , then texels will first be compared to a value
     * and the result of that comparison is used in filtering operations.
     * mainly used for percentage-closer filtering on shadow maps.
     */
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

    /*mipmapping later to configure*/
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(vulkanDevice->device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

}
#endif

#ifdef VERTEX_BUFFER_CREATE_CODE
/*helper function , to copy buffer in gpu*/
static void _copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    
    //create a command buffer 
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vulkanDevice->device, &allocInfo, &commandBuffer);
    
    //begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    //record copy command into command buffer
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    //submit command buffer into queue

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(vulkanDevice->transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vulkanDevice->transferQueue);

    vkFreeCommandBuffers(vulkanDevice->device, commandPool, 1, &commandBuffer);
}

/*
 * we tell vulkan the veretx buffer foramt in create pipeline object 
 * then we should crate a vertex buffer as a vetex data container,
 * and initial a data into container as default vertex data
 * GPU can modify vertex data at runtime to commincate vertex info with shader
 */
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
void createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size(); //cal vertex buffer size
    
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
        memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(vulkanDevice->device, stagingBufferMemory);

    //now we do not copy data to vetex buffer , copy opertion will happend by GPU
    //gpu will transfer stagingbuffer data to vertex buffer
    
    createBuffer( 
        vulkanDevice->device,  /*logic device*/
        vulkanDevice->physicalDevice, /*physical device*/
        bufferSize,  /*buffer size*/
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,  /*this is a transfer dest buffer and of course it is vertex buffer*/
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,  /*we can use gpu local memory which cpu can not touch , and it can not be map*/
        vertexBuffer,  /*return vertexBuffer handle*/
        vertexBufferMemory  /*return vertexBufferMemory handle*/
    );

    //we can copy stagingbuffer to vertex buffer use command buffer and queue commit
    _copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

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
    _copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    vkDestroyBuffer(vulkanDevice->device, stagingBuffer, nullptr);
    vkFreeMemory(vulkanDevice->device, stagingBufferMemory, nullptr);
}
#endif

#ifdef DESCRIPTOR_SET_LAYOUT_UNIFORM_CODE
/*
 * we need to provide details about every descriptor binding used in the shaders for pipeline createion
 * this function create a descript layout object 
 */
VkDescriptorSetLayout descriptorSetLayout;
void createDescriptorSetLayout() {

    /*
     * there are many discriptor type ,uniform is oen of the descriptor type
     * wehen wen create a descriptor set we should point out which descriptor type it is , just like buffer type
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


    /*now we add a new uniform descript layout for sampler*/
    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;//point out this descript type is sampler
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;//used in ps

    /*
     * create a acutally descriptor Set layout
     * now we have two binding point
     */
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(vulkanDevice->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

/*this function create a uniform buffer , and map it*/
std::vector<VkBuffer> uniformBuffers;
std::vector<VkDeviceMemory> uniformBuffersMemory;
std::vector<void*> uniformBuffersMapped;
void createUniformBuffers(){
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            
        createBuffer( 
            vulkanDevice->device,  /*logic device*/
            vulkanDevice->physicalDevice, /*physical device*/
            bufferSize,  /*buffer size*/
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,  /*this is a uniform buffer*/
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,  /*find memory section in physical device condition , cpu visable and non-cacheable*/
            uniformBuffers[i],  /*return  handle*/
            uniformBuffersMemory[i]  /*return Memory handle*/
        );
        /*uniform buffer is persistent mapping*/
        vkMapMemory(vulkanDevice->device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

void updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), vulkanDevice->swapChainExtent.width / (float) vulkanDevice->swapChainExtent.height, 0.1f, 10.0f);

    ubo.proj[1][1] *= -1;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

//create a descriptor pool
VkDescriptorPool descriptorPool;
void createDescriptorPool(){
    /*
     * create a bigger descriptor pool 
     */
    std::array<VkDescriptorPoolSize, 2> poolSizes{};

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    //create descriptor pool
    if (vkCreateDescriptorPool(vulkanDevice->device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }

}

//create descripots set
std::vector<VkDescriptorSet> descriptorSets;
void createDescriptorSets(){
    //create a array that has same value descriptorSetLayout
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};

    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data(); //layout has uniform format  information(descriptorSetLayout)

    //create descrpitsets array
    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    if (vkAllocateDescriptorSets(vulkanDevice->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        /*binding sampler and vkImage here*/
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        //this struct link descript set we created before in this function and uniform buffer created before 
        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i]; //get descriptor set object
        descriptorWrites[0].dstBinding = 0;//uniform buffer , now it is set as 0 in shader
        descriptorWrites[0].dstArrayElement = 0;//descriptor can be array so we specify 0
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;//descriptor type is uniform 
        descriptorWrites[0].descriptorCount = 1;//
        descriptorWrites[0].pBufferInfo = &bufferInfo;//link uniform buffer

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1; //binding code relate to shader code : layout(binding = 1) uniform sampler2D texSampler;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;//descriptor type is sampler 
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;//link image info with vkImage and sampler

        //link descript set(include descript layout (uniform data format)) and uniform buffer
        vkUpdateDescriptorSets(vulkanDevice->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}
#endif

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

    /*
     * vertex input related
     * when we use vertex buffer function ,it will insert some configuration herer
     */
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    //record binding index and format betweent each vertex data in vertex data array
    auto bindingDescription = Vertex::getBindingDescription();
    //record vertex buffer interal attribute , now we have two attribute in each vertex(pos and color)
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    //bake vertex description(such as vertex array format and vertex inner format information) into pipeline object
    // we just only use one description,refer to binding index in attributeDescriptions and bindingDescription
    //vertexBindingDescriptionCount is not 1 , is special usage
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    //tell vulkan how many attribute are there in the one vertex , now is two (pos ,color)
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;//bake to pipeline objecty
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();//bake to pipeline object
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
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

    /*pipeline layout related uniform related*/
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1; // number of descriptor layout(include uniform format info)
    pipelineLayoutInfo.pSetLayouts  = &descriptorSetLayout; // descriptor set layout inster here, we do not use  push const now

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

    //bind vertex buffer when record command
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    //bind index buffer
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

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

    //bind descriptor set and layout
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

    //draw command with index draw
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

    /*update uniform data*/
    updateUniformBuffer(currentFrame);

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

    createCommandPool();
    createCommandBuffer();

    createVertexBuffer();
    createIndexBuffer();
    
    createTextureImage();
    createTextureImageView();
    createTextureSampler();

    /*uniform related*/
    createDescriptorSetLayout();
    createUniformBuffers();

    createDescriptorPool();
    createDescriptorSets();

    createRenderPass();
    crateGraphicsPipeline();
    createFramebuffer();

    createSyncObjects();//loop sync object

    vulkanDevice->drawFrame = drawFrame;
    vulkanDevice->vkGraphicsDeviceHandle();

}

