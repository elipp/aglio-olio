#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <SDL/SDL.h>

#include "lin_alg.h"
#include "common.h"
#include "objloader.h"
#include "shader.h"
#include "model.h"
#include "texture.h"


#define WINDOW_WIDTH 1440.0
#define WINDOW_HEIGHT 960.0
#define HALF_WINDOW_WIDTH WINDOW_WIDTH/2.0
#define HALF_WINDOW_HEIGHT WINDOW_HEIGHT/2.0

using namespace std;

bool active=true;
bool fullscreen=false;
bool keys[256];

static const double gamma = 6.67;

SDL_Window *mainwindow; /* Our window handle */
SDL_GLContext maincontext; /* Our opengl context handle */

static struct mousePos {
	int x, y;
} cursorPos;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	// declare wndproc

float c_vel_fwd = 0, c_vel_side = 0;

GLuint  VBOid, 
		IBOid,
		facecount,
		uni_running_loc,
		uni_modelview_loc,
		uni_projection_loc,
		uni_sampler2d_loc,
		uni_heightmap_loc,
		uni_light_loc,
		uni_lightsrc_loc,
		frag_data_loc;

GLuint uni_NP_modelview_loc,
	   uni_NP_projection_loc,
	   uni_NP_heightmap_loc;

static GLint earth_tex_id, hmap_id;

bool mouseLocked;

GLfloat running = 0.0;

static ShaderProgram *regular_shader;
static ShaderProgram *normal_plot_shader;

static const float dt = 0.01;

mat4 view;
Quaternion viewq;
float qx = 0, qy = 0;
mat4 projection;
vec4 view_position;
vec4 cameraVel;

static std::vector<Model> models;

GLushort * indices; 

bool plot_normals = true;

#ifndef M_PI
#define M_PI 3.1415926535
#endif

void rotatex(float mod) {
	qy += 0.001*mod;
	Quaternion xq = Quaternion::fromAxisAngle(1.0, 0.0, 0.0, qx);
	Quaternion yq = Quaternion::fromAxisAngle(0.0, 1.0, 0.0, qy);
	viewq = yq * xq;
}

void rotatey(float mod) {
	qx -= 0.001*mod;
	Quaternion xq = Quaternion::fromAxisAngle(1.0, 0.0, 0.0, qx);
	Quaternion yq = Quaternion::fromAxisAngle(0.0, 1.0, 0.0, qy);
	viewq = yq * xq;
}

void update_c_pos() {
	view_position -= viewq * vec4(c_vel_side, 0.0, 0.0, 1.0);
	view_position += viewq * vec4(0.0, 0.0, c_vel_fwd, 1.0);
}

void control()
{
	static const float fwd_modifier = 0.008;
	static const float side_modifier = 0.005;

	if(mouseLocked) {
		GetCursorPos(cursorPos);
		SetCursorPos(HALF_WINDOW_WIDTH, HALF_WINDOW_HEIGHT);
		float dx = (HALF_WINDOW_WIDTH - cursorPos->x);
		float dy = -(HALF_WINDOW_HEIGHT - cursorPos->y);

		if (keys['W']) {
			c_vel_fwd += fwd_modifier;
			//keys['W'] = FALSE;
		} 
		if (keys['S']) {
			c_vel_fwd -= fwd_modifier;
			//keys['S'] = FALSE;
		}
		c_vel_fwd *= 0.97;

		if (keys['A']) {
			c_vel_side -= side_modifier;
			//keys['A'] = FALSE;
		} 
		if (keys['D']) {
			c_vel_side += side_modifier;
			//keys['D'] = FALSE;
		} 
		c_vel_side *= 0.95;


		if (keys['N']) {
			plot_normals = !plot_normals;
			keys['N'] = FALSE;
		}
		static GLint PMODE = GL_FILL;
		if (keys['P']) {
			glPolygonMode(GL_FRONT_AND_BACK, (PMODE = (PMODE == GL_FILL ? GL_LINE : GL_FILL)));
			keys['P'] = FALSE;
		}
		if (dy != 0) {
			rotatey(dy);
		}
		if (dx != 0) {
			rotatex(dx);
		}

	}
	/*if (keys[VK_UP])
	{
		location(0) += movevel;
	}


	if (keys[VK_DOWN])
	{
		location(0) -= movevel;
	}*/

	/*if (keys[VK_LEFT])
	{
		cameraVel(2) -= movevel;
	}
	

	if (keys[VK_RIGHT])
	{
		cameraVel(2) += movevel;
	}*/
	
	//view_position.print();

	//view_position += dt*cameraVel;
}



