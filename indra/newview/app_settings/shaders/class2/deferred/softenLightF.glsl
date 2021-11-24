/**
 * @file class2/deferred/softenLightF.glsl
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
#extension GL_ARB_shader_texture_lod : enable

/*[EXTRA_CODE_HERE]*/

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2DRect diffuseRect;
uniform sampler2DRect specularRect;
uniform sampler2DRect normalMap;
uniform sampler2DRect lightMap;
uniform sampler2DRect depthMap;
uniform samplerCube   environmentMap;
uniform sampler2D     lightFunc;

uniform float blur_size;
uniform float blur_fidelity;

// Inputs
uniform mat3 env_mat;

uniform vec3 sun_dir;
uniform vec3 moon_dir;
uniform int  sun_up_factor;
VARYING vec2 vary_fragcoord;

uniform mat4 inv_proj;
uniform vec2 screen_res;

vec4 getPositionWithDepth(vec2 pos_screen, float depth);

void calcAtmosphericVars(vec3 inPositionEye, vec3 light_dir, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 additive, out vec3 atten, bool use_ao);
float getAmbientClamp();
vec3  atmosFragLighting(vec3 l, vec3 additive, vec3 atten);
vec3  scaleSoftClipFrag(vec3 l);
vec3  fullbrightAtmosTransportFrag(vec3 light, vec3 additive, vec3 atten);
vec3  fullbrightScaleSoftClip(vec3 light);

vec3 decode_normal(vec2 enc);
vec3 linear_to_srgb(vec3 c);
vec3 srgb_to_linear(vec3 c);

#ifdef WATER_FOG
vec4 applyWaterFogView(vec3 pos, vec4 color);
#endif

vec2 ref2d;
vec3 refcol;
vec3 best_refn;
vec3 best_refcol;
vec3 reflight;
float best_refshad;
float best_refapprop;
float total_refapprop;
float rnd;
float bloomdamp;
float rnd2;
float gnfrac;
float rd;
float rdpow2;
float refdist;
float refdepth;

uniform float chroma_str;
uniform int ssr_res;
uniform float ssr_brightness;

