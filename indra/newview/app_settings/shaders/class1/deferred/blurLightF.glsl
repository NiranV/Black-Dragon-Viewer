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

#define DELUXE_SHADOW_SMOOTH
#define LINEAR

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
#ifdef LINEAR
  // very approx
  return cl * cl;
#else
  return cl;
#endif
}

vec3 xxlinear_to_srgb(vec3 cl)
{
#ifdef LINEAR
  // very approx
  return sqrt(cl);
#else
  return cl;
#endif
}

void main() 
{
    vec2 tc = vary_fragcoord.xy;
	vec3 norm = texture2DRect(normalMap, tc).xyz;
	norm = decode_normal(norm.xy); // unpack norm

	vec3 pos = getPosition(tc).xyz;
	vec4 ccol = texture2DRect(lightMap, tc).rgba;
	ccol.gba = xxsrgb_to_linear(ccol.gba);

#if MYKERN
#define KERNCOUNT 8
	vec3 kern[KERNCOUNT];

	kern[0] = vec3(1.000*0.50, 1.00*0.50, 0.000 * blur_size);
	kern[1] = vec3(0.333*0.50, 1.00*0.50, 0.500 * blur_size);
	kern[2] = vec3(0.111*0.75, 0.98*0.75, 1.000 * blur_size);
	kern[3] = vec3(0.080*1.00, 0.95*1.00, 2.000 * blur_size);
	kern[4] = vec3(0.060*1.00, 0.90*1.00, 3.000 * blur_size);
	kern[5] = vec3(0.040*1.00, 0.85*1.00, 4.000 * blur_size);
	kern[6] = vec3(0.020*1.00, 0.77*1.00, 5.000 * blur_size);
	kern[7] = vec3(0.001*1.00, 0.70*1.00, 6.000 * blur_size);

#else
#define KERNCOUNT 4
#endif
	
	vec2 dlt = kern_scale * (vec2(1.5,1.5)-norm.xy*norm.xy);
	dlt = delta * ceil(max(dlt.xy, vec2(1.0)));
	
	vec2 defined_weight = kern[0].xy; // special case the first (centre) sample's weight in the blur; we have to sample it anyway so we get it for 'free'
	vec4 col = defined_weight.xyyy * ccol;

	// relax tolerance according to distance to avoid speckling artifacts, as angles and distances are a lot more abrupt within a small screen area at larger distances
	float pointplanedist_tolerance_pow2 = pos.z * pos.z;
	pointplanedist_tolerance_pow2 =
	  pointplanedist_tolerance_pow2// * pointplanedist_tolerance_pow2 * 0.00001
	  * 0.0001;
	
	// perturb sampling origin slightly in screen-space to hide edge-ghosting artifacts where smoothing radius is quite large
	//vec2 tc_v = fract(0.5 * tc.xy); // we now have floor(mod(tc,2.0))*0.5
	//float tc_mod = 2.0 * abs(tc_v.x - tc_v.y); // diff of x,y makes checkerboard
	//tc += ( (tc_mod - 0.5) * kern[1].z * dlt * 0.5 ); // messes with purity
	//tc += ( (tc_mod - 0.5) * kern[1].z * dlt * 0.66667 ); // a, ab, b, bc, c // but messes w/pur.
	// alternate direction according to grid
	//dlt.xy = mix(dlt.xy, vec2(dlt.y, -dlt.x), tc_mod); //artifacts strong

	const float mindp = 0.70;

	for (int i = KERNCOUNT-1; i > 0; i--)
	  {
	    vec2 w = kern[i].xy;
	    vec2 samptc = (tc + kern[i].z * dlt);
	    vec3 samppos = getPosition(samptc).xyz; 
#ifdef DELUXE_SHADOW_SMOOTH
	    vec3 sampnorm = texture2DRect(normalMap, samptc).xyz;
	    sampnorm = decode_normal(sampnorm.xy); // unpack norm
#endif // DELUXE_SHADOW_SMOOTH
	  
	    float d = dot(norm.xyz, samppos.xyz-pos.xyz);// dist from plane
	  
	    if (d*d <= pointplanedist_tolerance_pow2
#ifdef DELUXE_SHADOW_SMOOTH
		&& dot(sampnorm.xyz, norm.xyz) >= mindp
#endif // DELUXE_SHADOW_SMOOTH
		)
	      {
		vec4 scol = texture2DRect(lightMap, samptc);
		scol.gba = xxsrgb_to_linear(scol.gba);
		col += scol*w.xyyy;
		defined_weight += w.xy;
	      }
	  }

	for (int i = 1; i < KERNCOUNT; i++)
	  {
	    vec2 w = kern[i].xy;
	    vec2 samptc = (tc - kern[i].z * dlt);
	    vec3 samppos = getPosition(samptc).xyz; 
#ifdef DELUXE_SHADOW_SMOOTH
	    vec3 sampnorm = texture2DRect(normalMap, samptc).xyz;
	    sampnorm = decode_normal(sampnorm.xy); // unpack norm
#endif // DELUXE_SHADOW_SMOOTH
	  
	    float d = dot(norm.xyz, samppos.xyz-pos.xyz);// dist from plane
	  
	    if (d*d <= pointplanedist_tolerance_pow2
#ifdef DELUXE_SHADOW_SMOOTH
		&& dot(sampnorm.xyz, norm.xyz) >= mindp
#endif // DELUXE_SHADOW_SMOOTH
		)
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