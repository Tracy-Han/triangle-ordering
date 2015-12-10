#include "platform.hpp"

// third-party libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>


// standard C++ libraries
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <cmath>


// tdogl classes
#include "Program.h"
#include "tdogl\Camera.h"

// constants
const glm::vec2 SCREEN_SIZE(900, 900);

// globals
GLFWwindow* gWindow = NULL;
tdogl::Program* gProgram = NULL;
GLuint gVAO = 0;
GLuint gVBO = 0;
GLuint eboID = 0;
GLuint ac_buffer = 0;
GLuint atomicCounterArray[1];
GLuint transformationMatrixBufferId;
GLuint gVAOtemp = 0;

// loads the vertex shader and fragment shader, and links them to make the global gProgram
static void LoadShaders() {
    std::vector<tdogl::Shader> shaders;
    shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath("vertex-shader.txt"), GL_VERTEX_SHADER));
    shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath("fragment-shader.txt"), GL_FRAGMENT_SHADER));
    gProgram = new tdogl::Program(shaders);
	//std::cout << gProgram << std::endl;
}


// loads a triangle into the VAO global
static void LoadTriangle() {
    // make and bind the VAO
    glGenVertexArrays(2, &gVAO);
    glBindVertexArray(gVAO);
    
    // make and bind the VBO
    glGenBuffers(1, &gVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gVBO);
    
    // Put the three triangle verticies into the VBO 
	// need update
    GLfloat vertexData[] = {
        //  X     Y     Z
         0.0f, 0.8f, 0.0f,
        -0.8f,-0.8f, 0.0f,
         0.8f,-0.8f, 0.0f,
    };
	float vertex[] = { 0.0f, 0.8f, 0.0f, -0.8f, -0.8f, 0.0f, 0.8f, -0.8f, 0.0f};
	GLfloat * vertexData2 = (GLfloat*)vertex;
	//std::cout << sizeof(vertexData2)<< std::endl;
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData2, GL_STATIC_DRAW);
    
    // connect the xyz to the "vert" attribute of the vertex shader
	int pos = glGetAttribLocation(gProgram->object(), "vert");
    glEnableVertexAttribArray(gProgram->attrib("vert"));
	std::cout << pos << std::endl;
	glVertexAttribPointer(gProgram->attrib("vert"), 3, GL_FLOAT, GL_FALSE, 0, NULL);
    
    // unbind the VBO and VAO
    //glBindBuffer(GL_ARRAY_BUFFER, 0);
    //glBindVertexArray(0);
	
	//need update
	GLuint indices[] = { 0, 1, 2 };
	glGenBuffers(1, &eboID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


	// bind the matrix
	// make and bind the VAO
	//glGenVertexArrays(1, &gVAOtemp);
	//glBindVertexArray(gVAOtemp);

	glGenBuffers(1, &transformationMatrixBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, transformationMatrixBufferId);

	float temp[16] = 
		{ 0.0, 0,0,0,
		0.5, 0, 0, 0,
		0, 0, 0, 0,
		0, 0, 0, 0 };
	//glm::mat4 fullTransform = glm::mat4(1.0);
	glm::mat4 fullTransform[] = 
	{
		glm::mat4(1.0),
		glm::mat4(-1.0)
	};
	
	//memcpy(glm::value_ptr(fullTransform),temp,sizeof(temp));
	std::cout << glm::to_string(fullTransform[0])<<" "<<glm::to_string(fullTransform[1]) << std::endl;
	//std::cout << fullTransform[0][0] << std::endl;
	//glEnableVertexAttribArray(gProgram->attrib("fullTransformMatrix"));
	pos = glGetAttribLocation(gProgram->object(), "fullTransformMatrix");
	std::cout << pos << std::endl;
	int pos1 = pos + 0;
	int pos2 = pos + 1;
	int pos3 = pos + 2;
	int pos4 = pos + 3;

	glBufferData(GL_ARRAY_BUFFER, sizeof(fullTransform), &fullTransform, GL_STATIC_DRAW);

	glEnableVertexAttribArray(pos1);
	glEnableVertexAttribArray(pos2);
	glEnableVertexAttribArray(pos3);
	glEnableVertexAttribArray(pos4);

	glVertexAttribPointer(pos1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (const GLvoid*)(sizeof(GLfloat)*0));
	glVertexAttribPointer(pos2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (const GLvoid*)(sizeof(GLfloat) * 4));
	glVertexAttribPointer(pos3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (const GLvoid*)(sizeof(GLfloat) * 8));
	glVertexAttribPointer(pos4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (const GLvoid*)(sizeof(GLfloat) * 12));

	glVertexAttribDivisor(pos1, 1);
	glVertexAttribDivisor(pos2, 1);
	glVertexAttribDivisor(pos3, 1);
	glVertexAttribDivisor(pos4, 1);


	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}


