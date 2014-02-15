/***********************************
 * exoGammaCorrectF.glsl
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

uniform vec3 exo_offset;
uniform vec3 exo_exposure;
uniform vec3 exo_gamma;

void main ()
{
	vec4 diff = texture2DRect(exo_screen, vary_fragcoord.xy);
	diff.rgb = max(vec3(0), diff.rgb + exo_offset);
	diff.rgb *= pow(vec3(2.0), exo_exposure);
	diff.rgb = pow(diff.rgb, exo_gamma);
	frag_color = diff;
}