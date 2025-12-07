//
// buffer.cpp
// As part of the Atlas project
// Created by Max Van den Eynde in 2025
// --------------------------------------------------
// Description: Vulkan Buffer functions
// Copyright (c) 2025 Max Van den Eynde
//

#ifdef VULKAN
#include <opal/opal.h>

namespace opal {

void Buffer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                        VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = Device::globalInstance->commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(Device::globalDevice, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(Device::globalInstance->graphicsQueue, 1, &submitInfo,
                  VK_NULL_HANDLE);
    vkQueueWaitIdle(Device::globalInstance->graphicsQueue);

    vkFreeCommandBuffers(Device::globalDevice,
                         Device::globalInstance->commandPool, 1,
                         &commandBuffer);
}

void Buffer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties, VkBuffer &buffer,
                          VkDeviceMemory &bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(Device::globalDevice, &bufferInfo, nullptr, &buffer) !=
        VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(Device::globalDevice, buffer,
                                  &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = Device::globalInstance->findMemoryType(
        memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(Device::globalDevice, &allocInfo, nullptr,
                         &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(Device::globalDevice, buffer, bufferMemory, 0);
}

} // namespace opal

#endif