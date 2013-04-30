#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string>
#include <stdarg.h>

#include <cassert>
#include <signal.h>

#include <Windows.h>
#define GLEW_STATIC 
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/wglew.h>

#include "glwindow_win32.h"
#include "lin_alg.h"
#include "common.h"
#include "objloader.h"
#include "shader.h"
#include "model.h"
#include "texture.h"
#include "text.h"

#define WINDOW_WIDTH 1440.0
#define WINDOW_HEIGHT 960.0
#define HALF_WINDOW_WIDTH WINDOW_WIDTH/2.0
#define HALF_WINDOW_HEIGHT WINDOW_HEIGHT/2.0

#ifndef M_PI
#define M_PI 3.14159265359
#endif
#define PI_PER_180 M_PI/180.0
#define RADIANS(angle_in_degrees) (angle_in_degrees)*PI_PER_180

namespace Text {
	mat4 Projection;
	mat4 ModelView(MAT_IDENTITY);
	GLuint texId;
}

static GLint PMODE = GL_FILL;	// polygon mode toggle
static LPPOINT cursorPos = new POINT;	/* holds cursor position */

extern bool active;
extern bool keys[];

static const double GAMMA = 6.67;

float c_vel_fwd = 0, c_vel_side = 0;

GLuint IBOid, FBOid, FBO_textureId;

GLfloat tess_level_inner = 1.0;
GLfloat tess_level_outer = 1.0;

static GLint earth_tex_id, hmap_id;

bool mouseLocked = false;

GLfloat running = 0.0;

static ShaderProgram *regular_shader;
static ShaderProgram *normal_plot_shader;
static ShaderProgram *text_shader;
static ShaderProgram *atmosphere_shader;

static const float dt = 0.01;

mat4 view;
Quaternion viewq;
float qx = 0, qy = 0;
mat4 projection;
vec4 view_position;
vec4 cameraVel;

static std::vector<Model> models;

GLushort * indices; 

bool plot_normals = false;
bool show_atmosphere = false;	 // "atmosphere" xXDDdd

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
	static const float mouse_modifier = 0.4;
	
	if(mouseLocked) {
		unsigned int buttonmask;
		
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
			keys['N'] = false;
		}

		if (keys['P']) {
			PMODE = (PMODE == GL_FILL ? GL_LINE : GL_FILL);
			keys['P'] = false;
		}
		if (keys['F']) {
			if (tess_level_inner > 1.0) { tess_level_inner *= 0.5; }
			keys['F'] = false;
		}
		if (keys['G']) {
			if (tess_level_inner < 64.0) { tess_level_inner *= 2.0; }
			keys['G'] = false;
		}
		if (keys['V']) {
			if (tess_level_outer > 1.0) { tess_level_outer *= 0.5; }
			keys['V'] = false;
		}
		if (keys['B']) {
			if (tess_level_outer < 64.0) { tess_level_outer *= 2.0; }
			keys['B'] = false;
		}

		if (keys['T']) {
			show_atmosphere = !show_atmosphere;
			keys['T'] = false;
		}
		if (dy != 0) {
			rotatey(mouse_modifier*dy);
		}
		if (dx != 0) {
			rotatex(mouse_modifier*dx);
		}


	}

}

static GLuint skybox_VBOid;
static GLuint skybox_facecount;
static mat4 skyboxmat = mat4(MAT_IDENTITY);

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

