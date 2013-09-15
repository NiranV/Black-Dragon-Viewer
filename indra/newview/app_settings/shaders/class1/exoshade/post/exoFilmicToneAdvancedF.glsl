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
uniform vec3 exo_advToneUA;
uniform vec3 exo_advToneUB;
uniform vec3 exo_advToneUC;

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
	diff.rgb *= exo_exposure;
	
   	vec3 curr = Uncharted2Tonemap(ExposureBias* diff.rgb);
	vec3 whiteScale = vec3(1.0)/Uncharted2Tonemap(vec3(W));
	vec3 color = curr*whiteScale;
	frag_color = vec4(color, diff.a);
}