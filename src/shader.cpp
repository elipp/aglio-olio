
#include "common.h"
#include "shader.h"

/*class ShaderProgram {
	GLuint programHandle;
public:
	GLuint getProgramHandle() const { return programHandle; }
	ShaderProgram(const std::string &vs_filename, const std::string &gs_filename, const std::string &fs_filename);	// supply string "none" if no gs is to be  used
	static char* readShaderFromFile(const std::string &filename);
	GLint checkShader(GLuint *shaderId, GLint QUERY); */

ShaderProgram::ShaderProgram(const std::string &vs_filename, const std::string &gs_filename, const std::string &fs_filename) {

	size_t vlen, glen, flen;
	char *vs_buf = NULL, *gs_buf = NULL, *fs_buf = NULL;
	vs_buf = ShaderProgram::readShaderFromFile(vs_filename, &vlen);
	if (!vs_buf) { bad = true; return; }

	if (gs_filename != "none") {
		gs_buf = ShaderProgram::readShaderFromFile(gs_filename, &glen);
		if (!gs_buf) { bad = true; return; }
	}
	else {
		shaderObjIDs[GeometryShader] = SHADER_NONE;
	}
	
	fs_buf = ShaderProgram::readShaderFromFile(fs_filename, &flen);
	if (!fs_buf) { bad = true; return; }


	shaderObjIDs[VertexShader] = glCreateShader(GL_VERTEX_SHADER);

	if (shaderObjIDs[GeometryShader] != SHADER_NONE) {
		shaderObjIDs[GeometryShader] = glCreateShader(GL_GEOMETRY_SHADER);
	}
	shaderObjIDs[FragmentShader] = glCreateShader(GL_FRAGMENT_SHADER);	


	
	glShaderSource(shaderObjIDs[VertexShader], 1, (const GLchar**)&vs_buf,  (const GLint*)&vlen);
	if (shaderObjIDs[GeometryShader] != SHADER_NONE) {
		glShaderSource(shaderObjIDs[GeometryShader], 1, (const GLchar**)&gs_buf, (const GLint*)&glen);
	}
	glShaderSource(shaderObjIDs[FragmentShader], 1, (const GLchar**)&fs_buf, (const GLint*)&flen);
	
	delete [] vs_buf;
	if (gs_buf) { delete [] gs_buf; }
	delete [] fs_buf;

	glCompileShader(shaderObjIDs[VertexShader]);
	if (shaderObjIDs[GeometryShader] != SHADER_NONE) {
		glCompileShader(shaderObjIDs[GeometryShader]);
	}
	glCompileShader(shaderObjIDs[FragmentShader]);

	programHandle = glCreateProgram();              

	
	glAttachShader(programHandle, shaderObjIDs[VertexShader]);
	if (shaderObjIDs[GeometryShader] != SHADER_NONE) {
		glAttachShader(programHandle, shaderObjIDs[GeometryShader]);
	}
	glAttachShader(programHandle, shaderObjIDs[FragmentShader]);

	
	glBindAttribLocation(programHandle, 0, "in_position");
	glBindAttribLocation(programHandle, 1, "in_normal");
	glBindAttribLocation(programHandle, 2, "in_texcoord");

	glLinkProgram(programHandle);
	glUseProgram(programHandle);

	if (!checkShaders(GL_COMPILE_STATUS))
	{
		bad = true;
		return;
	}


	bad = false;
}

char* ShaderProgram::readShaderFromFile(const std::string &filename, size_t *filesize)
{
	std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);

	if (!in.is_open()) { printf("ShaderProgram: couldn't open file %s (doesn't exist?)\n", filename.c_str()); return NULL; }
	size_t length = cpp_getfilesize(in);
	*filesize = length;
	char *buf = new char[length+1];
	in.read(buf, length);
	in.close();
	
	buf[length] = '\0';
	
	return buf;
}

GLint ShaderProgram::checkShaders(GLint QUERY) // QUERY = usually GL_COMPILE_STATUS
{	
	GLint succeeded[3] = { 1 };
	GLchar *log_buffers[3] = { NULL };
	GLint has_errors = 0;

	for (int i = 0; i < 3; i++) {
		if (shaderObjIDs[i] == SHADER_NONE) { continue; }

		glGetShaderiv(shaderObjIDs[i], QUERY, &succeeded[i]);

		if (!succeeded[i])
		{	
			GLint log_length = 0;
			has_errors = 1;
			glGetShaderiv(shaderObjIDs[i], GL_INFO_LOG_LENGTH, &log_length);
			log_buffers[i] = new GLchar[log_length + 1];
			memset(log_buffers[i], 0, log_length);
			
			glGetShaderInfoLog(shaderObjIDs[i], log_length, NULL, log_buffers[i]);
	
			log_buffers[i][log_length-1] = '\0';
		
			std::ofstream logfile("shader.log", std::ios::out | std::ios::app);	
			logfile << log_buffers[i];
			logfile.close();

			// also print to MSVC output window
		
		}
	}

	if (has_errors) {	
		std::cerr << "\nSHADER ERROR OUTPUT:\n-----------------------------------------------------------------";
		for (int i = 0; i < 3; i++) {
			if (!succeeded[i]) {
				std::cerr << "\n\n";
				std::cerr << log_buffers[i];
				std::cerr << "\n\n";
			}
		}
		std::cerr << "\n---------------------------------------------------\n\n";
	}
	for (int i = 0; i < 3; i++) {	
		delete [] log_buffers[i];
		log_buffers[i] = NULL;
	}
	return has_errors ? 0 : 1;
}


/*	std::ofstream logfile("shader.log", std::ios::out | std::ios::app);
	
	logfile << "compilation of GLSL shader source file "<< filename << " failed. Contents: \n\n";

	logfile.write(buf, length);
	logfile.put('\n');
	logfile.put('\n');
	logfile.close();
*/