void initializeStrings() {

	// NOTE: it wouldn't be such a bad idea to just take in a vector 
	// of strings, and to generate one single static VBO for them all.

	std::string string1 = "aglio-olio :D:DD: (biatch)";
	wpstring_holder::append(wpstring(string1, 15, 15), WPS_STATIC);

	std::string frames("Frames per second: ");
	wpstring_holder::append(wpstring(frames, WINDOW_WIDTH-180, WINDOW_HEIGHT-20), WPS_STATIC);

	// reserved index 2 for FPS display. 
	std::string initialfps = "00.00";
	wpstring_holder::append(wpstring(initialfps, WINDOW_WIDTH-50, WINDOW_HEIGHT-20), WPS_DYNAMIC);
	wpstring_holder::append(wpstring("Camera pos: ", 20, WINDOW_HEIGHT-20), WPS_STATIC);
	wpstring_holder::append(wpstring("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 102, WINDOW_HEIGHT-20), WPS_DYNAMIC);
	
	const std::string help2("'p' for polygonmode toggle.");
	wpstring_holder::append(wpstring(help2, WINDOW_WIDTH-220, 35), WPS_STATIC);

	const std::string help3("'n' for normal plot toggle.");
	wpstring_holder::append(wpstring(help3, WINDOW_WIDTH-220, 50), WPS_STATIC);
	
	const std::string help4("'t' for atmosphere toggle.");
	wpstring_holder::append(wpstring(help4, WINDOW_WIDTH-220, 65), WPS_STATIC);

	const std::string help5("'ESC' to lock/unlock mouse.");
	wpstring_holder::append(wpstring(help5, WINDOW_WIDTH-220, 80), WPS_STATIC);

	wpstring_holder::createBufferObjects();

}

void drawText() {
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);

	glBindBuffer(GL_ARRAY_BUFFER, wpstring_holder::get_static_VBOid());

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(0));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(2*sizeof(float)));
	
	glUseProgram(text_shader->getProgramHandle());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Text::texId);

	text_shader->update_uniform_1i("texture1", 0);
	text_shader->update_uniform_mat4("ModelView", (const GLfloat*)Text::ModelView.rawData());
	text_shader->update_uniform_mat4("Projection", (const GLfloat*)Text::Projection.rawData());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wpstring_holder::get_IBOid());
	
	glDrawElements(GL_TRIANGLES, 6*wpstring_holder::get_static_strings_total_length(), GL_UNSIGNED_SHORT, NULL);
		
	glBindBuffer(GL_ARRAY_BUFFER, wpstring_holder::get_dynamic_VBOid());
	// not sure if needed or not
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(0));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 16, BUFFER_OFFSET(2*sizeof(float)));

	glUseProgram(text_shader->getProgramHandle());
	text_shader->update_uniform_1i("texture1", 0);
	text_shader->update_uniform_mat4("ModelView", (const GLfloat*)Text::ModelView.rawData());
	text_shader->update_uniform_mat4("Projection", (const GLfloat*)Text::Projection.rawData());
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wpstring_holder::get_IBOid());
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, Text::texId);

	glDrawElements(GL_TRIANGLES, 6*wpstring_holder::getDynamicStringCount()*wpstring_max_length, GL_UNSIGNED_SHORT, NULL);

	glUseProgram(0);
	glEnable(GL_DEPTH_TEST);

}


#define uniform_assert_warn(uniform) do {\
if (uniform == -1) { \
	logWindowOutput("(warning: uniform optimized away by GLSL compiler: %s at %d:%d\n", #uniform, __FILE__, __LINE__);\
}\
} while(0)

