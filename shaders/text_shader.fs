#version 330

//in vec2 vpos;
in vec2 vtexcoord;

uniform sampler2D texture1;

layout(location = 0) out vec4 out_fragcolor;

void main(void) {

	vec2 tc_inverted_v = vec2(vtexcoord.x, 1-vtexcoord.y);
	vec4 col = texture2D(texture1, tc_inverted_v);
	//float real_a = col.g;
	if (col.g != 1.0) {
		discard;
	}
	else {
		out_fragcolor = vec4(1.0, 1.0, 1.0, 1.0);
	}

}


