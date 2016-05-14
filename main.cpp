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
	glm::mat4 model;
	object_struct(): model(glm::mat4(1.0f)){}
};

std::vector<object_struct> objects;//vertex array object,vertex buffer object and texture(color) for objs
std::vector<int> indicesCount;//Number of indice of objs

/* The ID corresponding to the shader type of shader program */
#define FLAT 0
#define GOURAUD 1
#define PHONG 2
#define BLINN_PHONG 3
#define NUM_OF_SHADER 4
unsigned int program, programs[NUM_OF_SHADER];
/* The index of the object which using flat shader in the rendering list. */
static int flatObject_ID;
static glm::vec3 sunPosition = glm::vec3(0.0f, 10.0f, 15.0f);
/* Toggle the value by pressing P key. If it's ture, the light will keep rotating. */
static bool keepRotate = true;
static GLuint fbo = 0;	// Reference to frame buffer object.
static GLuint rbo = 0;
static GLuint renderedTexture = 0;	// Reference to the screen rendered texture.
static object_struct renderPlane;
static int renderPlaneIndicesCount;

#define VIEW_WIDTH 800
#define VIEW_HEIGHT 600

static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	else if (key == GLFW_KEY_P && action == GLFW_PRESS)
		keepRotate = !keepRotate;
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
 * Return:
 * - The index of this obejct in the rendering list.
 */
static int add_obj(unsigned int program, const char *filename, const char *texbmp)
{
	object_struct new_node;

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
	if (texbmp != NULL)
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

		if (texbmp != NULL) {
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
		glDeleteProgram(programs[i]);
	}
	glDeleteProgram(program);

	glDeleteFramebuffers(1, &fbo);
	glDeleteVertexArrays(1, &renderPlane.vao);
	glDeleteBuffers(4, renderPlane.vbo);
	glDeleteTextures(1, &renderedTexture);
	glDeleteProgram(renderPlane.program);
	glDeleteRenderbuffers(1, &rbo);
}

/* Create a frame buffer for rendering the scene to the texture and bind to the rendering
 * pipeline.
 */
static bool generateFBO()
{
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	/* Create and initialize an empty texture */
	glGenTextures(1, &renderedTexture);
	glBindTexture(GL_TEXTURE_2D, renderedTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, VIEW_WIDTH, VIEW_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	// When the texture should be magified, use the nearest texture coordinates.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// When the texture should be minified, use the nearest texture coordinates.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	/* Attach the new texture to the frame buffer */
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderedTexture, 0);

	/* Create a depth buffer */
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, VIEW_WIDTH, VIEW_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	// Set the list of draw buffers
	GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, drawBuffers);

	// Always check that the framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
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
static void setUniformFloatA(unsigned int program, const std::string &name, const int count, const float* f_array)
{
	glUseProgram(program);
	GLint loc = glGetUniformLocation(program, name.c_str());
	if (loc == -1) return;

	glUniform1fv(loc, count, f_array);
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
static void setUniformVec4A(unsigned int program, const std::string &name, const int count, const glm::vec4 *vec_array)
{
	glUseProgram(program);
	GLint loc = glGetUniformLocation(program, name.c_str());
	if (loc == -1) return;

	// Convert the vec4 array to GLfloat array
	GLfloat float_array[count * 4];
	for (int i = 0; i < count; ++i) {
		float_array[i*4] = vec_array[i].x;
		float_array[i*4 + 1] = vec_array[i].y;
		float_array[i*4 + 2] = vec_array[i].z;
		float_array[i*4 + 3] = vec_array[i].w;
	}
	glUniform4fv(loc, count, float_array);
}

/* Create a render object for render plane.
 */
static void generateRenderPlane()
{
	unsigned int program_orthogonal = setup_shader(readfile("shader/vs_fbo.glsl").c_str(), readfile("shader/fs.glsl").c_str());
	glm::mat4 vp = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
	setUniformMat4(program_orthogonal, "vp", vp);

	// The render plane is a individual object, so remove from the object list.
	// For here, create the information of the plane object, and attach the texture later.
	add_obj(program_orthogonal, "plane.obj", NULL);
	renderPlane = objects.back();
	objects.pop_back();
	renderPlaneIndicesCount = indicesCount.back();
	indicesCount.pop_back();

	// The plane is on X-Z plane, so we need to rotate it to make it on X-Y plane.
	renderPlane.model = glm::rotate(90.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	setUniformMat4(program_orthogonal, "model", renderPlane.model);

	// Attach the texture which framebuffer would render to to the plane object.
	renderPlane.texture = renderedTexture;
}

static void render()
{
	/* Make OpenGL draw to the frame buffer first. */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	for(int i=0;i<objects.size();i++){
		glUseProgram(objects[i].program);
		glBindVertexArray(objects[i].vao);
		glBindTexture(GL_TEXTURE_2D, objects[i].texture);

		setUniformMat4(objects[i].program, "model", objects[i].model);

		// The flat shader
		if (i == flatObject_ID) {
			glShadeModel(GL_FLAT);
			glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
		} else
			glShadeModel(GL_SMOOTH);

		glDrawElements(GL_TRIANGLES, indicesCount[i], GL_UNSIGNED_INT, nullptr);
	}
	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

/* Add planets to the rendering list and initialize the model matrix of the sun.
 * Proceed the position and the color of the SUN to the rendering program.
 */
void initalPlanets()
{
	// Add planets to the rendering list
	add_obj(program, "sun.obj", "texture/sun.bmp");
	flatObject_ID = add_obj(programs[FLAT], "earth.obj", "texture/saturn.bmp");
	add_obj(programs[GOURAUD], "earth.obj", "texture/saturn.bmp");
	add_obj(programs[PHONG], "earth.obj", "texture/saturn.bmp");
	add_obj(programs[BLINN_PHONG], "earth.obj", "texture/saturn.bmp");

	// Initialize the position of 4 planets
	objects[1].model = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-7.5f, 0.0f, 0.0f)),
			glm::vec3(2.0f));
	objects[2].model = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-2.5f, 0.0f, 0.0f)),
			glm::vec3(2.0f));
	objects[3].model = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(2.5f, 0.0f, 0.0f)),
			glm::vec3(2.0f));
	objects[4].model = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(7.5f, 0.0f, 0.0f)),
			glm::vec3(2.0f));
}

