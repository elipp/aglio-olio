#version 410 core

layout (location = 0) in vec3 Position_VS_in;
layout (location = 1) in vec3 Normal_VS_in;
layout (location = 2) in vec2 TexCoord_VS_in;

uniform float running;
uniform mat4 ModelView;
uniform mat4 Projection;
uniform sampler2D texture_color;
uniform sampler2D heightmap;

//mat4 normalmatrix = transpose(inverse(ModelView));
//mat4 normalmatrix = ModelView;
//out vec3 vvertex;
out vec3 WorldPos_CS_in;
out vec3 Normal_CS_in;
out vec2 TexCoord_CS_in;

mat3 normal_matrix = transpose(inverse(mat3(ModelView)));

void main(void)
{
	
	//gl_Position = Projection*ModelView*vec4(Position_VS_in.x+0.2*sin(Position_VS_in.y+3*running), Position_VS_in.y, Position_VS_in.z, 1.0); // 
	float height_sample = length(texture2D(heightmap, vec2(TexCoord_VS_in.s, TexCoord_VS_in.t + 0.5)));
	vec3 normal_normalized = normalize(Normal_VS_in);
	vec4 texColor = texture2D(texture_color, TexCoord_VS_in);

	//if (height_sample > 0) { height_sample = 0.05+pow(height_sample, 1.2); }
	//else { height_sample = 0; }
	WorldPos_CS_in = ModelView*vec4(Position_VS_in + normal_normalized*height_sample, 1.0);
	Normal_CS_in = normal_matrix*Normal_VS_in; 
	TexCoord_CS_in = TexCoord_VS_in;
}



//vec4(in_position.x + sin(in_position.y  + running), in_position.y, in_position.z, 1.0)
