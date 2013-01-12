#version 150 core
 
precision highp float;

//in vec3 vvertex;

//in vec3 fcolor;
in vec3 vnormal;
in vec2 vtexcoord;

uniform float running;

uniform mat4 ModelView;
uniform sampler2D texture_color;
uniform sampler2D heightmap;
uniform vec4 light;
uniform int lightsrc;

const vec3 ambient = vec3( 0.02, 0.02, 0.02 );

float diffuse = 0.0;

vec3 lightColor = vec3( 0.8, 0.3, 0.3);
layout(location=0) out vec4 out_frag_color;
 
void main(void)
{
	
	vec4 normal_sample = texture2D(heightmap, vtexcoord);
	vec3 normal_adjusted = normalize(vnormal + normal_sample.xyz);
	if (lightsrc == 0) { diffuse = clamp( dot( light, normalize( normal_adjusted.xyz )), 0.0, 1.0); }
	else { diffuse = 1.0; }
	//float diffuse = clamp( dot( light.xyz, normalize( vnormal.xyz )), 0.0, 1.0);
	
	vec4 texColor = texture2D(texture_color, vtexcoord);
	const vec3 solid_blue = vec3(0.0, 0.0, 1.0);
	if (texColor.rgb == solid_blue) { out_frag_color = vec4(0.05*cos(normal_sample.y*running)+0.05, 0.05*cos(normal_sample.y*5.0*running)+0.2, 0.1*sin(normal_sample.x*1.3*running)+0.6, 1.0); }
	else {
		out_frag_color = vec4( ambient + diffuse*texColor.xyz, 1.0 );
	}


}