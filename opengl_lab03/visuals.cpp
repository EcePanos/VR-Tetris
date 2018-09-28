#include <iostream>	  // For string handling
#include <stdlib.h>	  // Just for some standard functions
#include <stdio.h>    // Just for some ASCII messages
//#include <glut.h> 
#include "visuals.h"  // Header file for our OpenGL functions
#include <Extras/OVR_Math.h>
#include <time.h>
#include "windows.h"
#include <sys/types.h>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include "soil.h"

#define KEY_ESC 27
#define SENSITIVITY 2.0
#define PI 3.1415927

using namespace std;
using namespace OVR;
using namespace tinyobj;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// OCULUS STUFF

void initOVR();

void cleanupOVR();

unsigned int nextPow2(unsigned int x);

void updateRenderTarget(int width, int height);

void quatToMatrix(const float *quat, float *mat);

static int win_width, win_height;

static TextureBuffer * eyeRenderTexture[2] = { nullptr, nullptr };
static DepthBuffer   * eyeDepthBuffer[2] = { nullptr, nullptr };
static ovrGLTexture  * mirrorTexture = nullptr;
static GLuint          mirrorFBO = 0;
static ovrHmd hmd;
static ovrGraphicsLuid luid;
static ovrHmdDesc hmdDesc;
static ovrSizei eyeres[2];
static ovrEyeRenderDesc EyeRenderDesc[2];
static ovrLayerEyeFov ovr_layers;
static ovrTrackingState hmdState;

static unsigned int distort_caps;
static unsigned int hmd_caps;
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/*******************************************************************************
State Variables
******************************************************************************/
static unsigned int chess_tex;

// Model variables
//float translateY = 0.0f;
//float vel = 40.0;

static float Yaw = 3.141592f;
static Vector3f Pos2(0.0f, 1.6f, -5.0f);

// Texture IDs

static GLuint barberTexID;
static GLuint blueTexID;
static GLuint greenTexID;
static GLuint orangeTexID;
static GLuint pinkTexID;
static GLuint purpleTexID;
static GLuint redTexID;
static GLuint yellowTexID;

static GLuint currentTexID;
static int tx = 0;
static int ty = 0;
// Model variables

mesh_t cube;
float translateY = 0.0f;
float RotateZ = 0.0f;
int rotflag = 0;
float rotatex = 0.0f;
float vel = 40.0;
float run1 = 0.0f;
int board[10][20][10];
int block[4][3];
int nextblock[4][3];
int full = 0;
bool pause = true;
float zoom = 350;
int score = 0;
bool gameover = false;
float midx = 0;
float midy = 0;
float midz = 0;
float x1 = 0;
float x2 = 0;
float x3 = 0;
float x4 = 0;
float y10 = 0;
float y2 = 0;
float y3 = 0;
float y4 = 0;
float z1 = 0;
float z2 = 0;
float z3 = 0;
float z4 = 0;
float tmp = 0;
int blockid = 0;
int nextblockid = 0;
int difficulty = 1;
int beginPos = 0;
int endPos = 9;
int boardSize = 100;
int play = 0;
static float dx = 0.0;
static float dy = 0.0;
static float dz = 0.0;
static float ry = 0.0;
// Camera variables
float zoomFactor = 0.1;
float cameraDist = 0.0;
float camx = 0;
int cameraToggle = 0;
bool armflag = false;
// Event handle variables
static bool mouseClickDown = false;
static int  mx0;
static int  my0;

/************************************************************************/
/*  Implementation of OCULUS-Related functions	                        */
/************************************************************************/
void initOVR()
{
	int i, x, y;
	unsigned int flags;
	ovrResult result = ovr_Create(&hmd, &luid);
	if (OVR_FAILURE(result))
	{
		fprintf(stderr, "failed to open Oculus HMD\n");
		exit(-1);
	}
	else
	{
		// Get description
		hmdDesc = ovr_GetHmdDesc(hmd);
		printf("initialized HMD: %s - %s\n", hmdDesc.Manufacturer, hmdDesc.ProductName);
	}

	// Resize
	ovrSizei windowSize = { hmdDesc.Resolution.w, hmdDesc.Resolution.h };
	glutReshapeWindow(hmdDesc.Resolution.w, hmdDesc.Resolution.h);
	win_width = hmdDesc.Resolution.w;
	win_height = hmdDesc.Resolution.h;

	/* enable rotation tracking */
	result = ovr_ConfigureTracking(hmd, ovrTrackingCap_Orientation | ovrTrackingCap_MagYawCorrection, 0);
	if (!OVR_SUCCESS(result))
	{
		printf("Failed to configure tracking.");
		exit(-1);
	}

	// Make eye render buffers
	for (int eye = 0; eye < 2; ++eye)
	{
		ovrSizei idealTextureSize = ovr_GetFovTextureSize(hmd, ovrEyeType(eye), hmdDesc.DefaultEyeFov[eye], 1);
		eyeRenderTexture[eye] = new TextureBuffer(hmd, true, true, idealTextureSize, 1, NULL, 1);
		eyeDepthBuffer[eye] = new DepthBuffer(eyeRenderTexture[eye]->GetSize(), 0);

		if (!eyeRenderTexture[eye]->TextureSet)
		{
			printf("Failed to create texture.");
			exit(-1);
		}
	}

	// Create mirror texture and an FBO used to copy mirror texture to back buffer
	result = ovr_CreateMirrorTextureGL(hmd, GL_SRGB8_ALPHA8, windowSize.w, windowSize.h, reinterpret_cast<ovrTexture**>(&mirrorTexture));
	if (!OVR_SUCCESS(result))
	{
		printf("Failed to create mirror texture.");
		exit(-1);
	}

	// Configure the mirror read buffer
	glGenFramebuffers(1, &mirrorFBO);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
	glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mirrorTexture->OGL.TexId, 0);
	glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	EyeRenderDesc[0] = ovr_GetRenderDesc(hmd, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
	EyeRenderDesc[1] = ovr_GetRenderDesc(hmd, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);
}
/*******************************************************************************
Implementation of Visual Functions
******************************************************************************/

void writeText(char *str, void *font)
{
	//-------------Task3a---------------
	char *c;
	for (c = str; *c != '\0'; c++)
		glutStrokeCharacter(font, *c);
}