GLvoid ResizeGLScene(GLsizei width, GLsizei height)		// Resize And Initialize The GL Window
{
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0,0,width,height);						// Reset The Current Viewport


	// Calculate The Aspect Ratio Of The Window
	//gluPerspective(45.0f,(GLfloat)width/(GLfloat)height,0.1f,100.0f);

}

static GLuint skybox_VBOid;
static GLuint skybox_facecount;
static mat4 skyboxmat(MAT_IDENTITY);

void drawSkybox() {

	
	skyboxmat = mat4::scale(vec4(10.0, 10.0, 10.0, 1.0));
	mat4 modelview = view * skyboxmat;
	
	glUniformMatrix4fv(uni_modelview_loc, 1, GL_FALSE, (const GLfloat*) modelview.rawData());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);	// something appropriate
	glUniform1i(uni_sampler2d_loc, 0);	// needs to be 0 explicitly :D
	glUniform1i(uni_lightsrc_loc, 1);
		
	glUniformMatrix4fv(uni_projection_loc, 1, GL_FALSE, (const GLfloat *)projection.rawData());
	
	glUseProgram(regular_shader->getProgramHandle());	


	glBindBuffer(GL_ARRAY_BUFFER, skybox_VBOid);
	glUniformMatrix4fv(uni_modelview_loc, 1, GL_FALSE, (const GLfloat *)skyboxmat.rawData());
	glDrawElements(GL_TRIANGLES, skybox_facecount*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 
	
}

inline double rand01() {
	return (double)rand()/(double)RAND_MAX;
}

vec4 randomvec4(float min, float max) {
	float r1, r2, r3, r4;
	r1 = rand01() * max + min;
	r2 = rand01() * max + min;
	r3 = rand01() * max + min;
	r4 = 0;
	vec4 r(r1, r2, r3, r4);
	//r.print();
	return r;
}

GLushort *generateIndices() {
	// due to the design of the .bobj file format, the index buffers just run arithmetically (0, 1, 2, ..., 0xFFFF)
	GLushort index_count = 0xFFFF;
	GLushort *indices = new GLushort[index_count];

	for (int i = 0; i < index_count; i++) {
		indices[i] = i;
	}
	return indices;
}

