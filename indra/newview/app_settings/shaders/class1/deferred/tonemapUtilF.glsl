/**
 * @file postDeferredTonemap.glsl
 *
 * $LicenseInfo:firstyear=2024&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2024, Linden Research, Inc.
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

uniform sampler2D exposureMap;
uniform vec2 screen_res;
in vec2 vary_fragcoord;

//===============================================================
// tone mapping taken from Khronos sample implementation
//===============================================================

// sRGB => XYZ => D65_2_D60 => AP1 => RRT_SAT
const mat3 ACESInputMat = mat3
(
    0.59719, 0.07600, 0.02840,
    0.35458, 0.90834, 0.13383,
    0.04823, 0.01566, 0.83777
);


// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat = mat3
(
    1.60475, -0.10208, -0.00327,
    -0.53108,  1.10813, -0.07276,
    -0.07367, -0.00605,  1.07602
);

// ACES tone map (faster approximation)
// see: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 toneMapACES_Narkowicz(vec3 color)
{
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    return clamp((color * (A * color + B)) / (color * (C * color + D) + E), 0.0, 1.0);
}


// ACES filmic tone map approximation
// see https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
vec3 RRTAndODTFit(vec3 color)
{
    vec3 a = color * (color + 0.0245786) - 0.000090537;
    vec3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
    return a / b;
}


// tone mapping
vec3 toneMapACES_Hill(vec3 color)
{
    color = ACESInputMat * color;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = ACESOutputMat * color;

    // Clamp to [0, 1]
    color = clamp(color, 0.0, 1.0);

    return color;
}

// Khronos Neutral tonemapping
// https://github.com/KhronosGroup/ToneMapping/tree/main
// Input color is non-negative and resides in the Linear Rec. 709 color space.
// Output color is also Linear Rec. 709, but in the [0, 1] range.
vec3 PBRNeutralToneMapping( vec3 color )
{
  const float startCompression = 0.8 - 0.04;
  const float desaturation = 0.15;

  float x = min(color.r, min(color.g, color.b));
  float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
  color -= offset;

  float peak = max(color.r, max(color.g, color.b));
  if (peak < startCompression) return color;

  const float d = 1. - startCompression;
  float newPeak = 1. - d * d / (peak + d - startCompression);
  color *= newPeak / peak;

  float g = 1. - 1. / (desaturation * (peak - newPeak) + 1.);
  return mix(color, newPeak * vec3(1, 1, 1), g);
}


vec3 PBRReinhardToneMapping(vec3 color)
{
	float white = 2.;
	float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float toneMappedLuma = luma * (1. + luma / (white*white)) / (1. + luma);
	color *= toneMappedLuma / luma;
	return color;
}

vec3 Uncharted2ToneMapping(vec3 color)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;
	float exposure = 2.;
	color *= exposure;
	color = ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;
	float white = ((W * (A * W + C * B) + D * E) / (W * (A * W + B) + D * F)) - E / F;
	color /= white;
	return color;
}

vec3 FilmicToneMapping(vec3 color)
{
	vec3 X = max(vec3(0.0), color - 0.004);
    vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
    return pow(result, vec3(2.2));
}


uniform float exposure;
uniform float tonemap_mix;
uniform int tonemap_type;

uniform float greyscale_str;
uniform float sepia_str;
uniform float num_colors;
uniform float chroma_str;

vec3 toneMap(vec3 color)
{
#ifndef NO_POST
    float exp_scale = texture(exposureMap, vec2(0.5,0.5)).r;

    color *= exposure * exp_scale;

    vec3 clamped_color = clamp(color.rgb, vec3(0.0), vec3(1.0));

    switch(tonemap_type)
    {
    case 0:
        color = PBRNeutralToneMapping(color);
        break;
    case 1:
        color = toneMapACES_Hill(color);
        break;
    case 2:
        color = PBRReinhardToneMapping(color);
        break;
    case 3:
        color = Uncharted2ToneMapping(color);
        break;
    case 4:
        color = FilmicToneMapping(color);
        break;
    }

    // mix tonemapped and linear here to provide adjustment
    color = mix(clamped_color, color, tonemap_mix);
#endif

    if(num_colors > 2)
	{
		color = pow(color, vec3(0.6));
		color = color * num_colors;
		color = floor(color);
		color = color / num_colors;
		color = pow(color, vec3(1.0/0.6));
	}

    vec3 col_gr = vec3((0.299 * color.r) + (0.587 * color.g) + (0.114 * color.b));
	color = mix(color, col_gr, greyscale_str);

    vec3 col_sep;
	col_sep.r = (color.r*0.3588) + (color.g*0.7044) + (color.b*0.1368);
	col_sep.g = (color.r*0.299) + (color.g*0.5870) + (color.b*0.114);
	col_sep.b = (color.r*0.2392) + (color.g*0.4696) + (color.b*0.0912);
	color = mix(color, col_sep, sepia_str);

    return color;
}


vec3 toneMapNoExposure(vec3 color)
{
#ifndef NO_POST
    vec3 clamped_color = clamp(color.rgb, vec3(0.0), vec3(1.0));

    switch(tonemap_type)
    {
    case 0:
        color = PBRNeutralToneMapping(color);
        break;
    case 1:
        color = PBRReinhardToneMapping(color);
        break;
    case 2:
        color = PBRReinhardToneMapping(color);
        break;
    case 3:
        color = Uncharted2ToneMapping(color);
        break;
    case 4:
        color = FilmicToneMapping(color);
        break;
    }

    // mix tonemapped and linear here to provide adjustment
    color = mix(clamped_color, color, tonemap_mix);
#endif

    if(num_colors > 2)
	{
		color = pow(color, vec3(0.6));
		color = color * num_colors;
		color = floor(color);
		color = color / num_colors;
		color = pow(color, vec3(1.0/0.6));
	}

    vec3 col_gr = vec3((0.299 * color.r) + (0.587 * color.g) + (0.114 * color.b));
	color = mix(color, col_gr, greyscale_str);

    vec3 col_sep;
	col_sep.r = (color.r*0.3588) + (color.g*0.7044) + (color.b*0.1368);
	col_sep.g = (color.r*0.299) + (color.g*0.5870) + (color.b*0.114);
	col_sep.b = (color.r*0.2392) + (color.g*0.4696) + (color.b*0.0912);
	color = mix(color, col_sep, sepia_str);

    return color;
}


//===============================================================

void debugExposure(inout vec3 color)
{
    float exp_scale = texture(exposureMap, vec2(0.5,0.5)).r;
    exp_scale *= 0.5;
    if (abs(vary_fragcoord.y-exp_scale) < 0.01 && vary_fragcoord.x < 0.1)
    {
        color = vec3(1,0,0);
    }
}
