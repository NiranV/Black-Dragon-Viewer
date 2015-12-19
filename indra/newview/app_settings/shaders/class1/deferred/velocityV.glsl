/** 
 * @file velocityV.glsl
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

uniform mat4 modelview_projection_matrix;
uniform mat4 projection_matrix;
uniform mat4 current_modelview_matrix;
uniform mat4 last_modelview_matrix;
uniform mat4 last_object_matrix;

uniform float far_clip;

ATTRIBUTE vec3 position;

void writeVaryVelocity(vec4 pos, vec4 last_pos, vec4 last_pos2);

void main()
{
	//transform vertex
	vec4 pos = modelview_projection_matrix*vec4(position.xyz, 1.0); 
	gl_Position = pos;

	vec4 last_pos = projection_matrix * last_modelview_matrix * last_object_matrix * vec4(position.xyz, 1.0);
	vec4 last_pos2 = projection_matrix * current_modelview_matrix * last_object_matrix * vec4(position.xyz, 1.0);
	
	writeVaryVelocity(pos, last_pos, last_pos2);
}
