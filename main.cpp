//
//  main.cpp
//  OpenGL Advances Lighting
//
//  Created by CGIS on 28/11/16.
//  Copyright � 2016 CGIS. All rights reserved.
//

#if defined (__APPLE__)
    #define GLFW_INCLUDE_GLCOREARB
    #define GL_SILENCE_DEPRECATION
#else
    #define GLEW_STATIC
    #include <GL/glew.h>
#endif

#include <GLFW/glfw3.h>
#include <algorithm>

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/matrix_inverse.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "Shader.hpp"
#include "Model3D.hpp"
#include "Camera.hpp"
#include "SkyBox.hpp"
#include <iostream>
#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")

#define SPOTLIGHT_NO 4

int glWindowWidth = 3072;
int glWindowHeight = 1920;
int retina_width, retina_height;
float angle = 0.0f;
GLFWwindow* glWindow = NULL;

const unsigned int SHADOW_WIDTH = 8192;
const unsigned int SHADOW_HEIGHT = 8192;

glm::mat4 model;
GLuint modelLoc;
glm::mat4 view;
GLuint viewLoc;
glm::mat4 projection;
GLuint projectionLoc;
glm::mat3 normalMatrix;
GLuint normalMatrixLoc;
glm::mat4 lightRotation;

glm::vec3 lightDir;
GLuint lightDirLoc;
glm::vec3 lightColor;
GLuint lightColorLoc;

struct spotLight {
	glm::vec3 position;
	glm::vec3 direction;
	float cutOff;
	float outerCutOff;
};
spotLight mySpotLight[SPOTLIGHT_NO];

struct pointLight {
	glm::vec3 position;
	float constant;
	float linear;
	float quadratic;
	glm::vec3 color;
};
pointLight myPointLight;
bool usePointLightShader = false;

gps::Camera myCamera(
	glm::vec3(0.0f, 2.0f, 25.5f),
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(0.0f, 1.0f, 0.0f));
float cameraSpeed = 0.50f;

bool pressedKeys[1024];
float angleY = 0.0f;
GLfloat lightAngle;

gps::Model3D farm;
gps::Model3D windmill;
gps::Model3D lightCube;
gps::Model3D screenQuad;
gps::Model3D geamFata;
gps::Model3D glass;


gps::Shader myCustomShader;
gps::Shader myCustomShader2;
gps::Shader currentShader;
gps::Shader lightShader;
gps::Shader screenQuadShader;
gps::Shader depthMapShader;

GLuint shadowMapFBO;
GLuint depthMapTexture;

bool showDepthMap;

GLuint textureID;

gps::SkyBox mySkyBox;
gps::Shader skyboxShader;
std::vector<const GLchar*> faces;

struct TransparentObject {
	gps::Model3D* object;   
	glm::vec3 position;      
	glm::vec3 rotation;    
	glm::vec3 scale;        
};

TransparentObject glassObject = {
	&glass,
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(0.0f, 0.0f, 0.0f),
	glm::vec3(1.0f, 1.0f, 1.0f)
};

TransparentObject geamfataObject = {
	&geamFata,
	glm::vec3(-12.494f, -6.00f, 10.70f),
	glm::vec3(11.60f, 265.0f, -40.0f),
	glm::vec3(1.0f, 1.0f, 1.0f)
};

std::vector<TransparentObject> transparentObjects = { glassObject, geamfataObject };

bool cinematicActive = false;      
std::vector<glm::vec3> cinematicPoints = {   
	glm::vec3(14.0f, 17.0f, 175.0f),
	glm::vec3(-9.0f, 10.0f, 67.0f),
	glm::vec3(16.0f, 10.0f, 2.0f),
	glm::vec3(81.0f, 10.0f, -30.0f),
	glm::vec3(81.0f, 10.0f, 106.2f),
	glm::vec3(49.0f, -2.0f, 95.0f)
};
float cinematicProgress = 0.0f;    
float cinematicSpeed = 0.2f;      
float cinematicYaw = 135.0f;       
float cinematicPitch = -15.0f;   

float lastX = 400;
float lastY = 300;
bool firstMouse = true;

