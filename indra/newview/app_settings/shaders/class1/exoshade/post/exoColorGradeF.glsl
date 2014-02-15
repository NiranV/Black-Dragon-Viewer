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
uniform vec2 screen_res;
uniform sampler2D textureLUT;

VARYING vec2 vary_fragcoord;

void main ()
{
	vec4 diff = texture2DRect(exo_screen, vary_fragcoord.xy);
	
	diff.r = texture2D(textureLUT, diff.xx).r;
	diff.g = texture2D(textureLUT, diff.yy).g;
	diff.b = texture2D(textureLUT, diff.zz).b;
	float lum = dot(diff.rgb, vec3(0.299, 0.587, 0.144));
	diff.rgb = mix(vec3(lum), diff.rgb, texture2D(textureLUT, vec2(lum)).a);
	frag_color = diff;
}