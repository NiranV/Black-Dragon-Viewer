/***********************************
 * exoGammaConvertF.glsl
 * Provides linear tone mapping functionality.
 * Copyright Jonathan Goodman, 2012
 ***********************************/
#extension GL_ARB_texture_rectangle : enable

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2DRect exo_screen;
VARYING vec2 vary_fragcoord;

void main ()
{
	vec4 diff = texture2DRect(exo_screen, vary_fragcoord.xy);
	diff.rgb = pow(diff.rgb, vec3(2.2));
	frag_color = diff;
}