GLenum glCheckError_(const char* file, int line) {
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR)
	{
		std::string error;
		switch (errorCode)
		{
		case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
		case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "window resized to width: %d , and height: %d\n", width, height);
	//TODO	
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_M && action == GLFW_PRESS)
		showDepthMap = !showDepthMap;

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			pressedKeys[key] = true;
		else if (action == GLFW_RELEASE)
			pressedKeys[key] = false;
	}
}


void mouseCallback(GLFWwindow* window, double xposition, double yposition) {
	if (firstMouse) {
		lastX = xposition;
		lastY = yposition;
		firstMouse = false;
	}

	float xOff = xposition - lastX;
	float yOff = lastY - yposition; 
	lastX = xposition;
	lastY = yposition;
	const float sensitivity = 0.05f;
	xOff *= sensitivity;
	yOff *= sensitivity;
	myCamera.rotate(yOff, xOff);
}

void processMovement()
{
	if (pressedKeys[GLFW_KEY_P]) {
		cinematicActive = true;
	}
	if (!cinematicActive) {
		if (pressedKeys[GLFW_KEY_Q]) {
			myCamera.rotate(0.0f, -1.0f);;
		}

		if (pressedKeys[GLFW_KEY_E]) {
			myCamera.rotate(0.0f, 1.0f);
		}

		if (pressedKeys[GLFW_KEY_W]) {
			myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		}

		if (pressedKeys[GLFW_KEY_S]) {
			myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
		}

		if (pressedKeys[GLFW_KEY_A]) {
			myCamera.move(gps::MOVE_LEFT, cameraSpeed);
		}

		if (pressedKeys[GLFW_KEY_D]) {
			myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
		}
		
		//----------------------------------
		if (pressedKeys[GLFW_KEY_X]) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}//solid

		if (pressedKeys[GLFW_KEY_C]) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}//pt wireframe


		if (pressedKeys[GLFW_KEY_V]) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		}//polygonal
		//----------------------------------
		if (pressedKeys[GLFW_KEY_I]) {
			lightDir.y += 0.1f;  // Ridică "soarele"
			lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		}
		if (pressedKeys[GLFW_KEY_K]) {
			lightDir.y -= 0.1f;  // Coboară "soarele"
			lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		}
		if (pressedKeys[GLFW_KEY_J]) {
			lightAngle -= 1.0f;
			lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		}

		if (pressedKeys[GLFW_KEY_L]) {
			lightAngle += 1.0f;
			lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		}

		if (pressedKeys[GLFW_KEY_O]) {
			usePointLightShader = !usePointLightShader; // Comută între cele două shadere
		}
	}
	if (angle > 360.0f) {
		angle -= 360.0f;
	}
	angle += 0.35f;
}

size_t currentPointIndex = 0;
void animateCamera(float deltaTime) {
	if (cinematicActive) {
		glm::vec3 startPoint = cinematicPoints[currentPointIndex];
		glm::vec3 endPoint = cinematicPoints[(currentPointIndex + 1) % cinematicPoints.size()];

		glm::vec3 interpolatedPos = glm::mix(startPoint, endPoint, cinematicProgress);
		myCamera.setPosition(interpolatedPos.x, interpolatedPos.y, interpolatedPos.z);
		myCamera.setDirection(endPoint);

		cinematicProgress += deltaTime * cinematicSpeed;

		if (cinematicProgress >= 1.0f) {
			cinematicProgress = 0.0f;
			currentPointIndex++;

			if (currentPointIndex >= cinematicPoints.size() - 1) {
				cinematicActive = false; 
				currentPointIndex = 0;  
			}
		}
	}
}

bool initOpenGLWindow() {
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Window scaling for HiDPI displays
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

	// For sRGB framebuffer
	glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

	// For anti-aliasing
	glfwWindowHint(GLFW_SAMPLES, 4);

	glWindow = glfwCreateWindow(glWindowWidth, glWindowHeight, "OpenGL Shader Example", NULL, NULL);
	if (!glWindow) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	glfwSetWindowSizeCallback(glWindow, windowResizeCallback);
	glfwSetKeyCallback(glWindow, keyboardCallback);
	glfwSetCursorPosCallback(glWindow, mouseCallback);

	// Lock the mouse inside the window and disable the cursor
	glfwSetInputMode(glWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwMakeContextCurrent(glWindow);

	glfwSwapInterval(1);

#if not defined (_APPLE_)
	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();
#endif

	// get version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version = glGetString(GL_VERSION); // version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	// For retina display
	glfwGetFramebufferSize(glWindow, &retina_width, &retina_height);

	return true;
}

void initSkybox() {
	faces.push_back("skybox/negx.jpg");
	faces.push_back("skybox/posx.jpg");
	faces.push_back("skybox/posyy.jpg");
	faces.push_back("skybox/negy.jpg");
	faces.push_back("skybox/negz.jpg");
	faces.push_back("skybox/posz.jpg");

	mySkyBox.Load(faces);
}

void initOpenGLState()
{
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, retina_width, retina_height);

	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE);
	// cull face
	//glCullFace(GL_FRONT);// cull back face
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise

	glEnable(GL_FRAMEBUFFER_SRGB);
}

