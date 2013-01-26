#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>
#include <GL/gl.h>
#include <fstream>
#include <cstring> // for memset and stuff
#include <iostream>

#define SHADER_NONE (GLuint)-1
#define SHADER_SUCCESS 1

typedef enum { VertexShader = 0, GeometryShader = 1, FragmentShader = 2 };

class ShaderProgram {
	std::string id_string;
	std::string shader_filenames[3];
	GLuint programHandle;
	GLuint shaderObjIDs[3] = { SHADER_NONE };	// [0] => VS_id, [1] => GS_id, [2] => FS_id
	bool bad;
public:
	GLuint getProgramHandle() const { return programHandle; }
	ShaderProgram(const std::string &id_string, const std::string &vs_filename, const std::string &gs_filename, const std::string &fs_filename);	// supply string "null" if none used
	GLint checkShaders(GLint QUERY);
	bool is_bad() const { return bad; }

	static char* readShaderFromFile(const std::string &filename, size_t *filesize);
	std::string get_id_string() const { return id_string; }
	std::string get_vs_filename() const { return shader_filenames[VertexShader]; }
	std::string get_gs_filename() const { return shader_filenames[GeometryShader]; }
	std::string get_fs_filename() const { return shader_filenames[FragmentShader]; }

};


#endif
