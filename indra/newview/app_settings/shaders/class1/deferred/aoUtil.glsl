/**
 * @file class1/deferred/aoUtil.glsl
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

uniform sampler2D   noiseMap;
uniform sampler2D   depthMap;

uniform float ssao_radius;
uniform float ssao_max_radius;
uniform float ssao_factor;
uniform float ssao_factor_inv;

uniform mat4 inv_proj;
uniform vec2 screen_res;


const int NUM_DIRECTIONS = 8;
const int NUM_STEPS = 12;

vec2 directions[NUM_DIRECTIONS];

float rand(vec2 p)
{
    p = fract(p * vec2(123.34, 345.45));
    p += dot(p, p + 34.345);
    return fract(p.x * p.y);
}

vec2 getScreenCoordinateAo(vec2 screenpos)
{
    vec2 sc = screenpos.xy * 2.0;
    return sc - vec2(1.0, 1.0);
}

float getDepthAo(vec2 pos_screen)
{
    float depth = texture(depthMap, pos_screen).r;
    return depth;
}

vec4 getPositionAo(vec2 pos_screen)
{
    float depth = getDepthAo(pos_screen);
    vec2 sc = getScreenCoordinateAo(pos_screen);
    vec4 ndc = vec4(sc.x, sc.y, 2.0*depth-1.0, 1.0);
    vec4 pos = inv_proj * ndc;
    pos /= pos.w;
    pos.w = 1.0;
    return pos;
}

vec2 getKern(int i)
{
    vec2 kern[8];
    // exponentially (^2) distant occlusion samples spread around origin
    kern[0] = vec2(-1.0, 0.0) * 0.125*0.125;
    kern[1] = vec2(1.0, 0.0) * 0.250*0.250;
    kern[2] = vec2(0.0, 1.0) * 0.375*0.375;
    kern[3] = vec2(0.0, -1.0) * 0.500*0.500;
    kern[4] = vec2(0.7071, 0.7071) * 0.625*0.625;
    kern[5] = vec2(-0.7071, -0.7071) * 0.750*0.750;
    kern[6] = vec2(-0.7071, 0.7071) * 0.875*0.875;
    kern[7] = vec2(0.7071, -0.7071) * 1.000*1.000;

    return kern[i] / screen_res;
}

//BD - Shitty ass local HBAO implementation (WIP)
//     Not optimized and doesn't put the resulting AO into a texture
//     which would allow for some proper blurring.
float calcHBAmbientOcclusion(vec4 pos, vec3 normal, vec2 pos_screen)
{
    for (int i = 0; i < NUM_DIRECTIONS; ++i)
    {
        float rand = rand(pos_screen);
        float rotation = rand * 6.2831853;
        float angle = (6.2831853 * i) / float(NUM_DIRECTIONS) + rotation;
        directions[i] = vec2(cos(angle), sin(angle));
    }

    float occlusion = 0.0;
    //BD - We want a default of 0.06 which looked decent.
    //     Assuming the default setting of 500, dividing by 10000 should give us 0.05.
    //     Multiply 500 by 1.2 to get 600 so we drop to 0.06 which is the desired default.
    float radius = (ssao_radius * 1.2) / 10000.0;

    for (int i = 0; i < NUM_DIRECTIONS; ++i)
    {
        float max_angle = -1000.0;

        vec2 dir = directions[i];

        for (int j = 1; j <= NUM_STEPS; ++j)
        {
            float step_scale = pow((float(j) + 0.5) / float(NUM_STEPS), 2.0);
            float view_scale = 1.0 / abs(pos.z);
            vec3 viewDir = normalize(-pos.xyz);
            vec2 offset = dir * radius * view_scale * step_scale;

            vec2 sampleUV = pos_screen.xy + offset;
            float sample_depth = getDepthAo(sampleUV);
            if (sample_depth >= 1.0) 
                continue;

            vec3 sample_pos = getPositionAo(sampleUV).xyz;
            vec3 diff = sample_pos - pos.xyz;
            float distance = length(diff);
            if (distance < 0.01) 
                continue;

            if (distance > ssao_factor) 
                continue;

            vec3 diff_norm = diff / distance;
            float dot = dot(normal, diff_norm);
            if (dot < 0.01) 
                continue;

            float attenuation = 1.0 / (1.0 + distance * distance * 2.0);
            max_angle = max(max_angle, dot * attenuation);
            //BD - Please kill me.
            max_angle *= clamp(sample_depth * sample_depth, 0.1, 1.0);
        }

        float horizon = clamp(max_angle, 0.0, 1.0);
        occlusion += 1.0 - horizon;
    }

    occlusion /= float(NUM_DIRECTIONS);
    
    float depth = getDepthAo(pos_screen);
    float weight_sum = 0.0;
    float result = 0.0;

    //BD - Rip this out into its own function
    for(int i = -4; i <= 4; i++)
    {
        vec2 offset = vec2(i, 0) / screen_res;

        float sample_depth = getDepthAo(pos_screen + offset);

        float depth_weight = exp(-abs(sample_depth - depth) * 50.0);
        float weight = depth_weight;

        result += occlusion * weight;
        weight_sum += weight;
    }

    for(int i = -4; i <= 4; i++)
    {
        vec2 offset = vec2(0, i) / screen_res;

        float sample_depth = getDepthAo(pos_screen + offset);

        float depth_weight = exp(-abs(sample_depth - depth) * 50.0);
        float weight = depth_weight;

        result += occlusion * weight;
        weight_sum += weight;
    }

    return result / weight_sum;
}

//calculate decreases in ambient lighting when crowded out (SSAO)
float calcAmbientOcclusion(vec4 pos, vec3 norm, vec2 pos_screen)
{
    float ret = 1.0;
    vec3 pos_world = pos.xyz;
    vec2 noise_reflect = texture(noiseMap, pos_screen.xy * (screen_res / 128)).xy;

    float angle_hidden = 0.0;
    float points = 0;

    float scale = min(ssao_radius / -pos_world.z, ssao_max_radius);

    // it was found that keeping # of samples a constant was the fastest, probably due to compiler optimizations (unrolling?)
    for (int i = 0; i < 8; i++)
    {
        vec2 samppos_screen = pos_screen + scale * reflect(getKern(i), noise_reflect);
        vec3 samppos_world = getPositionAo(samppos_screen).xyz;

        vec3 diff = pos_world - samppos_world;
        float dist2 = dot(diff, diff);

        // assume each sample corresponds to an occluding sphere with constant radius, constant x-sectional area
        // --> solid angle shrinking by the square of distance
        //radius is somewhat arbitrary, can approx with just some constant k * 1 / dist^2
        //(k should vary inversely with # of samples, but this is taken care of later)

        float funky_val = (dot((samppos_world - 0.05*norm - pos_world), norm) > 0.0) ? 1.0 : 0.0;
        angle_hidden = angle_hidden + funky_val * min(1.0/dist2, ssao_factor_inv);

        // 'blocked' samples (significantly closer to camera relative to pos_world) are "no data", not "no occlusion"
        float diffz_val = (diff.z > -1.0) ? 1.0 : 0.0;
        points = points + diffz_val;
    }

    angle_hidden = min(ssao_factor*angle_hidden/points, 1.0);

    float points_val = (points > 0.0) ? 1.0 : 0.0;
    ret = (1.0 - (points_val * angle_hidden));

    ret = max(ret, 0.0);
    return min(ret, 1.0);
}

