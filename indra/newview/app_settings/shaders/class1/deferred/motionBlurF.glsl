/** 
 * @file velocityF.glsl
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

uniform float time_step;
uniform float mblur_strength;

VARYING vec4 vary_fragcoord;
uniform vec2 screen_res;

void main() 
{
	vec2 frag = (vary_fragcoord.xy*0.5+0.5)*screen_res;
	
	vec3 v = texture2DRect(normalMap, frag.xy).rgb;
	
	if (v.b < 1.0)
	{
		discard;
	}

	//unpack to [-1, 1]
	vec3 velocity = v*2.0-1.0;
	
	float target_step = 1.0/mblur_strength; //180 degree shutter at 24fps (film)
	float ratio = target_step/time_step;

	velocity *= ratio;

	vec3 color = vec3(0,0,0);

	frag.xy += velocity.xy * 12 * 2.0;

	float total = 0.0;

	for (int i = 0; i < 32; ++i)
	{
		float w = 32-abs(i-16);
		total += w;
		color += texture2DRect(diffuseRect, frag.xy).rgb*w;
		frag.xy -= velocity.xy*2.0;
	}

	color /= total;
			
	frag_color = vec4(color, 1.0);
}