void newBlock() {
	if (play != 0) {
		blockid = nextblockid;
	}
	srand(time(NULL));
	srand(rand());
	nextblockid = rand() % 7;
	//blockid=0;
	switch (blockid) {
	case 0:
		block[0][0] = 4;
		block[0][1] = 0;
		block[0][2] = 5;

		block[1][0] = 5;
		block[1][1] = 0;
		block[1][2] = 5;

		block[2][0] = 4;
		block[2][1] = 1;
		block[2][2] = 5;

		block[3][0] = 5;
		block[3][1] = 1;
		block[3][2] = 5;
		break;
	case 1:
		block[0][0] = 4;
		block[0][1] = 0;
		block[0][2] = 5;

		block[1][0] = 4;
		block[1][1] = 1;
		block[1][2] = 5;

		block[2][0] = 5;
		block[2][1] = 1;
		block[2][2] = 5;

		block[3][0] = 6;
		block[3][1] = 1;
		block[3][2] = 5;
		break;
	case 2:
		block[0][0] = 4;
		block[0][1] = 0;
		block[0][2] = 5;

		block[1][0] = 5;
		block[1][1] = 0;
		block[1][2] = 5;

		block[2][0] = 6;
		block[2][1] = 0;
		block[2][2] = 5;

		block[3][0] = 7;
		block[3][1] = 0;
		block[3][2] = 5;
		break;
	case 3:
		block[0][0] = 4;
		block[0][1] = 1;
		block[0][2] = 5;

		block[1][0] = 5;
		block[1][1] = 1;
		block[1][2] = 5;

		block[2][0] = 6;
		block[2][1] = 1;
		block[2][2] = 5;

		block[3][0] = 6;
		block[3][1] = 0;
		block[3][2] = 5;
		break;
	case 4:
		block[0][0] = 4;
		block[0][1] = 0;
		block[0][2] = 5;

		block[1][0] = 5;
		block[1][1] = 0;
		block[1][2] = 5;

		block[2][0] = 5;
		block[2][1] = 1;
		block[2][2] = 5;

		block[3][0] = 6;
		block[3][1] = 1;
		block[3][2] = 5;
		break;
	case 5:
		block[0][0] = 5;
		block[0][1] = 0;
		block[0][2] = 5;

		block[1][0] = 6;
		block[1][1] = 0;
		block[1][2] = 5;

		block[2][0] = 4;
		block[2][1] = 1;
		block[2][2] = 5;

		block[3][0] = 5;
		block[3][1] = 1;
		block[3][2] = 5;
		break;
	case 6:
		block[0][0] = 5;
		block[0][1] = 0;
		block[0][2] = 5;

		block[1][0] = 4;
		block[1][1] = 1;
		block[1][2] = 5;

		block[2][0] = 5;
		block[2][1] = 1;
		block[2][2] = 5;

		block[3][0] = 6;
		block[3][1] = 1;
		block[3][2] = 5;
		break;
	default:
		break;
	}
	switch (nextblockid) {
	case 0:
		nextblock[0][0] = 4;
		nextblock[0][1] = 0;
		nextblock[0][2] = 5;

		nextblock[1][0] = 5;
		nextblock[1][1] = 0;
		nextblock[1][2] = 5;

		nextblock[2][0] = 4;
		nextblock[2][1] = 1;
		nextblock[2][2] = 5;

		nextblock[3][0] = 5;
		nextblock[3][1] = 1;
		nextblock[3][2] = 5;
		break;
	case 1:
		nextblock[0][0] = 4;
		nextblock[0][1] = 0;
		nextblock[0][2] = 5;

		nextblock[1][0] = 4;
		nextblock[1][1] = 1;
		nextblock[1][2] = 5;

		nextblock[2][0] = 5;
		nextblock[2][1] = 1;
		nextblock[2][2] = 5;

		nextblock[3][0] = 6;
		nextblock[3][1] = 1;
		nextblock[3][2] = 5;
		break;
	case 2:
		nextblock[0][0] = 4;
		nextblock[0][1] = 0;
		nextblock[0][2] = 5;

		nextblock[1][0] = 5;
		nextblock[1][1] = 0;
		nextblock[1][2] = 5;

		nextblock[2][0] = 6;
		nextblock[2][1] = 0;
		nextblock[2][2] = 5;

		nextblock[3][0] = 7;
		nextblock[3][1] = 0;
		nextblock[3][2] = 5;
		break;
	case 3:
		nextblock[0][0] = 4;
		nextblock[0][1] = 1;
		nextblock[0][2] = 5;

		nextblock[1][0] = 5;
		nextblock[1][1] = 1;
		nextblock[1][2] = 5;

		nextblock[2][0] = 6;
		nextblock[2][1] = 1;
		nextblock[2][2] = 5;

		nextblock[3][0] = 6;
		nextblock[3][1] = 0;
		nextblock[3][2] = 5;
		break;
	case 4:
		nextblock[0][0] = 4;
		nextblock[0][1] = 0;
		nextblock[0][2] = 5;

		nextblock[1][0] = 5;
		nextblock[1][1] = 0;
		nextblock[1][2] = 5;

		nextblock[2][0] = 5;
		nextblock[2][1] = 1;
		nextblock[2][2] = 5;

		nextblock[3][0] = 6;
		nextblock[3][1] = 1;
		nextblock[3][2] = 5;
		break;
	case 5:
		nextblock[0][0] = 5;
		nextblock[0][1] = 0;
		nextblock[0][2] = 5;

		nextblock[1][0] = 6;
		nextblock[1][1] = 0;
		nextblock[1][2] = 5;

		nextblock[2][0] = 4;
		nextblock[2][1] = 1;
		nextblock[2][2] = 5;

		nextblock[3][0] = 5;
		nextblock[3][1] = 1;
		nextblock[3][2] = 5;
		break;
	case 6:
		nextblock[0][0] = 5;
		nextblock[0][1] = 0;
		nextblock[0][2] = 5;

		nextblock[1][0] = 4;
		nextblock[1][1] = 1;
		nextblock[1][2] = 5;

		nextblock[2][0] = 5;
		nextblock[2][1] = 1;
		nextblock[2][2] = 5;

		nextblock[3][0] = 6;
		nextblock[3][1] = 1;
		nextblock[3][2] = 5;
		break;
	default:
		break;
	}
}

void planarMapping(GLfloat &u, GLfloat &v, float *point)
{
	//-------------Task2b---------------
	// The coordinates of the represented object range between [-50, 50]
	// When u, v texture coordinates between [0, 1]
	u = (point[0] + 5) / 10;
	v = (point[1] + 5) / 10;
	//u=(point[0]+50)/20;
	//v=(point[1]+50)/20;
}


void xzMapping(GLfloat &u, GLfloat &v, float *point)
{
	//-------------Task2b---------------
	// The coordinates of the represented object range between [-50, 50]
	// When u, v texture coordinates between [0, 1]
	u = (point[0] + 5) / 10;
	v = (point[2] + 5) / 10;
	//u=(point[0]+50)/20;
	//v=(point[1]+50)/20;
}

void yzMapping(GLfloat &u, GLfloat &v, float *point)
{
	//-------------Task2b---------------
	// The coordinates of the represented object range between [-50, 50]
	// When u, v texture coordinates between [0, 1]
	u = (point[1] + 5) / 10;
	v = (point[2] + 5) / 10;
	//u=(point[0]+50)/20;
	//v=(point[1]+50)/20;
}


string getExeDir()
{
	HMODULE hModule = GetModuleHandleW(NULL);
	WCHAR wPath[MAX_PATH];
	GetModuleFileNameW(hModule, wPath, MAX_PATH);
	char cPath[MAX_PATH];
	char DefChar = ' ';
	WideCharToMultiByte(CP_ACP, 0, wPath, -1, cPath, 260, &DefChar, NULL);
	string sPath(cPath);
	return sPath.substr(0, sPath.find_last_of("\\/")).append("\\");
}

//------------------------------------------------------------------------------

void calculateNormals(mesh_t &mesh)
{
	cout << endl << "Calculating Normals..." << endl;
	// Take the points, normals and faces of the obj
	vector<float> &positions = mesh.positions;
	vector<float> &normals = mesh.normals;
	vector<unsigned> &indices = mesh.indices;

	cout << "Before Calculations" << endl;
	cout << "Number of Positions: " << positions.size() << endl;
	cout << "Number of Normals: " << normals.size() << endl;
	cout << "Number of Indicies: " << indices.size() << endl << endl;

	// Set all normals (0.0 0.0 1.0)
	//*
	for (int i = 0; i < positions.size(); i += 3)
	{
		normals.push_back(0.0);
		normals.push_back(0.0);
		normals.push_back(1.0);
	}


	cout << "After Calculations" << endl;
	cout << "Number of Positions: " << positions.size() << endl;
	cout << "Number of Normals: " << normals.size() << endl;
	cout << "Number of Indicies: " << indices.size() << endl;
}

//------------------------------------------------------------------------------

void readObj(string filename, mesh_t &mesh)
{
	//-------------Task1a---------------
	vector<shape_t> shapes;
	vector<material_t> materials;

	string err;
	//string err = LoadObj(shapes, filename.c_str(), (getExeDir() + "..\\Models\\").c_str());
	
	bool ret = LoadObj(shapes, materials, err, filename.c_str(), (getExeDir() + "..\\Models\\").c_str());

	
	if (!err.empty())
	{
		cerr << err << endl;
		exit(1);
	}
	else
	{
		// The .obj file might contain more than one shapes(meshes)
		// we are intrested only for the first one
		mesh = shapes[0].mesh;
		if (mesh.normals.empty())
		{
			cout << "Normals does not exist!" << endl;
			calculateNormals(mesh);
		}
		//-----------Task1c-------------
		// Modify points here.
		for (int i = 0;i<mesh.positions.size();i++) {
			mesh.positions[i] /= 10;
		}
	}
}