int initGL(void)
{
	glClearColor(0.0, 0.0, 0.0, 1.0);

	glEnable(GL_DEPTH_TEST);

	GLenum err = glewInit();

	if (GLEW_OK != err)
	{
		logWindowOutput( "Error: %s\n", glewGetErrorString(err));
	}

	logWindowOutput( "OpenGL version: %s\n", glGetString(GL_VERSION));
	logWindowOutput( "GLSL version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	logWindowOutput( "Loading models...");
	GLuint sphere_facecount;
	GLuint sphere_VBOid = loadNewestBObj("models/maapallo_napa_korjattu.bobj", &sphere_facecount);
	skybox_VBOid = loadNewestBObj("models/skybox.bobj", &skybox_facecount);
	logWindowOutput( "done.\n");

	logWindowOutput( "Loading textures...");
	indices = generateIndices();
	TextureBank::add(Texture("textures/EarthTexture.jpg", GL_LINEAR));
	TextureBank::add(Texture("textures/earth_height_normal_map.jpg", GL_LINEAR));
	TextureBank::add(Texture("textures/dina_all.png", GL_NEAREST));
	logWindowOutput( "done.\n");
	
	if (!TextureBank::validate()) {
		logWindowOutput( "Error: failed to validate TextureBank (fatal).\n");
		return 0;
	}
	earth_tex_id = TextureBank::get_id_by_name("textures/EarthTexture.jpg");
	hmap_id = TextureBank::get_id_by_name("textures/earth_height_normal_map.jpg");
	Text::texId = TextureBank::get_id_by_name("textures/dina_all.png");
	
	regular_shader = new ShaderProgram("shaders/regular"); 
	normal_plot_shader = new ShaderProgram("shaders/normalplot");
	text_shader = new ShaderProgram("shaders/text_shader");
	atmosphere_shader = new ShaderProgram("shaders/atmosphere");

	if (regular_shader->is_bad() || normal_plot_shader->is_bad() || text_shader->is_bad() || atmosphere_shader->is_bad()) { 
		logWindowOutput( "Error: shader error (fatal).\n");
		return 0; 
	}


	glGenBuffers(1, &IBOid);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOid);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short int)*3*facecount, indices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*(GLushort)(0xFFFF), indices, GL_STATIC_DRAW);

	delete [] indices;	

	glPatchParameteri(GL_PATCH_VERTICES, 3);

	//GLuint programHandle = regular_shader->getProgramHandle();

	//glBindFragDataLocation(programHandle, 0, "out_frag_color");
	regular_shader->construct_uniform_map();
	normal_plot_shader->construct_uniform_map();
	text_shader->construct_uniform_map();
	atmosphere_shader->construct_uniform_map();


	//GLuint fragloc = glGetFragDataLocation( programHandle, "out_frag_color"); uniform_assert_warn(fragloc);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);


	view = mat4(MAT_IDENTITY);

	projection = mat4::proj_persp(M_PI/8.0, (float)WINDOW_WIDTH/(float)WINDOW_HEIGHT, 2.0, 1000.0);

	Text::Projection = mat4::proj_ortho(0.0, WINDOW_WIDTH, WINDOW_HEIGHT, 0.0, -1.0, 1.0);
	
	Text::ModelView = mat4(MAT_IDENTITY);

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

	// fbo generation for shadow mapping stuff.

	glGenFramebuffers(1, &FBOid);
	glBindFramebuffer(GL_FRAMEBUFFER, FBOid);

	glGenTextures(1, &FBO_textureId);
	glBindTexture(GL_TEXTURE_2D, FBO_textureId);

static int SHADOW_MAP_WIDTH = 1*WINDOW_WIDTH;
static int SHADOW_MAP_HEIGHT = 1*WINDOW_HEIGHT;

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );	// these two are related to artifact mitigation
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, FBO_textureId, 0);

	glDrawBuffer(GL_NONE);	// this, too, has something to do with only including the depth component 
	glReadBuffer(GL_NONE);

	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		logWindowOutput( "error: Shadow-map FBO initialization failed!\n");
		return 0;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);


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
	glPolygonMode(GL_FRONT_AND_BACK, PMODE);
	glUseProgram(regular_shader->getProgramHandle());	

	running += 0.015;
	if (running > 1000000) { running = 0; }

	regular_shader->update_uniform_1f("running", running);
	regular_shader->update_uniform_mat4("Projection", (const GLfloat*)projection.rawData());

	static const GLuint sphere_VBOid = models[0].getVBOid();
	static const GLuint sphere_facecount = models[0].getFaceCount();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOid);  // is still in full matafaking effizzect :D */
	glBindBuffer(GL_ARRAY_BUFFER, sphere_VBOid);	 // BIND BUFFER FOR ALL MATAFAKING SPHERES.
	static const float dt = 0.01;

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(0));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(3*sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), BUFFER_OFFSET(6*sizeof(float)));


	models[0].rotation = Quaternion::fromAxisAngle(1.0, 0.0, 0.0, RADIANS(45));
	//models[0].rotation *= Quaternion::fromAxisAngle(0.0, 1.0, 0.0, running);
	models[0].rotation.normalize();
