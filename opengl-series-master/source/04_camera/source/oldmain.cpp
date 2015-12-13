#include "platform.hpp"

// third-party libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/transform.hpp>

// standard C++ libraries
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <functional>
#include <algorithm>
#include <memory>
#include <limits>
#include <iomanip>
#include <ctime>

// tdogl classes
#include "tdogl/Program.h"
#include "tdogl/Texture.h"
#include "tdogl/Camera.h"
#define random(x) (rand()%x)

using std::sort;
#define max(a,b) ((a) > (b) ? (a) : (b))

#define cf(p, c, v) (((p-c+2*v) > iCacheSize) ? (0) : (p-c))

//#define INUMVERTICES 6675
//#define INUMFACES 12610
//#define INUMFRAMES 30
#define INUMVIEWS 162
#define CANVASXNUMS 13
#define CANVASYNUMS 13
#define INUMCLUSTERS 5
#define CANVASHEIGHT 50
#define CANVASWIDTH 50


// constants
const glm::vec2 SCREEN_SIZE(CANVASWIDTH*CANVASXNUMS, CANVASHEIGHT*CANVASYNUMS);

// globals
bool offScreen = false;
GLFWwindow* gWindow = NULL;
tdogl::Camera gCamera;
tdogl::Program* gProgram = NULL;
GLuint gVAO = 0;
GLuint gVBO = 0;
GLuint eboID = 0;
GLuint ac_buffer = 0;
GLuint pixel_buffer = 0;
GLuint atomicCounterArray[1];
GLuint transformationMatrixBufferId;
GLuint gColor;
GLuint rboDepth;
GLuint fbo;

class Vector
{
public:
	float v[3];
	Vector() {}
	Vector(float a, float b, float c) { v[0] = a; v[1] = b; v[2] = c; }
	Vector(float *a) { v[0] = a[0]; v[1] = a[1]; v[2] = a[2]; }
	void operator+=(const Vector a) { v[0] += a.v[0]; v[1] += a.v[1]; v[2] += a.v[2]; }
	void operator-=(const Vector a) { v[0] -= a.v[0]; v[1] -= a.v[1]; v[2] -= a.v[2]; }
	Vector operator+(const Vector a) { return Vector(v[0] + a.v[0], v[1] + a.v[1], v[2] + a.v[2]); }
	Vector operator-(const Vector a) { return Vector(v[0] - a.v[0], v[1] - a.v[1], v[2] - a.v[2]); }
	Vector operator/(const float n) { return Vector(v[0] / n, v[1] / n, v[2] / n); }
	void operator/=(const int n) { v[0] /= (float)n; v[1] /= (float)n; v[2] /= (float)n; }
	void operator/=(const float n) { v[0] /= n; v[1] /= n; v[2] /= n; }
	Vector operator*(const float a) { return Vector(v[0] * a, v[1] * a, v[2] * a); }
	Vector operator*(const Vector a) { return Vector(v[0] * a.v[0], v[1] * a.v[1], v[2] * a.v[2]); }
	Vector operator/(const Vector a) { return Vector(v[0] / a.v[0], v[1] / a.v[1], v[2] / a.v[2]); }
	float length() { float w = v[0] * v[0] + v[1] * v[1] + v[2] * v[2]; return w > 0.f ? sqrtf(w) : 0.f; }
	void normalize() { float w = v[0] * v[0] + v[1] * v[1] + v[2] * v[2]; if (w > 0.f) *this /= sqrtf(w); }
};

static float dot(const Vector a, const Vector b)
{
	return (a.v[0] * b.v[0] + a.v[1] * b.v[1] + a.v[2] * b.v[2]);
}

static Vector cross(const Vector a, const Vector b)
{
	return Vector(a.v[1] * b.v[2] - a.v[2] * b.v[1], a.v[2] * b.v[0] - a.v[0] * b.v[2], a.v[0] * b.v[1] - a.v[1] * b.v[0]);
}
static float dist(const Vector a, const Vector b)
{
	Vector c = Vector(a);
	return (c - b).length();
}

// structure used to sort patches
class patchSort
{
public:
	float dist;// distance
	int id;//index
};
class clusterAssign
{
public:
	int frameId;
	int viewId;
};
bool sortfunc(const patchSort &a, const patchSort &b)
{
	return a.dist < b.dist;
}
// sort functions
inline int min(const int a, const int b)
{
	return a < b ? a : b;
}
//function that computes size of scratch memory
int FanVertScratchSize(int iNumVertices, int iNumFaces)
{
	return (iNumFaces * 22 + iNumVertices + 3) * sizeof(int);
}


// loads the vertex shader and fragment shader, and links them to make the global gProgram
static void LoadShaders() {
	std::vector<tdogl::Shader> shaders;
	shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath("vertex-shader.txt"), GL_VERTEX_SHADER));
	shaders.push_back(tdogl::Shader::shaderFromFile(ResourcePath("fragment-shader.txt"), GL_FRAGMENT_SHADER));
	gProgram = new tdogl::Program(shaders);
	//std::cout << gProgram << std::endl;
}


