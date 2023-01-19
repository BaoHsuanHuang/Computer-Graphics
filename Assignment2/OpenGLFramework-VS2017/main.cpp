#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"

#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

using namespace std;

// Default window size
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;

//=============================================
int new_WIDTH = WINDOW_WIDTH;
int new_HEIGHT = WINDOW_WIDTH;
int draw = 0;;
float shininess = 64;
enum LightMode
{
	DirectionalLight = 0,
	PointLight = 1,
	SpotLight = 2,
};
LightMode cur_light_mode = DirectionalLight;

struct directional
{
	Vector3 position = Vector3(1, 1, 1);
	Vector3 direction = Vector3(0, 0, 0);
	Vector3 diffuse = Vector3(1, 1, 1);
	Vector3 ambient = Vector3(0.15, 0.15, 0.15);
	Vector3 specular = Vector3(1, 1, 1);
};
directional Directional;

struct point
{
	Vector3 position = Vector3(0, 2, 1);
	Vector3 diffuse = Vector3(1, 1, 1);
	Vector3 ambient = Vector3(0.15, 0.15, 0.15);
	Vector3 specular = Vector3(1, 1, 1);
	GLfloat attenuation_constant = 0.01;
	GLfloat attenuation_linear = 0.8;
	GLfloat attenuation_quadratic = 0.1;
};
point Point;

struct spot
{
	Vector3 position = Vector3(0, 0, 2);
	Vector3 direction = Vector3(0, 0, -1);
	Vector3 diffuse = Vector3(1, 1, 1);
	Vector3 ambient = Vector3(0.15, 0.15, 0.15);
	Vector3 specular = Vector3(1, 1, 1);
	GLfloat exponent = 50;
	GLfloat cutoff = 30;
	GLfloat attenuation_constant = 0.05;
	GLfloat attenuation_linear = 0.3;
	GLfloat attenuation_quadratic = 0.6;
};
spot Spot;
//==============================================

enum TransMode
{
	GeoTranslation = 0,
	GeoRotation = 1,
	GeoScaling = 2,
	LightEdit = 3,
	ShininessEdit = 4,
};

struct Uniform
{
	GLint iLocMVP;
	// =================================
	GLint iLocView;
	GLint iLocLightMode;
	GLint iLocDraw;
	GLint iLocShininess;
	GLint iLocKa;
	GLint iLocKd;
	GLint iLocKs;
	GLint iLocPos_d;
	GLint iLocPos_p;
	GLint iLocPos_s;
	GLint iLocDiff_d;
	GLint iLocDiff_p;
	GLint iLocDiff_s;
	GLint iLocAmbient_d;
	GLint iLocAmbient_p;
	GLint iLocAmbient_s;
	GLint iLocSpec_d;
	GLint iLocSpec_p;
	GLint iLocSpec_s;
	GLint iLocAtte_constant_p;
	GLint iLocAtte_linear_p;
	GLint iLocAtte_quadratic_p;
	GLint iLocAtte_constant_s;
	GLint iLocAtte_linear_s;
	GLint iLocAtte_quadratic_s;
	GLint iLocDirection_s;
	GLint iLocCutoff_s;
	GLint iLocExponent_s;
	GLint iLocOriginalViewMatrix;
	GLint iLocCamera;
	GLint iLocTRS;
	// =================================
};
Uniform uniform;

vector<string> filenames; // .obj filename list

struct PhongMaterial
{
	Vector3 Ka;
	Vector3 Kd;
	Vector3 Ks;
};

typedef struct
{
	GLuint vao;
	GLuint vbo;
	GLuint vboTex;
	GLuint ebo;
	GLuint p_color;
	int vertex_count;
	GLuint p_normal;
	PhongMaterial material;
	int indexCount;
	GLuint m_texture;
} Shape;

struct model
{
	Vector3 position = Vector3(0, 0, 0);
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 rotation = Vector3(0, 0, 0);	// Euler form

	vector<Shape> shapes;
};
vector<model> models;

struct camera
{
	Vector3 position;
	Vector3 center;
	Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
	GLfloat nearClip, farClip;
	GLfloat fovy;
	GLfloat aspect;
	GLfloat left, right, top, bottom;
};
project_setting proj;

