#ifndef MODEL_H
#define MODEL_H

#include <GL/gl.h>

#include "lin_alg.h"
#include "common.h"
#include <cstdlib>

#define ALIGNMENT 16

class Model {

public:	
	// apparently, the default allocator can return misaligned addresses, need to overload
	mat4 model_matrix;
	Quaternion rotation;
	vec4 position;
	vec4 velocity;
	
	float scale;
	float radius;

	float mass;
	//pyorimismaara? tensori :D:D
	
	Model(float mass, float scale, GLuint VBOid, GLuint texId, GLuint facecount, bool lightsrc, bool _fixed_pos = false);

	GLuint getVBOid() const { return VBOid; }
	GLuint getTextureId() const { return textureId; }
	GLuint getFaceCount() const { return facecount; }
	bool lightsrc() const { return is_a_lightsrc; }


	void translate(const vec4 &vec);
	void updatePosition();

	void *operator new(std::size_t size)  { 
		void *p;
		posix_memalign(&p, ALIGNMENT, size);
		return p;
	}
	
	void operator delete(void *p) { 
		Model* M = static_cast<Model*>(p);
		free(M);
	}
	
private:
	GLuint VBOid;
	GLuint textureId;
	GLuint facecount;
	bool is_a_lightsrc;
	bool fixed_pos;

} __attribute__((aligned(ALIGNMENT)));

#endif