//------------------------------------------------------------------------------


void drawxy(mesh_t &mesh,
	void(*Map)(GLfloat &u, GLfloat &v, float *point),
	int textureID)
{
	//-------------Task2a---------------
	// Take the points and faces of the first shape of obj
	vector<unsigned> &indices = mesh.indices;
	vector<float>  &positions = mesh.positions;
	vector<float>    &normals = mesh.normals;
	vector<float>  &texcoords = mesh.texcoords;



	float* point1;
	float* point2;
	float* point3;
	if (Map && textureID)
	{
		// Load texture using the texture ID and Transform given
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textureID);
		GLfloat coord1;
		GLfloat coord2;

		float* normal1;
		float* normal2;
		float* normal3;
		glBegin(GL_TRIANGLES);
		for (int i = 0; i < indices.size(); i += 3)
		{
			normal1 = &normals[3 * indices[i]];
			normal2 = &normals[3 * indices[i + 1]];
			normal3 = &normals[3 * indices[i + 2]];
			point1 = &positions[3 * indices[i]];
			point2 = &positions[3 * indices[i + 1]];
			point3 = &positions[3 * indices[i + 2]];
			if (point1[2] == point2[2] && point1[2] == point3[2]) {
				// Calculate texture coordinates
				Map(coord1, coord2, point1);

				glTexCoord2f(coord1, coord2);
				glNormal3fv((GLfloat*)normal1);
				glVertex3fv((GLfloat*)point1);

				Map(coord1, coord2, point2);

				glTexCoord2f(coord1, coord2);
				glNormal3fv((GLfloat*)normal2);
				glVertex3fv((GLfloat*)point2);

				Map(coord1, coord2, point3);

				glTexCoord2f(coord1, coord2);
				glNormal3fv((GLfloat*)normal3);
				glVertex3fv((GLfloat*)point3);
			}
		}
		glEnd();

		glDisable(GL_TEXTURE_2D);
	}

}

void drawxz(mesh_t &mesh,
	void(*Map)(GLfloat &u, GLfloat &v, float *point),
	int textureID)
{
	//-------------Task2a---------------
	// Take the points and faces of the first shape of obj
	vector<unsigned> &indices = mesh.indices;
	vector<float>  &positions = mesh.positions;
	vector<float>    &normals = mesh.normals;
	vector<float>  &texcoords = mesh.texcoords;



	float* point1;
	float* point2;
	float* point3;
	if (Map && textureID)
	{
		// Load texture using the texture ID and Transform given
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textureID);
		GLfloat coord1;
		GLfloat coord2;

		float* normal1;
		float* normal2;
		float* normal3;
		glBegin(GL_TRIANGLES);
		for (int i = 0; i < indices.size(); i += 3)
		{
			normal1 = &normals[3 * indices[i]];
			normal2 = &normals[3 * indices[i + 1]];
			normal3 = &normals[3 * indices[i + 2]];
			point1 = &positions[3 * indices[i]];
			point2 = &positions[3 * indices[i + 1]];
			point3 = &positions[3 * indices[i + 2]];
			if (point1[1] == point2[1] && point1[1] == point3[1]) {
				// Calculate texture coordinates
				Map(coord1, coord2, point1);

				glTexCoord2f(coord1, coord2);
				glNormal3fv((GLfloat*)normal1);
				glVertex3fv((GLfloat*)point1);

				Map(coord1, coord2, point2);

				glTexCoord2f(coord1, coord2);
				glNormal3fv((GLfloat*)normal2);
				glVertex3fv((GLfloat*)point2);

				Map(coord1, coord2, point3);

				glTexCoord2f(coord1, coord2);
				glNormal3fv((GLfloat*)normal3);
				glVertex3fv((GLfloat*)point3);
			}
		}
		glEnd();

		glDisable(GL_TEXTURE_2D);
	}

}

void drawyz(mesh_t &mesh,
	void(*Map)(GLfloat &u, GLfloat &v, float *point),
	int textureID)
{
	//-------------Task2a---------------
	// Take the points and faces of the first shape of obj
	vector<unsigned> &indices = mesh.indices;
	vector<float>  &positions = mesh.positions;
	vector<float>    &normals = mesh.normals;
	vector<float>  &texcoords = mesh.texcoords;



	float* point1;
	float* point2;
	float* point3;
	if (Map && textureID)
	{
		// Load texture using the texture ID and Transform given
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, textureID);
		GLfloat coord1;
		GLfloat coord2;

		float* normal1;
		float* normal2;
		float* normal3;
		glBegin(GL_TRIANGLES);
		for (int i = 0; i < indices.size(); i += 3)
		{
			normal1 = &normals[3 * indices[i]];
			normal2 = &normals[3 * indices[i + 1]];
			normal3 = &normals[3 * indices[i + 2]];
			point1 = &positions[3 * indices[i]];
			point2 = &positions[3 * indices[i + 1]];
			point3 = &positions[3 * indices[i + 2]];
			if (point1[0] == point2[0] && point1[0] == point3[0]) {
				// Calculate texture coordinates
				Map(coord1, coord2, point1);

				glTexCoord2f(coord1, coord2);
				glNormal3fv((GLfloat*)normal1);
				glVertex3fv((GLfloat*)point1);

				Map(coord1, coord2, point2);

				glTexCoord2f(coord1, coord2);
				glNormal3fv((GLfloat*)normal2);
				glVertex3fv((GLfloat*)point2);

				Map(coord1, coord2, point3);

				glTexCoord2f(coord1, coord2);
				glNormal3fv((GLfloat*)normal3);
				glVertex3fv((GLfloat*)point3);
			}
		}
		glEnd();

		glDisable(GL_TEXTURE_2D);
	}

}


//------------------------------------------------------------------------------

GLuint loadTexture(const char *filename)
{
	// load an image file directly as a new OpenGL texture
	GLuint texID;
	texID = SOIL_load_OGL_texture(filename, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
		SOIL_FLAG_INVERT_Y | SOIL_FLAG_MIPMAPS |
		SOIL_FLAG_TEXTURE_REPEATS);

	// check for an error during the load process
	if (!texID) cout << "texture: " << *filename << " .SOIL loading error: "
		<< SOIL_last_result() << endl;
	return texID;
}

void setup()
{
	// Parameter handling
	glewInit();

	initOVR();

	// Parameter handling
	for (int i = 0;i<10;i++) {
		for (int j = 0;j<20;j++) {
			for (int k = 0;k<10;k++) {
				board[i][j][k] = 0;
			}
		}
	}
	srand(time(NULL));
	blockid = rand() % 7;
	srand(time(NULL));
	nextblockid = rand() % 7;
	newBlock();
	// polygon rendering mode and material properties
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Make models with smooth edges
	glShadeModel(GL_SMOOTH);

	// Points represented as circles
	glEnable(GL_POINT_SMOOTH);

	// Blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Depth test
	glEnable(GL_DEPTH_TEST);
	// Renders a fragment if its z value is less or equal of the stored value
	glDepthFunc(GL_LEQUAL);
	glClearDepth(1);

	// Cull Faces
	// Don't draw triangles that cannot be seen
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);

	// Polygon rendering mode enable Material
	glEnable(GL_COLOR_MATERIAL);
	// We use Ambient and Diffuse color of each object as given in color3f
	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

	// Enable the Lighting
	glEnable(GL_LIGHTING);

	// Set up light source
	// Light
	GLfloat lightAmbient[] = { 0.2,   0.2,   0.2, 1.0 };
	GLfloat lightDiffuse[] = { 0.6,   0.6,   0.6, 1.0 };
	GLfloat lightSpecular[] = { 0.9,   0.9,   0.9, 1.0 };
	GLfloat lightPosition[] = { 0.0,   0.0,   0.0, 1.0 };
	GLfloat lightDirection[] = { 0.0,   0.0,  -1.0, 0.0 };

	// Set Ambient Light
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
	// Set Difuse Light
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
	// Set Specular Light
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
	// Set Position of the Light
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

	// Enable Light
	glEnable(GL_LIGHT0);

	//-------------Task2a---------------
	// Enable Texture
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_NORMALIZE);

	barberTexID = loadTexture((getExeDir() + "..\\Textures\\BarberPole.jpg").c_str());
	blueTexID = loadTexture((getExeDir() + "..\\Textures\\blue.bmp").c_str());
	greenTexID = loadTexture((getExeDir() + "..\\Textures\\green.bmp").c_str());
	orangeTexID = loadTexture((getExeDir() + "..\\Textures\\orange.bmp").c_str());
	pinkTexID = loadTexture((getExeDir() + "..\\Textures\\pink.bmp").c_str());
	purpleTexID = loadTexture((getExeDir() + "..\\Textures\\purple.bmp").c_str());
	redTexID = loadTexture((getExeDir() + "..\\Textures\\red.bmp").c_str());
	yellowTexID = loadTexture((getExeDir() + "..\\Textures\\yellow.bmp").c_str());
	// Set the current texture ID
	currentTexID = yellowTexID;


	readObj(getExeDir() + "..\\Models\\cube.obj", cube);

	//*/

	// Black background
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

