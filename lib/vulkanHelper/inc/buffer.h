#ifndef __BUFFER_HELPER__
#define __BUFFER_HELPER__

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

void createBuffer(VkDevice device , VkPhysicalDevice phyDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
uint32_t findMemoryType(VkPhysicalDevice phyDevice , uint32_t typeFilter, VkMemoryPropertyFlags properties);
void copyBuffer(VkBuffer srcBuffer , VkBuffer dstBuffer, VkDeviceSize size, VkDevice device,VkQueue Queue , VkCommandPool commandPool);
void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, VkDevice device,VkQueue Queue , VkCommandPool commandPool);
#endif