//	models[1].rotation = Quaternion::fromAxisAngle(-0.2, 1.0, 0.0, 0.5*running);
	models[1].rotation.normalize();

	viewq.normalize();
	view = viewq.toRotationMatrix();
	view = view*view_position.toTranslationMatrix();

	// no need to reconstruct iterator every time
	static std::vector<Model>::iterator current;
	current = models.begin();
	mat4 model_rotation = (*current).rotation.toRotationMatrix();

	//(*current)->model_matrix.print();

	//printMatrix4f(model_rotation);
	mat4 modelview = view * (*current).model_matrix * model_rotation;

	regular_shader->update_uniform_mat4("ModelView", (const GLfloat*)modelview.rawData());
	vec4 light_pos(sin(running), cos(0.11*running), -5.0, 1.0);
	
	//if (!(*current).lightsrc()) {
		vec4 light_dir = vec4(sin(running), 0, cos(running), 0.0);
		light_dir.normalize();
		regular_shader->update_uniform_vec4("light_dir", (const GLfloat*)light_dir.rawData());
	//}
	regular_shader->update_uniform_1f("TESS_LEVEL_INNER", tess_level_inner); 
	regular_shader->update_uniform_1f("TESS_LEVEL_OUTER", tess_level_outer);
	regular_shader->update_uniform_1i("lightsrc", current->lightsrc());

	glBindBuffer(GL_ARRAY_BUFFER, (*current).getVBOid());
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, (*current).getTextureId());
	regular_shader->update_uniform_1i("texture_color", 0);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, hmap_id);	// heightmap
	regular_shader->update_uniform_1i("height_map", 1);

	glDisable(GL_BLEND);
	glDrawElements(GL_PATCHES, (*current).getFaceCount()*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 

	// draw atmosphere

	if (show_atmosphere) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glUseProgram(atmosphere_shader->getProgramHandle());

		mat4 scaled = mat4::scale(vec4(1.1, 1.1, 1.1, 1.0));
		mat4 atmosphere_modelview = view * (scaled*(*current).model_matrix)*model_rotation;

		atmosphere_shader->update_uniform_mat4("ModelView", (const GLfloat*) atmosphere_modelview.rawData());
		atmosphere_shader->update_uniform_mat4("Projection", (const GLfloat*) projection.rawData());
		glDrawElements(GL_TRIANGLES, (*current).getFaceCount()*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
	}

	/*for (; current != models.end(); ++current)
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

		//glDrawElements(GL_TRIANGLES, (*current).getFaceCount()*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 
		glDrawElements(GL_PATCHES, (*current).getFaceCount()*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0)); 
	}
*/

	// draw normals for center sphere

	if (plot_normals) {
		glUseProgram(normal_plot_shader->getProgramHandle());
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, hmap_id);
		normal_plot_shader->update_uniform_1i("heightmap", 1);

		mat4 modelview = view * models[0].model_matrix * models[0].rotation.toRotationMatrix();
		normal_plot_shader->update_uniform_mat4("ModelView", (const GLfloat*) modelview.rawData());
		normal_plot_shader->update_uniform_mat4("Projection", (const GLfloat*) projection.rawData());

		glDrawElements(GL_POINTS, models[0].getFaceCount()*3, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
	}
}

/* callbacks */



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	std::string cpustr(checkCPUCapabilities());
	if (cpustr.find("ERROR") != std::string::npos) { MessageBox(NULL, cpustr.c_str(), "Fatal error.", MB_OK); return -1; }

	MSG msg;
	BOOL done=FALSE;

	if(!CreateGLWindow("opengl framework stolen from NeHe", WINDOW_WIDTH, WINDOW_HEIGHT, 32, FALSE)) { return 1; }
	
	logWindowOutput("%s\n", cpustr.c_str());
	//ShowCursor(FALSE);

	bool esc = false;
	initializeStrings();
	
	_timer timer;
	
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
					long us = timer.get_us();
					double fps = 1000000/us;
					static char buffer[128];
					int l = sprintf(buffer, "%4.2f", fps);
					buffer[l] = '\0';
					std::string fps_str(buffer);

					wpstring_holder::updateDynamicString(0, fps_str);
					l = sprintf(buffer, "(%4.2f, %4.2f, %4.2f)", view_position(V::x), view_position(V::y), view_position(V::z));

					buffer[l] = '\0';
					std::string pos_str(buffer);
					wpstring_holder::updateDynamicString(1, pos_str);
	
					drawText();

					window_swapbuffers();
					timer.begin();
				}
			}
		}

	}

	KillGLWindow();
	glDeleteBuffers(1, &IBOid);
	return (msg.wParam);
}