TransMode cur_trans_mode = GeoTranslation;

Matrix4 view_matrix;
Matrix4 project_matrix;

int cur_idx = 0; // represent which model should be rendered now


static GLvoid Normalize(GLfloat v[3])
{
	GLfloat l;

	l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] /= l;
	v[1] /= l;
	v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{

	n[0] = u[1] * v[2] - u[2] * v[1];
	n[1] = u[2] * v[0] - u[0] * v[2];
	n[2] = u[0] * v[1] - u[1] * v[0];
}


// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
	Matrix4 mat;
	//===============================
	mat = Matrix4(
		1, 0, 0, vec.x,
		0, 1, 0, vec.y,
		0, 0, 1, vec.z,
		0, 0, 0, 1
	);
	//===============================
	return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
	Matrix4 mat;
	//===============================
	mat = Matrix4(
		vec.x, 0, 0, 0,
		0, vec.y, 0, 0,
		0, 0, vec.z, 0,
		0, 0, 0, 1
	);
	//===============================
	return mat;
}


// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
	Matrix4 mat;
	//===============================
	mat = Matrix4(
		1, 0, 0, 0,
		0, cos(val), -sin(val), 0,
		0, sin(val), cos(val), 0,
		0, 0, 0, 1
	);
	//===============================
	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
	Matrix4 mat;
	//===============================
	mat = Matrix4(
		cos(val), 0, sin(val), 0,
		0, 1, 0, 0,
		-sin(val), 0, cos(val), 0,
		0, 0, 0, 1
	);
	//===============================
	return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
	Matrix4 mat;
	//===============================
	mat = Matrix4(
		cos(val), -sin(val), 0, 0,
		sin(val), cos(val), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	);
	//===============================
	return mat;
}

Matrix4 rotate(Vector3 vec)
{
	return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
	// view_matrix[...] = ...==========================
	// world space -> camera space
	Matrix4 A;
	Matrix4 B;
	Vector3 p1p2 = main_camera.center - main_camera.position;
	//Vector3 p1p3 = main_camera.up_vector - main_camera.position;
	Vector3 p1p3 = main_camera.up_vector;
	GLfloat Rx[3], Ry[3], Rz[3];
	GLfloat tmp[3], tmp2[3];

	tmp[0] = p1p2.x;
	tmp[1] = p1p2.y;
	tmp[2] = p1p2.z;

	tmp2[0] = p1p3.x;
	tmp2[1] = p1p3.y;
	tmp2[2] = p1p3.z;

	// Rx
	Cross(tmp, tmp2, Rx);
	Normalize(Rx);

	// Rz
	Normalize(tmp);
	Rz[0] = (float)(-1)*tmp[0];
	Rz[1] = (float)(-1)*tmp[1];
	Rz[2] = (float)(-1)*tmp[2];

	// Ry
	Cross(Rz, Rx, Ry);

	A = Matrix4(
		Rx[0], Rx[1], Rx[2], 0,
		Ry[0], Ry[1], Ry[2], 0,
		Rz[0], Rz[1], Rz[2], 0,
		0, 0, 0, 1
	);

	// translate camera position to the origin
	B = Matrix4(
		1, 0, 0, (float)(-1)*main_camera.position.x,
		0, 1, 0, (float)(-1)*main_camera.position.y,
		0, 0, 1, (float)(-1)*main_camera.position.z,
		0, 0, 0, 1
	);

	view_matrix = A * B;
	//================================================
}

