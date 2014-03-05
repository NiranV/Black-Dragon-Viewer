/** 
 * @file blurLightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2007, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

#extension GL_ARB_texture_rectangle : enable

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2DRect depthMap;
uniform sampler2DRect normalMap;
uniform sampler2DRect lightMap;

uniform float dist_factor;
uniform float blur_size;
uniform vec2 delta;
uniform vec3 kern[4];
uniform float kern_scale;

VARYING vec2 vary_fragcoord;

uniform mat4 inv_proj;
uniform vec2 screen_res;

vec4 getPosition(vec2 pos_screen)
{
	float depth = texture2DRect(depthMap, pos_screen.xy).r;
	vec2 sc = pos_screen.xy*2.0;
	sc /= screen_res;
	sc -= vec2(1.0,1.0);
	vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
	vec4 pos = inv_proj * ndc;
	pos /= pos.w;
	pos.w = 1.0;
	return pos;
}


vec3 decode_normal (vec2 enc)
{
    vec2 fenc = enc*4-2;
    float f = dot(fenc,fenc);
    float g = sqrt(1-f/4);
    vec3 n;
    n.xy = fenc*g;
    n.z = 1-f/2;
    return n;
}
vec3 xxsrgb_to_linear(vec3 cl)
{
  // very approx
  return cl * cl;
}

vec3 xxlinear_to_srgb(vec3 cl)
{
  // very approx
  return sqrt(cl);
}

void main() 
{
    vec2 tc = vary_fragcoord.xy;
	vec3 norm = texture2DRect(normalMap, tc).xyz;
	norm = decode_normal(norm.xy); // unpack norm

	vec3 pos = getPosition(tc).xyz;
	vec4 ccol = texture2DRect(lightMap, tc).rgba;
	ccol.gba = xxsrgb_to_linear(ccol.gba);

#define MYKERN 1
#if MYKERN
#define KERNCOUNT 8
	vec3 kern[KERNCOUNT];
	/*kern[0] = vec3(1.000, 1.00, 0.0);
	kern[1] = vec3(0.500, 0.95, 1.0);
	kern[2] = vec3(0.250, 0.85, 2.0);
	kern[3] = vec3(0.125, 0.70, 3.0);*/
	kern[0] = vec3(1.000, 1.00, 0.0);
	kern[1] = vec3(0.333, 1.00, 1.0);
	kern[2] = vec3(0.111, 0.98, 2.0);
	kern[3] = vec3(0.080, 0.95, 3.0);
	kern[4] = vec3(0.060, 0.90, 4.0);
	kern[5] = vec3(0.040, 0.85, 5.0);
	kern[6] = vec3(0.020, 0.77, 6.0);
	kern[7] = vec3(0.001, 0.70, 7.0);
#else
#define KERNCOUNT 4
#endif
	
	//vec2 dlt = kern_scale * delta / (vec2(1.0)+norm.xy*norm.xy);
	//vec2 dlt = kern_scale * delta * (vec2(1.5,1.5)-norm.xy*norm.xy);
	//vec2 dlt = kern_scale * delta * (1.0+abs(norm.z*norm.z));
	//dlt /= max(-pos.z*dist_factor, 1.0);
	//dlt = kern_scale * delta;
	//dlt = 1.0 * delta;

	vec2 dlt = kern_scale * (vec2(1.5,1.5)-norm.xy*norm.xy);
	dlt = delta * ceil(max(dlt.xy, vec2(1.0)));
	//dlt = 1.0 * delta;
	
	vec2 defined_weight = kern[0].xy; // special case the first (centre) sample's weight in the blur; we have to sample it anyway so we get it for 'free'
	vec4 col = defined_weight.xyyy * ccol;

	// relax tolerance according to distance to avoid speckling artifacts, as angles and distances are a lot more abrupt within a small screen area at larger distances
	float pointplanedist_tolerance_pow2 = pos.z * pos.z;
	pointplanedist_tolerance_pow2 =
	  pointplanedist_tolerance_pow2// * pointplanedist_tolerance_pow2 * 0.00001
	  * 0.0001;
	//pointplanedist_tolerance_pow2 = 99999.0;
	//float pointplanedist_tolerance_pow2 = (-pos.z * 0.001);
	//pointplanedist_tolerance_pow2 -= 30.5* ((length(fwidth(norm))));
	//pointplanedist_tolerance_pow2 -= (0.020* ((length(fwidth(norm)))));

	// tighten tolerance in ambiently dark areas; keeps creases crisp, disguises one artifact in particular...
	//float lum = dot(ccol.gba, vec3(0.3, 0.59, 0.11));
	//float oml = 1.0 - lum;
	//pointplanedist_tolerance_pow2 *= (oml*oml*oml*oml*oml*oml*oml);
	
	// perturb sampling origin slightly in screen-space to hide edge-ghosting artifacts where smoothing radius is quite large
	//vec2 tc_v = fract(0.5 * tc.xy); // we now have floor(mod(tc,2.0))*0.5
	//float tc_mod = 2.0 * abs(tc_v.x - tc_v.y); // diff of x,y makes checkerboard
	//tc += ( (tc_mod - 0.5) * kern[1].z * dlt * 0.5 ); // messes with purity
	//tc += ( (tc_mod - 0.5) * kern[1].z * dlt * 0.66667 ); // a, ab, b, bc, c // but messes w/pur.
	// alternate direction according to grid
	//dlt.xy = mix(dlt.xy, vec2(dlt.y, -dlt.x), tc_mod); //artifacts strong

	for (int i = KERNCOUNT-1; i > 0; i--)
	{
	  vec2 samptc = (tc + kern[i].z * dlt);
		vec3 samppos = getPosition(samptc).xyz; 
		vec2 w = kern[i].xy;

		float d = dot(norm.xyz, samppos.xyz-pos.xyz);// dist from plane
		
		if (d*d <= pointplanedist_tolerance_pow2)
		{
		  vec4 scol = texture2DRect(lightMap, samptc);
		  scol.gba = xxsrgb_to_linear(scol.gba);
		  col += scol*w.xyyy;
		  defined_weight += w.xy;
		}
	}

	for (int i = 1; i < KERNCOUNT; i++)
	{
	  vec2 samptc = (tc - kern[i].z * dlt);
		vec3 samppos = getPosition(samptc).xyz; 
		vec2 w = kern[i].xy;

		float d = dot(norm.xyz, samppos.xyz-pos.xyz);// dist from plane
		
		if (d*d <= pointplanedist_tolerance_pow2)
		{
		  vec4 scol = texture2DRect(lightMap, samptc);
		  scol.gba = xxsrgb_to_linear(scol.gba);
		  col += scol*w.xyyy;
		  defined_weight += w.xy;
		}
	}

	col /= defined_weight.xyyy;

	col.gba = xxlinear_to_srgb(col.gba);
	frag_color = col.xyzw;
}