void initObjects() {

	farm.LoadModel("objects/farm/v1.obj");
	windmill.LoadModel("objects/elice/untitled.obj");
	lightCube.LoadModel("objects/cube/cube.obj");
	screenQuad.LoadModel("objects/quad/quad.obj");
	glass.LoadModel("objects/glass/glass.obj");
	geamFata.LoadModel("objects/parteFata/parteFata.obj");	

	initSkybox();
}

void initSpotLight() {
	mySpotLight[0].position = glm::vec3(9.79f, 6.08f, 18.02f);
	mySpotLight[0].direction = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
	mySpotLight[0].cutOff = glm::cos(glm::radians(22.0f));
	mySpotLight[0].outerCutOff = glm::cos(glm::radians(27.0f));

	mySpotLight[1].position = glm::vec3(86.36f, 7.93f, 16.04f);
	mySpotLight[1].direction = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
	mySpotLight[1].cutOff = glm::cos(glm::radians(22.0f));
	mySpotLight[1].outerCutOff = glm::cos(glm::radians(27.0f));

	mySpotLight[2].position = glm::vec3(86.36f, 9.93f, 3.44f);
	mySpotLight[2].direction = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
	mySpotLight[2].cutOff = glm::cos(glm::radians(22.0f));
	mySpotLight[2].outerCutOff = glm::cos(glm::radians(27.0f));

	mySpotLight[3].position = glm::vec3(10.36f, 5.93f, 7.44f);
	mySpotLight[3].direction = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
	mySpotLight[3].cutOff = glm::cos(glm::radians(22.0f));
	mySpotLight[3].outerCutOff = glm::cos(glm::radians(27.0f));
}

void initPointLight() {
	myPointLight.position = glm::vec3(-19.0f, -4.13f, 37.04f);
	myPointLight.constant = 1.0f;
	myPointLight.linear = 0.09f;
	myPointLight.quadratic= 0.032f;
	myPointLight.color = glm::vec3(1.0f, 1.0f, 0.0f);
}

void initShaders() {
	myCustomShader.loadShader("shaders/shaderStart.vert", "shaders/shaderStart.frag");
	myCustomShader.useShaderProgram();
	myCustomShader2.loadShader("shaders/shaderStart2.vert", "shaders/shaderStart2.frag");
	myCustomShader2.useShaderProgram();
	lightShader.loadShader("shaders/lightCube.vert", "shaders/lightCube.frag");
	lightShader.useShaderProgram();
	screenQuadShader.loadShader("shaders/screenQuad.vert", "shaders/screenQuad.frag");
	screenQuadShader.useShaderProgram();
	depthMapShader.loadShader("shaders/depthMap.vert", "shaders/depthMap.frag");
	depthMapShader.useShaderProgram();
	skyboxShader.loadShader("shaders/skyboxShader.vert", "shaders/skyboxShader.frag");
	skyboxShader.useShaderProgram();
}

