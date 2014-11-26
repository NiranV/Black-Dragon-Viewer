/** 
 * @file dofCombineF.glsl
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
//uniform sampler2DRect depthMap;
uniform sampler2DRect lightMap;

uniform mat4 inv_proj;
uniform vec2 screen_res;

uniform float max_cof;
uniform float res_scale;
uniform float dof_width;
uniform float dof_height;

VARYING vec2 vary_fragcoord;

/*float shadamount;
float shaftify;
uniform int godray_res;
uniform float godray_multiplier;

uniform vec4 shadow_clip;
uniform float haze_density;

vec3 vary_SunlitColor;
vec3 vary_AdditiveColor;

vec3 vary_PositionEye;

// Inputs
uniform mat4 shadow_matrix[6];

uniform sampler2DShadow shadowMap0;
uniform sampler2DShadow shadowMap1;
uniform sampler2DShadow shadowMap2;
uniform sampler2DShadow shadowMap3;
uniform sampler2DShadow shadowMap4;
uniform sampler2DShadow shadowMap5;

uniform vec2 shadow_res;

uniform float shadow_bias;

vec3 getPositionEye()
{
	return vary_PositionEye;
}
vec3 getSunlitColor()
{
	return vary_SunlitColor;
}
vec3 getAdditiveColor()
{
	return vary_AdditiveColor;
}

void setAdditiveColor(vec3 v)
{
	vary_AdditiveColor = v;
}

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
      return nonpcfShadow(shadowMap3, pos, pos.zw);
    }
    else if (pos.z < near_split.y) {
      pos = shadow_matrix[2]*pos;
      return nonpcfShadow(shadowMap2, pos, pos.zw);
    }
    else if (pos.z < near_split.x) {
      pos = shadow_matrix[1]*pos;
      return nonpcfShadow(shadowMap1, pos, pos.zw);
    }
    else if (pos.z > far_split.x) {
      pos = shadow_matrix[0]*pos;
      return nonpcfShadow(shadowMap0, pos, pos.zw);
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
}*/

vec4 dofSample(sampler2DRect tex, vec2 tc)
{
  	tc.x = min(tc.x, dof_width);
  	tc.y = min(tc.y, dof_height);

  	return texture2DRect(tex, tc);
}

void main() 
{
	vec2 tc = vary_fragcoord.xy;
	
	vec4 diff = texture2DRect(lightMap, tc.xy);
	vec4 dof = dofSample(diffuseRect, tc.xy*res_scale);
	dof.a = 0.0;

	float a = min(abs(diff.a*2.0-1.0) * max_cof*res_scale, 1.0);

	// help out the transition from low-res dof buffer to full-rez full-focus buffer
	if (a > 0.25 && a < 0.75)
	{
		float sc = a/res_scale;
		
		vec4 col;
		col = diff;
		col.rgb += texture2DRect(lightMap, tc.xy+vec2(sc,sc)).rgb;
		col.rgb += texture2DRect(lightMap, tc.xy+vec2(-sc,sc)).rgb;
		col.rgb += texture2DRect(lightMap, tc.xy+vec2(sc,-sc)).rgb;
		col.rgb += texture2DRect(lightMap, tc.xy+vec2(-sc,-sc)).rgb;
		
		diff = mix(diff, col*0.2, a);
	}
	
	
	/*float depth = texture2DRect(depthMap, tc.xy).r;
	vec3 pos = getPosition_d(tc, depth).xyz;
	
	// craptacular rays
	shadamount = 0.0;
	float last_shadsample = 0.0;
	shaftify = 0.0;
	float roffset = rand(vary_fragcoord.xy + vec2(1));
	vec3 farpos = pos;
	const float maxzdist = 128.0;
	farpos *= min(-farpos.z, maxzdist) / -farpos.z;
	
	vec4 spos = vec4(mix(vec3(0,0,0), farpos, (godray_res-roffset)/(godray_res)), 1.0);
	last_shadsample = shadamount = nonpcfShadowAtPos(spos);

	for (int i=godray_res-1; i>0; --i) {
	  vec4 spos = vec4(mix(vec3(0,0,0), farpos, (i-roffset)/(godray_res)), 1.0);
	  float this_shadsample = nonpcfShadowAtPos(spos);
	  float this_shaftify = abs(this_shadsample);
	  last_shadsample = this_shadsample;
	  shadamount = mix(shadamount, this_shadsample, (1.0+1.0*haze_density)/godray_res);
	  shaftify += this_shaftify;
	}
	
	shadamount /= 32;
	shaftify /= godray_res-1;
	setAdditiveColor(getAdditiveColor() * (max(shadamount,1.0)) + 0.0 * 0.5 * (shadamount) * shaftify);
	
	diff += ((shaftify * (godray_multiplier)) * (0.5) * shadamount);*/

	frag_color = mix(diff, dof, a);
}