//------------------------------------------------------------------------------

void render()
{
	//////////////////////////////////////////////////////////////////////////
	// Get eye poses, feeding in correct IPD offset
	ovrVector3f               ViewOffset[2] = { EyeRenderDesc[0].HmdToEyeViewOffset,
		EyeRenderDesc[1].HmdToEyeViewOffset };
	ovrPosef                  EyeRenderPose[2];

	ovr_CalcEyePoses(hmdState.HeadPose.ThePose, ViewOffset, EyeRenderPose);
	//////////////////////////////////////////////////////////////////////////

	for (int eye = 0; eye < 2; ++eye)
	{
		// Increment to use next texture, just before writing
		eyeRenderTexture[eye]->TextureSet->CurrentIndex = (eyeRenderTexture[eye]->TextureSet->CurrentIndex + 1) % eyeRenderTexture[eye]->TextureSet->TextureCount;

		// Switch to eye render target
		eyeRenderTexture[eye]->SetAndClearRenderSurface(eyeDepthBuffer[eye]);

		// Get view and projection matrices
		Matrix4f rollPitchYaw = Matrix4f::RotationY(Yaw);
		Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(EyeRenderPose[eye].Orientation);
		Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
		Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));
		Vector3f shiftedEyePos = Pos2 + rollPitchYaw.Transform(EyeRenderPose[eye].Position);

		Matrix4f view = Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
		Matrix4f proj = ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], 0.2f, 1000.0f, ovrProjection_RightHanded);

		//use Projection and ModelView matrix
		glMatrixMode(GL_PROJECTION);
		glLoadTransposeMatrixf(proj.M[0]);

		glMatrixMode(GL_MODELVIEW);
		glLoadTransposeMatrixf(view.M[0]);

		// Draw the scene
		drawScene();

		// Avoids an error when calling SetAndClearRenderSurface during next iteration.
		// Without this, during the next while loop iteration SetAndClearRenderSurface
		// would bind a framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
		// associated with COLOR_ATTACHMENT0 had been unlocked by calling wglDXUnlockObjectsNV.
		eyeRenderTexture[eye]->UnsetRenderSurface();
	}

	// Do distortion rendering, Present and flush / sync

	// Set up positional data.
	ovrViewScaleDesc viewScaleDesc;
	viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
	viewScaleDesc.HmdToEyeViewOffset[0] = ViewOffset[0];
	viewScaleDesc.HmdToEyeViewOffset[1] = ViewOffset[1];

	ovrLayerEyeFov ld;
	ld.Header.Type = ovrLayerType_EyeFov;
	ld.Header.Flags = ovrLayerFlag_TextureOriginAtBottomLeft;   // Because OpenGL.

	for (int eye = 0; eye < 2; ++eye)
	{
		ld.ColorTexture[eye] = eyeRenderTexture[eye]->TextureSet;
		ld.Viewport[eye] = Recti(eyeRenderTexture[eye]->GetSize());
		ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
		ld.RenderPose[eye] = EyeRenderPose[eye];
	}

	ovrLayerHeader* layers = &ld.Header;
	ovrResult result = ovr_SubmitFrame(hmd, 0, &viewScaleDesc, &layers, 1);

	// Draw to Window mirror buffer
	// Blit mirror texture to back buffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, mirrorFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	GLint w = mirrorTexture->OGL.Header.TextureSize.w;
	GLint h = mirrorTexture->OGL.Header.TextureSize.h;
	glBlitFramebuffer(0, h, w, 0,
		0, 0, w, h,
		GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	glutSwapBuffers();
}

