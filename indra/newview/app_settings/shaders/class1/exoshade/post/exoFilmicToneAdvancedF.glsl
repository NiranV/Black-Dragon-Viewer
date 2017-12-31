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
uniform vec3 exo_advToneUA;
uniform vec3 exo_advToneUB;
uniform vec3 exo_advToneUC;

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

vec3 linear_to_srgb(vec3 cl)
{
	cl = clamp(cl, vec3(0), vec3(1));
	vec3 low_range  = cl * 12.92;
	vec3 high_range = 1.055 * pow(cl, vec3(0.41666)) - 0.055;
	bvec3 lt = lessThan(cl,vec3(0.0031308));

#ifdef OLD_SELECT
	vec3 result;
	result.r = lt.r ? low_range.r : high_range.r;
	result.g = lt.g ? low_range.g : high_range.g;
	result.b = lt.b ? low_range.b : high_range.b;
    return result;
#else
	return mix(high_range, low_range, lt);
#endif

}

/**
 * We setup the tone mapping settings as such:
 * A = exo_advToneUA.x
 * B = exo_advToneUA.y
 * C = exo_advToneUA.z
 * D = exo_advToneUA.w
 * E = exo_advToneUB.x
 * F = exo_advToneUB.y
 * W = exo_advToneUB.z
 * exposureBias = exo_advToneUB.w
 **/

float A;
float B;
float C;
float D;
float E;
float F;
float W;

VARYING vec2 vary_fragcoord;

vec3 Uncharted2Tonemap(vec3 x)
{
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

void main ()
{
	A = exo_advToneUA.x;
	B = exo_advToneUA.y;
	C = exo_advToneUA.z;
	D = exo_advToneUB.x;
	E = exo_advToneUB.y;
	F = exo_advToneUB.z;
	W = exo_advToneUC.x;
	float ExposureBias = exo_advToneUC.y;

	vec4 diff = texture2DRect(exo_screen, vary_fragcoord.xy);
	diff.rgb = srgb_to_linear(diff.rgb);
	diff.rgb *= exo_exposure;
	
   	vec3 curr = Uncharted2Tonemap(ExposureBias* diff.rgb);
	vec3 whiteScale = vec3(1.0)/Uncharted2Tonemap(vec3(W));
	vec3 color = curr*whiteScale;
	frag_color = vec4(linear_to_srgb(color), diff.a);
}