int InitGL(void)
{
	glClearColor(0.0, 0.0, 0.0, 1.0);

	glEnable(GL_DEPTH_TEST);

	GLenum err = glewInit();

	if (GLEW_OK != err)
	{
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}

	regular_shader = new ShaderProgram("shaders/regular.vs", "none", "shaders/regular.fs");
	if (regular_shader->is_bad()) { printf("Fatal: shader error (regular).\n"); return FALSE; }

	normal_plot_shader = new ShaderProgram("shaders/normalplot.vs", "shaders/normalplot.gs", "shaders/normalplot.fs");
	if (normal_plot_shader->is_bad()) { printf("Fatal: shader error (normalplot).\n"); return FALSE; }

	GLuint sphere_facecount;
	GLuint sphere_VBOid = loadNewestBObj("models/maapallo_napa_korjattu.bobj", &sphere_facecount);
	skybox_VBOid = loadNewestBObj("models/skybox.bobj", &skybox_facecount);

	indices = generateIndices();
	TextureBank::add(Texture("textures/EarthTexture.png", GL_LINEAR));
	TextureBank::add(Texture("textures/earth_height_normal_map.png", GL_LINEAR));

	earth_tex_id = TextureBank::get_id_by_name("textures/EarthTexture.png");
	hmap_id = TextureBank::get_id_by_name("textures/earth_height_normal_map.png");

	if (!TextureBank::validate())
		return FALSE;

	/*
	 * AFTER ALL THIS, THE TEXTURE DATA CAN AND MUST BE FREED!!!
	 * delete [] texture;
	 */

	printf("OpenGL version: %s\n", glGetString(GL_VERSION));

	glGenBuffers(1, &IBOid);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOid);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short int)*3*facecount, indices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*(GLushort)(0xFFFF), indices, GL_STATIC_DRAW);

	delete [] indices;	


	//glBindFragDataLocation(programHandle, 0, "out_frag_color");

	GLuint programHandle = regular_shader->getProgramHandle();
	
	glUseProgram(programHandle);

	uni_running_loc = glGetUniformLocation(programHandle, "running"); // no need to assert this, most likely optimized away by GLSL
	uni_modelview_loc = glGetUniformLocation(programHandle, "ModelView"); assert(uni_modelview_loc != -1);
	uni_projection_loc = glGetUniformLocation(programHandle, "Projection"); assert(uni_projection_loc != -1);
	uni_sampler2d_loc = glGetUniformLocation(programHandle, "texture_color"); assert(uni_sampler2d_loc != -1);
	uni_light_loc = glGetUniformLocation(programHandle, "light"); assert(uni_light_loc != -1);
	uni_lightsrc_loc = glGetUniformLocation(programHandle, "lightsrc"); assert(uni_lightsrc_loc != -1);
	uni_heightmap_loc = glGetUniformLocation(programHandle, "heightmap"); assert(uni_heightmap_loc != -1);

	GLuint NP_programHandle = normal_plot_shader->getProgramHandle();
	glUseProgram(NP_programHandle);
	uni_NP_modelview_loc =  glGetUniformLocation(NP_programHandle, "ModelView"); assert(uni_NP_modelview_loc != -1);
	uni_NP_projection_loc =  glGetUniformLocation(NP_programHandle, "Projection"); assert(uni_NP_modelview_loc != -1);
	uni_NP_heightmap_loc =  glGetUniformLocation(NP_programHandle, "heightmap"); assert(uni_NP_modelview_loc != -1);

	GLuint vPosition = glGetAttribLocation( programHandle, "in_position"); assert(vPosition != -1);
    GLuint nPosition = glGetAttribLocation( programHandle, "in_normal"); assert(nPosition != -1);
	GLuint tPosition = glGetAttribLocation( programHandle, "in_texcoord"); assert(tPosition != -1);
	GLuint fragloc = glGetFragDataLocation( programHandle, "out_frag_color"); assert(fragloc != -1);

	printf("\n\n%d, %d, %d, %d, %d\n\n", vPosition, nPosition, tPosition, fragloc, uni_light_loc);


	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	/* shader error checking */

	// view is initialized to zero by default, so it needs to be identity'ed :DD::DCXD
	view.identity();

	projection.make_proj_perspective(M_PI/8.0, (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 2.0, 1000.0);

	view_position = vec4(0.0, 0.0, 0.0, 1.0);
	cameraVel = vec4(0.0, 0.0, 0.0, 1.0);
	
	//view(3,2) = 3.0;	

	//printMatrix4f(projection);
		
	//glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
	//wglSwapIntervalEXT(2); // prefer hardware-forced vsync over this
	
	models.push_back(Model(10000.0, 5.0, sphere_VBOid, earth_tex_id, sphere_facecount, true, true));
	models.push_back(Model(50.0, 1.0, sphere_VBOid, earth_tex_id, sphere_facecount, false, false));
	models.push_back(Model(15.23, 0.2, sphere_VBOid, earth_tex_id, sphere_facecount, false, false));
	models.push_back(Model(33.05, 0.3, sphere_VBOid, earth_tex_id, sphere_facecount, false, false));
	models.push_back(Model(23.3, 0.4, sphere_VBOid, earth_tex_id, sphere_facecount, false, false));
	models.push_back(Model(15.0, 0.5, sphere_VBOid, earth_tex_id, sphere_facecount, false, false));

	/*for (int i = 0; i < 15; ++i) {
		Model m(5, 0.5, sphere_VBOid, textures[rand()%3+1].id(), sphere_facecount, false, false);
		m.translate(randomvec4(-20, 20));
		m.velocity = randomvec4(-80, 80.0);
		models.push_back(m);
	}*/


	models[0].velocity = vec4(0, 0, 0, 0);

	models[1].translate(vec4(100.0, 0.0, 0.0, 0.0));		
	models[1].velocity = vec4(0.0, 20.0, 0.0, 0.0);

	models[2].translate(models[1].position - vec4(-3.0, 4.0, -1.5, 0.0));
	models[2].velocity = vec4(2.0, 0.0, 3.0, 0.0);


	models[3].translate(models[1].position - vec4(3.2, 8.0, -1.0, 0.0));
	models[3].velocity = -vec4(2.0, 0.0, -2.0, 0.0);

	
	models[3].translate(models[1].position - vec4(1.3, -3.0, 3.0, 0.0));
	models[3].velocity = vec4(2.0, 0.0, -1.0, 0.0);
	
	models[4].translate(models[1].position - vec4(-3.0, -3.0, -3.0, 0.0));
	models[4].velocity = -vec4(0.0, 1.0, 2.0, 0.0);

	models[5].translate(models[1].position - vec4(-6.0, -3.0, -3.0, 0.0));
	models[5].velocity = vec4(0.0, 1.0, 2.0, 0.0);


	glBindBuffer(GL_ARRAY_BUFFER, sphere_VBOid);
	
	/* the glEnable/DisableVertexAttribArray calls are REDUNDANT, since these are the only attribute arrays around at this time. */

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(0));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(3*sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(6*sizeof(float)));

	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOid);
	return TRUE;

}


