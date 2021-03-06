#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>
#include <string>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <vector>
#include "tiny_obj_loader.h"

#define GLM_FORCE_RADIANS

struct object_struct{
	unsigned int program;
	unsigned int vao;
	unsigned int vbo[4];
	unsigned int texture;
	glm::vec4 materialEmission;
	glm::mat4 model;
	object_struct(): model(glm::mat4(1.0f)){}
};

std::vector<object_struct> objects;//vertex array object,vertex buffer object and texture(color) for objs
unsigned int program, program2;
std::vector<int> indicesCount;//Number of indice of objs

#include "planets.h"

#define EARTH_REV_RADIUS 10.0f
#define EARTH_SCALE_SIZE 0.8f
#define EARTH_ROT_DEG 0.8f
#define EARTH_REV_DEG 0.8f
static float planetRotDeg[NUM_OF_PLANETS];
static float planetRevDeg[NUM_OF_PLANETS];

/* Initialize the revolution radius, revolution period, rotate period, and radius ratio of
 * the planets to the earth, which you perfer to use in this program.
 * Therefore, the values set here are not equal to the real ratio in the solar system!
 */
static PlanetInfo planet_info[NUM_OF_PLANETS] = {
// { revRadius, revPeriod, rotPeriod, radius } ratio to the earth
	{ 0.0f, 0.0f, 0.5f, 100.0f },    // SUN, the value is not used here.
	{  0.383f, 1.4f, 0.8f,  0.378f },      // VERCURY
	{  0.724f, 1.2f, 0.6f,  0.958f },      // VENUS
	{  1.000f, 1.0f, 1.0f,  1.000f },      // EARTH
	{  1.523f, 0.8f, 1.2f,  0.531f },      // MARS
	{  5.221f, 0.6f, 2.0f, 10.958f },      // JUPITER
	{  6.281f, 0.4f, 1.8f,  9.138f },      // SATURN
	{  7.261f, 0.2f, 1.4f,  3.981f },      // URUANS
	{  8.187f, 0.1f, 1.6f,  3.864f },      // NEPTUNE
};

static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

/* Load and compile the vertex shader and fragment shader, and link them to a program object.
 * Parameter:
 * - vertex_shader: The char array contains the source code of vertex shader.
 * - fragment_shader: The char array contains the source code of fragment shader.
 * Return:
 * - The reference address to the created program
 */
static unsigned int setup_shader(const char *vertex_shader, const char *fragment_shader)
{
	// Compile the vertex shader
	GLuint vs=glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, (const GLchar**)&vertex_shader, nullptr);

	glCompileShader(vs);

	int status, maxLength;
	char *infoLog=nullptr;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
	if(status==GL_FALSE)
	{
		glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &maxLength);

		/* The maxLength includes the NULL character */
		infoLog = new char[maxLength];

		glGetShaderInfoLog(vs, maxLength, &maxLength, infoLog);

		fprintf(stderr, "Vertex Shader Error: %s\n", infoLog);

		/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
		/* In this simple program, we'll just leave */
		delete [] infoLog;
		return 0;
	}

	// Compile the fragment shader
	GLuint fs=glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, (const GLchar**)&fragment_shader, nullptr);
	glCompileShader(fs);

	glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
	if(status==GL_FALSE)
	{
		glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &maxLength);

		/* The maxLength includes the NULL character */
		infoLog = new char[maxLength];

		glGetShaderInfoLog(fs, maxLength, &maxLength, infoLog);

		fprintf(stderr, "Fragment Shader Error: %s\n", infoLog);

		/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
		/* In this simple program, we'll just leave */
		delete [] infoLog;
		return 0;
	}

	unsigned int program=glCreateProgram();
	// Attach our shaders to our program
	glAttachShader(program, vs);
	glAttachShader(program, fs);

	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &status);

	if(status==GL_FALSE)
	{
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);


		/* The maxLength includes the NULL character */
		infoLog = new char[maxLength];
		glGetProgramInfoLog(program, maxLength, NULL, infoLog);

		glGetProgramInfoLog(program, maxLength, &maxLength, infoLog);

		fprintf(stderr, "Link Error: %s\n", infoLog);

		/* Handle the error in an appropriate way such as displaying a message or writing to a log file. */
		/* In this simple program, we'll just leave */
		delete [] infoLog;
		return 0;
	}
	return program;
}