// [TODO] compute persepective projection matrix
void setPerspective()
{
	// GLfloat f = ...
	// project_matrix [...] = ... ===============================
	GLfloat angle;
	GLfloat ar;
	GLfloat n, f;
	GLfloat radian = proj.fovy / 180.0 * 3.1415926;
	angle = tan(radian / 2);
	ar = proj.aspect;
	n = proj.nearClip;
	f = proj.farClip;

	//if (proj.aspect > 1) {
	//	project_matrix = Matrix4(
	//		1 / (ar*angle), 0, 0, 0,
	//		0, 1 / angle, 0, 0,
	//		0, 0, -(f + n) / (f - n), -2 * f*n / (f - n),
	//		0, 0, -1, 0
	//	);
	//}
	//else {
	//	project_matrix = Matrix4(
	//		1 / angle, 0, 0, 0,
	//		0, ar / angle, 0, 0,
	//		0, 0, -(f + n) / (f - n), -2 * f*n / (f - n),
	//		0, 0, -1, 0
	//	);
	//}
	project_matrix = Matrix4(
		1 / (ar*angle), 0, 0, 0,
		0, 1 / angle, 0, 0,
		0, 0, -(f + n) / (f - n), -2 * f*n / (f - n),
		0, 0, -1, 0
	);
	//=============================================================
}

void setGLMatrix(GLfloat* glm, Matrix4& m) {
	glm[0] = m[0];  glm[4] = m[1];   glm[8] = m[2];    glm[12] = m[3];
	glm[1] = m[4];  glm[5] = m[5];   glm[9] = m[6];    glm[13] = m[7];
	glm[2] = m[8];  glm[6] = m[9];   glm[10] = m[10];   glm[14] = m[11];
	glm[3] = m[12];  glm[7] = m[13];  glm[11] = m[14];   glm[15] = m[15];
}

// Vertex buffers
GLuint VAO, VBO;

// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
	if (height == 0) height = 1;
	glViewport(0, 0, width, height);
	// [TODO] change your aspect ratio=========
	// default size (800x800)
	// glViewport : bottom-left point
	//glm::ortho(float left, float right, float bottom, float top, float zNear, float zFar)
	//glm::perspective(float fovy, float aspect, float zNear, float zFar)
	GLfloat half_width = (float)width / 2;
	new_WIDTH = width;
	new_HEIGHT = height;
	proj.aspect = (float)half_width / (float)height;
	setPerspective();
	//========================================
}


