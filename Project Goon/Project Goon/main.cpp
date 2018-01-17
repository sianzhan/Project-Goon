#define GLEW_STATIC
#define _CRT_SECURE_NO_WARNINGS
#define GLM_ENABLE_EXPERIMENTAL
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <fstream>
#include <time.h>
#include "Robot.h"
#include "Texture/Texture.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include "main.h"

GLFWwindow *window = nullptr;

mat4 Projection;
mat4 View;

void init();
void mainLoop();
void resize(GLFWwindow* window, int width, int height);
void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods);
void passive_mouse(GLFWwindow* window, double xpos, double ypos);
void updateView();

int frame_width = 0;
int frame_height = 0;
float frame_aspect = 0;

bool mouse_lock = 0;
char key_hold[400] = { 0 };
int key_modifier = 0;
vec3 lastCameraPos;
vec3 lastCameraOrigin;
vec3 viewCamera = { 0, 20, 20 }; //camera position
vec3 viewOrigin = { 0, 20, 0 }; //view origin
vec3 viewOrient = { 0, 1, 0 };
double incl_ = 0.5;
double delta = 0.1;

Robot robot, house;


int main(int argc, char* argv[])
{
	if (!glfwInit()) return -1;
	glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
	window = glfwCreateWindow(1280, 800, "Project Goon!", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return -2;
	}

	glfwMakeContextCurrent(window);

	GLenum glew_info = glewInit();
	if (glew_info != GLEW_OK) {
		fprintf(stderr, "GLEW Error:%s\n", glewGetErrorString(glew_info));
		return -3;
	}
	glfwSetFramebufferSizeCallback(window, resize);
	glfwSetKeyCallback(window, keyboard);
	glfwSetCursorPosCallback(window, passive_mouse);
	glfwSwapInterval(1);

	init();
	robot.init("robot.txt");
	house.init("house.txt");
	mainLoop();

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

GLuint FramebufferName = 0;
GLuint depthTexture;
Program shadow_program;
glm::mat4 depthBiasMVP;
glm::mat4 biasMatrix(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 0.5, 0.0,
	0.5, 0.5, 0.5, 1.0
);
void init()
{
	//initGL
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	//init Shader

	std::cout << std::endl;
	updateView();
	glfwGetFramebufferSize(window, &frame_width, &frame_height);
	resize(window, frame_width, frame_height);


	//Shadow Mapping in Progress
	//!
	//! IMCOMPLETD, BUGGY!
	/*glGenFramebuffers(1, &FramebufferName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
	shadow_program.init();
	shadow_program.loadShader(GL_VERTEX_SHADER, "shadow.vert");
	shadow_program.loadShader(GL_FRAGMENT_SHADER, "shadow.frag");
	shadow_program.link();
	glEnableVertexAttribArray(0);
	// Depth texture. Slower than a depth buffer, but you can sample it later in your shader
	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);

	glDrawBuffer(GL_NONE); // No color buffer is drawn to.

						   // Always check that our framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		throw std::runtime_error("BOO!");

	glm::vec3 lightInvDir = glm::vec3(0.0f, 30.0f, -5.0f);

	// Compute the MVP matrix from the light's point of view
	glm::mat4 depthProjectionMatrix = glm::ortho<float>(-10, 10, -10, 10, -10, 20);
	glm::mat4 depthViewMatrix = glm::lookAt(lightInvDir, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 depthModelMatrix = glm::mat4(1.0);
	glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
	depthBiasMVP = biasMatrix*depthMVP;

	// Send our transformation to the currently bound shader,
	// in the "MVP" uniform
	GLuint depthMatrixID = glGetUniformLocation(shadow_program.data(), "depthMVP");
	glUniformMatrix4fv(depthMatrixID, 1, GL_FALSE, &depthBiasMVP[0][0]);*/

	//SHADOW END!
}
void mainLoop()
{
	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		house.update();
		robot.update();
		updateView();
		
		//FOR SHADOW MAPPING
		/*glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
		house.render(0);
		robot.render(0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);*/
		//END
		
		house.render();
		robot.render();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}


void updateView()
{
	if (key_hold[GLFW_KEY_LEFT_CONTROL])
	{
		auto head = robot.getHeadPos();
		if(key_hold[GLFW_KEY_LEFT_CONTROL] == 1) 
			viewCamera = std::get<0>(head); //First Person View
		else 
			viewCamera = std::get<2>(head); //Third Person- Back following view
		viewOrigin = std::get<1>(head);
	}
	else
	{
		vec3 viewDirection = normalize(viewOrigin - viewCamera);
		float incl_x = viewDirection.x * incl_;
		float incl_z = viewDirection.z * incl_;
		if (key_hold[GLFW_KEY_W])
		{
			viewCamera.x += incl_x, viewCamera.z += incl_z;
			viewOrigin.x += incl_x, viewOrigin.z += incl_z;
		}
		if (key_hold[GLFW_KEY_S]) {
			viewCamera.x -= incl_x, viewCamera.z -= incl_z;
			viewOrigin.x -= incl_x, viewOrigin.z -= incl_z;
		}
		if (key_hold[GLFW_KEY_A])
		{
			viewCamera.x += incl_z, viewCamera.z -= incl_x;
			viewOrigin.x += incl_z, viewOrigin.z -= incl_x;
		}
		if (key_hold[GLFW_KEY_D]) {
			viewCamera.x -= incl_z; viewCamera.z += incl_x;
			viewOrigin.x -= incl_z, viewOrigin.z += incl_x;
		}
		if (key_hold[GLFW_KEY_SPACE])
		{
			viewCamera.y += incl_;
			viewOrigin.y += incl_;
		}
		if (key_hold[GLFW_KEY_LEFT_SHIFT])
		{
			viewCamera.y -= incl_;
			viewOrigin.y -= incl_;
		}
	}
	View = lookAt(viewCamera, viewOrigin, viewOrient);
}

void resize(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, frame_width = width, frame_height = height);
	if (width == 0 && height == 0) return;
	frame_aspect = (float)width / height;
	Projection = perspective(radians(80.0f), frame_aspect, 0.1f, 1000.0f);
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key) {
		case GLFW_KEY_R: robot.reload(); house.reload(); shadow_program.reload();  break;
		case GLFW_KEY_C:
			if (!mouse_lock)
			{
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
				glfwSetCursorPos(window, frame_width / 2, frame_height / 2);
				mouse_lock = 1;// , glutSetCursor(GLUT_CURSOR_NONE);
				//glutWarpPointer(window_width / 2, window_height / 2);
			}
			else
			{
				mouse_lock = 0, glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}
			break;
		case GLFW_KEY_W: 
		case GLFW_KEY_A: 
		case GLFW_KEY_S: 
		case GLFW_KEY_D: 
		case GLFW_KEY_SPACE:
		case GLFW_KEY_LEFT_SHIFT:
			key_hold[key] = 1; break;
		case GLFW_KEY_LEFT_CONTROL:
			if (!key_hold[key]) {
				key_hold[key] = 1;
				lastCameraPos = viewCamera;
				lastCameraOrigin = viewOrigin;
			}
			else if (key_hold[key] == 1) key_hold[key] = 2;
			else {
				key_hold[key] = 0;
				viewCamera = lastCameraPos;
				viewOrigin = lastCameraOrigin;
			}
			break;
		case GLFW_KEY_0:
			robot.act(""); break;
		case GLFW_KEY_1:
			robot.act("walk"); break;
		case GLFW_KEY_Q:
			incl_ /= 2; break;
		case GLFW_KEY_E:
			incl_ *= 2; break;
		case GLFW_KEY_UP:
			delta *= 2; break;
		case GLFW_KEY_DOWN:
			delta /= 2; break;
		case GLFW_KEY_LEFT:
			robot.prevProgram(); break;
		case GLFW_KEY_RIGHT:
			robot.nextProgram(); break;
		case GLFW_KEY_COMMA:
			house.prevProgram(); break;
		case GLFW_KEY_PERIOD:
			house.nextProgram(); break;
		}
	}
	else if (action == GLFW_RELEASE)
	{
		switch (key)
		{
		case GLFW_KEY_W:
		case GLFW_KEY_A:
		case GLFW_KEY_S:
		case GLFW_KEY_D:
		case GLFW_KEY_SPACE:
		case GLFW_KEY_LEFT_SHIFT:
			key_hold[key] = 0; break;
		}
	}
}


void passive_mouse(GLFWwindow* window, double xpos, double ypos)
{
	static const vec3 axis_y = vec3(0.0f, 1.0f, 0.0f);
	static const float delta = -0.0012; //minus for inverting direction
	if (mouse_lock)
	{
		//gl Windows height and y-axis is inverted. 0, 0 at top left
		float diff_x = xpos - (frame_width / 2);
		float diff_y = ypos - (frame_height / 2);  
		if (fabs(diff_x) > 5 || fabs(diff_y) > 5)
		{
			viewOrigin -= viewCamera;
			//axis_xz is vector perpendicular to (y-axis and direction from camera to view)
			vec3 axis_xz = normalize(cross(viewOrigin, axis_y)); 
			quat rot = angleAxis(delta * diff_x, axis_y) * angleAxis(delta * diff_y, axis_xz);
			viewOrigin = toMat4(rot) * vec4(viewOrigin, 1.0f);
			viewOrigin += viewCamera;
			glfwSetCursorPos(window, frame_width / 2, frame_height / 2);
			//glutWarpPointer(window_width / 2, window_height / 2);
		}
	}
}
