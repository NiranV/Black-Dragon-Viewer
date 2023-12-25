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

/*[EXTRA_CODE_HERE]*/

out vec4 frag_color;

uniform sampler2D diffuseRect;

uniform vec2 screen_res;
uniform float res_scale;

in vec2 vary_fragcoord;

uniform int godray_res;
uniform float godray_multiplier;
uniform float falloff_multiplier;
uniform vec3 sun_dir;

uniform vec4 shadow_clip;
uniform vec4 blue_density;
uniform float haze_density;
uniform vec4 sunlight_color;

float fade = 1.0;
float shadamount = 0.0;
float shaftify = 0.0;
float last_shadsample = 0.0;

float getDepth(vec2 pos_screen);
vec4 getPositionWithDepth(vec2 pos_screen, float depth);

float nonpcfShadowAtPos(vec4 pos_world)

float random (vec2 uv);

void main() 
{
    vec2 tc = vary_fragcoord.xy;
    vec4 diff = texture2D(diffuseRect, tc*res_scale);
    float depth = getDepth(tc);
    vec3 pos = getPositionWithDepth(tc, depth).xyz;
    
    vec4 haze_weight;
    vec4 temp1 = blue_density + vec4(haze_density);
    haze_weight = vec4(haze_density) / temp1;
    
    // craptacular rays
    float roffset = random(tc);
    vec3 farpos = pos;
    farpos *= min(-pos.z, 512.0) / -pos.z;
    
    for (int i=godray_res-1; i>0; --i)
    {
      vec4 spos = vec4(mix(vec3(0,0,0), farpos, (i-roffset)/(godray_res)), 1.0);
      float this_shadsample = 0.275 * nonpcfShadowAtPos(spos);
      float this_shaftify = 0.15 * (abs(this_shadsample + last_shadsample));
      last_shadsample = this_shadsample;
      this_shadsample *= i;
      this_shaftify /= i;
      shadamount += this_shadsample;
      shaftify += this_shaftify;
    }
    
    shadamount /= godray_res;
    shaftify /= godray_res;
    
    farpos = pos * (min(-pos.z, godray_res) / -pos.z);
    fade *= max(abs(falloff_multiplier / (farpos.z * farpos.z)), 1.0);
    shaftify = (shaftify / fade) * godray_multiplier;
#if GODRAYS_FADE
    fade = 0.0;
    if(sun_dir.z < 0.0)
    {
        fade = clamp(1 - dot(sun_dir.xy * 1.2, sun_dir.xy * 1.8), 0, 1);
    }
    shaftify *= fade;
#endif
    diff += ((shaftify * haze_weight.a) * shadamount) * sunlight_color;

    frag_color = diff;
}