/* @brief Load and initialize the shader programs.
 */
void initialShader()
{
	glm::vec3 viewPosition = glm::vec3(0.0f, 0.0f, 20.0f);
	glm::vec4 k[3] = {
		glm::vec4(0.2f, 0.2f, 0.2f, 0.0f),	// Ambient
		glm::vec4(0.9f, 0.9f, 0.9f, 0.0f),	// Diffuse
		glm::vec4(0.9f, 0.9f, 0.9f, 0.0f)	// Specular
	};
	glm::vec4 light[3] = {
		glm::vec4(1.0f, 1.0f, 1.0f, 0.0f),	// Ambient
		glm::vec4(1.0f, 1.0f, 1.0f, 0.0f),	// Diffuse
		glm::vec4(1.0f, 1.0f, 1.0f, 0.0f)	// Specular
	};
	float d_factor[3] = {1.0f, 0.01f, 0.001f};	// a, b, c of a + b*D + c*D^2

	// Matrix of MVP: M_pers * M_camera * M_model
	// - Model translation: orignal, no scale, no rotation.
	// - Camera: eye @ ( 0, 0, 20 ), look @ ( 0, 0, 0 ), Vup = ( 0, 1, 0 ).
	// - Perspective volume: fovy = 45 deg, aspect( x = 640, y = 480 ), zNear = 1, zFar = 200.
	glm::mat4 vp = glm::perspective(glm::radians(45.0f), 640.0f/480, 1.0f, 200.f)*
			glm::lookAt(viewPosition, glm::vec3(), glm::vec3(0, 1, 0))*
			glm::mat4(1.0f);

	program = setup_shader(readfile("shader/vs.glsl").c_str(), readfile("shader/fs.glsl").c_str());
	setUniformMat4(program, "vp", vp);

	programs[FLAT] = setup_shader(readfile("shader/vs_flat.glsl").c_str(), readfile("shader/fs_flat.glsl").c_str());
	programs[GOURAUD] = setup_shader(readfile("shader/vs_gouraud.glsl").c_str(), readfile("shader/fs_gouraud.glsl").c_str());
	programs[PHONG] = setup_shader(readfile("shader/vs_phong.glsl").c_str(), readfile("shader/fs_phong.glsl").c_str());
	programs[BLINN_PHONG] = setup_shader(readfile("shader/vs_blinn.glsl").c_str(), readfile("shader/fs_blinn.glsl").c_str());

	// Initialize the uniform variables
	for(int i = 0; i < NUM_OF_SHADER; ++i) {
		setUniformMat4(programs[i], "vp", vp);
		setUniformVec4(programs[i], "viewPosition", glm::vec4(viewPosition, 0.0f));
		setUniformVec4A(programs[i], "light", 3, light);
		setUniformVec4A(programs[i], "k", 3, k);
		setUniformFloat(programs[i], "shininess", 10.0f);
		setUniformFloatA(programs[i], "d_factor", 3, d_factor);
	}
}

/* Update the light position of each shader accroding to the position of the sun.
 * The sun rotates around y=0.
 */
void updateLightPosition(float radian)
{
	glm::vec3 rotatePosition = glm::rotateY(sunPosition, radian);

	objects[0].model = glm::scale(glm::translate(glm::mat4(1.0f), rotatePosition),
			glm::vec3(0.5f));

	for(int i = 0; i < NUM_OF_SHADER; ++i)
		setUniformVec4(programs[i], "lightPosition", glm::vec4(rotatePosition, 0.0f));
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

	glEnable(GL_DEPTH_TEST);
	glCullFace(GL_BACK);
	// Enable blend mode for billboard
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// load shader program
	initialShader();
	// Initialize the plantes
	initalPlanets();
	// Create a frame buffer of which size is the same as the default screen size.
	if (!generateFBO()) {
		std::cout << "Cannot generate fbo!\n" << std::endl;
		return EXIT_FAILURE;
	}
	generateRenderPlane();

	float last, start, sunRotateDeg = 0.0f;
	last = start = glfwGetTime();
	int fps=0;
	while (!glfwWindowShouldClose(window))
	{//program will keep draw here until you close the window
		float delta = glfwGetTime() - start;

		if (keepRotate) sunRotateDeg += 1.0f;
		if (sunRotateDeg > 359.5f) sunRotateDeg = 0.0f;
		updateLightPosition(glm::radians(sunRotateDeg));

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
