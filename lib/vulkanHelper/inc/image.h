#ifndef __IMAGE__
#define __IMAGE__

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

void createImage(VkDevice device , VkPhysicalDevice phyDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout , VkDevice device , VkQueue Queue , VkCommandPool commandPool);
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features,VkPhysicalDevice phyDevice);
VkImageView  createImageViewDefault(VkDevice device , VkImage image , VkFormat format , VkImageAspectFlags aspectFlags);
#endif