/* Create a string containing all contents in the specific file.
 */
static std::string readfile(const char *filename)
{
	std::ifstream ifs(filename);
	if(!ifs)
		exit(EXIT_FAILURE);
	// Copy string from input stream to EOF.
	return std::string( (std::istreambuf_iterator<char>(ifs)),
			(std::istreambuf_iterator<char>()));
}

// mini bmp loader written by HSU YOU-LUN
static unsigned char *load_bmp(const char *bmp, unsigned int *width, unsigned int *height, unsigned short int *bits)
{
	unsigned char *result=nullptr;
	FILE *fp = fopen(bmp, "rb");
	if(!fp)
		return nullptr;
	char type[2];
	unsigned int size, offset;
	// check for magic signature	
	fread(type, sizeof(type), 1, fp);
	if(type[0]==0x42 || type[1]==0x4d){
		fread(&size, sizeof(size), 1, fp);
		// ignore 2 two-byte reversed fields
		fseek(fp, 4, SEEK_CUR);
		fread(&offset, sizeof(offset), 1, fp);
		// ignore size of bmpinfoheader field
		fseek(fp, 4, SEEK_CUR);
		fread(width, sizeof(*width), 1, fp);
		fread(height, sizeof(*height), 1, fp);
		// ignore planes field
		fseek(fp, 2, SEEK_CUR);
		fread(bits, sizeof(*bits), 1, fp);
		unsigned char *pos = result = new unsigned char[size-offset];
		fseek(fp, offset, SEEK_SET);
		while(size-ftell(fp)>0)
			pos+=fread(pos, 1, size-ftell(fp), fp);
	}
	fclose(fp);
	return result;
}

/* Add a object to rendering list.
 * Parameters:
 * - program: Which shader program this object should use
 * - filename: The object file of this object
 * - texbmp: The texture file for this object
 * - emission: The emission material color of this object
 * Return:
 * - The index of this obejct in the rendering list.
 */
static int add_obj(unsigned int program, const char *filename, const char *texbmp,
		glm::vec4 emission)
{
	object_struct new_node;
	new_node.materialEmission = emission;

	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string err = tinyobj::LoadObj(shapes, materials, filename);

	if (!err.empty()||shapes.size()==0)
	{
		std::cerr<<err<<std::endl;
		exit(1);
	}

	// Create spaces for vertex array object, vertex buffer objects, and texture object.
	glGenVertexArrays(1, &new_node.vao);
	glGenBuffers(4, new_node.vbo);
	glGenTextures(1, &new_node.texture);

	glBindVertexArray(new_node.vao);

	// Upload postion array
	glBindBuffer(GL_ARRAY_BUFFER, new_node.vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*shapes[0].mesh.positions.size(),
			shapes[0].mesh.positions.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	// Upload texCoord array and texture arary
	if(shapes[0].mesh.texcoords.size()>0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, new_node.vbo[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*shapes[0].mesh.texcoords.size(),
				shapes[0].mesh.texcoords.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glBindTexture(GL_TEXTURE_2D, new_node.texture);
		unsigned int width, height;
		unsigned short int bits;
		unsigned char *bgr=load_bmp(texbmp, &width, &height, &bits);
		GLenum format = (bits == 24? GL_BGR: GL_BGRA);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, format, GL_UNSIGNED_BYTE, bgr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glGenerateMipmap(GL_TEXTURE_2D);
		delete [] bgr;
	}

	// Upload normal array
	if(shapes[0].mesh.normals.size()>0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, new_node.vbo[2]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*shapes[0].mesh.normals.size(),
				shapes[0].mesh.normals.data(), GL_STATIC_DRAW);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}

	// Setup index buffer for glDrawElements
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, new_node.vbo[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)*shapes[0].mesh.indices.size(),
			shapes[0].mesh.indices.data(), GL_STATIC_DRAW);

	indicesCount.push_back(shapes[0].mesh.indices.size());

	// Unbind the vao of this object
	glBindVertexArray(0);

	new_node.program = program;

	objects.push_back(new_node);
	return objects.size()-1;
}

