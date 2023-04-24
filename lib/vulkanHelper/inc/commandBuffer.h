#ifndef __COMMAND_BUFFER_HELPER__
#define __COMMAND_BUFFER_HELPER__

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

VkCommandBuffer beginSingleTimeCommands( VkDevice device , VkCommandPool commandPool);
void endSingleTimeCommands(VkDevice device , VkCommandBuffer commandBuffer , VkQueue Queue , VkCommandPool commandPool);

#endif