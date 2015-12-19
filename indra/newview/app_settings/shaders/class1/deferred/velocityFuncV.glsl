/** 
 * @file velocityFuncV.glsl
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

VARYING vec3 vary_velocity;
uniform vec4 viewport;

void writeVaryVelocity(vec4 pos, vec4 last_pos, vec4 last_pos2)
{
	pos.xyz /= pos.w;
	last_pos.xyz /= last_pos.w;
	last_pos2.xyz /= last_pos2.w;
	
	vec2 velocity = pos.xy - last_pos.xy;

	vec2 velocity2 = velocity - (last_pos2.xy-last_pos.xy);

	float a = dot(velocity,velocity);
	float b = dot(velocity2, velocity2);

	if (b < a)
	{
		velocity = velocity2;
	}


	//convert to pixel velocity
	velocity.xy *= vec2(viewport.zw-viewport.xy)/64.0;
		
	//cap length of velocity.xy to [-1, 1]
	float len = length(velocity.xy);
	
	if (len > 0)
	{
		velocity.xy /= len;
		velocity.xy *= min(len, 1.0);
	
		//convert from [-1, 1] to [0, 1];
		velocity.xy *= 0.5;
		velocity.xy += 0.5;
	}
	else
	{
		velocity.xy = vec2(0.5, 0.5);
	}

	vary_velocity = vec3(velocity, 1.0);
}