/* Delete all information of all objects in the rendering list.
 */
static void releaseObjects()
{
	for(int i=0;i<objects.size();i++){
		glDeleteVertexArrays(1, &objects[i].vao);
		glDeleteTextures(1, &objects[i].texture);
		glDeleteBuffers(4, objects[i].vbo);
	}
	glDeleteProgram(program);
}

/* Assign a new value to the mat4 variable of the specified shader program.
 * Parameter:
 * - program: At which shader program the mat4 variable that you want to assign is.
 * - name: The name of that variable
 * - mat: The new value
 */
static void setUniformMat4(unsigned int program, const std::string &name, const glm::mat4 &mat)
{
	// This line can be ignore. But, if you have multiple shader program
	// You must check if currect binding is the one you want
	glUseProgram(program);
	GLint loc=glGetUniformLocation(program, name.c_str());
	if(loc==-1) return;

	// mat4 of glm is column major, same as opengl
	// we don't need to transpose it. so..GL_FALSE
	glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
}

/* The same as setUniformMat4 but set a float value.
 * Parameter:
 * - f: The new float value
 */
static void setUniformFloat(unsigned int program, const std::string &name, const float f)
{
	glUseProgram(program);
	GLint loc = glGetUniformLocation(program, name.c_str());
	if (loc == -1) return;

	glUniform1f(loc, f);
}

/* The same as setUniformMat4 but set a vec4 value.
 * Parameter:
 * - vec: The new vec4 value
 */
static void setUniformVec4(unsigned int program, const std::string &name, const glm::vec4 &vec)
{
	glUseProgram(program);
	GLint loc = glGetUniformLocation(program, name.c_str());
	if (loc == -1) return;

	glUniform4fv(loc, 1, glm::value_ptr(vec));
}

static void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	for(int i=0;i<objects.size();i++){
		glUseProgram(objects[i].program);
		glBindVertexArray(objects[i].vao);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture);

		setUniformMat4(program, "model", objects[i].model);
		setUniformFloat(program, "rotateDeg", planetRotDeg[i]);
		setUniformVec4(program, "planetEmission", objects[i].materialEmission);

		glDrawElements(GL_TRIANGLES, indicesCount[i], GL_UNSIGNED_INT, nullptr);
	}
	glBindVertexArray(0);
}

/* Add planets to the rendering list and initialize the model matrix of the sun.
 * Proceed the position and the color of the SUN to the rendering program.
 */
void initalPlanets()
{
	// Add planets to the rendering list
	add_obj(program, "sun.obj", "texture/sun.bmp", glm::vec4(0.9f));
	add_obj(program, "earth.obj", "texture/mercury.bmp", glm::vec4(0.0f));
	add_obj(program, "earth.obj", "texture/venus.bmp", glm::vec4(0.0f));
	add_obj(program, "earth.obj", "texture/earth.bmp", glm::vec4(0.0f));
	add_obj(program, "earth.obj", "texture/mars.bmp", glm::vec4(0.0f));
	add_obj(program, "earth.obj", "texture/jupiter.bmp", glm::vec4(0.0f));
	add_obj(program, "earth.obj", "texture/saturn.bmp", glm::vec4(0.0f));
	add_obj(program, "earth.obj", "texture/uruans.bmp", glm::vec4(0.0f));
	add_obj(program, "earth.obj", "texture/neptune.bmp", glm::vec4(0.0f));

	// Initialize the model matrix, the position, and the light color of the SUN.
	objects[SUN].model = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
	setUniformVec4(program, "sunPosition", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
	setUniformVec4(program, "sunLightColor", glm::vec4(1.0f));
	// All planets use the same amibent and diffuse color.
	setUniformVec4(program, "planetAmbient", glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));
	setUniformVec4(program, "planetDiffuse", glm::vec4(1.1f));

	// Initialize the control variables
	for (int i = 0; i < NUM_OF_PLANETS; ++i)
		planetRevDeg[i] = planetRotDeg[i] = 0.0f;
}