// Render function for display rendering
void RenderScene(void) {	
	// clear canvas
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	Matrix4 T, R, S;
	// [TODO] update translation, rotation and scaling ============
	T = translate(models[cur_idx].position);
	R = rotate(models[cur_idx].rotation);
	S = scaling(models[cur_idx].scale);
	//=============================================================

	Matrix4 MVP;
	GLfloat mvp[16];

	// [TODO] multiply all the matrix ============================
	// row-major ---> column-major
	MVP = project_matrix * view_matrix * T * R * S;
	setGLMatrix(mvp, MVP);

	Matrix4 viewMatrix;
	GLfloat view[16];
	viewMatrix = project_matrix * view_matrix;
	setGLMatrix(view, viewMatrix);
	//setGLMatrix(view, view_matrix);
	GLfloat view_original[16];
	view_original[0] = view_matrix[0]; view_original[1] = view_matrix[1]; view_original[2] = view_matrix[2]; view_original[3] = view_matrix[3];
	view_original[4] = view_matrix[4]; view_original[5] = view_matrix[5]; view_original[6] = view_matrix[6]; view_original[7] = view_matrix[7];
	view_original[8] = view_matrix[8]; view_original[9] = view_matrix[9]; view_original[10] = view_matrix[10]; view_original[11] = view_matrix[11];
	view_original[12] = view_matrix[12]; view_original[13] = view_matrix[13]; view_original[14] = view_matrix[14]; view_original[15] = view_matrix[15];

	Matrix4 trsMatrix = T * R * S;
	GLfloat trs[16];
	setGLMatrix(trs, trsMatrix);

	//============================================================

	// use uniform to send mvp to vertex shader
	// uniform.iLocMVP : location in shader file
	// 1 : the number of matrices assigned
	// GL_FALSE : transpose or not
	// (Shapes, Material): (1, 1) (1, 1) (1, 1) (1, 4) (1, 1)
	PhongMaterial cur_material = models[cur_idx].shapes[0].material;
	glUniformMatrix4fv(uniform.iLocMVP, 1, GL_FALSE, mvp);
	//glUniformMatrix4fv(uniform.iLocView, 1, GL_FALSE, view_matrix.getTranspose());
	glUniformMatrix4fv(uniform.iLocView, 1, GL_FALSE, view);
	glUniformMatrix4fv(uniform.iLocOriginalViewMatrix, 1, GL_FALSE, view_original);
	glUniformMatrix4fv(uniform.iLocTRS, 1, GL_FALSE, trs);

	glUniform1i(uniform.iLocLightMode, cur_light_mode);
	glUniform3f(uniform.iLocKa, cur_material.Ka.x, cur_material.Ka.y, cur_material.Ka.z);
	glUniform3f(uniform.iLocKd, cur_material.Kd.x, cur_material.Kd.y, cur_material.Kd.z);
	glUniform3f(uniform.iLocKs, cur_material.Ks.x, cur_material.Ks.y, cur_material.Ks.z);
	glUniform3f(uniform.iLocPos_d, Directional.position.x, Directional.position.y, Directional.position.z);
	glUniform3f(uniform.iLocPos_p, Point.position.x, Point.position.y, Point.position.z);
	glUniform3f(uniform.iLocPos_s, Spot.position.x, Spot.position.y, Spot.position.z);
	glUniform1f(uniform.iLocShininess, shininess);
	glUniform3f(uniform.iLocDiff_d, Directional.diffuse.x, Directional.diffuse.y, Directional.diffuse.z);
	glUniform3f(uniform.iLocDiff_p, Point.diffuse.x, Point.diffuse.y, Point.diffuse.z);
	glUniform3f(uniform.iLocDiff_s, Spot.diffuse.x, Spot.diffuse.y, Spot.diffuse.z);
	glUniform3f(uniform.iLocAmbient_d, Directional.ambient.x, Directional.ambient.y, Directional.ambient.z);
	glUniform3f(uniform.iLocAmbient_p, Point.ambient.x, Point.ambient.y, Point.ambient.z);
	glUniform3f(uniform.iLocAmbient_s, Spot.ambient.x, Spot.ambient.y, Spot.ambient.z);
	glUniform3f(uniform.iLocSpec_d, Directional.specular.x, Directional.specular.y, Directional.specular.z);
	glUniform3f(uniform.iLocSpec_p, Point.specular.x, Point.specular.y, Point.specular.z);
	glUniform3f(uniform.iLocSpec_s, Spot.specular.x, Spot.specular.y, Spot.specular.z);
	glUniform1f(uniform.iLocAtte_constant_p, Point.attenuation_constant);
	glUniform1f(uniform.iLocAtte_linear_p, Point.attenuation_linear);
	glUniform1f(uniform.iLocAtte_quadratic_p, Point.attenuation_quadratic);
	glUniform1f(uniform.iLocAtte_constant_s, Spot.attenuation_constant);
	glUniform1f(uniform.iLocAtte_linear_s, Spot.attenuation_linear);
	glUniform1f(uniform.iLocAtte_quadratic_s, Spot.attenuation_quadratic);
	glUniform3f(uniform.iLocDirection_s, Spot.direction.x, Spot.direction.y, Spot.direction.z);
	glUniform1f(uniform.iLocCutoff_s, Spot.cutoff);
	glUniform1f(uniform.iLocExponent_s, Spot.exponent);
	glUniform3f(uniform.iLocCamera, main_camera.position.x, main_camera.position.y, main_camera.position.z);


	// Per-Vertex
	draw = 0;
	glUniform1i(uniform.iLocDraw, draw);
	glViewport(0, 0, new_WIDTH / 2, new_HEIGHT);
	for (int i = 0; i < models[cur_idx].shapes.size(); i++) 
	{
		// set glViewport and draw twice ... 
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
	}

	// Per-Pixel
	draw = 1;
	glUniform1i(uniform.iLocDraw, draw);
	glViewport(new_WIDTH / 2, 0, new_WIDTH / 2, new_HEIGHT);
	for (int i = 0; i < models[cur_idx].shapes.size(); i++)
	{
		// set glViewport and draw twice ... 
		glBindVertexArray(models[cur_idx].shapes[i].vao);
		glDrawArrays(GL_TRIANGLES, 0, models[cur_idx].shapes[i].vertex_count);
	}
}

