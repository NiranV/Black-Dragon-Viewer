/** 
 * @file skinnedVelocityV.glsl
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

uniform mat4 projection_matrix;
uniform mat4 modelview_matrix;
uniform mat4 last_modelview_matrix;

uniform mat4 lastMatrixPalette[110];

ATTRIBUTE vec3 position;
ATTRIBUTE vec4 weight4;  
ATTRIBUTE vec2 texcoord0;
ATTRIBUTE vec4 diffuse_color;

VARYING vec2 vary_texcoord0;
VARYING vec4 vertex_color;

mat4 getObjectSkinnedTransform();
void writeVaryVelocity(vec4 pos, vec4 last_pos, vec4 last_pos2);

mat4 getLastObjectSkinnedTransform()
{
	vary_texcoord0 = texcoord0;
	int i; 
	
	vec4 w = fract(weight4);
	vec4 index = floor(weight4);
	
	float scale = 1.0/(w.x+w.y+w.z+w.w);
	w *= scale;
	
	mat4 mat = lastMatrixPalette[int(index.x)]*w.x;
	mat += lastMatrixPalette[int(index.y)]*w.y;
	mat += lastMatrixPalette[int(index.z)]*w.z;
	mat += lastMatrixPalette[int(index.w)]*w.w;
		
	return mat;
}


void main()
{
	mat4 object_skin = getObjectSkinnedTransform();
	
	mat4 mat = modelview_matrix * object_skin;
	vec4 pos = (mat*vec4(position.xyz, 1.0));
	pos = projection_matrix*pos;
		
	gl_Position = pos;

	mat4 last_skin_mat = getLastObjectSkinnedTransform();
	
	mat4 last_mat = last_modelview_matrix * last_skin_mat;
	vec4 last_pos = last_mat*(vec4(position.xyz, 1.0));
	last_pos = projection_matrix*last_pos;

	mat4 last_mat2 = modelview_matrix * last_skin_mat;
	vec4 last_pos2 = last_mat2*(vec4(position.xyz, 1.0));
	last_pos2 = projection_matrix*last_pos2;

	vertex_color = diffuse_color;

	writeVaryVelocity(pos, last_pos, last_pos2);
}
