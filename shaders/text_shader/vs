#version 330 

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec2 in_texcoord;

uniform mat4 Projection;
uniform mat4 ModelView;

//out vec2 vpos;	// not really needed
out vec2 vtexcoord;

void main(void) {

	vec4 real_pos = Projection*ModelView*vec4(in_position, 0.0, 1.0);
//	vpos = real_pos.xy;
	gl_Position = real_pos;
	vtexcoord = in_texcoord;

}