void calculateCollision(Model *a, Model *b) {

	
	//vec4 r = b->position - a->position;

	float m1 = a->mass;
	float m2 = b->mass;
	float combinedmass = m1 + m2;

	float v1x = (m1*a->velocity(0) - m2 * (a->velocity(0) - 2*b->velocity(0)))/combinedmass;
	float v1y = (m1*a->velocity(1) - m2 * (a->velocity(1) - 2*b->velocity(1)))/combinedmass;
	float v1z = (m1*a->velocity(2) - m2 * (a->velocity(2) - 2*b->velocity(2)))/combinedmass;
	
	float v2x = (m2*(2*a->velocity(0) - b->velocity(0)) + m2*b->velocity(0))/combinedmass;
	float v2y = (m2*(2*a->velocity(1) - b->velocity(1)) + m2*b->velocity(1))/combinedmass;
	float v2z = (m2*(2*a->velocity(2) - b->velocity(2)) + m2*b->velocity(2))/combinedmass;
	
	vec4(v1x, v1y, v1z, 0.0);

	a->velocity = vec4(v1x, v1y, v1z, 0.0);
	b->velocity = vec4(v2x, v2y, v2z, 0.0);


	//return (retval.dot(r) < 0.0) ? retval : -retval;
	//return retval;
}