void drawScene()
{
	// Clean up the colour of the window and the depth buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// Set the camera
	if (cameraToggle == 0) {
	gluLookAt(zoom*-sin(camx*(PI / 180))*cos(cameraDist*(PI / 180)), zoom*sin(cameraDist*(PI / 180)), zoom*cos(camx*(PI / 180))*cos(cameraDist*(PI / 180)), //camera position
	0, 0, 0, //target position
	0, 1, 0);//up vector
	}
	// */


	glBegin(GL_LINE_LOOP);
	glVertex3f(-55, 65, -5);
	glVertex3f(45, 65, -5);
	glVertex3f(45, 65, 95);
	glVertex3f(-55, 65, 95);
	glEnd();

	glBegin(GL_LINE_LOOP);
	glVertex3f(-55, -135, -5);
	glVertex3f(45, -135, -5);
	glVertex3f(45, -135, 95);
	glVertex3f(-55, -135, 95);
	glEnd();

	glBegin(GL_LINE_LOOP);
	glVertex3f(-55, 65, -5);
	glVertex3f(-55, -135, -5);
	glVertex3f(45, -135, -5);
	glVertex3f(45, 65, -5);
	glEnd();

	glBegin(GL_LINE_LOOP);
	glVertex3f(-55, 65, 95);
	glVertex3f(-55, -135, 95);
	glVertex3f(45, -135, 95);
	glVertex3f(45, 65, 95);
	glEnd();

	glBegin(GL_LINES);
	glVertex3f(-55, -135, 5);
	glVertex3f(45, -135, 5);
	glVertex3f(-55, -135, 15);
	glVertex3f(45, -135, 15);
	glVertex3f(-55, -135, 25);
	glVertex3f(45, -135, 25);
	glVertex3f(-55, -135, 35);
	glVertex3f(45, -135, 35);
	glVertex3f(-55, -135, 45);
	glVertex3f(45, -135, 45);
	glVertex3f(-55, -135, 55);
	glVertex3f(45, -135, 55);
	glVertex3f(-55, -135, 65);
	glVertex3f(45, -135, 65);
	glVertex3f(-55, -135, 75);
	glVertex3f(45, -135, 75);
	glVertex3f(-55, -135, 85);
	glVertex3f(45, -135, 85);

	glVertex3f(-45, -135, -5);
	glVertex3f(-45, -135, 95);
	glVertex3f(-35, -135, -5);
	glVertex3f(-35, -135, 95);
	glVertex3f(-25, -135, -5);
	glVertex3f(-25, -135, 95);
	glVertex3f(-15, -135, -5);
	glVertex3f(-15, -135, 95);
	glVertex3f(5, -135, -5);
	glVertex3f(5, -135, 95);
	glVertex3f(-5, -135, -5);
	glVertex3f(-5, -135, 95);
	glVertex3f(15, -135, -5);
	glVertex3f(15, -135, 95);
	glVertex3f(25, -135, -5);
	glVertex3f(25, -135, 95);
	glVertex3f(35, -135, -5);
	glVertex3f(35, -135, 95);
	glVertex3f(45, -135, -5);
	glVertex3f(45, -135, 95);

	glVertex3f(45, -125, -5);
	glVertex3f(45, -125, 95);
	glVertex3f(45, -115, -5);
	glVertex3f(45, -115, 95);
	glVertex3f(45, -105, -5);
	glVertex3f(45, -105, 95);
	glVertex3f(45, -95, -5);
	glVertex3f(45, -95, 95);
	glVertex3f(45, -85, -5);
	glVertex3f(45, -85, 95);
	glVertex3f(45, -75, -5);
	glVertex3f(45, -75, 95);
	glVertex3f(45, -65, -5);
	glVertex3f(45, -65, 95);
	glVertex3f(45, -55, -5);
	glVertex3f(45, -55, 95);
	glVertex3f(45, -45, -5);
	glVertex3f(45, -45, 95);
	glVertex3f(45, -35, -5);
	glVertex3f(45, -35, 95);
	glVertex3f(45, -25, -5);
	glVertex3f(45, -25, 95);
	glVertex3f(45, -15, -5);
	glVertex3f(45, -15, 95);
	glVertex3f(45, -5, -5);
	glVertex3f(45, -5, 95);
	glVertex3f(45, 5, -5);
	glVertex3f(45, 5, 95);
	glVertex3f(45, 15, -5);
	glVertex3f(45, 15, 95);
	glVertex3f(45, 25, -5);
	glVertex3f(45, 25, 95);
	glVertex3f(45, 35, -5);
	glVertex3f(45, 35, 95);
	glVertex3f(45, 45, -5);
	glVertex3f(45, 45, 95);
	glVertex3f(45, 55, -5);
	glVertex3f(45, 55, 95);

	glVertex3f(45, -135, 5);
	glVertex3f(45, 65, 5);
	glVertex3f(45, -135, 15);
	glVertex3f(45, 65, 15);
	glVertex3f(45, -135, 25);
	glVertex3f(45, 65, 25);
	glVertex3f(45, -135, 35);
	glVertex3f(45, 65, 35);
	glVertex3f(45, -135, 45);
	glVertex3f(45, 65, 45);
	glVertex3f(45, -135, 55);
	glVertex3f(45, 65, 55);
	glVertex3f(45, -135, 65);
	glVertex3f(45, 65, 65);
	glVertex3f(45, -135, 75);
	glVertex3f(45, 65, 75);
	glVertex3f(45, -135, 85);
	glVertex3f(45, 65, 85);

	glVertex3f(35, -135, -5);
	glVertex3f(35, 65, -5);
	glVertex3f(25, -135, -5);
	glVertex3f(25, 65, -5);
	glVertex3f(15, -135, -5);
	glVertex3f(15, 65, -5);
	glVertex3f(5, -135, -5);
	glVertex3f(5, 65, -5);
	glVertex3f(-5, -135, -5);
	glVertex3f(-5, 65, -5);
	glVertex3f(-15, -135, -5);
	glVertex3f(-15, 65, -5);
	glVertex3f(-25, -135, -5);
	glVertex3f(-25, 65, -5);
	glVertex3f(-35, -135, -5);
	glVertex3f(-35, 65, -5);
	glVertex3f(-45, -135, -5);
	glVertex3f(-45, 65, -5);

	glVertex3f(45, -125, -5);
	glVertex3f(-55, -125, -5);
	glVertex3f(45, -115, -5);
	glVertex3f(-55, -115, -5);
	glVertex3f(45, -105, -5);
	glVertex3f(-55, -105, -5);
	glVertex3f(45, -95, -5);
	glVertex3f(-55, -95, -5);
	glVertex3f(45, -85, -5);
	glVertex3f(-55, -85, -5);
	glVertex3f(45, -75, -5);
	glVertex3f(-55, -75, -5);
	glVertex3f(45, -65, -5);
	glVertex3f(-55, -65, -5);
	glVertex3f(45, -55, -5);
	glVertex3f(-55, -55, -5);
	glVertex3f(45, -45, -5);
	glVertex3f(-55, -45, -5);
	glVertex3f(45, -35, -5);
	glVertex3f(-55, -35, -5);
	glVertex3f(45, -25, -5);
	glVertex3f(-55, -25, -5);
	glVertex3f(45, -15, -5);
	glVertex3f(-55, -15, -5);
	glVertex3f(45, -5, -5);
	glVertex3f(-55, -5, -5);
	glVertex3f(45, 5, -5);
	glVertex3f(-55, 5, -5);
	glVertex3f(45, 15, -5);
	glVertex3f(-55, 15, -5);
	glVertex3f(45, 25, -5);
	glVertex3f(-55, 25, -5);
	glVertex3f(45, 35, -5);
	glVertex3f(-55, 35, -5);
	glVertex3f(45, 45, -5);
	glVertex3f(-55, 45, -5);
	glVertex3f(45, 55, -5);
	glVertex3f(-55, 55, -5);

	glEnd();
	if (difficulty == 2) {
		glColor3f(1, 1, 0);
		glBegin(GL_LINE_LOOP);
		glVertex3f(-45, 65, 5);
		glVertex3f(35, 65, 5);
		glVertex3f(35, 65, 85);
		glVertex3f(-45, 65, 85);
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex3f(-45, -135, 5);
		glVertex3f(35, -135, 5);
		glVertex3f(35, -135, 85);
		glVertex3f(-45, -135, 85);
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex3f(-45, 65, 5);
		glVertex3f(-45, -135, 5);
		glVertex3f(35, -135, 5);
		glVertex3f(35, 65, 5);
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex3f(-45, 65, 85);
		glVertex3f(-45, -135, 85);
		glVertex3f(35, -135, 85);
		glVertex3f(35, 65, 85);
		glEnd();
	}
	if (difficulty == 3) {
		glColor3f(1, 0, 0);
		glBegin(GL_LINE_LOOP);
		glVertex3f(-35, 65, 15);
		glVertex3f(25, 65, 15);
		glVertex3f(25, 65, 75);
		glVertex3f(-35, 65, 75);
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex3f(-35, -135, 15);
		glVertex3f(25, -135, 15);
		glVertex3f(25, -135, 75);
		glVertex3f(-35, -135, 75);
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex3f(-35, 65, 15);
		glVertex3f(-35, -135, 15);
		glVertex3f(25, -135, 15);
		glVertex3f(25, 65, 15);
		glEnd();

		glBegin(GL_LINE_LOOP);
		glVertex3f(-35, 65, 75);
		glVertex3f(-35, -135, 75);
		glVertex3f(25, -135, 75);
		glVertex3f(25, 65, 75);
		glEnd();
	}

	glColor3f(1, 1, 1);
	glTranslatef(-50, 60, 0);
	for (int i = 0;i<10;i++) {
		for (int j = 0;j<20;j++) {
			for (int k = 0;k<10;k++) {
				if (board[i][j][k] != 0) {
					switch (board[i][j][k]) {
					case 1:
						currentTexID = blueTexID;
						break;
					case 2:
						currentTexID = greenTexID;
						break;
					case 3:
						currentTexID = orangeTexID;
						break;
					case 4:
						currentTexID = pinkTexID;
						break;
					case 5:
						currentTexID = purpleTexID;
						break;
					case 6:
						currentTexID = redTexID;
						break;
					case 7:
						currentTexID = yellowTexID;
						break;
					}
					glPushMatrix();
					glTranslatef(i * 10, -j * 10, k * 10);
					drawxy(cube, planarMapping, currentTexID);
					drawxz(cube, xzMapping, currentTexID);
					drawyz(cube, yzMapping, currentTexID);
					glPopMatrix();
				}
			}
		}
	}

	switch (blockid + 1) {
	case 1:
		currentTexID = blueTexID;
		break;
	case 2:
		currentTexID = greenTexID;
		break;
	case 3:
		currentTexID = orangeTexID;
		break;
	case 4:
		currentTexID = pinkTexID;
		break;
	case 5:
		currentTexID = purpleTexID;
		break;
	case 6:
		currentTexID = redTexID;
		break;
	case 7:
		currentTexID = yellowTexID;
		break;
	}

	glPushMatrix();
	glTranslatef(block[0][0] * 10, -block[0][1] * 10, block[0][2] * 10);
	drawxy(cube, planarMapping, currentTexID);
	drawxz(cube, xzMapping, currentTexID);
	drawyz(cube, yzMapping, currentTexID);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(block[1][0] * 10, -block[1][1] * 10, block[1][2] * 10);
	drawxy(cube, planarMapping, currentTexID);
	drawxz(cube, xzMapping, currentTexID);
	drawyz(cube, yzMapping, currentTexID);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(block[2][0] * 10, -block[2][1] * 10, block[2][2] * 10);
	drawxy(cube, planarMapping, currentTexID);
	drawxz(cube, xzMapping, currentTexID);
	drawyz(cube, yzMapping, currentTexID);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(block[3][0] * 10, -block[3][1] * 10, block[3][2] * 10);
	drawxy(cube, planarMapping, currentTexID);
	drawxz(cube, xzMapping, currentTexID);
	drawyz(cube, yzMapping, currentTexID);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(80, 0, 0);
	switch (nextblockid + 1) {
	case 1:
		currentTexID = blueTexID;
		break;
	case 2:
		currentTexID = greenTexID;
		break;
	case 3:
		currentTexID = orangeTexID;
		break;
	case 4:
		currentTexID = pinkTexID;
		break;
	case 5:
		currentTexID = purpleTexID;
		break;
	case 6:
		currentTexID = redTexID;
		break;
	case 7:
		currentTexID = yellowTexID;
		break;
	}

	glPushMatrix();
	glTranslatef(nextblock[0][0] * 10, -nextblock[0][1] * 10, nextblock[0][2] * 10);
	drawxy(cube, planarMapping, currentTexID);
	drawxz(cube, xzMapping, currentTexID);
	drawyz(cube, yzMapping, currentTexID);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(nextblock[1][0] * 10, -nextblock[1][1] * 10, nextblock[1][2] * 10);
	drawxy(cube, planarMapping, currentTexID);
	drawxz(cube, xzMapping, currentTexID);
	drawyz(cube, yzMapping, currentTexID);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(nextblock[2][0] * 10, -nextblock[2][1] * 10, nextblock[2][2] * 10);
	drawxy(cube, planarMapping, currentTexID);
	drawxz(cube, xzMapping, currentTexID);
	drawyz(cube, yzMapping, currentTexID);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(nextblock[3][0] * 10, -nextblock[3][1] * 10, nextblock[3][2] * 10);
	drawxy(cube, planarMapping, currentTexID);
	drawxz(cube, xzMapping, currentTexID);
	drawyz(cube, yzMapping, currentTexID);
	glPopMatrix();
	glPopMatrix();

	glEnd();
	glPushMatrix();
	glColor3f(1, 1, 1);
	glTranslatef(-140, 70, 0);
	glScalef(0.3, 0.3, 1);
	char* e;

	char c[10] = "lines:";

	string str = "";
	char d[10];
	str += itoa(score, d, 10);
	e = strcat(c, d);
	writeText(e, GLUT_STROKE_ROMAN);
	if (gameover == true) {
		glPushMatrix();
		glTranslatef(-100, -400, 110);
		e = "GAME OVER";
		writeText(e, GLUT_STROKE_ROMAN);
		glPopMatrix();
	}
	glPushMatrix();
	glTranslatef(380, -80, 0);
	e = "NEXT";
	writeText(e, GLUT_STROKE_ROMAN);
	glPopMatrix();
	glPopMatrix();
}
//------------------------------------------------------------------------------