void printInformation() {
	//Translation Matrix, Rotation Matrix, Scaling Matrix, Viewing Matrix, Projection Matrix
	cout << "cur_idx: " << cur_idx << endl;
	cout << "cur_light_mode: " << cur_light_mode << endl;
	cout << "Translation Matrix: " << endl;
	cout << translate(models[cur_idx].position) << endl;
	cout << "Rotation Matrix: " << endl;
	cout << rotate(models[cur_idx].rotation) << endl;
	cout << "Scaling Matrix: " << endl;
	cout << scaling(models[cur_idx].scale) << endl;
	cout << "===============================================" << endl;
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// [TODO] Call back function for keyboard ===========
	if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
		cur_idx = (cur_idx + 1) % 5;
	}
	else if (key == GLFW_KEY_X && action == GLFW_PRESS) {
		if (cur_idx == 0) cur_idx = 4;
		else cur_idx = cur_idx - 1;
	}
	else if (key == GLFW_KEY_T && action == GLFW_PRESS) {
		cur_trans_mode = GeoTranslation;
	}
	else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
		cur_trans_mode = GeoScaling;
	}
	else if (key == GLFW_KEY_R && action == GLFW_PRESS) {
		cur_trans_mode = GeoRotation;
	}
	else if (key == GLFW_KEY_L && action == GLFW_PRESS) {
		if (cur_light_mode == DirectionalLight) {
			cur_light_mode = PointLight;
			cout << "cur_light_mode : PointLight" << endl;
		}
		else if (cur_light_mode == PointLight) {
			cur_light_mode = SpotLight;
			cout << "cur_light_mode : SpotLight" << endl;
		}
		else if (cur_light_mode == SpotLight) {
			cur_light_mode = DirectionalLight;
			cout << "cur_light_mode : DirectionalLight" << endl;
		}
	}
	else if (key == GLFW_KEY_K && action == GLFW_PRESS) {
		cur_trans_mode = LightEdit;
	}
	else if (key == GLFW_KEY_J && action == GLFW_PRESS) {
		cur_trans_mode = ShininessEdit;
	}
	else if (key == GLFW_KEY_I && action == GLFW_PRESS) {
		printInformation();
	}
	// ===================================================
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// [TODO] scroll up positive, otherwise it would be negtive
	if (cur_trans_mode == GeoTranslation) {
		models[cur_idx].position.z += yoffset * 0.05;
	}
	else if (cur_trans_mode == GeoRotation) {
		models[cur_idx].rotation.z += yoffset * 0.05;
	}
	else if (cur_trans_mode == GeoScaling) {
		models[cur_idx].scale.z += yoffset * 0.05;
	}
	else if (cur_trans_mode == LightEdit) {
		if (cur_light_mode == DirectionalLight) {
			Directional.diffuse += Vector3(0.1, 0.1, 0.1) * yoffset;
		}
		else if (cur_light_mode == PointLight) {
			Point.diffuse += Vector3(0.1, 0.1, 0.1) * yoffset;
		}
		else if (cur_light_mode == SpotLight) {
			Spot.cutoff += yoffset * 1.0;
		}
	}
	else if (cur_trans_mode == ShininessEdit) {
		shininess += yoffset * 3.0;
	}
	// ===========================================================
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// [TODO] mouse press callback function
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		mouse_pressed = true;
	}
	else {
		mouse_pressed = false;
		starting_press_x = -1;
		starting_press_y = -1;
	}
	// =====================================================
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	// [TODO] cursor position callback function =======
	if (starting_press_x == -1 && starting_press_y == -1) {
		starting_press_x = xpos;
		starting_press_y = ypos;
	}
	if (mouse_pressed == true) {
		float diffx = xpos - starting_press_x;
		float diffy = ypos - starting_press_y;
		if (cur_trans_mode == GeoTranslation) {
			models[cur_idx].position.x += diffx * 0.008;
			models[cur_idx].position.y -= diffy * 0.008;
		}
		if (cur_trans_mode == GeoRotation) {
			models[cur_idx].rotation.x -= diffy * 0.008;
			models[cur_idx].rotation.y -= diffx * 0.008;
		}
		if (cur_trans_mode == GeoScaling) {
			models[cur_idx].scale.x += diffx * 0.006;
			models[cur_idx].scale.y -= diffy * 0.006;
		}
		if (cur_trans_mode == LightEdit) {
			if (cur_light_mode == DirectionalLight) {
				Directional.position.x += diffx * 0.006;
				Directional.position.y -= diffy * 0.006;
			}
			else if (cur_light_mode == PointLight) {
				Point.position.x += diffx * 0.006;
				Point.position.y -= diffy * 0.006;
			}
			else if (cur_light_mode == SpotLight) {
				Spot.position.x += diffx * 0.006;
				Spot.position.y -= diffy * 0.006;
			}
		}
		if (cur_trans_mode == ShininessEdit) {
			// do nothing
		}
	}
	starting_press_x = xpos;
	starting_press_y = ypos;
}

