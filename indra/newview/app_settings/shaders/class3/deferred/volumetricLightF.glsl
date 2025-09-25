/** 
 * @file volumetricLightF.glsl
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
#extension GL_ARB_shader_texture_lod : enable

/*[EXTRA_CODE_HERE]*/

out vec4 frag_color;

uniform sampler2D diffuseRect;
uniform sampler2D depthMap;

uniform vec2 screen_res;

in vec2 vary_fragcoord;

uniform int godray_res;
uniform float godray_multiplier;
uniform float falloff_multiplier;
uniform vec3 sun_dir;

uniform vec3 blue_density;
uniform float haze_density;
uniform vec3 sunlight_color;

uniform float seconds60;

float rand(vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float nonpcfShadowAtPos(vec4 pos_world, vec2 pos_screen);

vec4 getPosition(vec2 pos_screen);

void main() 
{
    vec2 tc = vary_fragcoord.xy;
    vec4 pos = getPosition(tc);
    vec4 diff = texture(diffuseRect, tc);
    float depth = texture(depthMap, tc).r;
    depth *= pow(depth, 100.0);
    
    vec3 haze_weight = vec3(1,1,1);
    vec3 temp1 = blue_density + vec3(haze_density);
    haze_weight = vec3(haze_density) / temp1;
    
    // craptacular rays
    float roffset = rand(tc);
    vec3 farpos = pos.xyz;
    farpos *= min(-pos.z, 512.0) / -pos.z;

    float shadamount = 0.0;
    float shaftify = 0.0;
    float last_shadsample = 0.0;

    for (int i=godray_res-1; i>0; --i)
    {
      vec4 spos = vec4(mix(vec3(0,0,0), farpos, (i-roffset)/(godray_res)), 1.0);
      float this_shadsample = 0.275 * nonpcfShadowAtPos(spos, tc);
      float this_shaftify = 0.15 * (abs(this_shadsample + last_shadsample));
      last_shadsample = this_shadsample;
      this_shadsample *= i;
      this_shaftify /= i;
      shadamount += this_shadsample;
      shaftify += this_shaftify;
    }
    
    shadamount /= godray_res;
    shaftify /= godray_res;
    shadamount *= clamp(depth, 0.0 , 0.5);
    
    float fade = max(falloff_multiplier / depth, 1.0);
    shaftify = (shaftify / fade) * godray_multiplier;
#if GODRAYS_FADE
    fade = 0.0;
    if(sun_dir.z < 0.0)
    {
        fade = clamp(1 - dot(sun_dir.xy * 1.2, sun_dir.xy * 1.8), 0, 1);
    }
    shaftify *= fade;
#endif
    diff.rgb += ((shaftify * haze_weight.x) * shadamount) * sunlight_color;

    frag_color = diff;
}