void resize(int w, int h)
{
	// define the visible area of the window ( in pixels )
	if (h == 0) h = 1;
	glViewport(0, 0, w, h);

	// Setup viewing volume

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(90.0, (float)w / (float)h, 1.0, 100.0);

	//////////////////////////////////////////////////////////////////////////
	win_width = w;
	win_height = h;
}

//------------------------------------------------------------------------------

void keyboardDown(unsigned char key, int x, int y)
{
	//*
	switch (key)
	{
	case 'W':
	case 'w':
		cout << "w pressed" << endl;
		
			if (block[2][2]>beginPos && block[0][2]>beginPos && block[1][2]>beginPos && block[3][2]>beginPos
				&& board[block[2][0]][block[2][1]][block[2][2] - 1] == 0 && board[block[3][0]][block[3][1]][block[3][2] - 1] == 0
				&& board[block[0][0]][block[0][1]][block[0][2] - 1] == 0 && board[block[1][0]][block[1][1]][block[1][2] - 1] == 0) {

				block[0][2]--;
				block[1][2]--;
				block[2][2]--;
				block[3][2]--;
			}
		

		break;
	case 'A':
	case 'a':
		cout << "a pressed" << endl;
		
			if (block[2][0]>beginPos && block[0][0]>beginPos && block[1][0]>beginPos && block[3][0]>beginPos
				&& board[block[2][0] - 1][block[2][1]][block[2][2]] == 0 && board[block[3][0] - 1][block[3][1]][block[3][2]] == 0
				&& board[block[0][0] - 1][block[0][1]][block[0][2]] == 0 && board[block[1][0] - 1][block[1][1]][block[1][2]] == 0) {

				block[0][0]--;
				block[1][0]--;
				block[2][0]--;
				block[3][0]--;
			}
		

		break;
	case 'S':
	case 's':
		cout << "s pressed" << endl;
		
			if (block[2][2]<endPos && block[0][2]<endPos && block[1][2]<endPos && block[3][2]<endPos
				&& board[block[2][0]][block[2][1]][block[2][2] + 1] == 0 && board[block[3][0]][block[3][1]][block[3][2] + 1] == 0
				&& board[block[0][0]][block[0][1]][block[0][2] + 1] == 0 && board[block[1][0]][block[1][1]][block[1][2] + 1] == 0) {

				block[0][2]++;
				block[1][2]++;
				block[2][2]++;
				block[3][2]++;
			}
		

		break;
	case 'D':
	case 'd':
		cout << "d pressed" << endl;
		
			if (block[2][0]<endPos && block[0][0]<endPos && block[1][0]<endPos && block[3][0]<endPos
				&& board[block[2][0] + 1][block[2][1]][block[2][2]] == 0 && board[block[3][0] + 1][block[3][1]][block[3][2]] == 0
				&& board[block[0][0] + 1][block[0][1]][block[0][2]] == 0 && board[block[1][0] + 1][block[1][1]][block[1][2]] == 0) {

				block[0][0]++;
				block[1][0]++;
				block[2][0]++;
				block[3][0]++;
			}
		

		break;
	case 'P':
	case 'p':
		if (pause == false) {
			pause = true;
		}
		else {
			pause = false;
		}
		break;
	case '=':
	case '+':
		zoom -= 10;
		break;
	case '-':
	case '_':
		zoom += 10;
		break;
	case 'r':
	case 'R':
		gameover = false;
		score = 0;
		play = 0;
		for (int i = 0;i<10;i++) {
			for (int j = 0;j<20;j++) {
				for (int k = 0;k<10;k++) {
					board[i][j][k] = 0;
				}
			}
		}
		srand(time(NULL));
		blockid = rand() % 7;
		srand(time(NULL));
		nextblockid = rand() % 7;
		newBlock();

		break;
	case 'z':
	case 'Z':

		midx = (block[0][0] + block[1][0] + block[2][0] + block[3][0]) / 4;
		midy = (block[0][1] + block[1][1] + block[2][1] + block[3][1]) / 4;
		x1 = block[0][0] - midx;
		x2 = block[1][0] - midx;
		x3 = block[2][0] - midx;
		x4 = block[3][0] - midx;
		y10 = block[0][1] - midy;
		y2 = block[1][1] - midy;
		y3 = block[2][1] - midy;
		y4 = block[3][1] - midy;

		tmp = x1;
		x1 = y10;
		y10 = -tmp;
		tmp = x2;
		x2 = y2;
		y2 = -tmp;
		tmp = x3;
		x3 = y3;
		y3 = -tmp;
		tmp = x4;
		x4 = y4;
		y4 = -tmp;
		
			if ((int)(x1 + midx) >= beginPos && (int)(x1 + midx) <= endPos && (int)(x2 + midx) >= beginPos && (int)(x2 + midx) <= endPos && (int)(x3 + midx) >= beginPos && (int)(x3 + midx) <= endPos && (int)(x4 + midx) >= beginPos && (int)(x4 + midx) <= endPos &&
				(int)(y10 + midy + 1) >= 0 && (int)(y10 + midy + 1) <= 19 && (int)(y2 + midy + 1) >= 0 && (int)(y2 + midy + 1) <= 19 && (int)(y3 + midy + 1) >= 0 && (int)(y3 + midy + 1) <= 19 && (int)(y4 + midy + 1) >= 0 && (int)(y4 + midy + 1) <= 19 &&
				board[(int)(x1 + midx)][(int)(y10 + midy + 1)][block[0][2]] == 0 &&
				board[(int)(x2 + midx)][(int)(y2 + midy + 1)][block[1][2]] == 0 &&
				board[(int)(x3 + midx)][(int)(y3 + midy + 1)][block[2][2]] == 0 &&
				board[(int)(x4 + midx)][(int)(y4 + midy + 1)][block[3][2]] == 0) {
				block[0][0] = (int)(x1 + midx);
				block[1][0] = (int)(x2 + midx);
				block[2][0] = (int)(x3 + midx);
				block[3][0] = (int)(x4 + midx);
				block[0][1] = (int)(y10 + midy + 1);
				block[1][1] = (int)(y2 + midy + 1);
				block[2][1] = (int)(y3 + midy + 1);
				block[3][1] = (int)(y4 + midy + 1);
			}
		
		break;
	case 'x':
	case 'X':

		midx = (block[0][0] + block[1][0] + block[2][0] + block[3][0]) / 4;
		midz = (block[0][2] + block[1][2] + block[2][2] + block[3][2]) / 4;
		x1 = block[0][0] - midx;
		x2 = block[1][0] - midx;
		x3 = block[2][0] - midx;
		x4 = block[3][0] - midx;
		z1 = block[0][2] - midz;
		z2 = block[1][2] - midz;
		z3 = block[2][2] - midz;
		z4 = block[3][2] - midz;

		tmp = x1;
		x1 = z1;
		z1 = -tmp;
		tmp = x2;
		x2 = z2;
		z2 = -tmp;
		tmp = x3;
		x3 = z3;
		z3 = -tmp;
		tmp = x4;
		x4 = z4;
		z4 = -tmp;
		
			if ((int)(x1 + midx) >= beginPos && (int)(x1 + midx) <= endPos && (int)(x2 + midx) >= beginPos && (int)(x2 + midx) <= endPos && (int)(x3 + midx) >= beginPos && (int)(x3 + midx) <= endPos && (int)(x4 + midx) >= beginPos && (int)(x4 + midx) <= endPos &&
				(int)(z1 + midz) >= beginPos && (int)(z1 + midz) <= endPos && (int)(z2 + midz) >= beginPos && (int)(z2 + midz) <= endPos && (int)(z3 + midz) >= beginPos && (int)(z3 + midz) <= endPos && (int)(z4 + midz) >= beginPos && (int)(z4 + midz) <= endPos &&
				board[(int)(x1 + midx)][block[0][1]][(int)(z1 + midz)] == 0 &&
				board[(int)(x2 + midx)][block[1][1]][(int)(z2 + midz)] == 0 &&
				board[(int)(x3 + midx)][block[2][1]][(int)(z3 + midz)] == 0 &&
				board[(int)(x4 + midx)][block[3][1]][(int)(z4 + midz)] == 0) {
				block[0][0] = (int)(x1 + midx);
				block[1][0] = (int)(x2 + midx);
				block[2][0] = (int)(x3 + midx);
				block[3][0] = (int)(x4 + midx);
				block[0][2] = (int)(z1 + midz);
				block[1][2] = (int)(z2 + midz);
				block[2][2] = (int)(z3 + midz);
				block[3][2] = (int)(z4 + midz);
			}
		
		break;
	case 'c':
	case 'C':

		midy = (block[0][1] + block[1][1] + block[2][1] + block[3][1]) / 4;
		midz = (block[0][2] + block[1][2] + block[2][2] + block[3][2]) / 4;
		y10 = block[0][1] - midy;
		y2 = block[1][1] - midy;
		y3 = block[2][1] - midy;
		y4 = block[3][1] - midy;
		z1 = block[0][2] - midz;
		z2 = block[1][2] - midz;
		z3 = block[2][2] - midz;
		z4 = block[3][2] - midz;

		tmp = y10;
		y10 = z1;
		z1 = -tmp;
		tmp = y2;
		y2 = z2;
		z2 = -tmp;
		tmp = y3;
		y3 = z3;
		z3 = -tmp;
		tmp = y4;
		y4 = z4;
		z4 = -tmp;
		
			if ((int)(y10 + midy + 1) >= 0 && (int)(y10 + midy + 1) <= 19 && (int)(y2 + midy + 1) >= 0 && (int)(y2 + midy + 1) <= 19 && (int)(y3 + midy + 1) >= 0 && (int)(y3 + midy + 1) <= 19 && (int)(y4 + midy + 1) >= 0 && (int)(y4 + midy + 1) <= 19 &&
				(int)(z1 + midz) >= beginPos && (int)(z1 + midz) <= endPos && (int)(z2 + midz) >= beginPos && (int)(z2 + midz) <= endPos && (int)(z3 + midz) >= beginPos && (int)(z3 + midz) <= endPos && (int)(z4 + midz) >= beginPos && (int)(z4 + midz) <= endPos &&
				board[block[0][0]][(int)(y10 + midy + 1)][(int)(z1 + midz)] == 0 &&
				board[block[1][0]][(int)(y2 + midy + 1)][(int)(z2 + midz)] == 0 &&
				board[block[2][0]][(int)(y3 + midy + 1)][(int)(z3 + midz)] == 0 &&
				board[block[3][0]][(int)(y4 + midy + 1)][(int)(z4 + midz)] == 0) {
				block[0][1] = (int)(y10 + midy + 1);
				block[1][1] = (int)(y2 + midy + 1);
				block[2][1] = (int)(y3 + midy + 1);
				block[3][1] = (int)(y4 + midy + 1);
				block[0][2] = (int)(z1 + midz);
				block[1][2] = (int)(z2 + midz);
				block[2][2] = (int)(z3 + midz);
				block[3][2] = (int)(z4 + midz);
			}
		
		break;
	case 'H':
	case 'h':
		if (difficulty == 1) {
			difficulty = 2;
			beginPos = 1;
			endPos = 8;
			boardSize = 64;
			gameover = false;
			score = 0;
			play = 0;
			for (int i = 0;i<10;i++) {
				for (int j = 0;j<20;j++) {
					for (int k = 0;k<10;k++) {
						board[i][j][k] = 0;
					}
				}
			}
			srand(time(NULL));
			blockid = rand() % 7;
			srand(time(NULL));
			nextblockid = rand() % 7;
			newBlock();
		}
		else if (difficulty == 2) {
			difficulty = 3;
			beginPos = 2;
			endPos = 7;
			boardSize = 36;
			gameover = false;
			score = 0;
			play = 0;
			for (int i = 0;i<10;i++) {
				for (int j = 0;j<20;j++) {
					for (int k = 0;k<10;k++) {
						board[i][j][k] = 0;
					}
				}
			}
			srand(time(NULL));
			blockid = rand() % 7;
			srand(time(NULL));
			nextblockid = rand() % 7;
			newBlock();
		}
		else if (difficulty == 3) {
			difficulty = 1;
			beginPos = 0;
			endPos = 9;
			boardSize = 100;
			gameover = false;
			score = 0;
			play = 0;
			for (int i = 0;i<10;i++) {
				for (int j = 0;j<20;j++) {
					for (int k = 0;k<10;k++) {
						board[i][j][k] = 0;
					}
				}
			}
			srand(time(NULL));
			blockid = rand() % 7;
			srand(time(NULL));
			nextblockid = rand() % 7;
			newBlock();
		}
		break;
	case 'E':
	case 'e':
		while (block[2][1]<19 && block[0][1]<19 && block[1][1]<19 && block[3][1]<19
			&& board[block[2][0]][block[2][1] + 1][block[2][2]] == 0 && board[block[3][0]][block[3][1] + 1][block[3][2]] == 0
			&& board[block[0][0]][block[0][1] + 1][block[0][2]] == 0 && board[block[1][0]][block[1][1] + 1][block[1][2]] == 0) {
			block[0][1]++;
			block[1][1]++;
			block[2][1]++;
			block[3][1]++;
		}
		break;
	case KEY_ESC:
		exit(0);
		break;
	default:
		break;
	}
	glutPostRedisplay();
	//*/
}