// loads a triangle into the VAO global
static void LoadTriangle(float * pfVertexPositionsIn, float * pfCameraPosiitons, int * piIndexBufferIn, int numVertices, int numFaces)
{
	// make and bind the VAO
	glGenVertexArrays(2, &gVAO);
	glBindVertexArray(gVAO);

	// make and bind the VBO
	glGenBuffers(1, &gVBO);
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);

	GLfloat * vertexData2 = (GLfloat *)pfVertexPositionsIn;
	glBufferData(GL_ARRAY_BUFFER, numVertices*3*4, vertexData2, GL_STATIC_DRAW);
	// connect the xyz to the "vert" attribute of the vertex shader
	glEnableVertexAttribArray(gProgram->attrib("vert"));
	glVertexAttribPointer(gProgram->attrib("vert"), 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glGenBuffers(1, &transformationMatrixBufferId);
	glBindBuffer(GL_ARRAY_BUFFER, transformationMatrixBufferId);

	// setup gCamera
	gCamera.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
	//std::cout << "X: "<<SCREEN_SIZE.x << " Y " << SCREEN_SIZE.y << std::endl;
	gCamera.setViewportAspectRatio(SCREEN_SIZE.x / SCREEN_SIZE.y);
	gCamera.setFieldOfView(40.0f);
	gCamera.setNearAndFarPlanes(1.0f, 2000.0f);

	gCamera.setDirection(glm::vec3(-50, -50, -200));

	float translatePos[CANVASXNUMS];
	for (int i = 0; i < CANVASXNUMS; i++){
		translatePos[i] = -1 + 2.0 / (CANVASXNUMS * 2) + (2.0 / CANVASXNUMS)*i;
	}
	glm::mat4 fullTransform[INUMVIEWS]; 
	int i,cameraId,canvasX,canvasY;
	double result, x = 1;
	for (cameraId = 0; cameraId < INUMVIEWS; cameraId++)
	{
		canvasX = cameraId%CANVASXNUMS; //width
		canvasY = cameraId / CANVASXNUMS; //height
		gCamera.setPosition(glm::vec3(pfCameraPosiitons[cameraId * 3], pfCameraPosiitons[cameraId * 3 + 1], pfCameraPosiitons[cameraId * 3 + 2]));
		gCamera.lookAt(glm::vec3(0.0f, 0.0f, 0.0f));
		fullTransform[cameraId] = glm::translate(glm::mat4(1.0), glm::vec3(translatePos[canvasX], translatePos[canvasY], 0.0f))*glm::scale(glm::mat4(1.0), glm::vec3(1.0/CANVASXNUMS, 1.0 / CANVASYNUMS, 1.0))*gCamera.matrix();
	}

	int pos = glGetAttribLocation(gProgram->object(), "fullTransformMatrix");
	int pos1 = pos + 0;
	int pos2 = pos + 1;
	int pos3 = pos + 2;
	int pos4 = pos + 3;

	glBufferData(GL_ARRAY_BUFFER, sizeof(fullTransform), &fullTransform, GL_STATIC_DRAW);
	glEnableVertexAttribArray(pos1);
	glEnableVertexAttribArray(pos2);
	glEnableVertexAttribArray(pos3);
	glEnableVertexAttribArray(pos4);

	glVertexAttribPointer(pos1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (const GLvoid*)(sizeof(GLfloat) * 0));
	glVertexAttribPointer(pos2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (const GLvoid*)(sizeof(GLfloat) * 4));
	glVertexAttribPointer(pos3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (const GLvoid*)(sizeof(GLfloat) * 8));
	glVertexAttribPointer(pos4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (const GLvoid*)(sizeof(GLfloat) * 12));

	glVertexAttribDivisor(pos1, 1);
	glVertexAttribDivisor(pos2, 1);
	glVertexAttribDivisor(pos3, 1);
	glVertexAttribDivisor(pos4, 1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	//need update
	GLuint * indices = (GLuint *)piIndexBufferIn;
	glGenBuffers(1, &eboID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, numFaces*3*4, indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	//// setup gCamera
	//gCamera.setPosition(glm::vec3(50, 50, 200));
	//gCamera.setViewportAspectRatio(SCREEN_SIZE.x / SCREEN_SIZE.y);
	//gCamera.setFieldOfView(60.0f);
	//gCamera.setNearAndFarPlanes(1.0f, 2000.0f);
	//gCamera.setDirection(glm::vec3(-50, -50, -200));
}


void overdrawRatio(){
	// read pixels
	int i, j;
	bool bMalloc = false;
	int * piScratch = NULL;
	if (piScratch == NULL)
	{
		int iScratchSize = INUMVIEWS*CANVASHEIGHT*CANVASWIDTH*sizeof(int);
		piScratch = (int *)malloc(iScratchSize);
		memset(piScratch, 0, iScratchSize);
		bMalloc = true;
	}
	int *piScratchBase = piScratch;
	unsigned char * pixel = (unsigned char *)piScratch;
	piScratch += INUMVIEWS*CANVASHEIGHT*CANVASWIDTH;

	if (offScreen)
		//glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	else
		glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, CANVASWIDTH * CANVASXNUMS, CANVASHEIGHT*CANVASYNUMS, GL_RED, GL_UNSIGNED_BYTE, pixel);
	//glReadBuffer(GL_NONE);
	//glBindBuffer(GL_READ_FRAMEBUFFER, 0);
	int drawnPixel,showedPixel,cameraId;
	float avgRatios[INUMVIEWS];
	//getchar();
	int x, y;
	for (cameraId = 0; cameraId < INUMVIEWS; cameraId++)
	{
		x = cameraId%CANVASXNUMS; //width
		y = cameraId / CANVASXNUMS; //height

		drawnPixel = 0; showedPixel = 0;
		for (int i = CANVASHEIGHT * y; i < CANVASHEIGHT*(y + 1); i++) //height
		{
			for (int j = CANVASWIDTH * x; j < CANVASWIDTH*(x + 1); j++)//width
			{
				if ((int)pixel[i * CANVASWIDTH * CANVASXNUMS + j] > 0)
				{
					drawnPixel += round((float)pixel[i * CANVASWIDTH * CANVASXNUMS + j] / 51.0f);
					showedPixel++;
				}
			}
		}
		avgRatios[cameraId] = (float)drawnPixel / (float)showedPixel;
		//std::cout << "drawn pixel numbers " << drawnPixel << std::endl;
		//std::cout << "showed pixel numbers " << showedPixel << std::endl;
		std::cout << "averageRatio" << avgRatios[cameraId] << std::endl;
	}
	if (piScratch - piScratchBase > 0)
	{
		memset(piScratchBase, 0, (piScratch - piScratchBase) * sizeof(int));
	}
	if (bMalloc)
	{
		free(piScratchBase);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

static void Render(GLuint baseInstance,int numFaces) {
	// clear everything
	if (offScreen)
		//glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glClearColor(0, 0, 0, 1); // black
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	
	// bind the program (the shaders)
	glUseProgram(gProgram->object());

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ac_buffer);
	GLuint * ptr = (GLuint*)glMapBufferRange(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_UNSYNCHRONIZED_BIT);
	atomicCounterArray[0] = 0;
	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ac_buffer);
	glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &atomicCounterArray);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	// bind the VAO (the triangle)
	glBindVertexArray(gVAO);
	//glBindVertexArray(gVBO);
	// bind the fulltransform matrix
	glBindBuffer(GL_ARRAY_BUFFER, transformationMatrixBufferId);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID);
	glIndexPointer(GL_UNSIGNED_INT, 0, 0);

	// draw the VAO

	glDrawElementsInstancedBaseInstance(GL_TRIANGLES, numFaces * 3, GL_UNSIGNED_INT, 0, INUMVIEWS, baseInstance);

	// unbind the VAO
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// unbind the program
	glUseProgram(0);

	// swap the display buffers (displays what was just drawn)
	glfwSwapBuffers(gWindow);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ac_buffer);
	GLuint userCounters[1];
	glGetBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), userCounters);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);
	//std::cout << userCounters[0] << std::endl;
	overdrawRatio();
	//glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OnError(int errorCode, const char* msg) {
	throw std::runtime_error(msg);
}

