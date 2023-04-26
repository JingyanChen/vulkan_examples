
#include <stdio.h>
#include <windowsVulkanArch.h>
#include <shaderVulkanArch.h>
#include <buffer.h>
#include <commandBuffer.h>
/*
 * helper function if format has stencil
 */
bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void createImage(VkDevice device , VkPhysicalDevice phyDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;//initial layout is undefine
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(phyDevice,memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

/*
 * transfer image's layout form old to new
 * we want gpu to execute image layout transition and wati gpu opertion done 
 * so we use begin end command buffer helper function
 */
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout , VkDevice device , VkQueue Queue , VkCommandPool commandPool) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(device,commandPool);

    /*
     * One of the most common ways to perform layout transitions is using an image memory barrier. 
     * A pipeline barrier like that is generally used to synchronize access to resources, 
     * like ensuring that a write to a buffer completes before reading from it, 
     * 
     * but it can also be used to transition image layouts 
     * and transfer queue family ownership when VK_SHARING_MODE_EXCLUSIVE is used. 
     * 
     * There is an equivalent buffer memory barrier to do this for buffers.
     * 
     * now , we should create a barrier
     */
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    /*
     * specify layout transition
     */
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;

    /*
     * also barrier can transfer queue family ownership
     * but it is not important here
     */
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    /*
     * the image and subresourceRange specify the image that is affected and the specific part
     * of the image. our image is not an array and does not have mipmapping levels,
     * so only one level and layer are specified
     */
    barrier.image = image;

    /*
     * special for VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL layout 
     * we will change aspectMask form fixed VK_IMAGE_ASPECT_COLOR_BIT to 
     * VK_IMAGE_ASPECT_STENCIL_BIT or VK_IMAGE_ASPECT_DEPTH_BIT
     */
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    barrier.srcAccessMask = 0; // TODO
    barrier.dstAccessMask = 0; // TODO

    /*
     * Barriers are primarily used for synchronization purpose , so you must specify which type of
     * opertions that involve the resource must happen before the barrier.
     * and which opertions that involve the resource must wait on the barrier.
     */

    /*
     * all types of pipeline barriers are submitted using the same function.  -- vkCmdPipelineBarrie
     * The first parameter after the command buffer specifies in which pipelilne stage the operations occur that should
     * happen before the barrier.
     * The second parameter specifies the pipeline stage in which operations will wait on the barrier.
     * 
     * the pipeline stages that you are allowed to specify before and after the barrier depend on how you use
     * the resource before and after the barrier.
     * 
     * for example if you are going to read from a uniform after the barrier, you would specify a usage of 
     * VK_ACCESS_UNIFORM_READ_BIT  and the earliest shader that will read from the uniform as pipeline stage, for example VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT.
     * It would not make sense to specify a non-shader pipeline stage for this type of 
     * usage and the validation layers will warn you when you specify a pipeline stage that does not match the type of usage.
     *
     * The last three pairs of parameters reference arrays of pipeline barriers of the three available types: 
     * memory barriers, 
     * buffer memory barriers, 
     * and image memory barriers 
     * 
     * image memory barriers like the one we're using here. 
     * Note that we're not using the VkFormat parameter yet, 
     * but we'll be using that one for special transitions in the depth buffer chapter.
     */

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        /*
         * change vkImage layout form VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
         * do not have to wait anything ,so we specify an empty access mask and the earliest possible pipeline
         * stage(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT is the earliest pipeline stage)
         */
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT; // it is a pseudo-stage where transfers happend
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        /*
         * if VK_IMAGE_LAYOUT_UNDEFINED transfer to VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL 
         * barrier of pipeline stage rang is VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT(0) to 
         * VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT(VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
         * pipeline read depth vkImage for depth test should happen in the VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT stage
         * and write new depth result in VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT , 
         * we should select the earliest pipeline stage , so that it is ready for usage as depth attachment when it needs to be.
         * 
         * in one word
         * before we use the depth vkimage(VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT) , transfer vkImage layout should be done.
         */
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage /* TODO */,  // which pipeline stages should happend before the barrier
        destinationStage /* TODO */,  // which pipeline stages should wait on the barrier
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(device,commandBuffer,Queue,commandPool);
}

/*
 * find device supporty format
 * quire candidates format in device ,if it have tiling and feature
 * if it is support diresed feature , then return the format
 */
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features,VkPhysicalDevice phyDevice) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(phyDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

/*warp image into imageView,subresourceRange use default configuration 2d*/
VkImageView  createImageViewDefault(VkDevice device , VkImage image , VkFormat format , VkImageAspectFlags aspectFlags){
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;//vkImage which should be warped into vkImageView
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture image view!");
    }
    
    return imageView;
}