float rand(vec2 co)
{
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main() 
{
    vec2  tc           = vary_fragcoord.xy;
    float depth        = texture2DRect(depthMap, tc.xy).r;
    vec4  pos          = getPositionWithDepth(tc, depth);
    vec4  norm         = texture2DRect(normalMap, tc);
    float envIntensity = norm.z;
    norm.xyz           = decode_normal(norm.xy);

    vec3  light_dir   = (sun_up_factor == 1) ? sun_dir : moon_dir;
    float da          = clamp(dot(norm.xyz, light_dir.xyz), 0.0, 1.0);
    float light_gamma = 1.0 / 1.3;
    da                = pow(da, light_gamma);

    vec4 diffuse = texture2DRect(diffuseRect, tc);
    
    vec2 fromCentre = vec2(0.0);
    if(chroma_str > 0.0)
    {
        fromCentre = (tc / screen_res) - vec2(0.5);
        float radius = length(fromCentre);
        fromCentre = (chroma_str * (radius*radius)) / vec2(1);
        
        diffuse.b = texture2DRect(diffuseRect, tc-fromCentre).b;
        diffuse.r = texture2DRect(diffuseRect, tc+fromCentre).r;
        diffuse.ga = texture2DRect(diffuseRect, tc).ga;
    }
    
    //convert to gamma space
    diffuse.rgb = linear_to_srgb(diffuse.rgb); // SL-14025
    vec4 spec    = texture2DRect(specularRect, vary_fragcoord.xy);

    vec2 scol_ambocc = texture2DRect(lightMap, vary_fragcoord.xy).rg;
    scol_ambocc      = pow(scol_ambocc, vec2(light_gamma));
    float scol       = max(scol_ambocc.r, diffuse.a);
    float ambocc     = scol_ambocc.g;

    vec3  color = vec3(0);
    float bloom = 0.0;

    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;
    calcAtmosphericVars(pos.xyz, light_dir, ambocc, sunlit, amblit, additive, atten, true);

    color.rgb = amblit;

    float ambient = min(abs(dot(norm.xyz, sun_dir.xyz)), 1.0);
    ambient *= 0.5;
    ambient *= ambient;
    ambient = (1.0 - ambient);
    color.rgb *= ambient;

    vec3 sun_contrib = min(da, scol) * sunlit;
    color.rgb += sun_contrib;
    color.rgb *= diffuse.rgb;

    vec3 refnormpersp = normalize(reflect(pos.xyz, norm.xyz));

#if USE_SSR
        if (spec.a > 0.0) // specular reflection
        {
         float fullbrightification = diffuse.a;
         // the old infinite-sky shiny reflection
         float sa = dot(refnormpersp, light_dir.xyz);
         
         // screen-space cheapish fakey reflection map
         vec3 refnorm = normalize(reflect(vec3(0,0,-1), norm.xyz));
         depth -= 0.5; // unbias depth
         vec2 orig_ref2d = (norm.xy) * (1.0- depth);
         
         // Offset the guess source a little according to a trivial
         // checkerboard dither function and spec.a.
         // This is meant to be similar to sampling a blurred version
         // of the diffuse map.  LOD would be better in that regard.
         // The goal of the blur is to soften reflections in surfaces
         // with low shinyness, and also to disguise our lameness.
         float checkerboard = floor(mod(tc.x+tc.y, 2.0)); // 0.0, 1.0
         
         // hack because I can't decide whether refnormpersp or refnorm are better :3
         vec2 orig_ref2dpersp = (refnormpersp.xy);
         
         best_refn = vec3(0);
         best_refshad = 0;
         best_refapprop = -1.0;
         best_refcol = vec3(0);
         total_refapprop = 0;
         rnd = rand(tc.xy);
         reflight = light_dir.xyz;//reflect(sun_dir.xyz, norm.xyz);
         bloomdamp = 0.0;
         for (int guessnum = 1; guessnum <= ssr_res; ++guessnum)
         {
          float guessnumfp = float(guessnum);
          guessnumfp -= (checkerboard*0.5 + rnd);
          rd = guessnumfp / ssr_res;
          rdpow2 = rd * rd;
          refdist = (-2.5/(-1.0+pos.z))*(1.0-(norm.z*norm.z))*(screen_res.y * (rdpow2));// / (-depth) ;
#if USE_RAND
          float rnd2 = rand(vec2(guessnum+rnd, tc.x));
          ref2d = (orig_ref2d + spec.a*0.25*vec2(rnd2-0.5)) * refdist;
#else
          ref2d = (orig_ref2d + spec.a*vec2(0.0)) * refdist;
#endif
          ref2d += tc.xy; // use as offset from destination
          
          if (ref2d.y < 0.0 || ref2d.y > screen_res.y ||
          ref2d.x < 0.0 || ref2d.x > screen_res.x) continue;
          
          // Get attributes from the 2D guess point.
          float refdepth = texture2DRect(depthMap, ref2d).r;
          vec3 refcol = texture2DRect(diffuseRect, ref2d).rgb;
          
          vec3 refpos = getPositionWithDepth(ref2d, refdepth).xyz;
          
          // figure out how appropriate our guess actually was, directionwise
          float refposdistpow2 = dot(refpos - pos.xyz, refpos - pos.xyz);
          float refapprop = 1.0;
          
          if (refdepth < 1.0)
          { // non-sky
           float angleapprop = sqrt(max(0.0, dot(refnorm, (refpos - pos.xyz)) / (1.0 + refposdistpow2 )));
           refapprop = min(refapprop, angleapprop);
           float refshad = texture2DRect(lightMap, ref2d).r;
           refshad = pow(refshad, light_gamma);
           vec3 refn = decode_normal(ref2d);
           
           // darken reflections from points which face away from the reflected ray - our guess was a back-face
           //refapprop = min(refapprop, step(dot(refnorm, refn), 0.001));
           
           // kill guesses which are 'behind' the reflector
           float ppdist = dot(norm.xyz, refpos.xyz - pos.xyz);
           
           refapprop = min(refapprop, step(0.01, ppdist));
           total_refapprop += refapprop;
           best_refn += refn.xyz * refapprop;
           best_refshad += refshad * refapprop;
           float sunc = max(0.0, dot(reflight, refn));
           
           // pow
           best_refcol += (((amblit + sunlit * min(sunc, refshad)) * (refcol.rgb) + additive)) * refapprop;
          }
          else  // sky
          {
           // avoid forward-pointing reflections picking up sky
           refapprop = min(refapprop, max(-refnorm.z, 0.0));//dot(refnorm, vec3(0.0, 0.0, -1.0));
           refapprop *= 0.5; // we just plain like the appropriateness of non-sky reflections better where available
           
           total_refapprop += refapprop;
           best_refn += reflight.xyz * refapprop; // treat sky samples as if they always face the sun
           best_refshad += 1.0 * refapprop; // sky is not shadowed
           best_refcol += refcol.rgb * refapprop;
          }
         }
         if (total_refapprop > 0.0)
         {
          // we must have the power of >= 25% voters, else damp progressively
          float use_refapprop = max(ssr_res*0.25, (total_refapprop));
          
          best_refn = normalize(best_refn);
          best_refshad /= use_refapprop;
          best_refcol /= use_refapprop * 2.0; // div2 cos we'll be doubled again
          bloomdamp /= use_refapprop;
          best_refapprop = min(1.0, use_refapprop);
         }
         else
         {
          best_refcol.rgb = vec3(0,0,0);
          best_refapprop = 0.0;
         }
         vec3 refprod = best_refcol.rgb * best_refapprop;
         vec3 ssshiny = (refprod);
         ssshiny *= spec.rgb;
         ssshiny *= ssr_brightness;
         
         vec3 dumbshiny = (sunlit)*(scol * 0.25)*(0.5 * texture2D(lightFunc, vec2(sa, spec.a)).r);
         dumbshiny = min(dumbshiny, vec3(1));
         
         // add the two types of shiny together
         vec3 spec_contrib = (ssshiny * (1.0 - fullbrightification) * 0.5 + dumbshiny);
         dumbshiny = sunlit*scol_ambocc.r*(texture2D(lightFunc, vec2(sa, spec.a)).r);
         spec_contrib = dumbshiny * spec.rgb;
         color.rgb = mix(color.rgb + ssshiny, diffuse.rgb, fullbrightification);
         bloom = dot(spec_contrib, spec_contrib) / 6;
         color.rgb += spec_contrib;
        }
#else
    if (spec.a > 0.0)  // specular reflection
    {
        float sa        = dot(refnormpersp, light_dir.xyz);
        vec3  dumbshiny = sunlit * scol * (texture2D(lightFunc, vec2(sa, spec.a)).r);

        // add the two types of shiny together
        vec3 spec_contrib = dumbshiny * spec.rgb;
        bloom             = dot(spec_contrib, spec_contrib) / 6;
        color.rgb += spec_contrib;
    }
#endif

    color.rgb = mix(color.rgb, diffuse.rgb, diffuse.a);

#if USE_ENV_MAP
    if (envIntensity > 0.0)
    {  // add environmentmap
        vec3 env_vec         = env_mat * refnormpersp;
        vec3 reflected_color = textureCube(environmentMap, env_vec).rgb;
        color                = mix(color.rgb, reflected_color, envIntensity);
    }
#endif

    if (norm.w < 0.5)
    {
        color = mix(atmosFragLighting(color, additive, atten), fullbrightAtmosTransportFrag(color, additive, atten), diffuse.a);
        color = mix(scaleSoftClipFrag(color), fullbrightScaleSoftClip(color), diffuse.a);
    }

#ifdef WATER_FOG
    vec4 fogged = applyWaterFogView(pos.xyz, vec4(color, bloom));
    color       = fogged.rgb;
    bloom       = fogged.a;
#endif

    // convert to linear as fullscreen lights need to sum in linear colorspace
    // and will be gamma (re)corrected downstream...
    frag_color.rgb = srgb_to_linear(color.rgb);
    frag_color.a   = bloom;
}
