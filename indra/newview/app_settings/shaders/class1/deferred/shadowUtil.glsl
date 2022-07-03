/** 
 * @file class1/deferred/shadowUtil.glsl
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

uniform sampler2DRect   normalMap;
uniform sampler2DRect   depthMap;
uniform sampler2DShadow shadowMap0;
uniform sampler2DShadow shadowMap1;
uniform sampler2DShadow shadowMap2;
uniform sampler2DShadow shadowMap3;
uniform sampler2DShadow shadowMap4;
uniform sampler2DShadow shadowMap5;

uniform vec3 sun_dir;
uniform vec3 moon_dir;
uniform vec4 shadow_res;
uniform vec2 proj_shadow_res;
uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;
uniform float shadow_bias;
uniform float shadow_offset;
uniform float spot_shadow_bias;
uniform float spot_shadow_offset;
uniform mat4 inv_proj;
uniform vec2 screen_res;
uniform int sun_up_factor;


float pcfSpotShadow(sampler2DShadow shadowMap, vec4 stc, float bias_scale, vec2 pos_screen, float shad_res)
{
    stc.xyz /= stc.w;
    stc.z += spot_shadow_bias * bias_scale;
    stc.x = floor(shad_res * stc.x + fract(pos_screen.y*0.666666666)) / shad_res; // snap

    float cs = shadow2D(shadowMap, stc.xyz).x;
    float shadow = cs;

    float off = 1.0/shad_res;
    
    shadow += shadow2D(shadowMap, stc.xyz+vec3(off*2.0, off, 0.0)).x;
    shadow += shadow2D(shadowMap, stc.xyz+vec3(off, -off, 0.0)).x;
    shadow += shadow2D(shadowMap, stc.xyz+vec3(-off, off, 0.0)).x;
    shadow += shadow2D(shadowMap, stc.xyz+vec3(-off*2.0, -off, 0.0)).x;
    return shadow*0.2;
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

float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen)
{
    float shadow = 0.0f;
    vec3 light_dir = normalize((sun_up_factor == 1) ? sun_dir : moon_dir);

    float dp_directional_light = max(0.0, dot(norm.xyz, light_dir));
          dp_directional_light = clamp(dp_directional_light, 0.0, 1.0);

    vec3 shadow_pos = pos.xyz;

    vec3 offset = light_dir.xyz * (1.0 - dp_directional_light);

    shadow_pos += offset * shadow_offset * 2.0;

    vec4 spos = vec4(shadow_pos.xyz, 1.0);

    if (spos.z > -shadow_clip.w)
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
         float contrib = pcfShadow(shadowMap3, lpos, 0.25, pos_screen, shadow_res.w, shadow_bias)*w;
         {
          shadow += contrib;
          weight += w;
         }
         shadow += max((pos.z+shadow_clip.z)/(shadow_clip.z-shadow_clip.w)*2.0-1.0, 0.0);
        }
     
        if (spos.z < near_split.y && spos.z > far_split.z)
        {
         lpos = shadow_matrix[2]*spos;
         
         float w = 1.0;
         w -= max(spos.z-far_split.y, 0.0)/transition_domain.y;
         w -= max(near_split.z-spos.z, 0.0)/transition_domain.z;
         shadow += pcfShadow(shadowMap2, lpos, 0.5, pos_screen, shadow_res.z, shadow_bias)*w;
         weight += w;
        }
     
        if (spos.z < near_split.x && spos.z > far_split.y)
        {
         lpos = shadow_matrix[1]*spos;
     
         float w = 1.0;
         w -= max(spos.z-far_split.x, 0.0)/transition_domain.x;
         w -= max(near_split.y-spos.z, 0.0)/transition_domain.y;
         shadow += pcfShadow(shadowMap1, lpos, 0.75, pos_screen, shadow_res.y, shadow_bias)*w;
         weight += w;
        }
     
        if (spos.z > far_split.x)
        {
         lpos = shadow_matrix[0]*spos;
         
         float w = 1.0;
         w -= max(near_split.x-spos.z, 0.0)/transition_domain.x;
         
         shadow += pcfShadow(shadowMap0, lpos, 1.0, pos_screen, shadow_res.x, shadow_bias)*w;
         weight += w;
        }
       
     
        shadow /= weight;
    }
    else
    {
        return 1.0f; // lit beyond the far split...
    }
    //shadow = min(dp_directional_light,shadow);
    return shadow;
}

float sampleSpotShadow(vec3 pos, vec3 norm, int index, vec2 pos_screen)
{
    float shadow = 0.0f;
    pos += norm * spot_shadow_offset;

    vec4 spos = vec4(pos,1.0);
    if (spos.z > -shadow_clip.w)
    {   
        vec4 lpos;
        
        vec4 near_split = shadow_clip*-0.75;
        vec4 far_split = shadow_clip*-1.25;
        vec4 transition_domain = near_split-far_split;
        float weight = 0.0;

        {
            float w = 1.0;
            w -= max(spos.z-far_split.z, 0.0)/transition_domain.z;

            if (index == 0)
            {        
                lpos = shadow_matrix[4]*spos;
                shadow += pcfSpotShadow(shadowMap4, lpos, 0.8, spos.xy, proj_shadow_res.x)*w;
            }
            else
            {
                lpos = shadow_matrix[5]*spos;
                shadow += pcfSpotShadow(shadowMap5, lpos, 0.8, spos.xy, proj_shadow_res.y)*w;
            }
            weight += w;
            shadow += max((pos.z+shadow_clip.z)/(shadow_clip.z-shadow_clip.w)*2.0-1.0, 0.0);
        }

        shadow /= weight;
    }
    else
    {
        shadow = 1.0f;
    }
    return shadow;
}

