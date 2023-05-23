/*
* Basic camera class
*
* Copyright (C) 2016 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
private:
	float fov;
	float aspect;
	float znear, zfar;
public:

	struct
	{
		glm::mat4 perspective;
		glm::mat4 view;
	} matrices;

	glm::vec3 cameraPos = glm::vec3();
	glm::vec3 cameraFront = glm::vec3();
	glm::vec3 cameraUp = glm::vec3();

	float camera_speed = 50.0;

	bool flipY = false;
	void setCamera(glm::vec3 cameraPos,glm::vec3 cameraFront,glm::vec3 cameraUp)
	{
		this->cameraPos = cameraPos;
		this->cameraFront = cameraFront;
		this->cameraUp = cameraUp;

		matrices.view = glm::lookAt( cameraPos,
						cameraPos + cameraFront,
						cameraUp);

	}

	void setCameraSpeed(float speed)
	{
		camera_speed = speed;
	}
	void setPerspective(float fov, float aspect, float znear, float zfar)
	{
		this->fov = fov;
		this->znear = znear;
		this->zfar = zfar;
		this->aspect = aspect;

		matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
		if (flipY) {
			matrices.perspective[1][1] *= -1.0f;
		}
	};

	void updateView()
	{
		setCamera(cameraPos,cameraFront,cameraUp);
	}

	float getCameraSpeed()
	{
		return camera_speed;
	}
};