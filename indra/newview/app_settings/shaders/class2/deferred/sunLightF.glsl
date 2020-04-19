/** 
 * @file sunLightF.glsl
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

/*[EXTRA_CODE_HERE]*/

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

//class 2, shadows, no SSAO

// Inputs
VARYING vec2 vary_fragcoord;

uniform vec3 sun_dir;
<<<<<<< HEAD

uniform vec4 shadow_res;
uniform vec4 shadow_bias;
uniform float shadow_offset;

uniform float spot_shadow_bias;
uniform float spot_shadow_offset;

vec2 encode_normal(vec3 n)
{
	float f = sqrt(8 * n.z + 8);
	return n.xy / f + 0.5;
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

float pcfShadow(sampler2DShadow shadowMap, vec4 stc, float scl, vec2 pos_screen, float shad_res, float bias)
{
	float recip_shadow_res = 1.0 / shad_res;
	stc.xyz /= stc.w;
	stc.z += shadow_bias;
	
	stc.x = floor(stc.x*shad_res + fract(pos_screen.y*0.5)) * recip_shadow_res;
	float cs = shadow2D(shadowMap, stc.xyz).x;
	
	float shadow = cs;
	
	shadow += shadow2D(shadowMap, stc.xyz+vec3(0.60*recip_shadow_res, 0.55*recip_shadow_res, 0.0)).x;
	shadow += shadow2D(shadowMap, stc.xyz+vec3(0.72*recip_shadow_res, -0.65*recip_shadow_res, 0.0)).x;
	shadow += shadow2D(shadowMap, stc.xyz+vec3(-0.60*recip_shadow_res, 0.55*recip_shadow_res, 0.0)).x;
	shadow += shadow2D(shadowMap, stc.xyz+vec3(-0.72*recip_shadow_res, -0.65*recip_shadow_res, 0.0)).x;
	         
    return shadow*0.2;
}

float pcfSpotShadow(sampler2DShadow shadowMap, vec4 stc, float scl, vec2 pos_screen, float shad_res)
{
	stc.xyz /= stc.w;
	stc.z += spot_shadow_bias*scl;
	stc.x = floor(shad_res * stc.x + fract(pos_screen.y*0.0)) / shad_res; // snap
		
	float cs = shadow2D(shadowMap, stc.xyz).x;
	float shadow = cs;
	
	shadow += shadow2D(shadowMap, stc.xyz+vec3(0.45/shad_res, 0.45/shad_res, 0.0)).x;
	shadow += shadow2D(shadowMap, stc.xyz+vec3(0.65/shad_res, -0.65/shad_res, 0.0)).x;
	shadow += shadow2D(shadowMap, stc.xyz+vec3(-0.45/shad_res, 0.45/shad_res, 0.0)).x;
	shadow += shadow2D(shadowMap, stc.xyz+vec3(-0.65/shad_res, -0.65/shad_res, 0.0)).x;

	return shadow*0.2;
}

void main() 
{
	vec2 pos_screen = vary_fragcoord.xy;
	vec4 pos = getPosition(pos_screen);
	vec3 norm = texture2DRect(normalMap, pos_screen).xyz;
	
	norm = decode_normal(norm.xy); // unpack norm
	
	float shadow = 0.0;
	float dp_directional_light = max(0.0, dot(norm, sun_dir.xyz));
	
	vec3 shadow_pos = pos.xyz;
	vec3 offset = sun_dir.xyz * (1.0-dp_directional_light);
	
	vec4 spos = vec4(shadow_pos+offset*shadow_offset, 1.0);
	
	if (spos.z > -shadow_clip.w)
	{	
		if (dp_directional_light == 0.0)
		{
			// if we know this point is facing away from the sun then we know it's in shadow without having to do a squirrelly shadow-map lookup
			shadow = 0.0;
		}
		else
		{
			vec4 lpos;
			
			vec4 near_split = shadow_clip*-0.75;
			vec4 far_split = shadow_clip*-1.25;
			vec4 transition_domain = near_split-far_split;
			float weight = 0.0;

			if (spos.z < near_split.z)
			{
				lpos = shadow_matrix[3]*spos;
				
				float w = 1.0;
				w -= max(spos.z-far_split.z, 0.0)/transition_domain.z;
				shadow += pcfShadow(shadowMap3, lpos, 0.25, pos_screen, shadow_res.w, shadow_bias.w)*w;
				weight += w;
				shadow += max((pos.z+shadow_clip.z)/(shadow_clip.z-shadow_clip.w)*2.0-1.0, 0.0);
			}

			if (spos.z < near_split.y && spos.z > far_split.z)
			{
				lpos = shadow_matrix[2]*spos;
				
				float w = 1.0;
				w -= max(spos.z-far_split.y, 0.0)/transition_domain.y;
				w -= max(near_split.z-spos.z, 0.0)/transition_domain.z;
				shadow += pcfShadow(shadowMap2, lpos, 0.5, pos_screen, shadow_res.z, shadow_bias.z)*w;
				weight += w;
			}

			if (spos.z < near_split.x && spos.z > far_split.y)
			{
				lpos = shadow_matrix[1]*spos;

				float w = 1.0;
				w -= max(spos.z-far_split.x, 0.0)/transition_domain.x;
				w -= max(near_split.y-spos.z, 0.0)/transition_domain.y;
				shadow += pcfShadow(shadowMap1, lpos, 0.75, pos_screen, shadow_res.y, shadow_bias.y)*w;
				weight += w;
			}

			if (spos.z > far_split.x)
			{
				lpos = shadow_matrix[0]*spos;
				
				float w = 1.0;
				w -= max(near_split.x-spos.z, 0.0)/transition_domain.x;
				
				shadow += pcfShadow(shadowMap0, lpos, 1.0, pos_screen, shadow_res.x, shadow_bias.x)*w;
				weight += w;
			}
		

			shadow /= weight;

			// take the most-shadowed value out of these two:
			//  * the blurred sun shadow in the light (shadow) map
			//  * an unblurred dot product between the sun and this norm
			// the goal is to err on the side of most-shadow to fill-in shadow holes and reduce artifacting
			shadow = min(shadow, dp_directional_light);
			
			//lpos.xy /= lpos.w*32.0;
			//if (fract(lpos.x) < 0.1 || fract(lpos.y) < 0.1)
			//{
			//	shadow = 0.0;
			//}
			
		}
	}
	else
	{
		// more distant than the shadow map covers
		shadow = 1.0;
	}
	
	frag_color[0] = shadow;
	frag_color[1] = 1.0;
	
	spos = vec4(shadow_pos+norm*spot_shadow_offset, 1.0);
	
	//spotlight shadow 1
	vec4 lpos = shadow_matrix[4]*spos;
	frag_color[2] = pcfSpotShadow(shadowMap4, lpos, 0.8, pos_screen, proj_shadow_res.x); 
	
	//spotlight shadow 2
	lpos = shadow_matrix[5]*spos;
	frag_color[3] = pcfSpotShadow(shadowMap5, lpos, 0.8, pos_screen, proj_shadow_res.y); 

	//frag_color.rgb = pos.xyz;
	//frag_color.b = shadow;
=======
uniform float shadow_bias;

vec3 getNorm(vec2 pos_screen);
vec4 getPosition(vec2 pos_screen);

float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen);
float sampleSpotShadow(vec3 pos, vec3 norm, int index, vec2 pos_screen);

void main() 
{
    vec2 pos_screen = vary_fragcoord.xy;
    vec4 pos        = getPosition(pos_screen);
    vec3 norm       = getNorm(pos_screen);

    frag_color.r = sampleDirectionalShadow(pos.xyz, norm, pos_screen);
    frag_color.g = 1.0f;
    frag_color.b = sampleSpotShadow(pos.xyz, norm, 0, pos_screen); 
    frag_color.a = sampleSpotShadow(pos.xyz, norm, 1, pos_screen);
>>>>>>> 693791f4ffdf5471b16459ba295a50615bbc7762
}