//function that implements the vcache optimization
float FanVertLinSort(int *piIndexBufferIn, int *piIndexBufferOut, int iNumFaces, int *piScratch, int iCacheSize, int *piClustersOut, int &iNumClusters)
{
	int i = 0;
	int iNumFaces3 = iNumFaces * 3;
	int sum = 0;
	int lowi = 0;
	int j = 0;
	int next = -1;
	int id;

	iNumClusters = 1;
	if (piClustersOut)
		piClustersOut[0] = 0;

	//set array pointers from scratch buffer
	int *piEmitted = piScratch;
	int *piFanList = piEmitted + iNumFaces;
	int *piTriList = piFanList + iNumFaces3;

	int *piStartList = piTriList + iNumFaces3;
	int *piStartListTail = piStartList;

	int *piRemValence = piStartList + iNumFaces3;

	int *piCachePos = piRemValence + iNumFaces3;

	int *piFanPos = piCachePos + iNumFaces3;


	int iCurCachePos = 1 + iCacheSize; //so that cache position of 0 is out of cache
	int iCurCachePosFan;

	int nv = 0;
	for (i = 0; i < iNumFaces3; i++)
	{
		register int ind = piIndexBufferIn[i];
		piFanPos[ind]++;
		if (piFanPos[ind] == 1)
			piFanList[nv++] = ind;
	}
	for (i = 0; i < nv; i++)
	{
		int x = piFanPos[piFanList[i]];
		piRemValence[sum] = x;
		sum = (piFanPos[piFanList[i]] += sum);
	}
	for (i = 0; i < iNumFaces3; i++)
	{
		piTriList[--piFanPos[piIndexBufferIn[i]]] = i;
	}

	i = 0;

	//loop through extracting the triangles for the optimized buffer
	while (lowi < iNumFaces3)
	{
		int bestemitted = -INT_MAX;
		int tribest = -1;

		//set current vertex id
		id = piIndexBufferIn[piTriList[i]];

		next = -1;

		iCurCachePosFan = iCurCachePos;

		//loop through extracting all faces from a vertex fan that were not previously written
		while (i < iNumFaces3 && piIndexBufferIn[piTriList[i]] == id)
		{
			//get triangle id, and starting index of that triangle in original buffer (tri3)
			int tri = piTriList[i] / 3;
			int tri3 = tri * 3;

			if (++piEmitted[tri] == 1)
			{
				int *pin = &piIndexBufferIn[tri3];
				int ord = 0;
				for (int ii = 0; ii < 3; ii++, pin++)
				{
					piIndexBufferOut[j++] = *pin;

					int x = piFanPos[*pin];

					int t = iCurCachePos - piCachePos[x] > iCacheSize;
					if (t)
					{
						piCachePos[x] = iCurCachePos++;
					}

					int v = --piRemValence[x];
					if (v > 0 && *pin != id)
					{
						if (t)
						{
							*(piStartListTail++) = *pin;
						}
						int f = cf(iCurCachePosFan, piCachePos[x], v);
						if (f > bestemitted)
						{
							bestemitted = f;
							next = *pin;
						}
					}
				}
			}

			//increment counters into piTriList
			if (i == lowi)
				lowi++;
			i++;
		}

		//reset its fan position, so that it doesn't get processed twice
		piFanPos[id] = 0;

		if (next == -1)
		{
			int notfound = 1;

			while (piStartListTail > piStartList)
			{
				i = piFanPos[*(--piStartListTail)];
				if (i > 0)
				{
					notfound = 0;
					break;
				}
			}

			if (notfound)
			{
				while (lowi < iNumFaces3 && !piFanPos[piIndexBufferIn[piTriList[lowi]]])
				{
					lowi++;
				}
				i = lowi;
			}

			//overdraw output
			if (piClustersOut && piClustersOut[iNumClusters - 1] != j / 3 && iCurCachePos - piCachePos[i] > iCacheSize * 2)
			{
				piClustersOut[iNumClusters++] = j / 3;
			}
		}
		//if we have a neighboring id to fan around, set it as current
		else
		{
			i = piFanPos[next];
		}
	}

	//clear temp array (only the elements used)
	for (i = 0; i < nv; i++)
	{
		piFanPos[piFanList[i]] = 0;
	}
	memset(piScratch, 0, (iNumFaces * 16) * sizeof(int));

	if (piClustersOut && piClustersOut[iNumClusters - 1] == iNumFaces)
		iNumClusters--;

	return (iCurCachePos - iCacheSize - 1) / (float)iNumFaces;
}

