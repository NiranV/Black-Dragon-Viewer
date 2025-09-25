/**
 * @file glowcombineF.glsl
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

/*[EXTRA_CODE_HERE]*/

out vec4 frag_color;

uniform sampler2D diffuseRect;
uniform sampler2D emissiveRect;

uniform float greyscale_str;
uniform float sepia_str;
uniform float num_colors;

in vec2 tc;

void main()
{
    vec4 diff = texture(diffuseRect, tc);
    vec4 emis = texture(emissiveRect, tc);

    diff = diff + emis;
    if(num_colors > 2)
	{
		diff.rgb = pow(diff.rgb, vec3(0.6));
		diff.rgb = diff.rgb * num_colors;
		diff.rgb = floor(diff.rgb);
		diff.rgb = diff.rgb / num_colors;
		diff.rgb = pow(diff.rgb, vec3(1.0/0.6));
	}

    vec3 col_gr = vec3((0.299 * diff.r) + (0.587 * diff.g) + (0.114 * diff.b));
	diff.rgb = mix(diff.rgb, col_gr, greyscale_str);

    vec3 col_sep;
	col_sep.r = (diff.r*0.3588) + (diff.g*0.7044) + (diff.b*0.1368);
	col_sep.g = (diff.r*0.299) + (diff.g*0.5870) + (diff.b*0.114);
	col_sep.b = (diff.r*0.2392) + (diff.g*0.4696) + (diff.b*0.0912);
	diff.rgb = mix(diff.rgb, col_sep, sepia_str);

    frag_color = diff;
}