void drawSpheres()
{
	
	glUseProgram(regular_shader->getProgramHandle());	
	
	running += 0.015;
	if (running > 1000000) { running = 0; }
	
	glUniform1f(uni_running_loc, running);

	glUniformMatrix4fv(uni_projection_loc, 1, GL_FALSE, (const GLfloat *)projection.rawData());
	
	static const GLuint sphere_VBOid = models[0].getVBOid();
	static const GLuint sphere_facecount = models[0].getFaceCount();
	
	/* glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOid);  // is still in full matafaking effizzect :D */
	glBindBuffer(GL_ARRAY_BUFFER, sphere_VBOid);	 // BIND BUFFER FOR ALL MATAFAKING SPHERES.
	static const float dt = 0.01;

	//models[0].rotation = Quaternion::fromAxisAngle(0.0, 0.0, 1.0, 90);
	//models[0].rotation *= Quaternion::fromAxisAngle(0.0, 1.0, 0.0, running);
	models[0].rotation.normalize();
	models[1].rotation = Quaternion::fromAxisAngle(-0.2, 1.0, 0.0, 0.5*running);
	models[1].rotation.normalize();

	viewq.normalize();
	view = viewq.toRotationMatrix();
	view = view*view_position.toTranslationMatrix();

	// no need to reconstruct iterator every time
	static std::vector<Model>::iterator current;
	current = models.begin();

	for (; current != models.end(); ++current)
	{
		//vec4 acceleration(0.0, 0.0, 0.0, 0.0);	// is initialized to zero by default though
		vec4 acceleration;
		static std::vector<Model>::const_iterator iter = models.begin();
		iter = models.begin();

		while(iter != models.end()) {

			if (iter != current) {
				vec4 r = (*iter).position - (*current).position;
				
				const float distance = r.length3();
				//printf("distance: %f\n", distance);

				if (fabs(distance) < (*iter).radius + (*current).radius) {
				
					if ((*current).velocity.length3() + (*iter).velocity.length3() < 0.5) {
						//(*current).velocity.zero();
					}
					else {
						//printf("COLLISION\n");
						const vec4 r_unit = r.normalized();
						vec4 p1 = (*current).mass * (*current).velocity;
						//vec4 p2 = (*iter)->mass * (*iter)->velocity;

						vec4 n_p1 = -(dot(p1, r)/(distance*distance))*r_unit;
		//				vec4 n_p2 = (p2.dot(r)/(distance*distance))*r_unit;

						// approximation; has its flaws 
					//	(*current).velocity = (2*n_p1 + p1)/(*current).mass; ***
	
					//	printVector4f(p1 + p2);			
						//printVector4f(n_p1 + n_p2);

					
						//vec4 p1 = (*current)->mass * (*current)->velocity;
						//vec4 p2 = (*iter)->mass * (*iter)->velocity;

						//printVector4f(p1 + p2);

						//calculateCollision(*current, *iter);
//
						//p1 = (*current)->mass * (*current)->velocity;
						//p2 = (*iter)->mass * (*iter)->velocity;

						//printVector4f(p1 + p2);

						// printVector4f(newVel);
						 //(*current)->velocity = newVel;	 
						// (*current)->translate((*current)->velocity*0.001);
						// (*iter)->velocity = -newVel;
						// (*iter)->translate((*iter)->velocity*0.001);

					
					}
				}

				else {
					acceleration += 0.008*(gamma*(*iter).mass)/(r.length3()*r.length3())*r;
				}
			}

			++iter;
		}

		// attraction to 0, 0, 0
		// acceleration += 0.008*(gamma*1000.0)/(((*current)->position.norm())*(*current)->position.norm())*(-(*current)->position);

		(*current).velocity += acceleration * dt;

		//(*current)->updatePosition();
		(*current).translate((*current).velocity*dt);

		mat4 model_rotation = (*current).rotation.toRotationMatrix();

		//(*current)->model_matrix.print();

		//printMatrix4f(model_rotation);
		mat4 modelview = view * (*current).model_matrix * model_rotation;
	
		glUniformMatrix4fv(uni_modelview_loc, 1, GL_FALSE, (const GLfloat*) modelview.rawData());
	
		vec4 light_pos(sin(running), cos(0.11*running), -5.0, 1.0);
		if (!(*current).lightsrc()) {
			vec4 light = (light_pos - (*current).position);
			light.w() = 0.0;
			light.normalize();
			light.w() = 1.0;

			glUniform4fv(uni_light_loc, 1, (const GLfloat*) light.rawData());
		}



		glUniform1i(uni_lightsrc_loc, (*current).lightsrc() ? 1 : 0);	
		glBindBuffer(GL_ARRAY_BUFFER, (*current).getVBOid());
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, (*current).getTextureId());
		glUniform1i(uni_sampler2d_loc, 0);	// needs to be 0 explicitly :D
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, hmap_id);	// heightmap
		glUniform1i(uni_heightmap_loc, 1);
		

		glDrawElements(GL_TRIANGLES, (*current).getFaceCount()*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 
	}

	// draw normals for center sphere

	if (plot_normals) {
		glUseProgram(normal_plot_shader->getProgramHandle());
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, hmap_id);
		glUniform1i(uni_NP_heightmap_loc, 1);
	
		mat4 modelview = view * models[0].model_matrix * models[0].rotation.toRotationMatrix();
		glUniformMatrix4fv(uni_NP_projection_loc, 1, GL_FALSE, (const GLfloat *)projection.rawData());
		glUniformMatrix4fv(uni_NP_modelview_loc, 1, GL_FALSE, (const GLfloat*) modelview.rawData());

		glDrawElements(GL_POINTS, models[0].getFaceCount()*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
	}
}

