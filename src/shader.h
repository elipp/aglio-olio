#ifndef SHADER_H
#define SHADER_H
#include <Windows.h>

#include <GL/glew.h>
#include <GL/GL.h>
#include <fstream>

#define SHADER_NONE -1

typedef enum { VertexShader = 0, GeometryShader = 1, FragmentShader = 2 };

class ShaderProgram {
	GLuint programHandle;
	GLuint shaderObjIDs[3];	// [0] => VS_id, [1] => GS_id, [2] => FS_id
	bool bad;
public:
	GLuint getProgramHandle() const { return programHandle; }
	ShaderProgram(const std::string &vs_filename, const std::string &gs_filename, const std::string &fs_filename);	// supply string "null" if none used
	GLint checkShaders(GLint QUERY);
	bool is_bad() const { return bad; }

	static char* readShaderFromFile(const std::string &filename, size_t *filesize);

};


#endif