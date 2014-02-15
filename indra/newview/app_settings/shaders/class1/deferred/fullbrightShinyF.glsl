/** 
 * @file fullbrightShinyF.glsl
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

#ifndef diffuseLookup
uniform sampler2D diffuseMap;
#endif

VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;
VARYING vec3 vary_texcoord1;

uniform float num_colors;
uniform float greyscale_str;
uniform float sepia_str;

uniform samplerCube environmentMap;

vec3 fullbrightShinyAtmosTransport(vec3 light);
vec3 fullbrightScaleSoftClip(vec3 light);

void main()
{
#if HAS_DIFFUSE_LOOKUP
	vec4 color = diffuseLookup(vary_texcoord0.xy);
#else
	vec4 color = texture2D(diffuseMap, vary_texcoord0.xy);
#endif

	
	color.rgb *= vertex_color.rgb;
	
	vec3 envColor = textureCube(environmentMap, vary_texcoord1.xyz).rgb;	
	color.rgb = mix(color.rgb, envColor.rgb, vertex_color.a);

	color.rgb = pow(color.rgb,vec3(2.2f,2.2f,2.2f));
	
	color.rgb = fullbrightShinyAtmosTransport(color.rgb);
	color.rgb = fullbrightScaleSoftClip(color.rgb);

	color.a = 1.0;

	color.rgb = pow(color.rgb, vec3(1.0/2.2));
	
	#if POSTERIZATION
		color.rgb = pow(color.rgb, vec3(0.6));
		color.rgb = color.rgb * num_colors;
		color.rgb = floor(color.rgb);
		color.rgb = color.rgb / num_colors;
		color.rgb = pow(color.rgb, vec3(1.0/0.6));
	#endif
	
	#if GREY_SCALE
		vec3 color_gr = vec3((0.299 * color.r) + (0.587 * color.g) + (0.114 * color.b));
		color.rgb = mix(color.rgb, color_gr, 1.0);
	#endif
	
	#if SEPIA
		float sepia_mix = dot(vec3(0.299, 0.587, 0.114), vec3(color.rgb));
		vec3 color_sep = mix(vec3(0.14, 0.03, 0.0), vec3(0.72, 0.63, 0.25), sepia_mix);
		color_sep = mix(color_sep, vec3(sepia_mix), 0.0);
		color.rgb = mix(color.rgb, color_sep, 1.0);
	#endif
	
	frag_color = color;
}

