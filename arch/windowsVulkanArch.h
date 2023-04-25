#ifndef __WINDOWS_VULKAN_ARCH__
#define __WINDOWS_VULKAN_ARCH__
#include <stdint.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>

/*
 * windows Vulkan Arch 
 * packaging windows platform window operation
 * and instance and find physical device with logic device corresponding
 * queue family.
 * 
 * only support graphics queue
 * compute queue is todo 
 */


/*related code maroco*/
#define GLFW_WINDOW_CODE
#define VK_KHR_SURFACE_CODE
#define VULKAN_INSTANCE_CODE
#define VALIDATION_LAYER_CODE
#define PHYSCIAL_DEVICE_AND_QUEUE_FAMILY_CODE
#define LOGIC_DEVICE_CODE
#define SWAPCHAIN_EXT_CODE
#define WARP_IMAGE_IN_SWAPCHAIN_TO_IMAGEVIEW_CODE

struct QueueFamilyIndices {
    /* in windwos case , in order to dispaly a window
     * we need to have two queue family
     * grapicsFamily and presentFamily
     */
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transferFamily;

    bool isComplete() {
        /*
         * if queue family is found they will have a vlaue such as 0,1,2 etc
         * in order to distinguish 0 and Notsupport , we use optional datastruct
         */
        return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value(); 
    }
};

/*
 * validation layer switch
 * use default validation layer : VK_LAYER_KHRONOS_validation
 * if _DEBUG_ on VK_LAYER_KHRONOS_validation  will be used
 */
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

/* 
 * need enable default extension list
 * device level extension
 */
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef _DEBUG_
    const bool enableValidationLayers = true;
#else
    const bool enableValidationLayers = false;
