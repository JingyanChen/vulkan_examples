#include <stdio.h>
#include <windowsVulkanArch.h>
#include <shaderVulkanArch.h>

/*
 * GPU can offer different types of memory to allocate from.each type of memory varies in terms of allowed 
 * opertions and performance characteristicxs.
 * 
 * we should combine the requirement of the buffer and our own application requirements to find the right type of 
 * memory to use.
 **/
uint32_t findMemoryType(VkPhysicalDevice phyDevice , uint32_t typeFilter, VkMemoryPropertyFlags properties) {

    /*
     * inquire what memory type does physical device support
     * memProperties will retuen tow info 
     * 1. memory tpye
     * 2. memoryHeaps:Memory heaps are distinct memory resources like dedicated VRAM and swap space in RAM for when VRAM runs out
     */
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(phyDevice, &memProperties);

    #if _DEBUG_
    printf("0x%x Physical device support %d memory type",(uint32_t)vulkanDevice->physicalDevice,(uint32_t)memProperties.memoryTypeCount);
    #endif
    //filter memory type and memory properties ,a select a most suitable memory type
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");

}

/*
 * create command buffer flow
 * 1. create buffer with usage ,such as VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
 * 2. get buffer's requirement refer to buffer usage
 * 3. allcate a range of memory for buffer , find most suitable memory section in physical device as memoryTypeIndex
 *    actually memory size is depend on result of vkGetBufferMemoryRequirements
 * 4. finally bind buffer memory and buffer handle use vkBindBufferMemory
 */
void createBuffer(VkDevice device , VkPhysicalDevice phyDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    //memory type info in come from vkGetBufferMemoryRequirements
    //properties come from ap 
    //two select memory section conditons is memory type and properties
    allocInfo.memoryTypeIndex = findMemoryType(phyDevice,memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}