//function that implements the linear cutting
int OverdrawOrderPartition(int *piIndexBufferIn,
	int iNumFaces,
	int *piClustersIn, //should have piClustersIn[iNumClusters] == iNumFaces
	int iNumClustersIn,
	int iCacheSize,
	float lambda,
	int *piClustersOut,
	int *piScratch)
{

	int *piScratchBase = piScratch;
	int *piCache = piScratch;
	piScratch += iCacheSize;

	int i;
	int j = 0;

	for (i = 0; i < iNumClustersIn; i++)
	{
		piClustersOut[j++] = piClustersIn[i];
		int *p = piIndexBufferIn;
		int n = piClustersIn[i + 1] - piClustersIn[i];
		int start = piClustersIn[i];
		int head = 0;
		int m, k;
		int iProc = 0;
		for (m = 0; m < iCacheSize; m++)
		{
			piCache[m] = -1;
		}
		for (k = 0; k < n; k++, p += 3)
		{
			for (m = 0; m < 3; m++)
			{
				int found = 0;
				for (int q = 0; q < iCacheSize; q++)
				{
					if (p[m] == piCache[q])
					{
						found = 1;
						break;
					}
				}

				if (!found)
				{
					iProc++;
					piCache[head] = p[m];
					head = (head + 1);
					if (head == iCacheSize)
						head = 0;
				}
			}

			float fEstProc = iProc / (float)(k + 1);
			if (k > 0 && lambda > fEstProc)
			{
				start += k;
				piClustersOut[j++] = start;
				n -= k;
				k = -1;
				p -= 3;
				iProc = 0;
				for (m = 0; m < iCacheSize; m++)
				{
					piCache[m] = -1;
				}
			}
		}
	}
	piClustersOut[j] = iNumFaces;

	memset(piScratchBase, 0, (piScratch - piScratchBase) * sizeof(int));

	return j;
}

// function implements the linear sorting
// output the linearFace(piIndexBufferOut), iNumColusters, piClustersOut (set patches)
void FanVertCluster(float *pfVertexPositionsIn,   //vertex buffer positions, 3 floats per vertex
	int *piIndexBufferIn,         //index buffer positions, 3 ints per vertex
	int *piIndexBufferOut,        //updated index buffer (the output of the algorithm)
	int iNumVertices,             //# of vertices in the vertex buffer
	int iNumFaces,                //# of faces in the index buffer
	int iCacheSize,               //hardware cache size
	float alpha,                  //constant parameter to compute lambda term from algorithm 
	int *piScratch = NULL,        //optional temp buffer for computations; its size in bytes should be:
	int *piClustersOut = NULL,    //optional buffer for the output cluster position (in faces) of each cluster
	int *piNumClustersOut = NULL) //the number of putput clusters
{
	bool bMalloc = false;
	if (piScratch == NULL)
	{
		int iScratchSize = FanVertScratchSize(iNumVertices, iNumFaces);
		piScratch = (int *)malloc(iScratchSize);
		memset(piScratch, 0, iScratchSize);
		bMalloc = true;
	}
	int *piScratchBase = piScratch;

	int *piIndexBufferTmp = piScratch;
	piScratch += iNumFaces * 3;

	int *piClustersIn = piScratch;
	piScratch += iNumFaces + 1;

	int *piClustersTmp = piScratch;
	piScratch += iNumFaces + 1;

	int *piClusterRemap = piScratch;
	piScratch += iNumFaces + 1;


	int iNumClusters;
	float lambda = FanVertLinSort(piIndexBufferIn, piIndexBufferTmp, iNumFaces,
		piScratch, iCacheSize, piClustersIn, iNumClusters);

	lambda = alpha;

	int iNumClustersOut = OverdrawOrderPartition(piIndexBufferTmp, iNumFaces,
		piClustersIn, iNumClusters, iCacheSize, lambda, piClustersTmp, piScratch);

	if (piNumClustersOut != NULL){
		*piNumClustersOut = iNumClustersOut;
	}
	if (piClustersTmp[iNumClustersOut] != iNumFaces)
	{
		printf("DOH\n");
	}

	for (int i = 0; i < iNumFaces * 3; i++)
	{
		piIndexBufferOut[i] = piIndexBufferTmp[i];
	}
	for (int i = 0; i < iNumClustersOut + 1; i++)
	{
		piClustersOut[i] = piClustersTmp[i];
	}
	if (piScratch - piScratchBase > 0)
		memset(piScratchBase, 0, (piScratch - piScratchBase) * sizeof(int)); //clear memory from tmp
	if (bMalloc)
	{
		free(piScratchBase);
	}
}