/* Update the model matrix of each planet per frame accroding to the status of the earth.
 */
void updatePlanets()
{
	float revRadius_planet, revRad_planet, size_planet;

	for (int i = 1; i < NUM_OF_PLANETS; ++i) {
		revRadius_planet = EARTH_REV_RADIUS * planet_info[i].revRadius_ratio;
		revRad_planet = glm::radians(planetRevDeg[i]);
		size_planet = EARTH_SCALE_SIZE * planet_info[i].planetRadius_ratio;

		objects[i].model = glm::scale(glm::translate(glm::mat4(1.0f),
				glm::rotateY(glm::vec3(revRadius_planet, 0.0f, 0.0f), revRad_planet)),
			glm::vec3(size_planet));
	}
}

int main(int argc, char *argv[])
{
	GLFWwindow* window;
	glfwSetErrorCallback(error_callback);
	if (!glfwInit())
		exit(EXIT_FAILURE);
	// OpenGL 3.3, Mac OS X is reported to have some problem. However I don't have Mac to test
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	// For Mac OS X
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(800, 600, "Simple Example", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return EXIT_FAILURE;
	}

	glfwMakeContextCurrent(window);

	// This line MUST put below glfwMakeContextCurrent
	glewExperimental = GL_TRUE;
	glewInit();

	// Enable vsync
	glfwSwapInterval(1);

	// Setup input callback
	glfwSetKeyCallback(window, key_callback);

	// load shader program
	program = setup_shader(readfile("shader/vs.glsl").c_str(), readfile("shader/fs.glsl").c_str());
	program2 = setup_shader(readfile("shader/vs.glsl").c_str(), readfile("shader/fs.glsl").c_str());

	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	// Enable blend mode for billboard
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Matrix for transform pipeline of 'program': M_pers * M_camera * M_model
	// - Model translation: orignal, no scale, no rotation.
	// - Camera: eye @ ( 30, 30, 30 ), look @ ( 0, 0, 0 ), Vup = ( 0, 1, 0 ).
	// - Perspective volume: fovy = 45 deg, aspect( x = 640, y = 480 ), zNear = 1, zFar = 200.
	setUniformMat4(program, "vp", glm::perspective(glm::radians(45.0f), 640.0f/480, 1.0f, 200.f)*
			glm::lookAt(glm::vec3(30.0f), glm::vec3(), glm::vec3(0, 1, 0))*glm::mat4(1.0f));
	// camera for 'program2': orthogonal volume
	setUniformMat4(program2, "vp", glm::mat4(1.0));

	// Initialize the plantes
	initalPlanets();

	float last, start;
	last = start = glfwGetTime();
	int fps=0;
	while (!glfwWindowShouldClose(window))
	{//program will keep draw here until you close the window
		float delta = glfwGetTime() - start;

		for (int i = 0; i < NUM_OF_PLANETS; ++i) {
			planetRevDeg[i] += EARTH_REV_DEG * planet_info[i].revPeriod_ratio;
			if (planetRevDeg[i] > 360.0f ) planetRevDeg[i] -= 360.0f;
			planetRotDeg[i] += EARTH_ROT_DEG * planet_info[i].rotPeriod_ratio;
			if (planetRotDeg[i] > 360.0f ) planetRotDeg[i] -= 360.0f;
		}
		updatePlanets();
		render();
		glfwSwapBuffers(window);
		glfwPollEvents();
		fps++;
		if(glfwGetTime() - last > 1.0)
		{
			std::cout<<(double)fps/(glfwGetTime()-last)<<std::endl;
			fps = 0;
			last = glfwGetTime();
		}
	}

	releaseObjects();
	glfwDestroyWindow(window);
	glfwTerminate();
	return EXIT_SUCCESS;
}
