#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>

#include <cassert>
#include <signal.h>

#define GL_GLEXT_PROTOTYPES 1

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include <GL/glfw.h>

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
bool keys[256] = { false };

static const double GAMMA = 6.67;

static struct mousePos {
	int x, y;
} cursorPos;

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

bool mouseLocked = true;

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
		glfwDisable(GLFW_MOUSE_CURSOR);
		glfwGetMousePos(&cursorPos.x, &cursorPos.y);
		glfwSetMousePos(HALF_WINDOW_WIDTH, HALF_WINDOW_HEIGHT);
		float dx = (HALF_WINDOW_WIDTH - cursorPos.x);
		float dy = -(HALF_WINDOW_HEIGHT - cursorPos.y);

		if (keys['W'] == GLFW_PRESS) {
			c_vel_fwd += fwd_modifier;
			//keys['W'] = FALSE;
		} 
		if (keys['S'] == GLFW_PRESS) {
			c_vel_fwd -= fwd_modifier;
			//keys['S'] = FALSE;
		}
		c_vel_fwd *= 0.97;

		if (keys['A'] == GLFW_PRESS) {
			c_vel_side -= side_modifier;
			//keys['A'] = FALSE;
		} 
		if (keys['D'] == GLFW_PRESS) {
			c_vel_side += side_modifier;
			//keys['D'] = FALSE;
		} 
		c_vel_side *= 0.95;


		if (keys['N'] == GLFW_PRESS) {
			plot_normals = !plot_normals;
			keys['N'] = false;
		}
		static GLint PMODE = GL_FILL;
		if (keys['P']) {
			glPolygonMode(GL_FRONT_AND_BACK, (PMODE = (PMODE == GL_FILL ? GL_LINE : GL_FILL)));
			keys['P'] = false;
		}
		if (dy != 0) {
			rotatey(dy);
		}
		if (dx != 0) {
			rotatex(dx);
		}

	}
	else {
		glfwEnable(GLFW_MOUSE_CURSOR);
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

int initGL(void)
{
	glClearColor(0.0, 0.0, 0.0, 1.0);

	glEnable(GL_DEPTH_TEST);

	GLenum err = glewInit();

	if (GLEW_OK != err)
	{
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}

	regular_shader = new ShaderProgram("shaders/regular.vs", "none", "shaders/regular.fs");
	if (regular_shader->is_bad()) { printf("Fatal: shader error (regular).\n"); return 0; }

	normal_plot_shader = new ShaderProgram("shaders/normalplot.vs", "shaders/normalplot.gs", "shaders/normalplot.fs");
	if (normal_plot_shader->is_bad()) { printf("Fatal: shader error (normalplot).\n"); return 0; }

	printf("Loading models...");
	GLuint sphere_facecount;
	GLuint sphere_VBOid = loadNewestBObj("models/maapallo_napa_korjattu.bobj", &sphere_facecount);
	skybox_VBOid = loadNewestBObj("models/skybox.bobj", &skybox_facecount);
	printf("done.\n");

	printf("Loading textures...");
	indices = generateIndices();
	TextureBank::add(Texture("textures/EarthTexture.png", GL_LINEAR));
	TextureBank::add(Texture("textures/earth_height_normal_map.png", GL_LINEAR));
	printf("done.\n");

	earth_tex_id = TextureBank::get_id_by_name("textures/EarthTexture.png");
	hmap_id = TextureBank::get_id_by_name("textures/earth_height_normal_map.png");

	if (!TextureBank::validate())
		return false;

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
	return 1;

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
					acceleration += 0.008*(GAMMA*(*iter).mass)/(r.length3()*r.length3())*r;
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
			light(V::w) = 0.0;
			light.normalize();
			light(V::w) = 1.0;

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

/* callbacks */

void signal_handler(int sig) {
	exit(sig);
}

void GLFWCALL key_callback(int key, int action) {
	keys[key] = (action != 0);	// action == GLFW_PRESS (==1) or GLFW_RELEASE (==0)
}

int createWindow() {

	if (!glfwInit()) {
			fprintf(stderr, "glfwInit failed.\n");
			return 0;
	}

	//glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4); // 4x antialiasing
	//glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3); // We want OpenGL 3.3
	//glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 3);
	//glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL

	// Open a window and create its OpenGL context
	if( !glfwOpenWindow( WINDOW_WIDTH, WINDOW_HEIGHT, 0,0,0,0, 32,0, GLFW_WINDOW ) )
	{
	    fprintf( stderr, "Failed to open GLFW window\n" );
	    glfwTerminate();
	    return 0;
	}
	glfwSetWindowTitle( "lolz" );

	// Initialize GLEW
	//glewExperimental=true; // Needed in core profile
	if (glewInit() != GLEW_OK) {
	    fprintf(stderr, "Failed to initialize GLEW\n");
	    return 0;
	}

	glfwSetKeyCallback(key_callback);

}

int main(int argc, char* argv[]) {

	if (!createWindow()) { fprintf(stderr, "couldn't create window.\n"); exit(1); }
	if (!initGL()) { fprintf(stderr, "Failed to initialized OpenGL.\n"); exit(1); }


	signal(SIGINT, signal_handler);

	int c;
	
	bool esc = false;

	std::string cpustr(checkCPUCapabilities());
	if (cpustr != "OK") { fprintf(stderr, "cpuid error (\"%s\")\n", cpustr.c_str()); return 1; }

	bool done=false;

	while(!done)
	{

		control();
		update_c_pos();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		drawSpheres();
		if (glfwGetKey(GLFW_KEY_ESC) == GLFW_PRESS) { if (!keys[GLFW_KEY_ESC]) { mouseLocked = !mouseLocked; keys[GLFW_KEY_ESC] = true; }  else { keys[GLFW_KEY_ESC] = false; } }
		glfwSwapBuffers();

		done = !glfwGetWindowParam(GLFW_OPENED);
		//	drawSkybox();


	}

	glfwTerminate();
	glDeleteBuffers(1, &VBOid);
	glDeleteBuffers(1, &IBOid);


	return 0;

}

