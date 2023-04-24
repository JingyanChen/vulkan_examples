#include <stdio.h>
#include <commandBuffer.h>

/*
 * when we record command into command we should begin command buffer
 * and end command buffer, here is the helper function
 */

/*
 * begin a signal time commandBuffer from command pool
 * device : logic device
 * command pool :
 */
VkCommandBuffer beginSingleTimeCommands( VkDevice device , VkCommandPool commandPool) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

/*
 * end command buffer record and submit command buffer into Queue 
 * then free command pool
 */
void endSingleTimeCommands(VkDevice device , VkCommandBuffer commandBuffer , VkQueue Queue , VkCommandPool commandPool) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(Queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(Queue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
