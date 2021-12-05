/*
Student Information
Student ID: 1155110941
Student Name: Lee Man Ho
*/

#include "Dependencies/glew/glew.h"
#include "Dependencies/GLFW/glfw3.h"
#include "Dependencies/glm/glm.hpp"
#include "Dependencies/glm/gtc/matrix_transform.hpp"
//#include "Dependencies/freeglut/freeglut.h"

#include "Dependencies/stb_image/stb_image.h"
#define STB_IMAGE_IMPLEMENTATION

#include "Shader.h"
#include "Texture.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <map>

// screen setting
const int SCR_WIDTH = 800;
const int SCR_HEIGHT = 600;

GLint programID;
int astroidnum;
int textureNum[3];

glm::mat4 spaceshippos = glm::mat4(1.0f);
float theta = 20.0f;

//camera
glm::mat4 spaceshipLocal = glm::mat4(1.0f);
glm::mat4 spaceship_Rotation = glm::mat4(1.0f);
float SSInitPos[3] = { 50.0f, 0.0f, 50.0f };
float SSTrans[3] = { 1.0f, 0.0f, 1.0f };
glm::vec4 cameraTarget, SS_world_FB_Dir, SS_world_RL_Dir;
glm::vec3 SS_local_fb = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 SS_local_rl = glm::vec3(-1.0f, 0.0f, 0.0f);
float zoom = 45.0f;
glm::vec4 cameraLocation;
glm::vec3 cameraFront = glm::vec3(0, 400, -1700);

// struct for storing the obj file
struct Vertex {
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec3 normal;
};

struct Model {
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
};

Model loadOBJ(const char* objPath)
{
	// function to load the obj file
	// Note: this simple function cannot load all obj files.

	struct V {
		// struct for identify if a vertex has showed up
		unsigned int index_position, index_uv, index_normal;
		bool operator == (const V& v) const {
			return index_position == v.index_position && index_uv == v.index_uv && index_normal == v.index_normal;
		}
		bool operator < (const V& v) const {
			return (index_position < v.index_position) ||
				(index_position == v.index_position && index_uv < v.index_uv) ||
				(index_position == v.index_position && index_uv == v.index_uv && index_normal < v.index_normal);
		}
	};

	std::vector<glm::vec3> temp_positions;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	std::map<V, unsigned int> temp_vertices;

	Model model;
	unsigned int num_vertices = 0;

	std::cout << "\nLoading OBJ file " << objPath << "..." << std::endl;

	std::ifstream file;
	file.open(objPath);

	// Check for Error
	if (file.fail()) {
		std::cerr << "Impossible to open the file! Do you use the right path? See Tutorial 6 for details" << std::endl;
		exit(1);
	}

	while (!file.eof()) {
		// process the object file
		char lineHeader[128];
		file >> lineHeader;

		if (strcmp(lineHeader, "v") == 0) {
			// geometric vertices
			glm::vec3 position;
			file >> position.x >> position.y >> position.z;
			temp_positions.push_back(position);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			// texture coordinates
			glm::vec2 uv;
			file >> uv.x >> uv.y;
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			// vertex normals
			glm::vec3 normal;
			file >> normal.x >> normal.y >> normal.z;
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			// Face elements
			V vertices[3];
			for (int i = 0; i < 3; i++) {
				char ch;
				file >> vertices[i].index_position >> ch >> vertices[i].index_uv >> ch >> vertices[i].index_normal;
			}

			// Check if there are more than three vertices in one face.
			std::string redundency;
			std::getline(file, redundency);
			if (redundency.length() >= 5) {
				std::cerr << "There may exist some errors while load the obj file. Error content: [" << redundency << " ]" << std::endl;
				std::cerr << "Please note that we only support the faces drawing with triangles. There are more than three vertices in one face." << std::endl;
				std::cerr << "Your obj file can't be read properly by our simple parser :-( Try exporting with other options." << std::endl;
				exit(1);
			}

			for (int i = 0; i < 3; i++) {
				if (temp_vertices.find(vertices[i]) == temp_vertices.end()) {
					// the vertex never shows before
					Vertex vertex;
					vertex.position = temp_positions[vertices[i].index_position - 1];
					vertex.uv = temp_uvs[vertices[i].index_uv - 1];
					vertex.normal = temp_normals[vertices[i].index_normal - 1];

					model.vertices.push_back(vertex);
					model.indices.push_back(num_vertices);
					temp_vertices[vertices[i]] = num_vertices;
					num_vertices += 1;
				}
				else {
					// reuse the existing vertex
					unsigned int index = temp_vertices[vertices[i]];
					model.indices.push_back(index);
				}
			} // for
		} // else if
		else {
			// it's not a vertex, texture coordinate, normal or face
			char stupidBuffer[1024];
			file.getline(stupidBuffer, 1024);
		}
	}
	file.close();

	std::cout << "There are " << num_vertices << " vertices in the obj file.\n" << std::endl;
	return model;
}


