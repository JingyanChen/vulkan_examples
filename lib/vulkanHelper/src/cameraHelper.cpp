#include <stdio.h>
#include <windowsVulkanArch.h>
#include <shaderVulkanArch.h>

vkGraphicsDevice * vulkanDeviceCpy = 0;

void key_callback(GLFWwindow * window,int key , int scancode , int action ,int mods)
{
        
    float cameraSpeed = vulkanDeviceCpy->camera.getCameraSpeed() * vulkanDeviceCpy->deltaTime; // adjust accordingly
        
    if(key == GLFW_KEY_W && (action == GLFW_REPEAT || action == GLFW_PRESS) ){
        printf("up pressed\r\n");
        vulkanDeviceCpy->camera.cameraPos += cameraSpeed * vulkanDeviceCpy->camera.cameraFront;

        }
    if(key == GLFW_KEY_S && (action == GLFW_REPEAT || action == GLFW_PRESS)){
        printf("down pressed\r\n");
        vulkanDeviceCpy->camera.cameraPos -= cameraSpeed * vulkanDeviceCpy->camera.cameraFront;
    }

    if(key == GLFW_KEY_A && (action == GLFW_REPEAT || action == GLFW_PRESS)){
        printf("left pressed\r\n");
        vulkanDeviceCpy->camera.cameraPos -= glm::normalize(glm::cross(vulkanDeviceCpy->camera.cameraFront, vulkanDeviceCpy->camera.cameraUp)) * cameraSpeed;
    }

    if(key == GLFW_KEY_D && (action == GLFW_REPEAT || action == GLFW_PRESS)){
        printf("right pressed\r\n");
        vulkanDeviceCpy->camera.cameraPos += glm::normalize(glm::cross(vulkanDeviceCpy->camera.cameraFront, vulkanDeviceCpy->camera.cameraUp)) * cameraSpeed;
    }
    #ifdef __DEBUG__
    printf("camera pos = (%f,%f,%f)\r\n",vulkanDeviceCpy->camera.cameraPos[0],vulkanDeviceCpy->camera.cameraPos[1],vulkanDeviceCpy->camera.cameraPos[2]);
    printf("camera front = (%f,%f,%f)\r\n",vulkanDeviceCpy->camera.cameraFront[0],vulkanDeviceCpy->camera.cameraFront[1],vulkanDeviceCpy->camera.cameraFront[2]);
    #endif

    vulkanDeviceCpy->camera.updateView();
}

bool firstMouse = true;
float yaw   = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
float pitch =  0.0f;
float lastX =  800.0f / 2.0;
float lastY =  600.0 / 2.0;
float fov   =  45.0f;
bool mousePressed=false;
static void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.3f; // change this value to your liking
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    if(mousePressed == false)
    {
        return ;
    }
    
    yaw += xoffset;
    pitch -= yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    vulkanDeviceCpy->camera.cameraFront = glm::normalize(front);


    vulkanDeviceCpy->camera.updateView();
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  if(fov >= 1.0f && fov <= 45.0f)
    fov -= yoffset;
  if(fov <= 1.0f)
    fov = 1.0f;
  if(fov >= 45.0f)
    fov = 45.0f;
}

static void mouse_press_callback(GLFWwindow* window, int button, int action, int mods){

    if (action == GLFW_PRESS)
    {
        switch (button)
        {
            case GLFW_MOUSE_BUTTON_LEFT:
                mousePressed = true;
                break;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                break;
            case GLFW_MOUSE_BUTTON_RIGHT:
                break;
            default:
                return;
        }
    }else if(action == GLFW_RELEASE){
        mousePressed = false;
    }
}
void cameraDeviceInit(vkGraphicsDevice * vulkanDevice){
    vulkanDeviceCpy = vulkanDevice;

    lastX =  vulkanDevice->window_width / 2.0;
    lastY =  vulkanDevice->window_height / 2.0;

    //register key and mouse and scroll callback
    glfwSetKeyCallback(vulkanDevice->window,key_callback);
    glfwSetCursorPosCallback(vulkanDevice->window, mouse_callback);
    glfwSetMouseButtonCallback(vulkanDevice->window,mouse_press_callback);
    //glfwSetScrollCallback(window, scroll_callback);
}