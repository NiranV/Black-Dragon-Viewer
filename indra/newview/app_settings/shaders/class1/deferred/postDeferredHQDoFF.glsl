/** 
 * @file postDeferredF.glsl
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

/*[EXTRA_CODE_HERE]*/

out vec4 frag_color;

uniform sampler2D diffuseRect;
uniform sampler2D depthMap;

uniform mat4 inv_proj;
uniform vec2 screen_res;
uniform float max_cof;
uniform float res_scale;

uniform float chroma_str;

in vec2 vary_fragcoord;

void dofSample(inout vec4 diff, inout float w, float min_sc, vec2 tc, float depth)
{
#if HAS_DOF_CHROMA
	vec3 col_offset = vec3(0.0015, 0.0001, 0.0005);
	float mult = min_sc * chroma_str;
	col_offset *= vec3(mult);

    vec4 s;
    s.r = texture(diffuseRect, tc + vec2(col_offset.x)).r;
    s.g  = texture(diffuseRect, tc + vec2(col_offset.y)).g;
    s.b = texture(diffuseRect, tc + vec2(col_offset.z)).b;
    s.a = texture(diffuseRect, tc).a;
#else
	vec4 s = texture(diffuseRect, tc);
#endif

	if(s.a <= depth*0.50)
	{
		float sc = abs(s.a*2.0-1.0)*(max_cof*4);

		if (sc > min_sc) //sampled pixel is more "out of focus" than current sample radius
		{
			float wg = 0.25;
			
			// de-weight dull areas to make highlights 'pop'
			wg += s.r + s.g + s.b;
			diff += wg * s;
			
			w += wg;
		}
	}
}

void dofSampleNear(inout vec4 diff, inout float w, float min_sc, vec2 tc)
{
	vec4 s = texture(diffuseRect, tc);

	float wg = 0.25;

	// de-weight dull areas to make highlights 'pop'
	wg += s.r+s.g+s.b;

	diff += wg*s;
		
	w += wg;
}

void main() 
{
	vec2 tc = vary_fragcoord.xy;
	
	vec4 diff = texture(diffuseRect, vary_fragcoord.xy);
 float depth = texture(depthMap, tc).r;
	
	{ 
		float w = 1.0;
		
		float sc = (diff.a*2.0-1.0)*(max_cof*4);
			
		float PI = 3.14159265358979323846264;
  //int its = 64;
		// sample quite uniformly spaced points within a circle, for a circular 'bokeh'
#if FRONT_BLUR
		if (sc > 0.5)
		{
			while (sc > 0.5)
			{
				int its = int(max(1.0,(sc*3.7)));
				for (int i=0; i<its; ++i)
				{
					float ang = sc+i*2*PI/its; // sc is added for rotary perturbance
					float samp_x = sc*sin(ang);
					float samp_y = sc*cos(ang);
					// you could test sample coords against an interesting non-circular aperture shape here, if desired.
					dofSampleNear(diff, w, sc, vary_fragcoord.xy + vec2(samp_x,samp_y) / screen_res);
				}
				sc -= 1.0;
			}
		}
		else if (sc < -0.5)
#else
  if (sc < -0.5)
#endif
		{
			sc = abs(sc);
			while (sc > 0.5)
			{
				int its = int(max(1.0,(sc*3.7)));
				for (int i=0; i<its; ++i)
				{
					float ang = sc+i*2*PI/its; // sc is added for rotary perturbance
					float samp_x = sc*sin(ang);
					float samp_y = sc*cos(ang);
					// you could test sample coords against an interesting non-circular aperture shape here, if desired.
     dofSample(diff, w, sc, vary_fragcoord.xy + vec2(samp_x,samp_y) / screen_res, depth);
				}
				sc -= 1.0;
			}
		}

		diff /= w;
	}
		
	frag_color = diff;
}