#endif
class vkGraphicsDevice{
    public:
    /*
     * a) create glfw windows
     * b) create vulkan instance
     */
    vkGraphicsDevice(uint32_t width , uint32_t hight){
        window_width = width;
        window_height = hight;

        initialWindowWithGLFW();
        initialVulkanInstance();
        /*crate instance level extentsion surface object */
        initialSurfaceExtWithGLFW();
        pickPhysicalDevice();
        createLogicalDevice();
        /*
         * use surface object created by initialSurfaceExtWithGLFW()
         * and device level extension VK_KHR_SWAPCHAIN_EXTENSION_NAME
         * to create a swap chain object
         */
        createSwapChain();
        /*warp vkimage into vkimageview*/
        createImageViews();
    }
    ~vkGraphicsDevice(){
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    /*
     * need user to implement and point out 
     * which draw should be recorded
     */
    void (* drawFrame)(void) = NULL;
    /* 
     * This Function should run in loop
     * and is windows related
     */
    void vkGraphicsDeviceHandle(void){
        if(!drawFrame){
            printf("draw function is empty");
        }
        while (!glfwWindowShouldClose(window)) {
            if(drawFrame)
                drawFrame();
            glfwPollEvents();
        }
    }

    /* glfw windows related*/
    #ifdef GLFW_WINDOW_CODE
    GLFWwindow* window;
    uint32_t window_width = 0;
    uint32_t window_height = 0;
    bool framebufferResized = false;
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<vkGraphicsDevice*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }
    void initialWindowWithGLFW(void){
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(window_width, window_height, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
    }

    void recreateSwapChain()
    {
        printf("recreateSwapChain not support");
    }
    #endif
    /* extesion : surface */
    #ifdef VK_KHR_SURFACE_CODE
    /*
     * when we create instance we can inquire what extesion does vulkan library support
     * VK_KHR_surface is one of the extension that instance support
     * 
     * Note : VK_KHR_surface is instance level extesion
     * 
     * two opertion should be done
     * 1. inqure whether the physical is support present queue family : findQueueFamilies as a condition in pickPhysicalDevice
     * 2. create present queue when we create logic device : createLogicalDevice
     */
    VkSurfaceKHR surface;
    void initialSurfaceExtWithGLFW(){
        /*
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hwnd = glfwGetWin32Window(window); //get windiows handle from glfw
        createInfo.hinstance = GetModuleHandle(nullptr); 
        if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
        //This is glfwCreateWindowSurface implement
        */
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }
    #endif
    /* vulkan instance related*/
    #ifdef VULKAN_INSTANCE_CODE
    VkInstance instance;
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    void initialVulkanInstance(void){
        
        /* if use validation layer , you should check the validation layer list 
        * which you want is inside avaliable validation layer list(vulkan lib offer)
        */
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }
        /*For brevity, some values are set to default*/
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "vulkan examples";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        /*
        * vulkan is a platform agnostic API,which means that you need an
        * extension to interface with the window system.
        * GLFW has a handy built-in function that returns the extension 
        * it need to do , as follow code
        */
        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;
        
        //tell vulkan lib if we want to use validation layer
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
        }
        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
        #ifdef _DEBUG_
        /*it will print detail of extensions which vulkan instance support*/
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
        printf("instance level - available %d extensions:\n",extensionCount);
        for (const auto& extension : extensions) {
            for(uint32_t i=0; i<glfwExtensionCount; i++)
                if(strcmp(extension.extensionName,glfwExtensions[i])==0){
                    std::cout << "\t [*]" << extension.extensionName << '\n';   
                    continue; 
                }
            std::cout << "\t    " << extension.extensionName << '\n';
        }
        #endif
    }
    #endif
    /*validation layer related*/
    #ifdef VALIDATION_LAYER_CODE
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }
    bool checkValidationLayerSupport() {
        /* 
        * get vulkan liib support validation layer
        * stored in layerCount and availableLayers
        */
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
        /*
        * check that the validation layer which we want to used
        * is inside avaliable layer list
        */
        for (const char* layerName : validationLayers) {
            bool layerFound = false;
            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }
            if (!layerFound) {
                return false;
            }
        }
        #ifdef _DEBUG_
        printf("validation layer is endable:\r\n");
        for (const char* layerName : validationLayers) {
            printf("\t%s\r\n",layerName);
        }
        #endif
        return true;
    }
    #endif    
    /*physical device and queue familes related*/
    #ifdef PHYSCIAL_DEVICE_AND_QUEUE_FAMILY_CODE
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    /*
     * check physical device is support device level extension or not
     */
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    /*
     * This function is a helper function 
     * to indicate what queue family does physical device support
     * store final support information at QueueFamilyIndices
     */
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {

            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }
            
            if(queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT){
                indices.transferFamily = i;    
            }
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    /*
     * helper function for query swap chain detail (device level extsion)
     */
    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        //query basic surface capabilities , stored at SwapChainSupportDetails->capabilities
        //All of the support querying functions have these two as first parameters because they are the core components of the swap chain
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        //query supported surface formats
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }    

        //query supported surface present mode
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        /* 
         * 1. check device Properties is suitable or not 
         * 2. check device supporting queue family is suitable or not
         * 
         * in this case , we are only concern graphicsFamily and presentFamily
         * 
         */
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        /*
         * as comment in header of VK_KHR_SURFACE_CODE
         * queue family is support or not is important condition for picking up a physical device
         */
        
        QueueFamilyIndices indices = findQueueFamilies(device);

        /*
         * for dispaly something in windows , we need physical device support VK_KHR_SWAPCHAIN_EXTENSION_NAME
         * VK_KHR_SWAPCHAIN_EXTENSION_NAME is device level extsion
         * device level extension support or not is important conditon for picking up a physical device
         */
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        /*
         * physical device support swap chain is not sufficient , because it may not actually be compatible
         * with our window surface. so we should query for more details of the VK_KHR_SWAPCHAIN_EXTENSION_NAME
         * 
         * so we should quiry VK_KHR_SWAPCHAIN_EXTENSION_NAME support format and present mode list , and 
         * it should not be empty , at least support one format and present mode.
         */
        bool swapChainAdequate = false;

        if(extensionsSupported){
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();    
        }

        return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && 
                                        indices.isComplete() && 
                                        extensionsSupported && 
                                        swapChainAdequate &&
                                        deviceFeatures.samplerAnisotropy;
    }
    void pickPhysicalDevice() {

        //find all physical device from vulkan instance
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            //pick up a most suitable device form physcial device list which is inquire
            //form vulkan instance
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                #ifdef _DEBUG_
                QueueFamilyIndices indices = findQueueFamilies(device);
                std::cout<<"selected physical device: "<< device <<" support queueFamilies:\r\n";

                if(indices.graphicsFamily.has_value())
                    std::cout<<"\t Support graphics queue family index:" << indices.graphicsFamily.value() << "\r\n";
                if(indices.presentFamily.has_value())
                    std::cout<<"\t Support present queue family index:" << indices.presentFamily.value() << "\r\n";
                if(indices.transferFamily.has_value())
                    std::cout<<"\t Support transfer queue family index:"<< indices.transferFamily.value() << "\r\n";

                uint32_t extensionCount;
                vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

                std::vector<VkExtensionProperties> availableExtensions(extensionCount);
                vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

                std::cout<<"selected physical device: "<< device <<" support device level extsion list:\r\n";

                bool swapChainSupport = false;
                for (const auto& extension : availableExtensions) {
                   
                    if(strcmp(extension.extensionName,VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0 ){
                        std::cout << "\t [*]" << extension.extensionName;
                        swapChainSupport = true;
                    }else{
                        std::cout << "\t    " << extension.extensionName;
                    }
                    std::cout<< "\r\n";
                }
                if(swapChainSupport){

                    std::cout<<"VK_KHR_swapchain detail is : \r\n";

                    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
                    std::cout<<"\t support format count : " << swapChainSupport.formats.size()<< "\r\n";
                    std::cout<<"\t support present mode count : " << swapChainSupport.presentModes.size()<< "\r\n";
                }
                #endif
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }
    #endif
    /*logic device related*/    
    #ifdef LOGIC_DEVICE_CODE
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkQueue transferQueue;
    void createLogicalDevice() {
        //physicalDevice will stored the most suitable device in physical device list by vulkan lib
        //find picked physical device supporting queue family again
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value(), indices.transferFamily.value() };

        /*crate queue family info*/
        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        /* create two queue family */
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());//todo list
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();//todo list

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
        vkGetDeviceQueue(device, indices.transferFamily.value(),0, &transferQueue);
    }
    #endif
    /* swap chain extesion related*/
    #ifdef SWAPCHAIN_EXT_CODE

    /*
     * we prefer VK_FORMAT_B8G8R8A8_SRGB as swap chain color format
     */
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }
    /*
     * we prefer VK_PRESENT_MODE_MAILBOX_KHR as swap chain present mode
     * VK_PRESENT_MODE_FIFO_KHR is default support present mode
     */
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    /*
     * select images's resolution in swap chain. it is always exactly equal to the resolution of
     * the window that we are drawing to in pixels.
     * 
     * the range of the possible resolution is defined in the VkSurfaceCapabilitiesKHR
     * vulkan tells us to match the resolution of the window by setting the width and height in the 
     * currentExtent member
     */
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            //use glfw function to conveter screen coordinates(such as (width , height)) into
            //pixel format as vulkan exten wanted format 
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    /*
     * if swap chain device level extension is support 
     * and VK_KHR_surface instance level extension is support
     * then we can create a swapchain object by three information
     * 
     * 1. surface format (color and depth format)
     * 2. presentation mode(conditions for "swapping" images to the screen)
     * 3. swap extent(resolution of images in swap chain)
     */
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        /*
         * we use presentastion mode is VK_PRESENT_MODE_FIFO_KHR 
         * so we should select queue size 
         * make sure that image count do not exceed the maximum number of images 
         */
        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        #ifdef _DEBUG_
        std::cout<< "swap chain image count in queue is " << imageCount <<"\r\n";
        #endif

        /*use instance level extension surface to init a swapchain*/
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        /*
         * Next, we need to specify how to handle swap chain images that will be used across multiple queue families.
         * That will be the case in our application if the graphics queue family is different from the presentation queue.
         * We'll be drawing on the images in the swap chain from the graphics queue and then submitting them on the presentation queue.
         * VK_SHARING_MODE_EXCLUSIVE : An image is owned by one queue family at a time and ownership must be explicitly transferred
         * before using it in another queue family.This option offers the best performance
         * 
         * VK_SHARING_MODE_CONCURRENT: image can be used across multiple queue families without explicit ownership transfers
         */
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value() ,indices.transferFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily && indices.presentFamily != indices.transferFamily) {
            /*
             * if the queue families differ , then we will be using the concurrent mode to avoid having to do the ownership chapters.
             * concurrent mode requires you to specifu in advance between which queue families ownership will be shared using the
             * queueFamilyIndexCount and pQueueFamilyIndices
             */
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 3;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
            #ifdef _DEBUG_
            std::cout<< "swap chain image Sharing Mode is VK_SHARING_MODE_CONCURRENT\r\n";
            #endif
        } else {
            /*
             *  if the graphics queue family and presentation queue family are the same
             *  which will be the case on most hardrware , then we should stick to exclusive mode , because concurrent mode 
             *  requires you to specify at least two distinct queue families
             */
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
            #ifdef _DEBUG_
            std::cout<< "swap chain image Sharing Mode is VK_SHARING_MODE_EXCLUSIVE\r\n";
            #endif
        }

        /*
         * We can specify that a certain transform should be applied to images in the swap chain if it is supported
         * To specify that you do not want any transformation, simply specify the current transformation.
         * now we do not want do any transform operation before so we choice currentTransform
         */
        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        /*
         * The compositeAlpha field specifies if the alpha channel should be used for blending with other windows in the window system. 
         * You'll almost always want to simply ignore the alpha channel, hence VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR.
         */
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        //present mode we select before
        createInfo.presentMode = presentMode;
        /*
         * If the clipped member is set to VK_TRUE then that means that we don't care about the color of pixels that are obscured, 
         * for example because another window is in front of them. 
         * Unless you really need to be able to read these pixels back and get predictable results, 
         * you'll get the best performance by enabling clipping.
         */
        createInfo.clipped = VK_TRUE;

        /*record old swap chain when new swap chain is created*/
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        /* 
         * create VkImage for rendering , as we know swap chain will mange a image queue 
         * so we should create the image queue by swapchain extension api and retrieving the image handle
         */
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;

        #ifdef _DEBUG_
        std::cout<< "swap chain image count actually created is " << imageCount <<"\r\n";
        std::cout<< "swap chain image final format is " << swapChainImageFormat <<"\r\n";
        std::cout<< "swap chain image final extent is (" << swapChainExtent.width << "," << swapChainExtent.height << ")" << "\r\n";
        #endif
    }   
    #endif
    /*warp vkImage form swap chain extsion into imageview related*/
    #ifdef WARP_IMAGE_IN_SWAPCHAIN_TO_IMAGEVIEW_CODE
    /*
     * To use any VkImage, including those in the swap chain, in the render pipeline we have to create a VkImageView object
     * An image view is quite literally a view into an image
     * it describes how to access the image and which part of the image to access.
     */
    std::vector<VkImageView> swapChainImageViews;
    void createImageViews() {
        /*one vkImage is corresponding to one vkImageView*/
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            /*
             * The components field allows you to swizzle the color channels around. 
             * For example, you can map all of the channels to the red channel for a monochrome texture.
             * You can also map constant values of 0 and 1 to a channel. In our case we'll stick to the default mapping.
             */
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            /*
             * The subresourceRange field describes what the image's purpose is and which part of the image should be accessed. 
             * Our images will be used as color targets without any mipmapping levels or multiple layers.
             * 
             * If you were working on a stereographic 3D application, then you would create a swap chain with multiple layers. 
             * You could then create multiple image views for each image representing the views for the left and right eyes by accessing different layers.
             * 
             */
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
            /*
             * An image view is sufficient to start using an image as a texture, but it's not quite ready to be used as a render target just yet. 
             * That requires one more step of indirection, known as a framebuffer.
             * 
             * so vkImageView is not final render target , it should be warped into framebuffer 
             */
        }

    }
    #endif
};

#endif