//function that implements getting patches positions
void pvPatchesPostions(int *piIndexBufferIn,
	int iNumFaces,
	float *pfVertexPositionsIn,
	int iNumVertices,
	int *piClustersIn,
	int iNumClusters,
	Vector * pvPatchesPositions,
	int *piScratch
	)
{
	int i, j;
	int c = 0, cstart = 0;
	int cnext = piClustersIn[1];
	int *p = piIndexBufferIn;
	Vector *pvVertexPositionsIn = (Vector *)pfVertexPositionsIn;
	Vector vMeshPositions = Vector(0, 0, 0);
	float fMArea = 0.f;

	bool bMalloc = false;
	if (piScratch == NULL)
	{
		int iScratchSize = FanVertScratchSize(iNumVertices, iNumFaces);
		piScratch = (int *)malloc(iScratchSize);
		memset(piScratch, 0, iScratchSize);
		bMalloc = true;
	}
	int *piScratchBase = piScratch;
	Vector *pvClusterPositions = (Vector *)piScratch;
	piScratch += iNumClusters * 3;

	Vector *pvClusterNormals = (Vector *)piScratch;
	piScratch += iNumClusters * 3;

	float *pfClusterAreas = (float *)piScratch;
	piScratch += iNumClusters;

	for (i = 0; i < iNumClusters; i++)
	{
		pvClusterPositions[i] = Vector(0, 0, 0);
		pvClusterNormals[i] = Vector(0, 0, 0);
	}
	float fCArea = 0.f;

	for (i = 0; i <= iNumFaces; i++)
	{
		if (i == cnext)
		{
			pfClusterAreas[c] = fCArea;
			pvClusterPositions[c] /= fCArea * 3.f;
			pvClusterNormals[c].normalize();
			c++;
			if (c == iNumClusters)
				break;
			cstart = i;
			cnext = piClustersIn[c + 1];
			fCArea = 0.f;
		}

		Vector vNormal = cross(pvVertexPositionsIn[p[2]] - pvVertexPositionsIn[p[0]],
			pvVertexPositionsIn[p[1]] - pvVertexPositionsIn[p[0]]);
		float fArea = vNormal.length();
		if (fArea > 0.f)
		{
			vNormal /= fArea;
		}
		else
		{
			fArea = 0.f;
			vNormal = Vector(0, 0, 0);
		}

		for (j = 0; j < 3; j++)
		{
			Vector *vp = (Vector *)&pfVertexPositionsIn[(*p) * 3];
			vMeshPositions += *vp * fArea;
			pvClusterPositions[c] += *vp * fArea;
			p++;
		}

		pvClusterNormals[c] += vNormal;

		fMArea += fArea;
		fCArea += fArea;
	}
	vMeshPositions /= fMArea * 3.f;
	for (int i = 0; i < iNumClusters; i++){
		pvPatchesPositions[i] = Vector(pvClusterPositions[i].v[0], pvClusterPositions[i].v[1], pvClusterPositions[i].v[2]);
	}
	
	if (piScratch - piScratchBase > 0)
	{
		memset(piScratchBase, 0, (piScratch - piScratchBase) * sizeof(int));
	}
	if (bMalloc)
	{
		free(piScratchBase);
	}
}

// function that implements rank faces from near to far
void depthSortPatch(Vector viewpoint, Vector * pvAvgPatchesPositions, int numPatches, int *piIndexBufferIn, int *piClustersIn, int * piIndexBufferTmp)
{
	//std::cout << "linear sort face"<<piIndexBufferIn[0] << " "<<piIndexBufferIn[1] << " "<< piIndexBufferIn[2] << std::endl;
	//std::cout << "linear sort face" << piIndexBufferIn[INUMFACES*3-3] << " " << piIndexBufferIn[INUMFACES*3-2] << " " << piIndexBufferIn[INUMFACES*3-1] << std::endl;
	//std::cout << " patch Id" << piClustersIn[0] << " "<< piClustersIn[numPatches] << std::endl;
	int i, j;
	int * piScratch = NULL;
	bool bMalloc = false;
	if (piScratch == NULL)
	{
		int iScratchSize = numPatches * 2 * sizeof(int);
		piScratch = (int *)malloc(iScratchSize);
		memset(piScratch, 0, iScratchSize);
		bMalloc = true;
	}
	int *piScratchBase = piScratch;
	patchSort *viewToPatch = (patchSort *)piScratch;
	piScratch += numPatches * 2;

	for (i = 0; i < numPatches; i++)
	{
		viewToPatch[i].id = i;
		viewToPatch[i].dist = dist(viewpoint, pvAvgPatchesPositions[i]);
	}
	std::sort(viewToPatch, viewToPatch + numPatches, sortfunc);
	//std::cout << viewToPatch[0].dist << " " << viewToPatch[1].dist << " " << viewToPatch[2].dist << std::endl;

	int jj = 0;
	for (i = 0; i < numPatches; i++)
	{
		for (j = piClustersIn[viewToPatch[i].id] * 3; j < piClustersIn[viewToPatch[i].id + 1] * 3; j++)
		{
			piIndexBufferTmp[jj++] = piIndexBufferIn[j];
		}
	}
	//std::cout << " means[0][0] " << piIndexBufferTmp[0] << " " << piIndexBufferTmp[1] << " " << piIndexBufferTmp[2] << std::endl;
	//std::cout << "means[0][inumFaces] " << piIndexBufferTmp[INUMFACES * 3 - 3] << " " << piIndexBufferTmp[INUMFACES * 3 - 2] << " " << piIndexBufferTmp[INUMFACES * 3 - 1] << std::endl;

	if (piScratch - piScratchBase > 0)
	{
		memset(piScratchBase, 0, (piScratch - piScratchBase) * sizeof(int));
	}
	if (bMalloc)
	{
		free(piScratchBase);
	}
}
// function that implements the initializition
void initMeans(int ** means, Vector ** pvFramesPatchesPositions, int * piIndexBufferIn, int * piClustersIn, int numFrames, int numClusters, int numPatches, int numFaces, int * pickIds, float * pfCameraPositions, int * piScratch)
{
	int i, j;
	bool bMalloc = false;
	if (piScratch == NULL)
	{
		int iScratchSize = numPatches * 3 * sizeof(int) + numFaces * 3 * sizeof(int);
		piScratch = (int *)malloc(iScratchSize);
		memset(piScratch, 0, iScratchSize);
		bMalloc = true;
	}
	int *piScratchBase = piScratch;
	Vector *pvAvgPatchesPositions = (Vector *)piScratch;
	piScratch += numPatches * 3;
	int *piIndexBufferTmp = piScratch;
	piScratch += numFaces * 3;

	for (i = 0; i < numFrames; i++)
	{
		for (j = 0; j < numPatches; j++)
		{
			pvAvgPatchesPositions[j] += pvFramesPatchesPositions[i][j];
		}
	}
	for (j = 0; j < numPatches; j++)
	{
		pvAvgPatchesPositions[j] /= numFrames;
	}

	for (i = 0; i < numClusters; i++)
	{
		Vector viewpoint = Vector(pfCameraPositions[pickIds[i] * 3], pfCameraPositions[pickIds[i] * 3 + 1], pfCameraPositions[pickIds[i] * 3 + 2]);
		depthSortPatch(viewpoint, pvAvgPatchesPositions, numPatches, piIndexBufferIn, piClustersIn, means[i]);
	}
	if (piScratch - piScratchBase > 0)
	{
		memset(piScratchBase, 0, (piScratch - piScratchBase) * sizeof(int));
	}
	if (bMalloc)
	{
		free(piScratchBase);
	}
}