void setShaders()
{
	GLuint v, f, p;
	char *vs = NULL;
	char *fs = NULL;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead("shader.vs");
	fs = textFileRead("shader.fs");

	glShaderSource(v, 1, (const GLchar**)&vs, NULL);
	glShaderSource(f, 1, (const GLchar**)&fs, NULL);

	free(vs);
	free(fs);

	GLint success;
	char infoLog[1000];
	// compile vertex shader
	glCompileShader(v);
	// check for shader compile errors
	glGetShaderiv(v, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(v, 1000, NULL, infoLog);
		std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// compile fragment shader
	glCompileShader(f);
	// check for shader compile errors
	glGetShaderiv(f, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(f, 1000, NULL, infoLog);
		std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
	}

	// create program object
	p = glCreateProgram();

	// attach shaders to program object
	glAttachShader(p,f);
	glAttachShader(p,v);

	// link program
	glLinkProgram(p);
	// check for linking errors
	glGetProgramiv(p, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(p, 1000, NULL, infoLog);
		std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(v);
	glDeleteShader(f);

	// glGetUniformLocation() :
	// get the location of the object which named "mvp" is shader file
	uniform.iLocMVP = glGetUniformLocation(p, "mvp");
	uniform.iLocView = glGetUniformLocation(p, "view");
	uniform.iLocLightMode = glGetUniformLocation(p, "cur_light_mode");
	uniform.iLocKa = glGetUniformLocation(p, "Ka");
	uniform.iLocKd = glGetUniformLocation(p, "Kd");
	uniform.iLocKs = glGetUniformLocation(p, "Ks");
	uniform.iLocPos_d = glGetUniformLocation(p, "dPos");
	uniform.iLocPos_p = glGetUniformLocation(p, "pPos");
	uniform.iLocPos_s = glGetUniformLocation(p, "sPos");
	uniform.iLocDiff_d = glGetUniformLocation(p, "dDiff");
	uniform.iLocDiff_p = glGetUniformLocation(p, "pDiff");
	uniform.iLocDiff_s = glGetUniformLocation(p, "sDiff");
	uniform.iLocAmbient_d = glGetUniformLocation(p, "dAmbient");
	uniform.iLocAmbient_p = glGetUniformLocation(p, "pAmbient");
	uniform.iLocAmbient_s = glGetUniformLocation(p, "sAmbient");
	uniform.iLocSpec_d = glGetUniformLocation(p, "dSpec");
	uniform.iLocSpec_p = glGetUniformLocation(p, "pSpec");
	uniform.iLocSpec_s = glGetUniformLocation(p, "sSpec");
	uniform.iLocDraw = glGetUniformLocation(p, "draw");
	uniform.iLocShininess = glGetUniformLocation(p, "shininess");
	uniform.iLocAtte_constant_p = glGetUniformLocation(p, "pAtteConstant");
	uniform.iLocAtte_linear_p = glGetUniformLocation(p, "pAtteLinear");
	uniform.iLocAtte_quadratic_p = glGetUniformLocation(p, "pAtteQuad");
	uniform.iLocAtte_constant_s = glGetUniformLocation(p, "sAtteConstant");
	uniform.iLocAtte_linear_s = glGetUniformLocation(p, "sAtteLinear");
	uniform.iLocAtte_quadratic_s = glGetUniformLocation(p, "sAtteQuad");
	uniform.iLocDirection_s = glGetUniformLocation(p, "sDirection");
	uniform.iLocCutoff_s = glGetUniformLocation(p, "sCutoff");
	uniform.iLocExponent_s = glGetUniformLocation(p, "sExponent");
	uniform.iLocOriginalViewMatrix = glGetUniformLocation(p, "originView");
	uniform.iLocCamera = glGetUniformLocation(p, "camera");
	uniform.iLocTRS = glGetUniformLocation(p, "trs");

	if (success)
		glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, vector<GLfloat>& normals, tinyobj::shape_t* shape)
{
	vector<float> xVector, yVector, zVector;
	float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

	// find out min and max value of X, Y and Z axis
	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//maxs = max(maxs, attrib->vertices.at(i));
		if (i % 3 == 0)
		{

			xVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minX)
			{
				minX = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxX)
			{
				maxX = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 1)
		{
			yVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minY)
			{
				minY = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxY)
			{
				maxY = attrib->vertices.at(i);
			}
		}
		else if (i % 3 == 2)
		{
			zVector.push_back(attrib->vertices.at(i));

			if (attrib->vertices.at(i) < minZ)
			{
				minZ = attrib->vertices.at(i);
			}

			if (attrib->vertices.at(i) > maxZ)
			{
				maxZ = attrib->vertices.at(i);
			}
		}
	}

	float offsetX = (maxX + minX) / 2;
	float offsetY = (maxY + minY) / 2;
	float offsetZ = (maxZ + minZ) / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		if (offsetX != 0 && i % 3 == 0)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
		}
		else if (offsetY != 0 && i % 3 == 1)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
		}
		else if (offsetZ != 0 && i % 3 == 2)
		{
			attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
		}
	}

	float greatestAxis = maxX - minX;
	float distanceOfYAxis = maxY - minY;
	float distanceOfZAxis = maxZ - minZ;

	if (distanceOfYAxis > greatestAxis)
	{
		greatestAxis = distanceOfYAxis;
	}

	if (distanceOfZAxis > greatestAxis)
	{
		greatestAxis = distanceOfZAxis;
	}

	float scale = greatestAxis / 2;

	for (int i = 0; i < attrib->vertices.size(); i++)
	{
		//std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
		attrib->vertices.at(i) = attrib->vertices.at(i) / scale;
	}
	size_t index_offset = 0;
	for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
		int fv = shape->mesh.num_face_vertices[f];

		// Loop over vertices in the face.
		for (size_t v = 0; v < fv; v++) {
			// access to vertex
			tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
			vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
			// Optional: vertex colors
			colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
			colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
			// Optional: vertex normals
			if (idx.normal_index >= 0) {
				normals.push_back(attrib->normals[3 * idx.normal_index + 0]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 1]);
				normals.push_back(attrib->normals[3 * idx.normal_index + 2]);
			}
		}
		index_offset += fv;
	}
}

