#version 330 core

precision highp float;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_texcoord;	// not used though

uniform mat4 ModelView;
uniform sampler2D heightmap;

out vec3 vnormal;
out vec2 vtexcoord;

mat3 normal_matrix = mat3(inverse(transpose(mat3(ModelView))));

void main(void)
{
	float height_sample = length(texture2D(heightmap, vec2(in_texcoord.x, in_texcoord.y + 0.5)));
	vec4 normal_map_sample = texture2D(heightmap, in_texcoord);
	vec3 normal_adjusted = normalize(in_normal + normal_map_sample.xyz);
	vec3 normal_normalized = normalize(in_normal);
	gl_Position = vec4(in_position + normal_normalized*height_sample, 1.0);
	//vnormal = (ModelView*vec4(in_normal, 0.0)).xyz;
	vnormal=normal_matrix*(in_normal + normal_adjusted);
	//vnormal = vec4(in_normal,1.0);
	vtexcoord = in_texcoord;
}