//------------------------------------------------------------------------------

void keyboardUp(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'W':
	case 'w':
		cout << "w released" << endl;
		break;
	case 'A':
	case 'a':
		cout << "a released" << endl;
		break;
	case 'S':
	case 's':
		cout << "s released" << endl;
		break;
	case 'D':
	case 'd':
		cout << "d released" << endl;
		break;
	case KEY_ESC:
		exit(0);
		break;
	default:
		break;
	}
	glutPostRedisplay();

}

//------------------------------------------------------------------------------

void mouseClick(int button, int state, int x, int y)
{

	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			cout << "mouse click down" << endl;
			// Set click down state flag true  
			mouseClickDown = true;
			// Save initial mouse position
			mx0 = x;
			my0 = y;
		}
		else
		{
			cout << "mouse click up" << endl;
			// Set click down state flag false
			mouseClickDown = false;
		}
	}

}

//------------------------------------------------------------------------------

void mouseMotion(int x, int y)
{
	if (mouseClickDown)
	{
		// Simple FPS Camera
		//azim += (x - mx0)*SENSITIVITY/10.0f;
		//elev += (y - my0)*SENSITIVITY/10.0f;
		cameraDist += (y - my0) * zoomFactor;
		camx += (x - mx0) * zoomFactor;
		// Save current mouse position
		mx0 = x;
		my0 = y;

		cout << "mouse moving" << endl;
		glutPostRedisplay();
	}
}

