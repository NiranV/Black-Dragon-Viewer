/***********************************
 * exolineartoneF.glsl
 * Provides linear tone mapping functionality.
 * Copyright Jonathan Goodman, 2012
 ***********************************/
#extension GL_ARB_texture_rectangle : enable

/*[EXTRA_CODE_HERE]*/

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2DRect exo_screen;
uniform float exo_exposure;
VARYING vec2 vary_fragcoord;

vec3 srgb_to_linear(vec3 cs)
{
	vec3 low_range = cs / vec3(12.92);
	vec3 high_range = pow((cs+vec3(0.055))/vec3(1.055), vec3(2.4));
	bvec3 lte = lessThanEqual(cs,vec3(0.04045));

#ifdef OLD_SELECT
	vec3 result;
	result.r = lte.r ? low_range.r : high_range.r;
	result.g = lte.g ? low_range.g : high_range.g;
	result.b = lte.b ? low_range.b : high_range.b;
    return result;
#else
	return mix(high_range, low_range, lte);
#endif

}

void main ()
{
	vec4 diff = texture2DRect(exo_screen, vary_fragcoord.xy);
	diff.rgb = srgb_to_linear(diff.rgb);
	diff.rgb *= exo_exposure;
   	vec3 x = max(vec3(0), diff.rgb-0.004);
   	diff.rgb = (x*(6.2*x+.5))/(x*(6.2*x+1.7)+0.06);
	frag_color = diff;
}