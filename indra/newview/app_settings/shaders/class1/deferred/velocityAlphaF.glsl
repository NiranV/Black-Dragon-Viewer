/** 
 * @file velocityAlphaF.glsl
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
 
#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

vec4 diffuseLookup(vec2 texcoord);

VARYING vec3 vary_velocity;
VARYING vec2 vary_texcoord0;
VARYING vec4 vertex_color;

void main() 
{
	float alpha = diffuseLookup(vary_texcoord0.xy).a;
	alpha *= vertex_color.a;

	if (alpha < 0.75)
	{
		discard;
	}

	vec2 ref = vary_velocity.xy - vec2(0.5,0.5);
	
	if (dot(ref,ref) < 0.035)
	{
		discard;
	}

	frag_color = vec4(vary_velocity, 1.0);
}