// draws a single frame
static void Render() {
    // clear everything
    glClearColor(0, 0, 0, 1); // black
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // bind the program (the shaders)
    glUseProgram(gProgram->object());
    
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ac_buffer);
	GLuint * ptr = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	//std::cout << ptr[0] << std::endl;
	//ptr[0] = 0;
	//memset(ptr, 0, sizeof(GLuint));
	//std::cout << ptr[0] << std::endl;
	atomicCounterArray[0] = 0;
	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ac_buffer);
	glBufferSubData(GL_ATOMIC_COUNTER_BUFFER,0,sizeof(GLuint),&atomicCounterArray);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

    // bind the VAO (the triangle)
      glBindVertexArray(gVAO);
	 // glBindVertexArray(gVAOtemp);
	//glBindVertexArray(gVBO);
	  // bind the fulltransform matrix
	
	glBindBuffer(GL_ARRAY_BUFFER, transformationMatrixBufferId);
    
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID);
	glIndexPointer(GL_UNSIGNED_INT, 0, 0);
    // draw the VAO
    //glDrawArrays(GL_TRIANGLES, 0, 3);
	//glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
	//glDrawElementsInstanced(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (const GLvoid*)(sizeof(GLint) * 0), 2);
	glDrawElementsInstancedBaseInstance(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0, 1,1);

    // unbind the VAO
    glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // unbind the program
    glUseProgram(0);
    
    // swap the display buffers (displays what was just drawn)
    glfwSwapBuffers(gWindow);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ac_buffer);
	/*GLuint * src = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	if (src)
	{
		memcpy((void*)atomicCounterArray, src, 4);
	}
	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

	unsigned int numPixel = atomicCounterArray[0];
	std::cout << numPixel<<std::endl;
	std::cout << src[0] << std::endl;*/
	GLuint userCounters[1];
	glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER,0,sizeof(GLuint),userCounters);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	std::cout << userCounters[0] << std::endl;
}

void OnError(int errorCode, const char* msg) {
    throw std::runtime_error(msg);
}

// the program starts here
void AppMain() {
    // initialise GLFW
    glfwSetErrorCallback(OnError);
    if(!glfwInit())
        throw std::runtime_error("glfwInit failed");
    
    // open a window with GLFW
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    gWindow = glfwCreateWindow((int)SCREEN_SIZE.x, (int)SCREEN_SIZE.y, "OpenGL Tutorial", NULL, NULL);
    if(!gWindow)
        throw std::runtime_error("glfwCreateWindow failed. Can your hardware handle OpenGL 3.2?");

    // GLFW settings
    glfwMakeContextCurrent(gWindow);
    
    // initialise GLEW
    glewExperimental = GL_TRUE; //stops glew crashing on OSX :-/
    if(glewInit() != GLEW_OK)
        throw std::runtime_error("glewInit failed");

    // print out some info about the graphics drivers
    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;

    // make sure OpenGL version 4.5 API is available
    if(!GLEW_VERSION_4_5)
        throw std::runtime_error("OpenGL 4.5 API is not available.");

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

    // load vertex and fragment shaders into opengl
    LoadShaders();

    // create buffer and fill it with the points of the triangle
    LoadTriangle();

	atomicCounterArray[0] = 0;
	glGenBuffers(1, &ac_buffer);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ac_buffer);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	//GLuint * ptr = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	//memset(ptr, 0, sizeof(GLuint));
	//std::cout << ptr[0] << std::endl;
	//glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, ac_buffer);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	Render();
    // run while the window is open
   //while(!glfwWindowShouldClose(gWindow)){
   //     // process pending events
   //     glfwPollEvents();
   //     
   //     // draw one frame
   //     Render();

   // }

	

    // clean up and exit
    glfwTerminate();
}


int main(int argc, char *argv[]) {
    try {
        AppMain();
    } catch (const std::exception& e){
        std::cerr << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
