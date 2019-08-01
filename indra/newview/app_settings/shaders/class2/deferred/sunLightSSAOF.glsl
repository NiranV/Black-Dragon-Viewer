/** 
 * @file sunLightSSAOF.glsl
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

//class 2 -- shadows and SSAO

uniform sampler2DRect depthMap;
uniform sampler2DRect normalMap;
uniform sampler2DShadow shadowMap0;
uniform sampler2DShadow shadowMap1;
uniform sampler2DShadow shadowMap2;
uniform sampler2DShadow shadowMap3;
uniform sampler2DShadow shadowMap4;
uniform sampler2DShadow shadowMap5;
uniform sampler2D noiseMap;


// Inputs
uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;
uniform float ssao_radius;
uniform float ssao_max_radius;
uniform float ssao_factor;
uniform float ssao_factor_inv;

VARYING vec2 vary_fragcoord;

uniform mat4 inv_proj;
uniform vec2 screen_res;
uniform vec2 proj_shadow_res;
uniform vec3 sun_dir;

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
    float g = sqrt(1-f*0.25);
    vec3 n;
    n.xy = fenc*g;
    n.z = 1-f*0.5;
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

vec2 getKern(int i)
{
	vec2 kern[8];
	// exponentially (^2) distant occlusion samples spread around origin
	kern[0] = vec2(-0.015625, 0.0);
	kern[1] = vec2(0.0625, 0.0);
	kern[2] = vec2(0.0, 0.140625);
	kern[3] = vec2(0.0, -0.25);
	kern[4] = vec2(0.2762, 0.2762);
	kern[5] = vec2(-0.3977, -0.3977);
	kern[6] = vec2(-0.5414, 0.5414);
	kern[7] = vec2(0.7071, -0.7071);
       
	return kern[i];
}

//calculate decreases in ambient lighting when crowded out (SSAO)
float calcAmbientOcclusion(vec4 pos, vec3 norm)
{
	float ret = 1.0;

	vec2 pos_screen = vary_fragcoord.xy;
	vec3 pos_world = pos.xyz;
	vec2 noise_reflect = texture2D(noiseMap, vary_fragcoord.xy*0.015625).xy;
		
	float angle_hidden = 0.0;
	float points = 0;
		
	float scale = min(ssao_radius / -pos_world.z, ssao_max_radius);
	
	// it was found that keeping # of samples a constant was the fastest, probably due to compiler optimizations (unrolling?)
	for (int i = 0; i < 8; i++)
	{
		vec2 samppos_screen = pos_screen + scale * reflect(getKern(i), noise_reflect);
		vec3 samppos_world = getPosition(samppos_screen).xyz; 
		
		vec3 diff = pos_world - samppos_world;
		float dist2 = dot(diff, diff);
			
		// assume each sample corresponds to an occluding sphere with constant radius, constant x-sectional area
		// --> solid angle shrinking by the square of distance
		//radius is somewhat arbitrary, can approx with just some constant k * 1 / dist^2
		//(k should vary inversely with # of samples, but this is taken care of later)
		
		float funky_val = (dot((samppos_world - 0.05*norm - pos_world), norm) > 0.0) ? 1.0 : 0.0;
		angle_hidden = angle_hidden + funky_val * min(1.0/dist2, ssao_factor_inv);
			
		// 'blocked' samples (significantly closer to camera relative to pos_world) are "no data", not "no occlusion" 
		float diffz_val = (diff.z > -1.0) ? 1.0 : 0.0;
		points = points + diffz_val;
	}
		
	angle_hidden = min(ssao_factor*angle_hidden/points, 1.0);
	
	float points_val = (points > 0.0) ? 1.0 : 0.0;
	ret = (1.0 - (points_val * angle_hidden));

	ret = max(ret, 0.0);
	return min(ret, 1.0);
}

float pcfShadow(sampler2DShadow shadowMap, vec4 stc, float scl, vec2 pos_screen, float shad_res, float bias)
{
	float recip_shadow_res = 1.0 / shad_res;
	stc.xyz /= stc.w;
	stc.z += bias;
	
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
	
	//shadow += shadow2D(shadowMap, stc.xyz+vec3(0.45/shad_res, 0.45/shad_res, 0.0)).x;
	//shadow += shadow2D(shadowMap, stc.xyz+vec3(0.65/shad_res, -0.65/shad_res, 0.0)).x;
	//shadow += shadow2D(shadowMap, stc.xyz+vec3(-0.45/shad_res, 0.45/shad_res, 0.0)).x;
	//shadow += shadow2D(shadowMap, stc.xyz+vec3(-0.65/shad_res, -0.65/shad_res, 0.0)).x;

	return shadow;
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
#if USE_SSAO
	frag_color[1] = calcAmbientOcclusion(pos, norm);
#else
	frag_color[1] = 1.0;
#endif
	
	spos = vec4(shadow_pos+norm*spot_shadow_offset, 1.0);
	
	//spotlight shadow 1
	vec4 lpos = shadow_matrix[4]*spos;
	frag_color[2] = pcfSpotShadow(shadowMap4, lpos, 0.8, pos_screen, proj_shadow_res.x);

	//spotlight shadow 2
	lpos = shadow_matrix[5]*spos;
	frag_color[3] = pcfSpotShadow(shadowMap5, lpos, 0.8, pos_screen, proj_shadow_res.y);
 
	//frag_color.rgb = pos.xyz;
	//frag_color.b = shadow;
}