//function that implements converting assignments to clusterAssignments

// function that implements the assignments
void makeAssignment(float ** assignments, float ** minRatios, int numFrames, int numViews, int numClusters, int *piScratch)
{
	//float assignments[INUMFRAMES][INUMVIEWS], float minRatios[INUMFRAMES][INUMVIEWS]
	int x, y,z;
	bool bMalloc = false;
	if (piScratch == NULL)
	{
		int iScratchSize = numFrames*numClusters*numViews * sizeof(int);
		piScratch = (int *)malloc(iScratchSize);
		memset(piScratch, 0, iScratchSize);
		bMalloc = true;
	}
	int *piScratchBase = piScratch;
	float * ratios = (float * ) piScratch;
	piScratch += numFrames*numClusters*numViews;
	
	//float ratios[INUMFRAMES][INUMCLUSTERS][INUMVIEWS];
	// numframes depth, numClusters width, numViews height

	// test the sort
	// makeup the part for compute ovr in opengl

	// 3 dimension array 

	patchSort tempRatio[INUMCLUSTERS];
	for ( x= 0; x < numFrames; x++) 
	{
		for (z = 0; z < numViews; z++)
		{
			for (int y = 0; y < numClusters; y++)
			{
				tempRatio[y].id = y;
				//tempRatio[k].id = ratios[i][k][j];
				tempRatio[y].id = ratios[x + numClusters * (y + numFrames * z)];
			}
			std::sort(tempRatio, tempRatio + numClusters, sortfunc);
			minRatios[x][z] = tempRatio[0].dist;
			assignments[x][z] = tempRatio[0].id;
		}
	}

	if (piScratch - piScratchBase > 0)
	{
		memset(piScratchBase, 0, (piScratch - piScratchBase) * sizeof(int));
	}
	if (bMalloc)
	{
		free(piScratchBase);
	}
}

float newClusterRatio()
{
	return 0.0;
}
// moveClusterMean
bool moveClusterMean(int *clusterMean, int clusterId, int* piIndexBufferIn, int * piClustersIn, Vector ** pvFramesPatchesPositions, Vector * pvCameraPosiitons, int ** assignments, float ** minRatios, int numPatches, int numViews, int numFrames,int numFaces, int *piScratch)
{
	int i, j;
	bool bMalloc = false;
	bool moved = false;
	if (piScratch == NULL)
	{
		int iScratchSize = (numFrames*numViews * 2 + numPatches * 2+numFaces*3)* sizeof(int);
		piScratch = (int *)malloc(iScratchSize);
		memset(piScratch, 0, iScratchSize);
		bMalloc = true;
	}
	int *piScratchBase = piScratch;
	clusterAssign * cluster = (clusterAssign*)piScratch;
	piScratch += 2 * numFrames*numViews;
	patchSort * viewToPatch = (patchSort *)piScratch;
	piScratch += numPatches * 2;
	int * newMean = piScratch;
	piScratch += numFaces * 3;

	int count = 0; float avgRatio = 0;
	for (i = 0; i < numFrames; i++)
	{
		for (j = 0; j < numViews; j++)
		{
			if (assignments[i][j] == clusterId)
			{
				
				cluster[count].frameId = i;
				cluster[count].viewId = j;
				avgRatio += minRatios[i][j];
				count++;
			}
		}
	}
	avgRatio /= count;

	int frameId, viewId;
	for (i = 0; i < numPatches; i++)
	{
		viewToPatch[i].id = i;
	}
	for (i = 0; i < count; i++)
	{
		frameId = cluster[i].frameId;
		viewId = cluster[i].viewId;
		for (j = 0; j < numPatches; j++)
		{
			viewToPatch[j].dist += dist(pvCameraPosiitons[viewId], pvFramesPatchesPositions[frameId][j]);
		}
	}
	std::sort(viewToPatch, viewToPatch + numPatches, sortfunc);
	//if (clusterId == 0)
	//{
	//	viewToPatch[417].id = 431;
	//	viewToPatch[418].id = 346;
	//}
	int jj = 0;
	for (i = 0; i < numPatches; i++)
	{
		for (j = piClustersIn[viewToPatch[i].id] * 3; j < piClustersIn[viewToPatch[i].id + 1] * 3; j++)
		{
			newMean[jj++] = piIndexBufferIn[j];
		}
	}
	float newRatio = newClusterRatio();
	if (newRatio < avgRatio)
	{
		moved = true;
		std::cout << "new Ratio is less" << std::endl;
		// copy the new mean to old Mean
		memcpy(clusterMean, newMean,numFaces*3* sizeof(newMean));
	}
	else{
		std::cout << "old ratio is less" << std::endl;
	}

	if (piScratch - piScratchBase > 0)
	{
		memset(piScratchBase, 0, (piScratch - piScratchBase) * sizeof(int));
	}
	if (bMalloc)
	{
		free(piScratchBase);
	}

	return moved;
}
// moveMeans
bool moveMeans(int ** means, int* piIndexBufferIn, int * piClustersIn, Vector  ** pvFramesPatchesPositions, Vector * pvCameraPositions, int ** assignments, float ** minRatios, int numClusters, int numPatches, int numViews, int numFrames,int numFaces, int *piScratch)
{
	int i, j, clusterId;
	bool moved = false;
	bool clusterMoved;
	for (i = 0; i < 1; i++)
	{
		clusterId = i;
		clusterMoved = moveClusterMean(means[clusterId], clusterId, piIndexBufferIn, piClustersIn, pvFramesPatchesPositions, pvCameraPositions, assignments, minRatios, numPatches, numViews, numFrames, numFaces,piScratch);
		if (clusterMoved == true)
		{
			moved == true;
		}
	}
	return moved;

}