string GetBaseDir(const string& filepath) {
	if (filepath.find_last_of("/\\") != std::string::npos)
		return filepath.substr(0, filepath.find_last_of("/\\"));
	return "";
}

void LoadModels(string model_path)
{
	vector<tinyobj::shape_t> shapes;
	vector<tinyobj::material_t> materials;
	tinyobj::attrib_t attrib;
	vector<GLfloat> vertices;
	vector<GLfloat> colors;
	vector<GLfloat> normals;

	string err;
	string warn;

	string base_dir = GetBaseDir(model_path); // handle .mtl with relative path

#ifdef _WIN32
	base_dir += "\\";
#else
	base_dir += "/";
#endif

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str(), base_dir.c_str());

	if (!warn.empty()) {
		cout << warn << std::endl;
	}

	if (!err.empty()) {
		cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	printf("Load Models Success ! Shapes size %d Material size %d\n", int(shapes.size()), int(materials.size()));
	model tmp_model;

	vector<PhongMaterial> allMaterial;
	for (int i = 0; i < materials.size(); i++)
	{
		PhongMaterial material;
		material.Ka = Vector3(materials[i].ambient[0], materials[i].ambient[1], materials[i].ambient[2]);
		material.Kd = Vector3(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2]);
		material.Ks = Vector3(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2]);
		allMaterial.push_back(material);
	}

	for (int i = 0; i < shapes.size(); i++)
	{

		vertices.clear();
		colors.clear();
		normals.clear();
		normalization(&attrib, vertices, colors, normals, &shapes[i]);
		// printf("Vertices size: %d", vertices.size() / 3);

		Shape tmp_shape;
		glGenVertexArrays(1, &tmp_shape.vao);
		glBindVertexArray(tmp_shape.vao);

		glGenBuffers(1, &tmp_shape.vbo);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
		tmp_shape.vertex_count = vertices.size() / 3;

		glGenBuffers(1, &tmp_shape.p_color);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
		glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glGenBuffers(1, &tmp_shape.p_normal);
		glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_normal);
		glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals.at(0), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// not support per face material, use material of first face
		if (allMaterial.size() > 0)
			tmp_shape.material = allMaterial[shapes[i].mesh.material_ids[0]];
		tmp_model.shapes.push_back(tmp_shape);
	}
	shapes.clear();
	materials.clear();
	models.push_back(tmp_model);
}

