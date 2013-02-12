#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h>
#include <GL/gl.h>
#include <fstream>
#include <cstring> // for memset and stuff
#include <iostream>

#define SHADER_NONE (GLuint)-1
#define SHADER_SUCCESS GL_TRUE

typedef enum { 
		VertexShader = 0, 
		TessellationControlShader = 1, 
		TessellationEvaluationShader = 2, 
		GeometryShader = 3, 
		FragmentShader = 4
};

class ShaderProgram {
	std::string id_string;
	std::string shader_filenames[5];
	GLuint programHandle;
	GLuint shaderObjIDs[5] = { SHADER_NONE, SHADER_NONE, SHADER_NONE, SHADER_NONE, SHADER_NONE };	// [0] => VS_id, [1] => TCS_id, [2] => TES_id, [3] => GS_id, [4] => FS_id
	bool bad;
public:
	GLuint getProgramHandle() const { return programHandle; }
	ShaderProgram(const std::string &name_base); // extensions are appended to the name base, see shader.cpp

	void printStatus() const;
	GLint checkShaderCompileStatus_all();
	GLint checkProgramLinkStatus();
	bool is_bad() const { return bad; }

	static char* readShaderFromFile(const std::string &filename, GLsizei *filesize);
	std::string get_id_string() const { return id_string; }
	std::string get_vs_filename() const { return shader_filenames[VertexShader]; }
	std::string get_tcs_filename() const { return shader_filenames[TessellationControlShader]; }
	std::string get_tes_filename() const { return shader_filenames[TessellationEvaluationShader]; }
	std::string get_gs_filename() const { return shader_filenames[GeometryShader]; }
	std::string get_fs_filename() const { return shader_filenames[FragmentShader]; }

};


#endif