// the program starts here
void AppMain(float ** pfVertexPositionsIn,float * pfCameraPosiitons, int ** piIndexBufferIn, int numVertices, int numFaces)
{
	// initialise GLFW
	glfwSetErrorCallback(OnError);
	glfwInit();
	if (!glfwInit())
		throw std::runtime_error("glfwInit failed");

	// open a window with GLFW
	if (offScreen)
	{
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
		gWindow = glfwCreateWindow(CANVASHEIGHT, CANVASHEIGHT, "", NULL, NULL);
		glfwHideWindow(gWindow);
	}
	else{
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		gWindow = glfwCreateWindow(CANVASWIDTH*CANVASXNUMS, CANVASHEIGHT*CANVASYNUMS, "openglTutrials", NULL, NULL);
	}
	
	if (!gWindow)
		throw std::runtime_error("glfwCreateWindow failed. Can your hardware handle OpenGL 3.2?");
	glfwMakeContextCurrent(gWindow);
	// initialise GLEW
	glewExperimental = GL_TRUE; //stops glew crashing on OSX :-/
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		fprintf(stderr,"ERROR: %s\n",glewGetErrorString(err));
		//throw std::runtime_error("glewInit failed");
	}
		
	glGenBuffers(1, &pixel_buffer);
	
	if (offScreen)
	{
		// Framebuffer
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		// color renderbuffer
		glGenRenderbuffers(1, &gColor);
		glBindRenderbuffer(GL_RENDERBUFFER, gColor);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RED, CANVASXNUMS*CANVASWIDTH, CANVASHEIGHT*CANVASYNUMS);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, gColor);

		glGenRenderbuffers(1, &rboDepth);
		glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, CANVASXNUMS*CANVASWIDTH, CANVASYNUMS*CANVASHEIGHT);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "framebuffer error:" << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	glReadBuffer(GL_COLOR_ATTACHMENT0);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE,GL_ONE);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
	// load vertex and fragment shaders into opengl
	LoadShaders();

	atomicCounterArray[0] = 0;
	glGenBuffers(1, &ac_buffer);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ac_buffer);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, ac_buffer);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	GLuint baseInstance = 0;
	GLuint numDraws = 0;
	for (int i = 0; i < 30; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			LoadTriangle(pfVertexPositionsIn[i], pfCameraPosiitons, piIndexBufferIn[j], numVertices, numFaces);
			glViewport(0, 0, CANVASXNUMS*CANVASWIDTH, CANVASYNUMS*CANVASHEIGHT);
			Render(baseInstance, numFaces);
			numDraws++;
		}
	}
	glfwTerminate();
}

// malloc 2 dimension array
template <typename T>
T** new_Array2D(int row, int col)
{
	int size = sizeof(T);
	int point_size = sizeof(T*);
	//
	T **arr = (T **)malloc(point_size * row + size * row * col);
	if (arr != NULL)
	{
		T *head = (T*)((int)arr + point_size * row);
		for (int i = 0; i < row; ++i)
		{
			arr[i] = (T*)((int)head + i * col * size);
			for (int j = 0; j < col; ++j)
				new (&arr[i][j]) T;
		}
	}
	return (T**)arr;
}
//release
template <typename T>
void delete_Array2D(T **arr, int row, int col)
{
	for (int i = 0; i < row; ++i)
		for (int j = 0; j < col; ++j)
			arr[i][j].~T();
	if (arr != NULL)
		free((void**)arr);
}

