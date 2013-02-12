
#include "common.h"
#include "shader.h"

static char logbuffer[1024];

#define RED_BOLD "\033[1;31m"
#define COLOR_RESET "\033[0m"
#define set_bad() do {\
	bad = false;\
	std::cerr << RED_BOLD << "Program " << id_string << ": bad flag set @ " << __FILE__ << ":"<< __LINE__ << COLOR_RESET << "\n";\
} while(0)

ShaderProgram::ShaderProgram(const std::string &name_base) { 	

	bad = false;

	id_string = name_base;
	shader_filenames[VertexShader] = name_base + "/vs";
	shader_filenames[TessellationControlShader] = name_base + "/tcs";
	shader_filenames[TessellationEvaluationShader] = name_base + "/tes";
	shader_filenames[GeometryShader] = name_base + "/gs";
	shader_filenames[FragmentShader] = name_base + "/fs";

	GLsizei vs_len, tcs_len, tes_len, gs_len, fs_len;

	char *vs_buf = NULL, 
	     *tcs_buf = NULL,
	     *tes_buf = NULL,
	     *gs_buf = NULL, 
	     *fs_buf = NULL;

	
	// note: program will leak memory on error, but then again these kinds of errors are considered "fatal"

	// read applicable shader source files into buffers, run glCreateShader

	vs_buf = ShaderProgram::readShaderFromFile(shader_filenames[VertexShader], &vs_len);
	if (!vs_buf) { set_bad(); goto cleanup; }	// vertex shader is mandatory
	shaderObjIDs[VertexShader] = glCreateShader(GL_VERTEX_SHADER);

	tcs_buf = ShaderProgram::readShaderFromFile(shader_filenames[TessellationControlShader], &tcs_len);
	if (tcs_buf) {
		shaderObjIDs[TessellationControlShader] = glCreateShader(GL_TESS_CONTROL_SHADER);

		tes_buf = ShaderProgram::readShaderFromFile(shader_filenames[TessellationEvaluationShader], &tes_len);
		if (tes_buf) { 
			shaderObjIDs[TessellationEvaluationShader] = glCreateShader(GL_TESS_EVALUATION_SHADER);
		}	
		else {
			std::cerr << "ShaderProgram error:" << name_base << ": TessellationControlShader enabled but no TessellationEvaluationShader provided.\n";
			set_bad(); goto cleanup;
		}
	}

	gs_buf = ShaderProgram::readShaderFromFile(shader_filenames[GeometryShader], &gs_len);
	if (gs_buf) {
		shaderObjIDs[GeometryShader] = glCreateShader(GL_GEOMETRY_SHADER);
	}

	fs_buf = ShaderProgram::readShaderFromFile(shader_filenames[FragmentShader], &fs_len);
	if (!fs_buf) { set_bad(); goto cleanup; } // fragment shader is mandatory
	shaderObjIDs[FragmentShader] = glCreateShader(GL_FRAGMENT_SHADER);

	// shader sources
	glShaderSource(shaderObjIDs[VertexShader], 1, (const GLchar**)&vs_buf,  (const GLint*)&vs_len);

	if (shaderObjIDs[TessellationControlShader] != SHADER_NONE) {
		glShaderSource(shaderObjIDs[TessellationControlShader], 1, (const GLchar**)&tcs_buf, (const GLint*)&tcs_len);
	}
	if (shaderObjIDs[TessellationEvaluationShader] != SHADER_NONE) {
		glShaderSource(shaderObjIDs[TessellationEvaluationShader], 1, (const GLchar**)&tes_buf, (const GLint*)&tes_len);
	}
	
	if (shaderObjIDs[GeometryShader] != SHADER_NONE) {
		glShaderSource(shaderObjIDs[GeometryShader], 1, (const GLchar**)&gs_buf, (const GLint*)&gs_len);
	}
	glShaderSource(shaderObjIDs[FragmentShader], 1, (const GLchar**)&fs_buf, (const GLint*)&fs_len);

	// compile everything
	for (int i = VertexShader; i <= FragmentShader; i++) {
		if (shaderObjIDs[i] != SHADER_NONE) { 
			glCompileShader(shaderObjIDs[i]);
		}
	}
	programHandle = glCreateProgram();              

	// attach
	
	for (int i = VertexShader; i <= FragmentShader; i++) {
		if (shaderObjIDs[i] != SHADER_NONE) {
			glAttachShader(programHandle, shaderObjIDs[i]);
		}
	}

	glBindAttribLocation(programHandle, 0, "Position_VS_in");
	glBindAttribLocation(programHandle, 1, "Normal_VS_in");
	glBindAttribLocation(programHandle, 2, "TexCoord_VS_in");

	glLinkProgram(programHandle);
	glUseProgram(programHandle);

	if (!checkShaderCompileStatus_all()) 
	{
		set_bad();
		goto cleanup;
	}
	if (!checkProgramLinkStatus()) {
		set_bad();
		goto cleanup;
	}

	#define my_delete_arr(arr) if(arr) delete [] arr; arr = NULL;
cleanup:
	my_delete_arr(vs_buf);
	my_delete_arr(tcs_buf);
	my_delete_arr(tes_buf);
	my_delete_arr(gs_buf); 
	my_delete_arr(fs_buf);

	printStatus();
}

