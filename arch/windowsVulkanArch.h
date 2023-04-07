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
        initialSurfaceExtWithGLFW();
        pickPhysicalDevice();
        createLogicalDevice();
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

    private:

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
    #endif
    /* extesion : surface */
    #ifdef VK_KHR_SURFACE_CODE
    /*
     * when we create instance we can inquire what extesion does vulkan library support
     * VK_KHR_surface is one of the extension that instance support
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
            std::cout << '\t' << extension.extensionName << '\n';
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

    struct QueueFamilyIndices {
        /* in windwos case , in order to dispaly a window 
         * we need to have two queue family
         * grapicsFamily and presentFamily
         */
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            /*
             * if queue family is found they will have a vlaue such as 0,1,2 etc
             * in order to distinguish 0 and Notsupport , we use optional datastruct 
             */
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

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


        return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && indices.isComplete();
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
                    std::cout<<"\t Support graphics queue family\r\n";
                if(indices.presentFamily.has_value())
                    std::cout<<"\t Support presentFamily queue family\r\n";
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
    void createLogicalDevice() {
        //physicalDevice will stored the most suitable device in physical device list by vulkan lib
        //find picked physical device supporting queue family again
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

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
    }
    #endif
};