void KillGLWindow(void)
{
	if(hRC)
	{
		if(!wglMakeCurrent(NULL,NULL))
		{
			MessageBox(NULL, "wglMakeCurrent(NULL,NULL) failed", "erreur", MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC))
		{
			MessageBox(NULL, "RELEASE of rendering context failed.", "error", MB_OK | MB_ICONINFORMATION);
		}
		hRC=NULL;

		if(hDC && !ReleaseDC(hWnd, hDC))
		{
			MessageBox(NULL, "Release DC failed.", "ERREUX", MB_OK | MB_ICONINFORMATION);
			hDC=NULL;
		}

		if(hWnd && !DestroyWindow(hWnd))
		{
			MessageBox(NULL, "couldn't release hWnd.", "erruexz", MB_OK|MB_ICONINFORMATION);
			hWnd=NULL;
		}

		if (!UnregisterClass("OpenGL", hInstance))
		{
			MessageBox(NULL, "couldn't unregister class.", "err", MB_OK | MB_ICONINFORMATION);
			hInstance=NULL;
		}

	}

}


BOOL CreateGLWindow(char* title, int width, int height, int bits, bool fullscreenflag)
{
	GLuint PixelFormat;
	WNDCLASS wc;
	DWORD dwExStyle;
	DWORD dwStyle;

	RECT WindowRect;
	WindowRect.left=(long)0;
	WindowRect.right=(long)width;
	WindowRect.top=(long)0;
	WindowRect.bottom=(long)height;

	fullscreen = fullscreenflag;

	hInstance = GetModuleHandle(NULL);
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = (WNDPROC) WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "OpenGL";

	if (!RegisterClass(&wc))
	{
		MessageBox(NULL, "FAILED TO REGISTER THE WINDOW CLASS.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE;
	}

	DEVMODE dmScreenSettings;
	memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));
	dmScreenSettings.dmSize = sizeof(dmScreenSettings);
	dmScreenSettings.dmPelsWidth = width;
	dmScreenSettings.dmPelsHeight = height;
	dmScreenSettings.dmBitsPerPel = bits;
	dmScreenSettings.dmFields= DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

	/*
	 * no need to test this now that fullscreen is turned off
	 *
	if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
	{
		if (MessageBox(NULL, "The requested fullscreen mode is not supported by\nyour video card. Use Windowed mode instead?", "warn", MB_YESNO | MB_ICONEXCLAMATION)==IDYES)
		{
			fullscreen=FALSE;
		}
		else {

			MessageBox(NULL, "Program willl now close.", "ERROR", MB_OK|MB_ICONSTOP);
			return FALSE;
		}
	}*/

	ShowCursor(FALSE);
	if (fullscreen)
	{
		dwExStyle=WS_EX_APPWINDOW;
		dwStyle=WS_POPUP;
	
	}

	else {
		dwExStyle=WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		dwStyle=WS_OVERLAPPEDWINDOW;
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle);

	if(!(hWnd=CreateWindowEx( dwExStyle, "OpenGL", title,
							  WS_CLIPSIBLINGS | WS_CLIPCHILDREN | dwStyle,
							  0, 0,
							  WindowRect.right-WindowRect.left,
							  WindowRect.bottom-WindowRect.top,
							  NULL,
							  NULL,
							  hInstance,
							  NULL)))
	{
		KillGLWindow();
		MessageBox(NULL, "window creation error.", "ERROR", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}


	static PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		bits,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		16,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	if (!(hDC=GetDC(hWnd)))
	{
		KillGLWindow();
		MessageBox(NULL, "CANT CREATE A GL DEVICE CONTEXT.", "ERROR", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if (!(PixelFormat = ChoosePixelFormat(hDC, &pfd)))
	{
		KillGLWindow();
		MessageBox(NULL, "cant find a suitable pixelformat.", "ERROUE", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}


	if(!SetPixelFormat(hDC, PixelFormat, &pfd))
	{
		KillGLWindow();
		MessageBox(NULL, "Can't SET ZE PIXEL FORMAT.", "ERROU", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if(!(hRC=wglCreateContext(hDC)))
	{
		KillGLWindow();
		MessageBox(NULL, "WGLCREATECONTEXT FAILED.", "ERREUHX", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	if(!wglMakeCurrent(hDC, hRC))
	{
		KillGLWindow();
		MessageBox(NULL, "Can't activate the gl rendering context.", "ERAIX", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	ShowWindow(hWnd, SW_SHOW);
	SetForegroundWindow(hWnd);
	SetFocus(hWnd);
	ResizeGLScene(width, height);
	mouseLocked = true;

	if (!InitGL())
	{
		KillGLWindow();
		MessageBox(NULL, "InitGL() failed.", "ERRROR", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	return TRUE;
}



LRESULT CALLBACK WndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_ACTIVATE:
		if(!HIWORD(wParam))
		{
			active=TRUE;
		}

		else 
		{
			active=FALSE;
		}
		return 0;

	case WM_SYSCOMMAND:
		switch(wParam)
		{
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			return 0;
		}
		break;
	
	case WM_CLOSE:
		{
		PostQuitMessage(0);
		return 0;
		}

	case WM_KEYDOWN:
		{
			keys[wParam]=TRUE;
			return 0;
		}
	case WM_KEYUP:
		{
			keys[wParam]=FALSE;
			return 0;
		}
	case WM_SIZE:
		{
			ResizeGLScene(LOWORD(lParam), HIWORD(lParam));
		}
	;
	}

	/* the rest shall be passed to defwindowproc. (default window procedure) */
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	/* allocate console for debug output (only works with printf doe) */

	if(AllocConsole()) {
    freopen("CONOUT$", "wt", stdout);
    SetConsoleTitle("debug output");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED);
	}
	
	std::string cpustr(checkCPUCapabilities());
	if (cpustr != "OK") { MessageBox(NULL, cpustr.c_str(), "Fatal error.", MB_OK); return -1; }

	MSG msg;
	BOOL done=FALSE;

	//if(MessageBox(NULL, "Would you like to run in fullscreen mode?", "Start fullscreen?", MB_YESNO|MB_ICONQUESTION)==IDNO)
	//{  // no need to ask this, since it doesn't work anyway
	fullscreen=FALSE;
	//}

	/*
	 * Dirty trick to "delete" the previously existing shader.log file
	 */

	std::ofstream logfile("shader.log", std::ios::trunc);
	logfile << "[shader.log]\n\n";
	logfile.close();

	if(!CreateGLWindow("opengl framework stolen from NeHe", WINDOW_WIDTH, WINDOW_HEIGHT, 32, fullscreen))
	{
		return 1;
	}
	
	bool esc = false;





	while(!done)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if(msg.message == WM_QUIT)
			{
				done=TRUE;
			}
			else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else {
			if (active)
			{
				if(keys[VK_ESCAPE])
				{
					if (!esc) {
						mouseLocked = !mouseLocked;
						ShowCursor(mouseLocked ? FALSE : TRUE);
						esc = true;
					}
					//done=TRUE;
				}
				else{
					esc=false;
					
					control();
					update_c_pos();
					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					drawSpheres();
				//	drawSkybox();

					SwapBuffers(hDC);
				}
			}
		}

	}

	KillGLWindow();
	glDeleteBuffers(1, &VBOid);
	glDeleteBuffers(1, &IBOid);
	return (msg.wParam);
}

