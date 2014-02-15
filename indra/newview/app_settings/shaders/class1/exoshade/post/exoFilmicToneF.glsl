/***********************************
 * exolineartoneF.glsl
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
uniform float exo_exposure;
VARYING vec2 vary_fragcoord;

void main ()
{
	vec4 diff = texture2DRect(exo_screen, vary_fragcoord.xy);
	diff.rgb *= exo_exposure;
   	vec3 x = max(vec3(0), diff.rgb-0.004);
   	diff.rgb = (x*(6.2*x+.5))/(x*(6.2*x+1.7)+0.06);
	frag_color = diff;
}