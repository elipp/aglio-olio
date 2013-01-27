#version 330 core

precision highp float;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord;

uniform float running;
uniform mat4 ModelView;
uniform mat4 Projection;
uniform sampler2D texture_color;
uniform sampler2D heightmap;

//mat4 normalmatrix = transpose(inverse(ModelView));
//mat4 normalmatrix = ModelView;
//out vec3 vvertex;
out vec3 vnormal;
out vec2 vtexcoord;

mat3 normal_matrix = transpose(inverse(mat3(ModelView)));

void main(void)
{
	
	//gl_Position = Projection*ModelView*vec4(in_position.x+0.2*sin(in_position.y+3*running), in_position.y, in_position.z, 1.0); // 
	float height_sample = length(texture2D(heightmap, vec2(in_texcoord.s, in_texcoord.t + 0.5)));
	vec3 normal_normalized = normalize(in_normal);
	vec4 texColor = texture2D(texture_color, in_texcoord);

	if (height_sample > 0) { height_sample = 0.05+pow(height_sample, 1.2); }
	else { height_sample = 0; }
	gl_Position = Projection*ModelView*vec4(in_position + normal_normalized*height_sample, 1.0);
	vnormal = normal_matrix*in_normal;
	vtexcoord = in_texcoord;
}



//vec4(in_position.x + sin(in_position.y  + running), in_position.y, in_position.z, 1.0)
