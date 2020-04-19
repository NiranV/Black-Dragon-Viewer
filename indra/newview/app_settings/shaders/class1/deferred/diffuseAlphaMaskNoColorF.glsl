/** 
 * @file diffuseAlphaMaskNoColorF.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2005, Linden Research, Inc.
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
<<<<<<< HEAD

=======
 
>>>>>>> 693791f4ffdf5471b16459ba295a50615bbc7762
/*[EXTRA_CODE_HERE]*/

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_data[3];
#else
#define frag_data gl_FragData
#endif

uniform float minimum_alpha;

uniform sampler2D diffuseMap;

VARYING vec3 vary_normal;
VARYING vec2 vary_texcoord0;

vec2 encode_normal(vec3 n);

void main() 
{
	vec4 col = texture2D(diffuseMap, vary_texcoord0.xy);
	
	if (col.a < minimum_alpha)
	{
		discard;
	}

	frag_data[0] = vec4(col.rgb, 0.0);
	frag_data[1] = vec4(0,0,0,0); // spec
	vec3 nvn = normalize(vary_normal);
	frag_data[2] = vec4(encode_normal(nvn.xyz), 0.0, 0.0);
}

