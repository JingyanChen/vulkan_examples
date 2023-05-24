#include <windowsVulkanArch.h>
#include <shaderVulkanArch.h>
#include <cameraHelper.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <buffer.h>
#include <chrono>
#include <image.h>

static vkGraphicsDevice * vulkanDevice = new vkGraphicsDevice(1280, 720);
vulkanShader* vs = new vulkanShader("../src/t_pipeline/meta/vs.spv", VK_SHADER_STAGE_VERTEX_BIT);
vulkanShader* ps = new vulkanShader("../src/t_pipeline/meta/ps.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

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
	float position[3];
	float color[2];

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

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        return attributeDescriptions;
    }


};

//    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
//    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
std::vector<Vertex> vertices =
{
    {{-0.5f, -0.5f, -0.5f},  {0.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f},  {1.0f, 1.0f}},
    {{ 0.5f, -0.5f, -0.5f},  {1.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f},  {1.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f},  {0.0f, 0.0f}},
    {{-0.5f,  0.5f, -0.5f},  {0.0f, 1.0f}},

    {{-0.5f, -0.5f,  0.5f},  {0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f},  {1.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f},  {1.0f, 1.0f}},
    {{ 0.5f,  0.5f,  0.5f},  {1.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.5f},  {0.0f, 1.0f}},
    {{-0.5f, -0.5f,  0.5f},  {0.0f, 0.0f}},

    {{-0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}},
    {{-0.5f,  0.5f, -0.5f},  {1.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f},  {0.0f, 1.0f}},
    {{-0.5f, -0.5f, -0.5f},  {0.0f, 1.0f}},
    {{-0.5f, -0.5f,  0.5f},  {0.0f, 0.0f}},
    {{-0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}},
 
    {{ 0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f},  {0.0f, 1.0f}},
    {{ 0.5f,  0.5f, -0.5f},  {1.0f, 1.0f}},
    {{ 0.5f, -0.5f, -0.5f},  {0.0f, 1.0f}},
    {{ 0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f},  {0.0f, 0.0f}},

    {{-0.5f, -0.5f, -0.5f},  {0.0f, 1.0f}},
    {{ 0.5f, -0.5f, -0.5f},  {1.0f, 1.0f}},
    {{ 0.5f, -0.5f,  0.5f},  {1.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f},  {1.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f},  {0.0f, 0.0f}},
    {{-0.5f, -0.5f, -0.5f},  {0.0f, 1.0f}},

    {{-0.5f,  0.5f, -0.5f},  {0.0f, 1.0f}},
    {{ 0.5f,  0.5f, -0.5f},  {1.0f, 1.0f}},
    {{ 0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f},  {1.0f, 0.0f}},
    {{-0.5f,  0.5f,  0.5f},  {0.0f, 0.0f}},
    {{-0.5f,  0.5f, -0.5f},  {0.0f, 1.0f}}

};

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
    stbi_uc* pixels = stbi_load("../src/t_pipeline/texture/wl.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
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

VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
void createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    //create staging buffer

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
    ubo.model = glm::mat4(1.0f);
    ubo.view = vulkanDevice->camera.matrices.view;
    ubo.proj = vulkanDevice->camera.matrices.perspective;

    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

/*
 * we should crate a depth attachment , and it is based on an image.so we need a vkImage(its memory and warp imageView required)
 * swap chain will not automatically create depth images 
 * 1 create a vkImage for depth format
 * 2 warp vkImage into a vkImageView
 */
VkImage depthImage;
VkDeviceMemory depthImageMemory;
VkImageView depthImageView;
void createDepthResources() {
    /*
     * note it should have the same resolution as the color attachment defined by the swap chain extent
     * an image usage appropriate for a depth attachment,optimal tiling and device local memory
     * it's format should contain a depth component , indicated by _D??_ in the VK_FORMAT
     * 
     * depth format just like 
     * VK_FORMAT_D32_SFLOAT
     * VK_FORMAT_D32_SFLOAT_S8_UINT
     * VK_FORMAT_D24_UNORM_S8_UINT
     */

    VkFormat depthFormat = findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, //candidates format
        VK_IMAGE_TILING_OPTIMAL,//tile
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,//feature 
        vulkanDevice->physicalDevice
    );

    /*create vkImage*/
    createImage(vulkanDevice->device,
                vulkanDevice->physicalDevice,
                vulkanDevice->swapChainExtent.width, vulkanDevice->swapChainExtent.height,  depthFormat, VK_IMAGE_TILING_OPTIMAL,  //vkImage attribitue
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, //vkImage related memory attribute
                depthImage, depthImageMemory);
    //warp vkImage into vkImaeView, point out it is a VK_IMAGE_ASPECT_DEPTH_BIT and it's format
    depthImageView = createImageViewDefault(vulkanDevice->device, 
                           depthImage,
                           depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);//vkImage foramt
    /*
     * when we crate a vkImage by createImage it's default layout is VK_IMAGE_LAYOUT_UNDEFINED , so we should
     * transfer its layout into  VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL layout use barrier
     */

    transitionImageLayout(depthImage, //vkImage we create before
                         depthFormat,//vkImage format
                         VK_IMAGE_LAYOUT_UNDEFINED,//old image layout , do not care
                         VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,//new image layout
                         //gpu commit comotent
                         vulkanDevice->device,
                         vulkanDevice->transferQueue,
                         commandPool);

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
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

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
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding,samplerLayoutBinding};
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 2;
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(vulkanDevice->device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

std::vector<VkDescriptorSet> descriptorSets;
void createDescriptorSets(){
    //create a array that has same value descriptorSetLayout
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};

    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data(); //layout has uniform format  information(descriptorSetLayout)

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    if (vkAllocateDescriptorSets(vulkanDevice->device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    //above , first create a descriptor set from descriptor pool with bluprint descriptorLayout

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
    //above , second updeate actually resource point into descriptor
}


#ifdef PIPELINE_CREATE_RELATED_CODE
VkRenderPass renderPass;
void createRenderPass() {

    /* now we should create attachement description for depth*/
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}, //candidates format
        VK_IMAGE_TILING_OPTIMAL,//tile
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,//feature 
        vulkanDevice->physicalDevice
    );
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


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

    /*now we should create another attachement reference*/
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;//depth layout 

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;


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
    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();  //attachment info 
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;//subpass(include subpass)
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    if (vkCreateRenderPass(vulkanDevice->device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
    printf("create Renderpass Success\r\n");
}

VkPipeline graphicsPipelineSoild;
VkPipelineLayout pipelineLayout;
void createGraphicsPipelineSoild() {

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

    /*
     * in this case , we will enable depth test by filling VkPipelineDepthStencilStateCreateInfo 
     * and attach it into pipeline object pDepthStencilState  slots
     * in VkPipelineDepthStencilStateCreateInfo we will configure some depth test configuration
     */
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    /*
     * 
     * The depthTestEnable field specifies if the depth of new fragments should be compared to the depth buffer to see if they should be discarded. 
     * The depthWriteEnable field specifies if the new depth of fragments that pass the depth test should actually be written to the depth buffer.
     */
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    /*depth test cmpare mode*/
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    /*
     * optional configuration
     */
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional

    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional


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
    /*depth test */
    pipelineInfo.pDepthStencilState = &depthStencil;

    //final create pipeline object
    if (vkCreateGraphicsPipelines(vulkanDevice->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipelineSoild) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    printf("create pipeline soild Success\r\n");
}

VkPipeline graphicsPipelineLine;
void createGraphicsPipelineLine() {

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
    rasterizer.polygonMode = VK_POLYGON_MODE_LINE;
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

    /*
     * in this case , we will enable depth test by filling VkPipelineDepthStencilStateCreateInfo 
     * and attach it into pipeline object pDepthStencilState  slots
     * in VkPipelineDepthStencilStateCreateInfo we will configure some depth test configuration
     */
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    /*
     * 
     * The depthTestEnable field specifies if the depth of new fragments should be compared to the depth buffer to see if they should be discarded. 
     * The depthWriteEnable field specifies if the new depth of fragments that pass the depth test should actually be written to the depth buffer.
     */
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    /*depth test cmpare mode*/
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    /*
     * optional configuration
     */
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional

    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional


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
    /*depth test */
    pipelineInfo.pDepthStencilState = &depthStencil;

    //final create pipeline object
    if (vkCreateGraphicsPipelines(vulkanDevice->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipelineLine) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    printf("create pipeline Line Success\r\n");
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
            vulkanDevice->swapChainImageViews[i],
            depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;//select renderpass we create before
        framebufferInfo.attachmentCount = 2;
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

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // bind pipeline 
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineSoild);

    //dynamic state cmd , in this case we enable dynamic cmd
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(vulkanDevice->swapChainExtent.width)/2.0;
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

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

    //draw command
    /*
     *vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
     *instanceCount: Used for instanced rendering, use 1 if you're not doing that.
     *firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
     *firstInstance: Used as an offset for instanced rendering, defines the lowest value of
     */
    vkCmdDraw(commandBuffer, vertices.size(), 1, 0, 0);

    // bind pipeline 
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLine);

    //dynamic state cmd , in this case we enable dynamic cmd
    viewport.x = static_cast<float>(vulkanDevice->swapChainExtent.width)/2.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(vulkanDevice->swapChainExtent.width)/2.0;
    viewport.height = static_cast<float>(vulkanDevice->swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    //draw command
    /*
     *vertexCount: Even though we don't have a vertex buffer, we technically still have 3 vertices to draw.
     *instanceCount: Used for instanced rendering, use 1 if you're not doing that.
     *firstVertex: Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
     *firstInstance: Used as an offset for instanced rendering, defines the lowest value of
     */
    vkCmdDraw(commandBuffer, vertices.size(), 1, 0, 0);

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

    /*
     * camera init,
     * register key handle, mouse handle 
     */
    cameraDeviceInit(vulkanDevice);

    createCommandPool();
    createCommandBuffer();

    createTextureImage();
    createTextureImageView();
    createTextureSampler();

    createDepthResources();

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
    createGraphicsPipelineSoild();
    createGraphicsPipelineLine();
    createFramebuffer();

    createVertexBuffer();

    createSyncObjects();//loop sync object
    
    vulkanDevice->drawFrame = drawFrame;
    vulkanDevice->vkGraphicsDeviceHandle();

}

