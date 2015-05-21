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

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2DRect diffuseRect;
uniform sampler2DRect normalMap;
uniform sampler2DRect depthMap;
uniform sampler2D bloomMap;

uniform mat4 inv_proj;
uniform vec2 screen_res;
uniform float res_scale;

VARYING vec2 vary_fragcoord;

uniform int godray_res;
uniform float godray_multiplier;
uniform float falloff_multiplier;
uniform vec3 sun_dir;

uniform vec4 shadow_clip;
uniform vec4 blue_density;
uniform float haze_density;
uniform vec4 sunlight_color;

uniform float seconds60;

vec3 vary_SunlitColor;
vec3 vary_AdditiveColor;

vec3 vary_PositionEye;

uniform mat4 shadow_matrix[6];

uniform sampler2DShadow shadowMap0;
uniform sampler2DShadow shadowMap1;
uniform sampler2DShadow shadowMap2;
uniform sampler2DShadow shadowMap3;
uniform sampler2DShadow shadowMap4;
uniform sampler2DShadow shadowMap5;

uniform vec2 shadow_res;

uniform float shadow_bias;

float rand(vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

float nonpcfShadow(sampler2DShadow shadowMap, vec4 stc, vec2 pos_screen)
{
  vec2 recip_shadow_res = 1.0 / shadow_res.xy;
  stc.xyz /= stc.w;
  stc.z += shadow_bias;
  
  stc.x = floor(stc.x*shadow_res.x + fract(pos_screen.y*0.666666666)) * recip_shadow_res.x;
  return shadow2D(shadowMap, stc.xyz).x;
}

float nonpcfShadowAtPos(vec4 pos_world)
{
  vec2 pos_screen = vary_fragcoord.xy;
  vec4 pos = pos_world;
  if (pos.z > -shadow_clip.w) {	
    vec4 near_split = shadow_clip*-0.75;
    vec4 far_split = shadow_clip*-1.25;
    
    if (pos.z < near_split.z) {
      pos = shadow_matrix[3]*pos;
      return nonpcfShadow(shadowMap3, pos, pos_screen);
    }
    else if (pos.z < near_split.y) {
      pos = shadow_matrix[2]*pos;
      return nonpcfShadow(shadowMap2, pos, pos_screen);
    }
    else if (pos.z < near_split.x) {
      pos = shadow_matrix[1]*pos;
      return nonpcfShadow(shadowMap1, pos, pos_screen);
    }
    else if (pos.z > far_split.x) {
      pos = shadow_matrix[0]*pos;
      return nonpcfShadow(shadowMap0, pos, pos_screen);
    }
  }
  return 1.0;
}

vec4 getPosition_d(vec2 pos_screen, float depth)
{
	vec2 sc = pos_screen.xy*2.0;
	sc /= screen_res;
	sc -= vec2(1.0,1.0);
	vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
	vec4 pos = inv_proj * ndc;
	pos /= pos.w;
	pos.w = 1.0;
	return pos;
}

void main() 
{
    vec2 tc = vary_fragcoord.xy;
    vec4 diff = texture2DRect(diffuseRect, tc*res_scale);
    
    float depth = texture2DRect(depthMap, tc).r;
    vec3 pos = getPosition_d(tc, depth).xyz;
    
    vec4 haze_weight;
    vec4 temp1 = blue_density + vec4(haze_density);
    haze_weight = vec4(haze_density) / temp1;
    
    // craptacular rays
    float fade = 1.0;
    float shadamount = 0.0;
    float shaftify = 0.0;
    float last_shadsample = 0.0;
    float roffset = rand(tc + vec2(seconds60));
    vec3 farpos = pos;
    farpos *= min(-pos.z, 512.0) / -pos.z;
    
    for (int i=godray_res-1; i>0; --i)
    {
      vec4 spos = vec4(mix(vec3(0,0,0), farpos, (i-roffset)/(godray_res)), 1.0);
      float this_shadsample = 0.275 * nonpcfShadowAtPos(spos);
      float this_shaftify = 0.15 * (abs(this_shadsample + last_shadsample));
      last_shadsample = this_shadsample;
      shadamount += this_shadsample;
      shaftify += this_shaftify;
    }
    
    shadamount /= godray_res;
    shaftify /= godray_res;
    
    farpos = pos * (min(-pos.z, godray_res) / -pos.z);
    fade *= max(abs(falloff_multiplier / farpos.z), 1.0);
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
    
#if HAS_NO_DOF
    vec4 bloom = texture2D(bloomMap, tc/screen_res);
    diff.rgb += bloom.rgb;
#endif
    frag_color = diff;
}