void get_OpenGL_info()
{
	// OpenGL information
	const GLubyte* name = glGetString(GL_VENDOR);
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* glversion = glGetString(GL_VERSION);
	std::cout << "OpenGL company: " << name << std::endl;
	std::cout << "Renderer name: " << renderer << std::endl;
	std::cout << "OpenGL version: " << glversion << std::endl;
}


GLuint vao[10];
GLuint vbo[10];
GLuint ebo[10];
Model obj[10];

//OBJECT spaceship, planet, craft, rock;

Texture spaceshipTexture0, spaceshipTexture1;
Texture planetTexture0, planetTexture1;
Texture rockTexture;
Texture craftTexture0, craftTexture1;

Shader Objshader;

void loadSpaceship()
{
	obj[0] = loadOBJ("CourseProjectMaterials/CourseProjectMaterials/object/spacecraft.obj");
	spaceshipTexture0.setupTexture("CourseProjectMaterials/CourseProjectMaterials/texture/spacecraftTexture.bmp");
	spaceshipTexture1.setupTexture("CourseProjectMaterials/CourseProjectMaterials/texture/gold.bmp");

	//setup vao
	glGenVertexArrays(1, &vao[0]);
	glBindVertexArray(vao[0]);

	//vbo
	glGenBuffers(1, &vbo[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, obj[0].vertices.size() * sizeof(Vertex), &obj[0].vertices[0], GL_STATIC_DRAW);

	//ebo
	glGenBuffers(1, &ebo[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj[0].indices.size() * sizeof(unsigned int), &obj[0].indices[0], GL_STATIC_DRAW);

	//vertex position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
}

void loadPlanet() {
	// load the planet data from the OBJ file 
	obj[1] = loadOBJ("CourseProjectMaterials/CourseProjectMaterials/object/planet.obj");
	planetTexture0.setupTexture("CourseProjectMaterials/CourseProjectMaterials/texture/earthTexture.bmp");
	planetTexture1.setupTexture("CourseProjectMaterials/CourseProjectMaterials/texture/earthNormal.bmp");

	//setup vao
	glGenVertexArrays(1, &vao[1]);
	glBindVertexArray(vao[1]);

	//vbo
	glGenBuffers(1, &vbo[1]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, obj[1].vertices.size() * sizeof(Vertex), &obj[1].vertices[0], GL_STATIC_DRAW);

	//ebo
	glGenBuffers(1, &ebo[1]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj[1].indices.size() * sizeof(unsigned int), &obj[1].indices[0], GL_STATIC_DRAW);

	//vertex position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
}

void loadCraft() {

	obj[2] = loadOBJ("CourseProjectMaterials/CourseProjectMaterials/object/craft.obj");
	craftTexture0.setupTexture("CourseProjectMaterials/CourseProjectMaterials/texture/ringTexture.bmp");
	craftTexture1.setupTexture("CourseProjectMaterials/CourseProjectMaterials/texture/red.bmp");

	//vao
	glGenVertexArrays(1, &vao[2]);
	glBindVertexArray(vao[2]);

	//vbo
	glGenBuffers(1, &vbo[2]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
	glBufferData(GL_ARRAY_BUFFER, obj[2].vertices.size() * sizeof(Vertex), &obj[2].vertices[0], GL_STATIC_DRAW);

	//ebo
	glGenBuffers(1, &ebo[2]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj[2].indices.size() * sizeof(unsigned int), &obj[2].indices[0], GL_STATIC_DRAW);

	//vertex position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
}

void loadRock() {
	obj[3] = loadOBJ("CourseProjectMaterials/CourseProjectMaterials/object/rock.obj");
	rockTexture.setupTexture("CourseProjectMaterials/CourseProjectMaterials/texture/rockTexture.bmp");

	//vao
	glGenVertexArrays(1, &vao[3]);
	glBindVertexArray(vao[3]);

	//vbo
	glGenBuffers(1, &vbo[3]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
	glBufferData(GL_ARRAY_BUFFER, obj[3].vertices.size() * sizeof(Vertex), &obj[3].vertices[0], GL_STATIC_DRAW);

	//ebo
	glGenBuffers(1, &ebo[3]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj[3].indices.size() * sizeof(unsigned int), &obj[3].indices[0], GL_STATIC_DRAW);
	
	//vertex position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
}

void sendDataToOpenGL()
{
	//TODO
	//Load objects and bind to VAO and VBO
	//Load textures
	loadSpaceship();
	loadPlanet();
	//loadCraft();
	loadRock();
}

void initializedGL(void) //run only once
{
	if (glewInit() != GLEW_OK) {
		std::cout << "GLEW not OK." << std::endl;
	}

	get_OpenGL_info();
	sendDataToOpenGL();

	//TODO: set up the camera parameters	
	//TODO: set up the vertex shader and fragment shader
	Objshader.setupShader("VertexShaderCode.glsl", "FragmentShaderCode.glsl");
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
}

//light

//camera and spaceship move together
void sscamera() 
{
	float scale = 0.005;

	glm::mat4 spaceship_scale = glm::scale(glm::mat4(1.0f), glm::vec3(scale));

	glm::mat4 spaceship_trans_M = glm::translate(glm::mat4(1.0f),
		glm::vec3(SSInitPos[0] + SSTrans[0], SSInitPos[1] + SSTrans[1], SSInitPos[2] + SSTrans[2]));

	spaceship_Rotation = glm::rotate(glm::mat4(1.0f), glm::radians(theta), glm::vec3(0.0f, 1.0f, 0.0f));

	spaceshipLocal = spaceship_trans_M * spaceship_Rotation * spaceship_scale;
	cameraTarget = spaceshipLocal * glm::vec4(SS_local_fb, 1.0f);

	SS_world_FB_Dir = spaceshipLocal * glm::vec4(SS_local_fb, 1.0f);
	SS_world_FB_Dir = normalize(SS_world_FB_Dir);

	SS_world_RL_Dir = spaceshipLocal * glm::vec4(SS_local_fb, 1.0f);
	SS_world_RL_Dir = normalize(SS_world_RL_Dir);

	cameraLocation = spaceshipLocal * glm::vec4(cameraFront, 1.0f);
}

//object position
glm::vec3 planet_location = glm::vec3(100.0f, 0.0f, 100.0f);

glm::vec3 craft_location_1 = glm::vec3(100.0f, 0.0f, 100.0f);

//draw objects to send to paintGL
void draw(int objID)
{
	GLint modelTransformMatrixUniformLocation;
	glm::mat4 modelTransformMatrix;
	GLuint textplus;
	GLuint TextureID;

	if (objID == 1) //spaceship
	{
		textplus = 0;
		TextureID = glGetUniformLocation(programID, "myTextureSampler");
		glActiveTexture(GL_TEXTURE0 + textplus);
		if (textureNum[0] == 0)
			spaceshipTexture0.bind(0);
		else
			spaceshipTexture1.bind(0);

		glUniform1i(TextureID, textplus);

		modelTransformMatrix = spaceshippos;

		modelTransformMatrixUniformLocation = glGetUniformLocation(programID, "modelTransformMatrix");
		glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
	}

	if (objID == 2) //planet
	{
		textplus = 0;
		TextureID = glGetUniformLocation(programID, "myTextureSampler");
		glActiveTexture(GL_TEXTURE0 + textplus);
		if (textureNum[0] == 0)
			planetTexture0.bind(0);
		else
			planetTexture1.bind(0);

		glUniform1i(TextureID, textplus);

		modelTransformMatrix = spaceshippos;

		modelTransformMatrixUniformLocation = glGetUniformLocation(programID, "modelTransformMatrix");
		glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
	}

	if (objID == 3) //craft
	{
		textplus = 0;
		TextureID = glGetUniformLocation(programID, "myTextureSampler");
		glActiveTexture(GL_TEXTURE0 + textplus);
		if (textureNum[0] == 0)
			craftTexture0.bind(0);
		else
			craftTexture1.bind(0);

		glUniform1i(TextureID, textplus);

		modelTransformMatrix = spaceshippos;

		modelTransformMatrixUniformLocation = glGetUniformLocation(programID, "modelTransformMatrix");
		glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
	}

	if (objID == 4) //rock
	{
		textplus = 0;
		TextureID = glGetUniformLocation(programID, "myTextureSampler");
		glActiveTexture(GL_TEXTURE0 + textplus);
		rockTexture.bind(0);

		glUniform1i(TextureID, textplus);

		modelTransformMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0, 1.0, 1.0));;
		modelTransformMatrix *= glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, 0.0f));;
		modelTransformMatrix *= glm::rotate(glm::mat4(1.0f), glm::radians((float)glfwGetTime() * 100), glm::vec3(0, 1, 0));;
		modelTransformMatrix *= glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.0f, 0.0f));;

		modelTransformMatrixUniformLocation = glGetUniformLocation(programID, "modelTransformMatrix");
		glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelTransformMatrix[0][0]);
	}

	//view camera	
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	viewMatrix = glm::lookAt(glm::vec3(cameraLocation), glm::vec3(cameraTarget), glm::vec3(0.0f, 1.0f, 0.0f));

	GLint viewMatrixUniformLocation =
		glGetUniformLocation(programID, "viewMatrix");
	glUniformMatrix4fv(viewMatrixUniformLocation, 1,
		GL_FALSE, &viewMatrix[0][0]);

	//projection
	glm::mat4 projectionMatrix;
	projectionMatrix = glm::perspective(glm::radians(zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
	GLint projectionMatrixUniformLocation = glGetUniformLocation(programID, "projectionMatrix");
	glUniformMatrix4fv(projectionMatrixUniformLocation, 1, GL_FALSE, &projectionMatrix[0][0]);
}

//random asteroids
void Asteroids(int asteroidnum) {
	glm::mat4* modelMatrix;
	modelMatrix = new glm::mat4[asteroidnum];
	srand(glfwGetTime() * 0.000001); // initialize random
	float radius = 15;
	float offset = 5.0f;
	float angle, displacement, x, y, z, scale, rotationAngle;
	int i;

	for (GLuint i = 0; i < asteroidnum; i++)
	{
		glm::mat4 model = glm::mat4(1.0f);

		// rotate at planet centre
		model = glm::translate(model, glm::vec3(planet_location.x * 6, planet_location.y, planet_location.z * 6));
		model = glm::rotate(model, glm::radians((float)glfwGetTime() * 10), glm::vec3(0.0f, 1.0f, 0.0f));

		//translation
		angle = (float)i / (float)asteroidnum * 360.0f;

		displacement = (rand() % (int)(2 * offset * 100)) / 100.0f - offset;

		x = sin(angle) * radius + displacement;
		y = displacement * 0.5f;
		z = cos(angle) * radius + displacement;

		model = glm::translate(model, glm::vec3(x, y, z));

		//rotation: add random rotation
		rotationAngle = (rand() % 360);
		model = glm::rotate(model, rotationAngle, glm::vec3(0.5f, 0.5f, 1.0f));

		//add to model matrix
		modelMatrix[i] = model;
	}

	for (i = 0; i < asteroidnum; i++) 
	{
		GLint modelTransformMatrixUniformLocation = glGetUniformLocation(programID, "modelTransformMatrix");
		glUniformMatrix4fv(modelTransformMatrixUniformLocation, 1, GL_FALSE, &modelMatrix[i][0][0]);
		glDrawElements(GL_TRIANGLES, obj[4].indices.size(), GL_UNSIGNED_INT, 0);
	}
}


void paintGL(void)  //always run
{
	glClearColor(0.0f, 3.0f, 1.0f, 0.5f); //specify the background color, this is just an example
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//TODO:
	//Set lighting information, such as position and color of lighting source
	//Set transformation matrix
	//Bind different textures
	sscamera();

	//spaceship
	glBindVertexArray(vao[0]);
	draw(1);
	glDrawElements(GL_TRIANGLES, obj[0].indices.size(), GL_UNSIGNED_INT, 0);

	//earth
	glBindVertexArray(vao[1]);
	draw(2);
	glDrawElements(GL_TRIANGLES, obj[1].indices.size(), GL_UNSIGNED_INT, 0);

	//craft
	glBindVertexArray(vao[2]);
	draw(3);
	glDrawElements(GL_TRIANGLES, obj[1].indices.size(), GL_UNSIGNED_INT, 0);

	//asteroids
	glBindVertexArray(vao[3]);
	draw(4);
	Asteroids(astroidnum);
}

struct MouseController 
{
	bool LEFT_BUTTON = false;
	bool RIGHT_BUTTON = false;
	double MOUSE_Clickx = 0.0, MOUSE_Clicky = 0.0;
	double MOUSE_X = 0.0, MOUSE_Y = 0.0;
	int click_time = glfwGetTime();
};

MouseController controlMouse;
bool Mousef;
double finalX;

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (Mousef)
	{
		finalX = xpos;
		Mousef = false;
	}

	if (finalX > xpos)
	{
		theta += 5.0f;
		spaceship_Rotation = glm::rotate(glm::mat4(1.0f), glm::radians(theta), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	if (finalX < xpos)
	{
		theta -= 5.0f;
		spaceship_Rotation = glm::rotate(glm::mat4(1.0f), glm::radians(theta), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	finalX = xpos;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// Sets the mouse-button callback for the current window.	
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		controlMouse.LEFT_BUTTON = true;
	}
}

void cursor_position_callback(GLFWwindow* window, double x, double y)
{
	// Sets the cursor position callback for the current window
	if (controlMouse.LEFT_BUTTON)
	{
		mouse_callback(window, x, y);
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	zoom -= (float)yoffset;

	if (zoom < 1.0f)
		zoom = 1.0f;

	if (zoom > 45.0f)
		zoom = 45.0f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// Sets the Keyboard callback for the current window.
	if (key == GLFW_KEY_DOWN) 
	{
		SSTrans[2] += cos(glm::radians(theta)) * -SS_world_FB_Dir[2];
		SSTrans[0] += sin(glm::radians(theta)) * -SS_world_FB_Dir[2];
	}

	if (key == GLFW_KEY_UP) 
	{
		SSTrans[2] += cos(glm::radians(theta)) * SS_world_FB_Dir[2];
		SSTrans[0] += sin(glm::radians(theta)) * SS_world_FB_Dir[2];
	}

	if (key == GLFW_KEY_LEFT) 
	{
		SSTrans[2] += cos(glm::radians(theta) + 90) * SS_world_RL_Dir[0];
		SSTrans[0] += sin(glm::radians(theta) + 90) * SS_world_RL_Dir[0];
	}

	if (key == GLFW_KEY_RIGHT) 
	{
		SSTrans[2] += cos(glm::radians(theta) + 90) * -SS_world_RL_Dir[0];
		SSTrans[0] += sin(glm::radians(theta) + 90) * -SS_world_RL_Dir[0];
	}

}


int main(int argc, char* argv[])
{
	GLFWwindow* window;

	/* Initialize the glfw */
	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW" << std::endl;
		return -1;
	}
	/* glfw: configure; necessary for MAC */
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Assignment 2", NULL, NULL);
	if (!window) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	/*register callback functions*/
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);                                                                   
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	initializedGL();

	while (!glfwWindowShouldClose(window)) {
		/* Render here */
		paintGL();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}






