/**
 * @file postDeferredNoDoFF.glsl
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
uniform sampler2D depthMap;

uniform vec2 screen_res;
in vec2 vary_fragcoord;

uniform float chroma_str;

//=================================
// borrowed noise from:
//  <https://www.shadertoy.com/view/4dS3Wd>
//  By Morgan McGuire @morgan3d, http://graphicscodex.com
//
float hash(float n) { return fract(sin(n) * 1e4); }
float hash(vec2 p) { return fract(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x)))); }

float noise(float x) {
    float i = floor(x);
    float f = fract(x);
    float u = f * f * (3.0 - 2.0 * f);
    return mix(hash(i), hash(i + 1.0), u);
}

float noise(vec2 x) {
    vec2 i = floor(x);
    vec2 f = fract(x);

    // Four corners in 2D of a tile
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    // Simple 2D lerp using smoothstep envelope between the values.
    // return vec3(mix(mix(a, b, smoothstep(0.0, 1.0, f.x)),
    //          mix(c, d, smoothstep(0.0, 1.0, f.x)),
    //          smoothstep(0.0, 1.0, f.y)));

    // Same code, with the clamps in smoothstep and common subexpressions
    // optimized away.
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

//=============================

vec3 clampHDRRange(vec3 color);


void main()
{
    vec4 diff = texture(diffuseRect, vary_fragcoord.xy);
#if HAS_DOF_CHROMA == 0
    vec3 col = diff.rgb;

    float luma = dot(col, vec3(0.2126, 0.7152, 0.0722));

    vec2 p = (vary_fragcoord.xy / screen_res.xy) * 2.0 - 1.0;
    float r = dot(p, p);

    float depth = texture(depthMap, vary_fragcoord.xy).r;
    float depthWeight = smoothstep(0.2, 0.8, depth);
    depthWeight *= depthWeight;

    vec3 gx = dFdx(col);
    vec3 gy = dFdy(col);
    vec3 grad = gx + gy;
    float edge = length(grad);

    // Blue disperses most, green least
    const float wR = 0.60;
    const float wG = 0.15;
    const float wB = 1.00;

    float ca =
        chroma_str *
        edge *
        smoothstep(0.15, 1.0, luma) *
        smoothstep(0.0, 1.2, r) *
        depthWeight;

    vec2 dir = normalize(p + 1e-5);

    col.r += ca * wR * (grad.r) * dir.x;
    col.g += ca * wG * (grad.g) * dir.x * 0.25; // very subtle
    col.b -= ca * wB * (grad.b) * dir.x;

    col.r += ca * wR * gy.r * 0.30;
    col.b -= ca * wB * gy.b * 0.30;

    diff.rgb = col;
    diff.rgb = clamp(diff.rgb, 0.0, 1.0);
#endif


#ifdef HAS_NOISE
    vec2 tc_q = vary_fragcoord.xy*screen_res*4.0;
    vec3 seed = (diff.rgb+vec3(1.0))*vec3(tc_q.xy, tc_q.x+tc_q.y);
    vec3 nz = vec3(noise(seed.rg), noise(seed.gb), noise(seed.rb));
    diff.rgb += nz*0.003;
#endif

    diff.rgb = clampHDRRange(diff.rgb);
    frag_color = diff;

    gl_FragDepth = texture(depthMap, vary_fragcoord.xy).r;
}

