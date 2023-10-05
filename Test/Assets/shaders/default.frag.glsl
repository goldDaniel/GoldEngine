#version 460

in vec3 Color;
in vec2 TexCoords;

out vec4 color0;

uniform sampler2D u_texture;

void main()
{
	vec4 color = vec4(texture(u_texture, TexCoords));
	if(color.a < 0.5)
	{
		discard;
	}
	color0 = vec4(color.rgb, 1.0);
}