void drawBox(float xsz, float ysz, float zsz, float norm_sign)
{
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glScalef(xsz * 0.5, ysz * 0.5, zsz * 0.5);

	if (norm_sign < 0.0) {
		glFrontFace(GL_CW);
	}

	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1 * norm_sign);
	glTexCoord2f(0, 0); glVertex3f(-1, -1, 1);
	glTexCoord2f(1, 0); glVertex3f(1, -1, 1);
	glTexCoord2f(1, 1); glVertex3f(1, 1, 1);
	glTexCoord2f(0, 1); glVertex3f(-1, 1, 1);
	glNormal3f(1 * norm_sign, 0, 0);
	glTexCoord2f(0, 0); glVertex3f(1, -1, 1);
	glTexCoord2f(1, 0); glVertex3f(1, -1, -1);
	glTexCoord2f(1, 1); glVertex3f(1, 1, -1);
	glTexCoord2f(0, 1); glVertex3f(1, 1, 1);
	glNormal3f(0, 0, -1 * norm_sign);
	glTexCoord2f(0, 0); glVertex3f(1, -1, -1);
	glTexCoord2f(1, 0); glVertex3f(-1, -1, -1);
	glTexCoord2f(1, 1); glVertex3f(-1, 1, -1);
	glTexCoord2f(0, 1); glVertex3f(1, 1, -1);
	glNormal3f(-1 * norm_sign, 0, 0);
	glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
	glTexCoord2f(1, 0); glVertex3f(-1, -1, 1);
	glTexCoord2f(1, 1); glVertex3f(-1, 1, 1);
	glTexCoord2f(0, 1); glVertex3f(-1, 1, -1);
	glEnd();
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0, 1 * norm_sign, 0);
	glTexCoord2f(0.5, 0.5); glVertex3f(0, 1, 0);
	glTexCoord2f(0, 0); glVertex3f(-1, 1, 1);
	glTexCoord2f(1, 0); glVertex3f(1, 1, 1);
	glTexCoord2f(1, 1); glVertex3f(1, 1, -1);
	glTexCoord2f(0, 1); glVertex3f(-1, 1, -1);
	glTexCoord2f(0, 0); glVertex3f(-1, 1, 1);
	glEnd();
	glBegin(GL_TRIANGLE_FAN);
	glNormal3f(0, -1 * norm_sign, 0);
	glTexCoord2f(0.5, 0.5); glVertex3f(0, -1, 0);
	glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
	glTexCoord2f(1, 0); glVertex3f(1, -1, -1);
	glTexCoord2f(1, 1); glVertex3f(1, -1, 1);
	glTexCoord2f(0, 1); glVertex3f(-1, -1, 1);
	glTexCoord2f(0, 0); glVertex3f(-1, -1, -1);
	glEnd();

	glFrontFace(GL_CCW);
	glPopMatrix();
}


/* generate a chessboard texture with tiles colored (r0, g0, b0) and (r1, g1, b1) */
unsigned int genChessTexture(float r0, float g0, float b0, float r1, float g1, float b1)
{
	int i, j;
	unsigned int tex;
	unsigned char img[8 * 8 * 3];
	unsigned char *pix = img;

	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			int black = (i & 1) == (j & 1);
			pix[0] = (black ? r0 : r1) * 255;
			pix[1] = (black ? g0 : g1) * 255;
			pix[2] = (black ? b0 : b1) * 255;
			pix += 3;
		}
	}

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 8, 8, 0, GL_RGB, GL_UNSIGNED_BYTE, img);

	return tex;
}

unsigned int nextPow2(unsigned int x)
{
	x -= 1;
	x |= x >> 1;
	x |= x >> 2;
	x |= x >> 4;
	x |= x >> 8;
	x |= x >> 16;
	return x + 1;
}

void idle(int value)
//void idle()
{
	ovrFrameTiming   ftiming = ovr_GetFrameTiming(hmd, 0);
	hmdState = ovr_GetTrackingState(hmd, ftiming.DisplayMidpointSeconds);

	glMatrixMode(GL_MODELVIEW);
	if (pause == false && (play%difficulty) == 0) {

		if (block[2][1]<19 && block[0][1]<19 && block[1][1]<19 && block[3][1]<19
			&& board[block[2][0]][block[2][1] + 1][block[2][2]] == 0 && board[block[3][0]][block[3][1] + 1][block[3][2]] == 0
			&& board[block[0][0]][block[0][1] + 1][block[0][2]] == 0 && board[block[1][0]][block[1][1] + 1][block[1][2]] == 0) {

			block[0][1]++;
			block[1][1]++;
			block[2][1]++;
			block[3][1]++;
		}
		else {
			if (board[4][0][5] != 0 || board[5][0][5] != 0 || board[6][0][5] != 0) { gameover = true; }
			else {
				board[block[0][0]][block[0][1]][block[0][2]] = (blockid + 1);
				board[block[1][0]][block[1][1]][block[1][2]] = (blockid + 1);
				board[block[2][0]][block[2][1]][block[2][2]] = (blockid + 1);
				board[block[3][0]][block[3][1]][block[3][2]] = (blockid + 1);
				newBlock();
			}
		}
	}
	// Make your changes here

	play++;
	//*/
	
		for (int control = 0;control<20;control++) {
			for (int x = 0;x<10;x++) {
				for (int z = 0;z<10;z++) {
					if (board[x][control][z] != 0) {
						full++;
					}
				}
			}
			if (full == boardSize) {
				score++;
				for (int y = control;y>0;y--) {
					for (int x = 0;x<10;x++) {
						for (int z = 0;z<10;z++) {
							board[x][y][z] = board[x][y - 1][z];
						}
					}
				}
			}
			full = 0;
		}
	


	glutPostRedisplay();
	glutTimerFunc(1000, idle, 0);
}
