#ifndef __SHADER_VULKAN_ARCH__
#define __SHADER_VULKAN_ARCH__
/*
 * Vulkan Shader Helper Arch 
 */
#include <iostream>
#include <fstream>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

/*
 * describe a shader need shader source code , and compile into SPIRV bytecode
 * then warp bytecode into VkShaderModule(thin warp)
 * then record shaderModule information such as shader type and code size and start function name into
 * VkPipelineShaderStageCreateInfo object
 */
class vulkanShader{
    public:
        /*
         * common shaderType is 
         * VK_SHADER_STAGE_VERTEX_BIT
         * VK_SHADER_STAGE_FRAGMENT_BIT
         */
        vulkanShader(const std::string& filename,VkShaderStageFlagBits shaderType){
            std::ifstream file(filename, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                printf("failed to open shader source code file!\r\n");
                return ;
            }

            shaderType_ = shaderType;
            memset(&shaderStateCreateInfo_, 0, sizeof(shaderStateCreateInfo_));
            size_t fileSize = (size_t) file.tellg();

            buffer_.resize(fileSize);

            file.seekg(0);
            file.read(buffer_.data(), fileSize);

            file.close();

            #ifdef _DEBUG_
            printf("Create Shader Object Success! Path = %s Type=0x%x size=0x%zx\r\n",filename.data(),shaderType,buffer_.size());
            #endif
        }
        /*
         * before we can pass the code to the pipeline we have to 
         * warp it in a VkShaderModule object.
         * the compilation and linking of the SPIR-V bytecode to 
         * machine code for execution by GPU does not happen until the
         * graphics pipeline is created
         */
        VkShaderModule createShaderModule(VkDevice device,const std::vector<char>& code) {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            VkShaderModule shaderModule;
            if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
                throw std::runtime_error("failed to create shader module!");
            }

            return shaderModule;
        }

        /* 
         * when create pipeline object , we should record shader stage into shader state create info object
         * This is helper function to set up VkPipelineShaderStageCreateInfo object for shader
         */
        VkPipelineShaderStageCreateInfo getShaderStageInfo(VkDevice device){
            shaderStateCreateInfo_.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStateCreateInfo_.stage = shaderType_;
            shaderStateCreateInfo_.module = createShaderModule(device, buffer_);
            shaderStateCreateInfo_.pName = "main";
            return shaderStateCreateInfo_;
        }
    private:
        std::vector<char> buffer_;
        VkShaderModule shaderModule_;
        VkPipelineShaderStageCreateInfo shaderStateCreateInfo_;
        VkShaderStageFlagBits shaderType_;
};

#endif