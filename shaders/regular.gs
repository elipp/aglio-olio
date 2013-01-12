#version 400

layout(triangles) in;
//layout(line_strip) out;
layout(triangle_strip) out;
layout(max_vertices = 3) out;

precision highp float;

uniform mat4 ModelView;
uniform mat4 Projection;
uniform sampler2D texture_color;
uniform vec4 light;
uniform int lightsrc;
uniform float running;
in vec2 vtexcoord[];
in vec3 vnormal[];

out vec3 fcolor;

void main()
{
	// simple pass-through
	for (int i = 0; i < 3; i++) {
		const vec4 normalt = normalize(ModelView*vec4(vnormal[i], 0.0));
		const float dotp = dot(normalt, light);
		const vec4 c = texture2D(texture_color, vtexcoord[i]);
		const vec4 specular = c*pow(dotp, 1.2);

		gl_Position = Projection*ModelView*gl_in[i].gl_Position;
		if (lightsrc == 0) {
			fcolor = c.xyz*clamp(dotp, 0.0, 1.0) + specular.xyz;
		}
		else { 
			fcolor = c;
		}	
		EmitVertex();
	}
	EndPrimitive();

	/*const float dotp = dot(normalize(vec4(vnormal[0], 1.0)), light);
	const vec4 c = texture2D(texture_color, vtexcoord[0]);
	vec4 pos = gl_in[0].gl_Position;
	vec4 crossp = 0.5*vec4(normalize(cross(pos.xyz, vnormal[0])), 0.0);
	
	fcolor = (0.1*c*clamp(dotp, 0.0, 1.0)).xyz;
	gl_Position = Projection*ModelView*gl_in[0].gl_Position;
	EmitVertex();
	
	fcolor = vec3(c.y*clamp(dotp, 0.0, 1.0), c.x*sin(running), dotp);
	gl_Position = Projection*ModelView*(pos + vec4(vnormal[0], 0.0));
	EmitVertex();

	fcolor = vec3(c.z*clamp(dotp, 0.0, 1.0), c.y*sin(0.3*running), 1.0);
	gl_Position = Projection*ModelView*(pos + vec4(vnormal[0], 0.0) + crossp);
	EmitVertex();*/
	
	EndPrimitive();
}