int main(int argc, char *argv[]) {
	// parameters needed
	int characterId = 1; int aniId = 0; float alpha = 0.85; int iCacheSize = 20;
	char Character[5][20] = { "Ganfaul_M_Aure", "Kachujin_G_Rosales", "Maw_J_Laygo", "Nightshade" };
	char Animation[7][40] = { "Crouch_Walk_Left", "dancing_maraschino_step", "Standing_2H_Cast_Spell", "Standing_2H_Magic_Area_Attack", "Standing_Jump", "Standing_React_Death_Backward", "Standing_React_Large_From_Back" };
	int charVertices[4] = { 7865, 6675, 7482, 6691};
	int charFaces[4] = { 13801, 12610, 13908, 12996};
	int charPatches[4] = { 475, 482, 611, 387 };
	int aniDuration[7] = { 30,75, 50, 70, 50, 45, 40 };
	int numFrames = aniDuration[aniId]; int iNumVertices = charVertices[characterId]; int iNumFaces = charFaces[characterId];int numPatches = charPatches[characterId]; int numViews = 162;
	int pickIds[5] = { 148, 54, 17, 92, 45 }; int numClusters = 5; 

	// set memory
	int * miScratch = NULL;
	bool bMalloc = false;
	if (miScratch == NULL)
	{
		int iScratchSize =  (iNumFaces* 3*3+ numViews*3) * sizeof(int);
		miScratch = (int *)malloc(iScratchSize);
		memset(miScratch, 0, iScratchSize);
		bMalloc = true;
	}
	int *piScratchBase = miScratch;
	int *piIndexBufferIn = miScratch;
	miScratch += iNumFaces * 3;
	int * piIndexBufferOut = miScratch;
	miScratch += iNumFaces * 3;
	int * piClustersOut = miScratch;
	miScratch += iNumFaces * 3;
	float * pfCameraPositions = (float*)miScratch;
	miScratch += numViews * 3;
	
	float ** pfFramesVertexPositionsIn = new_Array2D<float>(numFrames,iNumVertices*3);
	Vector ** pvFramesPatchesPositions = new_Array2D<Vector>(numFrames, numPatches);
	int ** means = new_Array2D<int>(numClusters, iNumFaces * 3);
	//int means[5][INUMFACES * 3];
	time_t tstart, tend;

	
	//int piIndexBufferIn[INUMFACES * 3];
	//int piIndexBufferOut[INUMFACES * 3];
	//int piClustersOut[INUMFACES * 3];
	//float pfFramesVertexPositionsIn[30][INUMVERTICES * 3];
	//Vector  pvFramesPatchesPositions[numFrames][482];
	//float pfCameraPositions[162 * 3];

	int *piScratch = NULL; int iNumClusters;
	char vfFolder[150]; char facePath[150]; char verticesPath[150]; char cameraPath[150];
	strcpy(vfFolder, "D:/Hansf/Research/triangleordering/webstorm/VerticeFace/");
	strcat(vfFolder, Character[characterId]);
	strcat(vfFolder, "/");
	strcat(vfFolder, Animation[aniId]);
	strcat(vfFolder, "/");

	FILE * myFile;
	strcpy(facePath, vfFolder);
	strcat(facePath, "face.txt");
	std::cout << facePath << std::endl;
	myFile = fopen(facePath, "r");
	if (myFile == NULL)
	{
		printf("ERROR: File cannot be opened\n");
	}
	for (int i = 0; i < iNumFaces * 3; i++)
	{
		fscanf(myFile, "%d \n", &piIndexBufferIn[i]);
	}
	fclose(myFile);

	strcpy(cameraPath, vfFolder);
	strcat(cameraPath, "newViewpoint3.txt");

	myFile = fopen(cameraPath, "r");
	for (int i = 0; i < numViews * 3; i++)
	{
		fscanf(myFile, "%f \n", &pfCameraPositions[i]);
	}
	fclose(myFile);
	Vector *pvCameraPositions = (Vector *)pfCameraPositions;
	for (int frameId = 0; frameId < numFrames; frameId++)
	{
		char buffer[50];
		itoa(frameId + 1, buffer, 10);
		strcpy(verticesPath, vfFolder);
		strcat(verticesPath, "frame");
		strcat(verticesPath, buffer);
		strcat(verticesPath, "v.txt");

		myFile = fopen(verticesPath, "r");
		if (myFile == NULL)
		{
			printf("ERROR : File cannot be opened\n");
		}
		for (int i = 0; i < iNumVertices * 3; i++)
		{
			fscanf(myFile, "%f\n", &pfFramesVertexPositionsIn[frameId][i]);
		}
		fclose(myFile);
	}

	FanVertCluster(pfFramesVertexPositionsIn[0], piIndexBufferIn, piIndexBufferOut, iNumVertices, iNumFaces, iCacheSize, alpha, piScratch, piClustersOut, &iNumClusters);
	
	for (int i = 0; i < numFrames; i++)
	{
		pvPatchesPostions(piIndexBufferOut, iNumFaces, pfFramesVertexPositionsIn[i], iNumVertices, piClustersOut, iNumClusters, pvFramesPatchesPositions[i], piScratch);
	}

	// start point
	tstart = time(0);
	initMeans(means, pvFramesPatchesPositions, piIndexBufferOut, piClustersOut, numFrames, numClusters, numPatches, iNumFaces, pickIds, pfCameraPositions, piScratch);
	//initMeans(pvFramesPatchesPositions, piIndexBufferOut, piClustersOut, numFrames, numClusters, numPatches, pickIds, pfCameraPositions, means, piScratch);
	//// delete later
	//int assignments[INUMFRAMES][INUMVIEWS];
	//myFile = fopen("assignments.txt","r");
	//int count = 0,viewId,frameId;
	//for (int i = 0; i < INUMFRAMES*INUMVIEWS; i++)
	//{
	//	frameId = i / INUMVIEWS;
	//	viewId = i%INUMVIEWS;
	//	fscanf(myFile, "%d \n", &assignments[frameId][viewId]);
	//}
	//fclose(myFile);
	//float minRatios[INUMFRAMES][INUMVIEWS];
	//for (int i = 0; i < INUMFRAMES; i++)
	//{
	//	for (int j = 0; j < INUMVIEWS; j++)
	//	{
	//		minRatios[i][j] = 1.0;
	//	}
	//}
	
	
	/*char meansFile[50];
	char meansbuffer[50];
	itoa(clusterId, meansbuffer, 10);
	strcpy(meansFile, "newMeans");
	strcat(meansFile, meansbuffer);
	strcat(meansFile, ".txt");
	myFile = fopen(meansFile, "w");
	for (int j = 0; j < INUMFACES * 3; j++)
	{
	fprintf(myFile, "%d \n", means[0][j]);
	}
	fclose(myFile);*/

	//AppMain(pfFramesVertexPositionsIn, pfCameraPositions, means, iNumVertices, iNumFaces);
	tend = time(0);
	std::cout << "It took" << difftime(tend, tstart) << "second(s)." << std::endl;
	getchar();
	return EXIT_SUCCESS;
}