void initParameter()
{
	// [TODO] Setup some parameters if you need
	proj.left = -1;
	proj.right = 1;
	proj.top = 1;
	proj.bottom = -1;
	proj.nearClip = 0.001;
	proj.farClip = 100.0;
	proj.fovy = 80;
	proj.aspect = (float)WINDOW_WIDTH / 2 / (float)WINDOW_HEIGHT;

	main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
	main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
	main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

	Directional.position = Vector3(1, 1, 1);
	Directional.direction = Vector3(0, 0, 0);
	Directional.ambient = Vector3(0.15, 0.15, 0.15);
	Directional.diffuse = Vector3(1, 1, 1);
	Directional.specular = Vector3(1, 1, 1);

	Point.position = Vector3(0, 2, 1);
	Point.diffuse = Vector3(1, 1, 1);
	Point.ambient = Vector3(0.15, 0.15, 0.15);
	Point.specular = Vector3(1, 1, 1);
	Point.attenuation_constant = 0.01;
	Point.attenuation_linear = 0.8;
	Point.attenuation_quadratic = 0.1;

	Spot.position = Vector3(0, 0, 2);
	Spot.direction = Vector3(0, 0, -1);
	Spot.diffuse = Vector3(1, 1, 1);
	Spot.ambient = Vector3(0.15, 0.15, 0.15);
	Spot.specular = Vector3(1, 1, 1);
	Spot.exponent = 50;
	Spot.cutoff = 30;
	Spot.attenuation_constant = 0.05;
	Spot.attenuation_linear = 0.3;
	Spot.attenuation_quadratic = 0.6;

	setViewingMatrix();
	setPerspective();	//set default projection matrix as perspective matrix
}

void setupRC()
{
	// setup shaders
	setShaders();
	initParameter();

	// OpenGL States and Values
	glClearColor(0.2, 0.2, 0.2, 1.0);
	vector<string> model_list{ "../NormalModels/bunny5KN.obj", "../NormalModels/dragon10KN.obj", "../NormalModels/lucy25KN.obj", "../NormalModels/teapot4KN.obj", "../NormalModels/dolphinN.obj"};
	// [TODO] Load five model at here =========
	for (int i = 0; i < 5; i++) {
		LoadModels(model_list[i]);
	}
	//===========================================
}

void glPrintContextInfo(bool printExtension)
{
	cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
	cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
	cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
	cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	if (printExtension)
	{
		GLint numExt;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
		cout << "GL_EXTENSIONS =" << endl;
		for (GLint i = 0; i < numExt; i++)
		{
			cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
		}
	}
}


int main(int argc, char **argv)
{
    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Student ID HW2", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
	// register glfw callback functions
    glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);

    glfwSetFramebufferSizeCallback(window, ChangeSize);
	glEnable(GL_DEPTH_TEST);
	// Setup render context
	setupRC();

	// main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
        RenderScene();
        
        // swap buffer from back to front
        glfwSwapBuffers(window);
        
        // Poll input event
        glfwPollEvents();
    }
	
	// just for compatibiliy purposes
	return 0;
}
