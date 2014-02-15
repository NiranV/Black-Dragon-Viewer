/** 
 * @file impostorF.glsl
 *
 * $LicenseInfo:firstyear=2011&license=viewerlgpl$
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
out vec4 frag_data[3];
#else
#define frag_data gl_FragData
#endif

uniform float minimum_alpha;

uniform float num_colors;
uniform float greyscale_str;
uniform float sepia_str;

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D specularMap;

VARYING vec2 vary_texcoord0;

vec3 decode_normal (vec2 enc)
{
    vec2 fenc = enc*4-2;
    float f = dot(fenc,fenc);
    float g = sqrt(1-f/4);
    vec3 n;
    n.xy = fenc*g;
    n.z = 1-f/2;
    return n;
}

vec2 encode_normal(vec3 n)
{
	float f = sqrt(8 * n.z + 8);
	return n.xy / f + 0.5;
}

vec3 linear_to_srgb(vec3 cl)
{
	cl = clamp(cl, vec3(0), vec3(1));
	vec3 low_range  = cl * 12.92;
	vec3 high_range = 1.055 * pow(cl, vec3(0.41666)) - 0.055;
	bvec3 lt = lessThan(cl,vec3(0.0031308));

#ifdef OLD_SELECT
	vec3 result;
	result.r = lt.r ? low_range.r : high_range.r;
	result.g = lt.g ? low_range.g : high_range.g;
	result.b = lt.b ? low_range.b : high_range.b;
    return result;
#else
	return mix(high_range, low_range, lt);
#endif

}

void main() 
{
	vec4 col = texture2D(diffuseMap, vary_texcoord0.xy);

	if (col.a < minimum_alpha)
	{
		discard;
	}

	vec4 norm = texture2D(normalMap,   vary_texcoord0.xy);
	vec4 spec = texture2D(specularMap, vary_texcoord0.xy);

	col.rgb = linear_to_srgb(col.rgb);
	
	#if POSTERIZATION
		col.rgb = pow(col.rgb, vec3(0.6));
		col.rgb = col.rgb * num_colors;
		col.rgb = floor(col.rgb);
		col.rgb = col.rgb / num_colors;
		col.rgb = pow(col.rgb, vec3(1.0/0.6));
	#endif
	
	#if GREY_SCALE
		vec3 col_gr = vec3((0.299 * col.r) + (0.587 * col.g) + (0.114 * col.b));
		col.rgb = mix(col.rgb, col_gr, greyscale_str);
	#endif
	
	#if SEPIA
		float sepia_mix = dot(vec3(0.299, 0.587, 0.114), vec3(col.rgb));
		vec3 col_sep = mix(vec3(0.14, 0.03, 0.0), vec3(0.72, 0.63, 0.25), sepia_mix);
		col_sep = mix(col_sep, vec3(sepia_mix), 0.0);
		col.rgb = mix(col.rgb, col_sep, sepia_str);
	#endif

	frag_data[0] = vec4(col.rgb, 0.0);
	frag_data[1] = spec;
	frag_data[2] = vec4(norm.xy,0,0);
}