void initUniforms() {
	myCustomShader.useShaderProgram();

	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myCustomShader.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(3.1f, 22.68f, 27.61f);
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	lightDirLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	//set spotlight
	initSpotLight();

	for (int i = 0; i <SPOTLIGHT_NO; ++i) {
		std::string baseName = "mySpotLight[" + std::to_string(i) + "]";

		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + ".position").c_str()), 1, glm::value_ptr(mySpotLight[i].position));
		glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + ".direction").c_str()), 1, glm::value_ptr(mySpotLight[i].direction));
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + ".cutOff").c_str()), mySpotLight[i].cutOff);
		glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + ".outerCutOff").c_str()), mySpotLight[i].outerCutOff);
	}

	initPointLight();

	std::string baseName = "myPointLight";

	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + ".position").c_str()), 1, glm::value_ptr(myPointLight.position));
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + ".constant").c_str()), myPointLight.constant);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + ".linear").c_str()), myPointLight.linear);
	glUniform1f(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + ".quadratic").c_str()), myPointLight.quadratic);
	glUniform3fv(glGetUniformLocation(myCustomShader.shaderProgram, (baseName + ".color").c_str()), 1, glm::value_ptr(myPointLight.color));

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void initUniforms2() {
	myCustomShader2.useShaderProgram();

	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myCustomShader2.shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myCustomShader2.shaderProgram, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	normalMatrixLoc = glGetUniformLocation(myCustomShader2.shaderProgram, "normalMatrix");
	glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

	projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.1f, 1000.0f);
	projectionLoc = glGetUniformLocation(myCustomShader2.shaderProgram, "projection");
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(3.1f, 22.68f, 27.61f);
	lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
	lightDirLoc = glGetUniformLocation(myCustomShader2.shaderProgram, "lightDir");
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

	//set light color
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myCustomShader2.shaderProgram, "lightColor");
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

	//set spotlight
	initSpotLight();

	for (int i = 0; i < SPOTLIGHT_NO; ++i) {
		std::string baseName = "mySpotLight[" + std::to_string(i) + "]";

		glUniform3fv(glGetUniformLocation(myCustomShader2.shaderProgram, (baseName + ".position").c_str()), 1, glm::value_ptr(mySpotLight[i].position));
		glUniform3fv(glGetUniformLocation(myCustomShader2.shaderProgram, (baseName + ".direction").c_str()), 1, glm::value_ptr(mySpotLight[i].direction));
		glUniform1f(glGetUniformLocation(myCustomShader2.shaderProgram, (baseName + ".cutOff").c_str()), mySpotLight[i].cutOff);
		glUniform1f(glGetUniformLocation(myCustomShader2.shaderProgram, (baseName + ".outerCutOff").c_str()), mySpotLight[i].outerCutOff);
	}

	initPointLight();

	std::string baseName = "myPointLight";

	glUniform3fv(glGetUniformLocation(myCustomShader2.shaderProgram, (baseName + ".position").c_str()), 1, glm::value_ptr(myPointLight.position));
	glUniform1f(glGetUniformLocation(myCustomShader2.shaderProgram, (baseName + ".constant").c_str()), myPointLight.constant);
	glUniform1f(glGetUniformLocation(myCustomShader2.shaderProgram, (baseName + ".linear").c_str()), myPointLight.linear);
	glUniform1f(glGetUniformLocation(myCustomShader2.shaderProgram, (baseName + ".quadratic").c_str()), myPointLight.quadratic);
	glUniform3fv(glGetUniformLocation(myCustomShader2.shaderProgram, (baseName + ".color").c_str()), 1, glm::value_ptr(myPointLight.color));

	lightShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void initFBO() {
	//TODO - Create the FBO, the depth texture and attach the depth texture to the FBO
	glGenFramebuffers(1, &shadowMapFBO);
	glGenTextures(1, &depthMapTexture);
	glBindTexture(GL_TEXTURE_2D, depthMapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture,
		0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 computeLightSpaceTrMatrix() {
	//TODO - Return the light-space transformation matrix
	glm::mat4 lightView = glm::lookAt((glm::inverseTranspose(glm::mat3(lightRotation)) * lightDir), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const GLfloat near_plane = -300.0f, far_plane = 300.0f;
	glm::mat4 lightProjection = glm::ortho(-150.0f, 150.0f, -150.0f, 150.0f, near_plane, far_plane);
	glm::mat4 lightSpaceTrMatrix = lightProjection * lightView ;
	return lightSpaceTrMatrix;
}

void drawObjects(gps::Shader shader, bool depthPass) {

	shader.useShaderProgram();
	model = glm::mat4(1.0f);
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	farm.Draw(shader);

	glm::mat4 modelAux = model;
	modelAux = glm::translate(modelAux, glm::vec3(80.946f, 13.939f, 130.33f));
	modelAux = glm::rotate(modelAux, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
	glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelAux));

	if (!depthPass) {
		normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
	}
	windmill.Draw(shader);
}

void drawTransparentObjects(gps::Shader shader, bool depthPass, const gps::Camera& myCamera) {
	shader.useShaderProgram();

	std::vector<std::pair<float, const TransparentObject*>> distances;
	for (const TransparentObject& obj : transparentObjects) {
		float distance = glm::length(myCamera.getPosition() - obj.position);
		distances.emplace_back(distance, &obj);
	}

	std::sort(distances.begin(), distances.end(), [](const auto& a, const auto& b) {
		return a.first > b.first;
		});

	for (const TransparentObject& obj : transparentObjects) {
		glm::mat4 model = glm::mat4(1.0f); 
		model = glm::translate(model, obj.position);
		model = glm::rotate(model, glm::radians(obj.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(obj.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); 
		model = glm::rotate(model, glm::radians(obj.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); 
		
		model = glm::scale(model, obj.scale);

		glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

		if (!depthPass) {
			glm::mat3 normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
			glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
		}
	
		obj.object->Draw(shader);
	}
}

void renderScene() {

	// depth maps creation pass
	//TODO - Send the light-space transformation matrix to the depth map creation shader and
	//		 render the scene in the depth map

	depthMapShader.useShaderProgram();
	glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
		1,
		GL_FALSE,
		glm::value_ptr(computeLightSpaceTrMatrix()));
	glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	drawObjects(depthMapShader, true);
	
	glDisable(GL_BLEND);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// render depth map on screen - toggled with the M key
	if (showDepthMap) {
		glViewport(0, 0, retina_width, retina_height);

		glClear(GL_COLOR_BUFFER_BIT);

		screenQuadShader.useShaderProgram();

		//bind the depth map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(screenQuadShader.shaderProgram, "depthMap"), 0);

		glDisable(GL_DEPTH_TEST);
		screenQuad.Draw(screenQuadShader);
		glEnable(GL_DEPTH_TEST);
		//glDepthFunc(GL_LESS);
	}
	else {

		// final scene rendering pass (with shadows)

		glViewport(0, 0, retina_width, retina_height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (usePointLightShader) {
			currentShader = myCustomShader2;
			lightColor = glm::vec3(0.0f, 1.0f, 0.0f);
		}
		else {
			currentShader = myCustomShader;
			lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
		}

		currentShader.useShaderProgram();
		view = myCamera.getViewMatrix();
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

		lightRotation = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniform3fv(lightDirLoc, 1, glm::value_ptr(glm::inverseTranspose(glm::mat3(view * lightRotation)) * lightDir));

		//bind the shadow map
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, depthMapTexture);
		glUniform1i(glGetUniformLocation(currentShader.shaderProgram, "shadowMap"), 3);

		glUniformMatrix4fv(glGetUniformLocation(currentShader.shaderProgram, "lightSpaceTrMatrix"),
			1,
			GL_FALSE,
			glm::value_ptr(computeLightSpaceTrMatrix()));

		drawObjects(currentShader, false);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		drawTransparentObjects(currentShader, false, myCamera);  // Render transparent objects
		glDisable(GL_BLEND);
		//draw a white cube around the light

		lightShader.useShaderProgram();

		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));

		model = lightRotation;
		model = glm::translate(model, 1.0f * lightDir);
		model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
		glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

		lightCube.Draw(lightShader);

		mySkyBox.Draw(skyboxShader, view, projection);
	}
}

void cleanup() {
	glDeleteTextures(1, &depthMapTexture);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &shadowMapFBO);
	glfwDestroyWindow(glWindow);
	//close GL context and any other GLFW resources
	glfwTerminate();
}

int main(int argc, const char* argv[]) {

	if (!initOpenGLWindow()) {
		glfwTerminate();
		return 1;
	}
	
	initOpenGLState();
	initObjects();
	initShaders();
	initUniforms2();
	initUniforms();
	initFBO();

	PlaySound(L"sound/natures-serenity-soundscape.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);

	glCheckError();

	float lastFrameTime = 0.0f;
	float deltaTime = 0.0f;

	while (!glfwWindowShouldClose(glWindow)) {
		float currentFrameTime = glfwGetTime();
		deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		processMovement();
		animateCamera(deltaTime);
		renderScene();

		glfwPollEvents();
		glfwSwapBuffers(glWindow);
	}

	cleanup();
	PlaySound(NULL, NULL, 0);

	return 0;
}