inline const std::string shader_present(const GLuint *objIDs, GLint index) {
	if (objIDs[index] != SHADER_NONE) { return "yes"; }
	else { return "no"; }
}

void ShaderProgram::printStatus () const {
	std::cerr << "ShaderProgram " << id_string << " status:\nshader\t\t\tpresent?\tid\n";
	std::cerr << "Vertex shader: \t\t" << shader_present(shaderObjIDs, VertexShader) << "\t\t" << shaderObjIDs[VertexShader] << "\n";
	std::cerr << "TessCtrl shader: \t" << shader_present(shaderObjIDs, TessellationControlShader) << "\t\t" << shaderObjIDs[TessellationControlShader] << "\n";
	std::cerr << "TessEval shader: \t" << shader_present(shaderObjIDs, TessellationEvaluationShader) << "\t\t" << shaderObjIDs[TessellationEvaluationShader] << "\n";
	std::cerr << "Geometry shader: \t" << shader_present(shaderObjIDs, GeometryShader) << "\t\t" << shaderObjIDs[GeometryShader] << "\n";
	std::cerr << "Fragment shader: \t" << shader_present(shaderObjIDs, FragmentShader) << "\t\t" << shaderObjIDs[FragmentShader] << "\n";
	std::cerr << "bad flag:" << bad << "\n\n";
}

char* ShaderProgram::readShaderFromFile(const std::string &filename, GLsizei *filesize)
{
	std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);

	if (!in.is_open()) { 
		fprintf(stderr, "(warning: ShaderProgram: couldn't open file %s: no such file or directory)\n", filename.c_str());
		return NULL; 
	}
	size_t length = cpp_getfilesize(in);
	*filesize = length;
	char *buf = new char[length+1];
	in.read(buf, length);
	in.close();

	buf[length] = '\0';

	return buf;
}

GLint ShaderProgram::checkShaderCompileStatus_all() // GL_COMPILE_STATUS
{	
	// should check GL_LINK_STATUS as well (glGetProgramInfoLog for linker)
	GLint succeeded[5] = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };
	GLchar *log_buffers[5] = { NULL, NULL, NULL, NULL, NULL };
	GLint num_errors = 0;

	for (int i = VertexShader; i <= FragmentShader; i++) {
		if (shaderObjIDs[i] != SHADER_NONE) {
			glGetShaderiv(shaderObjIDs[i], GL_COMPILE_STATUS, &succeeded[i]);

			if (succeeded[i] == GL_FALSE)
			{	
				std::cerr << "glGetShaderiv returned GL_FALSE for query GL_COMPILE_STATUS for shader " << shader_filenames[i] << ", id:"<< shaderObjIDs[i]<<"\n";
				GLint log_length = 0;
				++num_errors;
				glGetShaderiv(shaderObjIDs[i], GL_INFO_LOG_LENGTH, &log_length);
				log_buffers[i] = new GLchar[log_length + 1];
				memset(log_buffers[i], 0, log_length);

				glGetShaderInfoLog(shaderObjIDs[i], log_length, NULL, log_buffers[i]);

				log_buffers[i][log_length-1] = '\0';

				std::ofstream logfile("shader.log", std::ios::out | std::ios::app);	
				logfile << log_buffers[i];
				logfile.close();

				// also print to Visual Studio output window
			}
		}
	}

	if (num_errors > 0) {	

		fprintf(stderr, "\nShader %s: error log (glGetShaderInfoLog):\n-----------------------------------------------------------------\n\n", id_string.c_str());
		for (int i = VertexShader; i <= FragmentShader; i++) {
			if (succeeded[i] != GL_TRUE) {
				fprintf(stderr, "filename: \033[1m %s\033[0m\n\n", shader_filenames[i].c_str());
				fprintf(stderr, "%s\n\n", log_buffers[i]);
			}
		}
		std::cerr << "\n---------------------------------------------------\n";
	}
	for (int i = VertexShader; i <= FragmentShader; i++) {	
		if (log_buffers[i]) delete [] log_buffers[i];
		log_buffers[i] = NULL;
	}
	return num_errors > 0 ? 0 : 1;
}

GLint ShaderProgram::checkProgramLinkStatus() {
	GLint status;
	GLsizei log_len;
	glGetProgramiv(programHandle, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		glGetProgramInfoLog(programHandle, sizeof(logbuffer), &log_len, logbuffer);
		std::cerr << "ShaderProgram::checkProgramLinkStatus: shader program link error. Log:\n" << logbuffer << "\n";
		return 0;
	}
	else {
		glGetProgramInfoLog(programHandle, sizeof(logbuffer), &log_len, logbuffer);
		std::cerr << "Program " << id_string << " link: \n";
		if (log_len > 0) { std::cerr  << "Link log: \n" << logbuffer << "\n\n"; }

		glValidateProgram(programHandle);
		glGetProgramInfoLog(programHandle, sizeof(logbuffer), &log_len, logbuffer);
		std::cerr << "Program " << id_string << " validation: \n";
		if (log_len > 0) { std::cerr  << logbuffer << "\n"; }

		std::cerr << "\n";
		return 1;
	}
}



/*	std::ofstream logfile("shader.log", std::ios::out | std::ios::app);

	logfile << "compilation of GLSL shader source file "<< filename << " failed. Contents: \n\n";

	logfile.write(buf, length);
	logfile.put('\n');
	logfile.put('\n');
	logfile.close();
 */
