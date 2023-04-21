#ifndef __BUFFER_HELPER__
#define __BUFFER_HELPER__

void createBuffer(VkDevice device , VkPhysicalDevice phyDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
#endif