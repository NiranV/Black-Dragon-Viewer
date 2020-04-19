/** 
 * @file alphaF.glsl
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

//class1/deferred/alphaF.glsl

#extension GL_ARB_texture_rectangle : enable

/*[EXTRA_CODE_HERE]*/

#define INDEXED 1
#define NON_INDEXED 2
#define NON_INDEXED_NO_COLOR 3

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform mat3 env_mat;
<<<<<<< HEAD
uniform float ssao_effect;

uniform vec3 sun_dir;

uniform sampler2DShadow shadowMap0;
uniform sampler2DShadow shadowMap1;
uniform sampler2DShadow shadowMap2;
uniform sampler2DShadow shadowMap3;

uniform vec4 shadow_res;

uniform mat4 shadow_matrix[6];
uniform vec4 shadow_clip;
uniform vec4 shadow_bias;
=======
uniform vec3 sun_dir;
uniform vec3 moon_dir;
>>>>>>> 693791f4ffdf5471b16459ba295a50615bbc7762

#ifdef USE_DIFFUSE_TEX
uniform sampler2D diffuseMap;
#endif

VARYING vec3 vary_fragcoord;
VARYING vec3 vary_position;
VARYING vec2 vary_texcoord0;
VARYING vec3 vary_norm;

#ifdef USE_VERTEX_COLOR
VARYING vec4 vertex_color; //vertex color should be treated as sRGB
#endif

uniform mat4 proj_mat;
uniform mat4 inv_proj;
uniform vec2 screen_res;
uniform int sun_up_factor;
uniform vec4 light_position[8];
uniform vec3 light_direction[8];
uniform vec4 light_attenuation[8]; 
uniform vec3 light_diffuse[8];

<<<<<<< HEAD
uniform vec4 waterPlane;
uniform vec4 waterFogColor;
uniform float waterFogDensity;
uniform float waterFogKS;

uniform float global_light_strength;

vec3 srgb_to_linear(vec3 cs)
{
	vec3 low_range = cs / vec3(12.92);
	vec3 high_range = pow((cs+vec3(0.055))/vec3(1.055), vec3(2.4));
	bvec3 lte = lessThanEqual(cs,vec3(0.04045));

#ifdef OLD_SELECT
	vec3 result;
	result.r = lte.r ? low_range.r : high_range.r;
	result.g = lte.g ? low_range.g : high_range.g;
	result.b = lte.b ? low_range.b : high_range.b;
    return result;
#else
	return mix(high_range, low_range, lte);
#endif
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
=======
#ifdef WATER_FOG
vec4 applyWaterFogView(vec3 pos, vec4 color);
#endif

vec3 srgb_to_linear(vec3 c);
vec3 linear_to_srgb(vec3 c);

vec2 encode_normal (vec3 n);
vec3 scaleSoftClipFrag(vec3 l);
vec3 atmosFragLighting(vec3 light, vec3 additive, vec3 atten);

void calcAtmosphericVars(vec3 inPositionEye, vec3 light_dir, float ambFactor, out vec3 sunlit, out vec3 amblit, out vec3 atten, out vec3 additive, bool use_ao);
>>>>>>> 693791f4ffdf5471b16459ba295a50615bbc7762

#ifdef HAS_SHADOW
float sampleDirectionalShadow(vec3 pos, vec3 norm, vec2 pos_screen);
#endif

float getAmbientClamp();

vec3 calcPointLightOrSpotLight(vec3 light_col, vec3 diffuse, vec3 v, vec3 n, vec4 lp, vec3 ln, float la, float fa, float is_pointlight, float ambiance)
{
    vec3 col = vec3(0);

	//get light vector
	vec3 lv = lp.xyz-v;

	//get distance
	float dist = length(lv);
	float da = 1.0;

    /*if (dist > la)
    {
        return col;
    }

    clip to projector bounds
     vec4 proj_tc = proj_mat * lp;

    if (proj_tc.z < 0
     || proj_tc.z > 1
     || proj_tc.x < 0
     || proj_tc.x > 1 
     || proj_tc.y < 0
     || proj_tc.y > 1)
    {
        return col;
    }*/

	if (dist > 0.0 && la > 0.0)
	{
        dist /= la;

		//normalize light vector
		lv = normalize(lv);
	
		//distance attenuation
		float dist_atten = clamp(1.0-(dist-1.0*(1.0-fa))/fa, 0.0, 1.0);
		dist_atten *= dist_atten;
        dist_atten *= 2.0f;

        if (dist_atten <= 0.0)
        {
           return col;
        }

		// spotlight coefficient.
		float spot = max(dot(-ln, lv), is_pointlight);
		da *= spot*spot; // GL_SPOT_EXPONENT=2

		//angular attenuation
		da *= dot(n, lv);
        da = max(0.0, da);

		float lit = 0.0f;

        float amb_da = 0.0;//ambiance;
        if (da > 0)
        {
		    lit = max(da * dist_atten,0.0);
            col = lit * light_col * diffuse;
            amb_da += (da*0.5+0.5) * ambiance;
        }
        amb_da += (da*da*0.5 + 0.5) * ambiance;
        amb_da *= dist_atten;
        amb_da = min(amb_da, 1.0f - lit);

        // SL-10969 ... need to work out why this blows out in many setups...
        //col.rgb += amb_da * light_col * diffuse;

        // no spec for alpha shader...
    }
    col = max(col, vec3(0));
    return col;
}

<<<<<<< HEAD
vec4 applyWaterFogDeferred(vec3 pos, vec4 color)
{
	//normalize view vector
	vec3 view = normalize(pos);
	float es = -(dot(view, waterPlane.xyz));

	//find intersection point with water plane and eye vector
	
	//get eye depth
	float e0 = max(-waterPlane.w, 0.0);
	
	vec3 int_v = waterPlane.w > 0.0 ? view * waterPlane.w/es : vec3(0.0, 0.0, 0.0);
	
	//get object depth
	float depth = length(pos - int_v);
		
	//get "thickness" of water
	float l = max(depth, 0.1);

	float kd = waterFogDensity;
	float ks = waterFogKS;
	vec4 kc = waterFogColor;
	
	float F = 0.98;
	
	float t1 = -kd * pow(F, ks * e0);
	float t2 = kd + ks * es;
	float t3 = pow(F, t2*l) - 1.0;
	
	float L = min(t1/t2*t3, 1.0);
	
	float D = pow(0.98, l*kd);
	
	color.rgb = color.rgb * D + kc.rgb * L;
	color.a = kc.a + color.a;
	
	return color;
}

vec3 getSunlitColor()
{
	return vary_SunlitColor;
}

vec3 getAmblitColor()
{
	return vary_AmblitColor;
}

vec3 getAdditiveColor()
{
	return vary_AdditiveColor;
}

vec3 getAtmosAttenuation()
{
	return vary_AtmosAttenuation;
}

void setPositionEye(vec3 v)
{
	vary_PositionEye = v;
}

void setSunlitColor(vec3 v)
{
	vary_SunlitColor = v;
}

void setAmblitColor(vec3 v)
{
	vary_AmblitColor = v;
}

void setAdditiveColor(vec3 v)
{
	vary_AdditiveColor = v;
}

void setAtmosAttenuation(vec3 v)
{
	vary_AtmosAttenuation = v;
}

void calcAtmospherics(vec3 inPositionEye, float ambFactor) {

	vec3 P = inPositionEye;
	setPositionEye(P);
	
	vec3 tmpLightnorm = lightnorm.xyz;

	vec3 Pn = normalize(P);
	float Plen = length(P);

	vec4 temp1 = vec4(0);
	vec3 temp2 = vec3(0);
	vec4 blue_weight;
	vec4 haze_weight;
	vec4 sunlight = sunlight_color;
	vec4 light_atten;

	//sunlight attenuation effect (hue and brightness) due to atmosphere
	//this is used later for sunlight modulation at various altitudes
	light_atten = (blue_density + vec4(haze_density * 0.25)) * (density_multiplier * max_y);
		//I had thought blue_density and haze_density should have equal weighting,
		//but attenuation due to haze_density tends to seem too strong

	temp1 = blue_density + vec4(haze_density);
	blue_weight = blue_density / temp1;
	haze_weight = vec4(haze_density) / temp1;

	//(TERRAIN) compute sunlight from lightnorm only (for short rays like terrain)
	temp2.y = max(0.0, tmpLightnorm.y);
	temp2.y = 1. / temp2.y;
	sunlight *= exp( - light_atten * temp2.y);

	// main atmospheric scattering line integral
	temp2.z = Plen * density_multiplier;

	// Transparency (-> temp1)
	// ATI Bugfix -- can't store temp1*temp2.z*distance_multiplier in a variable because the ati
	// compiler gets confused.
	temp1 = exp(-temp1 * temp2.z * distance_multiplier);

	//final atmosphere attenuation factor
	setAtmosAttenuation(temp1.rgb);
	
	//compute haze glow
	//(can use temp2.x as temp because we haven't used it yet)
	temp2.x = dot(Pn, tmpLightnorm.xyz);
	temp2.x = 1. - temp2.x;
		//temp2.x is 0 at the sun and increases away from sun
	temp2.x = max(temp2.x, .03);	//was glow.y
		//set a minimum "angle" (smaller glow.y allows tighter, brighter hotspot)
	temp2.x *= glow.x;
		//higher glow.x gives dimmer glow (because next step is 1 / "angle")
	temp2.x = pow(temp2.x, glow.z);
		//glow.z should be negative, so we're doing a sort of (1 / "angle") function

	//add "minimum anti-solar illumination"
	temp2.x += .25;
	
	//increase ambient when there are more clouds
	vec4 tmpAmbient = ambient + (vec4(1.) - ambient) * cloud_shadow * 0.5;
	
	//haze color
	setAdditiveColor(
		vec3(blue_horizon * blue_weight * (sunlight*(1.-cloud_shadow) + tmpAmbient)
	  + (haze_horizon * haze_weight) * (sunlight*(1.-cloud_shadow) * temp2.x
		  + tmpAmbient)));
		  
	// decrease ambient value for occluded areas
	tmpAmbient *= mix(ssao_effect, 1.0, ambFactor);

	//brightness of surface both sunlight and ambient
	/*setSunlitColor(pow(vec3(sunlight * .5), vec3(global_gamma)) * global_gamma);
	setAmblitColor(pow(vec3(tmpAmbient * .25), vec3(global_gamma)) * global_gamma);
	setAdditiveColor(pow(getAdditiveColor() * vec3(1.0 - temp1), vec3(global_gamma)) * global_gamma);*/

	setSunlitColor(vec3(sunlight * .5));
	setAmblitColor(vec3(tmpAmbient * .25));
	setAdditiveColor(getAdditiveColor() * vec3(1.0 - temp1));
}
=======
void main() 
{
    vec2 frag = vary_fragcoord.xy/vary_fragcoord.z*0.5+0.5;
    frag *= screen_res;
    
    vec4 pos = vec4(vary_position, 1.0);
    vec3 norm = vary_norm;

    float shadow = 1.0f;

#ifdef HAS_SHADOW
    shadow = sampleDirectionalShadow(pos.xyz, norm.xyz, frag);
#endif

#ifdef USE_DIFFUSE_TEX
    vec4 diffuse_tap = texture2D(diffuseMap,vary_texcoord0.xy);
#endif
>>>>>>> 693791f4ffdf5471b16459ba295a50615bbc7762

#ifdef USE_INDEXED_TEX
    vec4 diffuse_tap = diffuseLookup(vary_texcoord0.xy);
#endif

<<<<<<< HEAD
vec3 atmosTransport(vec3 light) {
	light *= getAtmosAttenuation().r;
	light += getAdditiveColor() * 2.0;
	return light;
}

vec3 atmosGetDiffuseSunlightColor()
{
	return getSunlitColor();
}
=======
    vec4 diffuse_srgb = diffuse_tap;
    vec4 diffuse_linear = vec4(srgb_to_linear(diffuse_srgb.rgb), diffuse_srgb.a);
>>>>>>> 693791f4ffdf5471b16459ba295a50615bbc7762

#ifdef FOR_IMPOSTOR
    vec4 color;
    color.rgb = diffuse_srgb.rgb;
    color.a = 1.0;

    float final_alpha = diffuse_srgb.a * vertex_color.a;
    diffuse_srgb.rgb *= vertex_color.rgb;
    diffuse_linear.rgb = srgb_to_linear(diffuse_srgb.rgb); 
    
    // Insure we don't pollute depth with invis pixels in impostor rendering
    //
    if (final_alpha < 0.01)
    {
        discard;
    }
#else
    
    vec3 light_dir = (sun_up_factor == 1) ? sun_dir: moon_dir;

    float final_alpha = diffuse_linear.a;

#ifdef USE_VERTEX_COLOR
    final_alpha *= vertex_color.a;
    diffuse_srgb.rgb *= vertex_color.rgb;
    diffuse_linear.rgb = srgb_to_linear(diffuse_srgb.rgb);
#endif

    vec3 sunlit;
    vec3 amblit;
    vec3 additive;
    vec3 atten;

    calcAtmosphericVars(pos.xyz, light_dir, 1.0, sunlit, amblit, additive, atten, false);

    vec2 abnormal = encode_normal(norm.xyz);

    float da = dot(norm.xyz, light_dir.xyz);
          da = clamp(da, -1.0, 1.0);
          da = pow(da, 1.0/1.3);
 
    float final_da = da;
          final_da = clamp(final_da, 0.0f, 1.0f);

    vec4 color = vec4(0.0);

    color.a   = final_alpha;

    float ambient = min(abs(dot(norm.xyz, sun_dir.xyz)), 1.0);
    ambient *= 0.5;
    ambient *= ambient;
    ambient = (1.0 - ambient);

<<<<<<< HEAD
float pcfShadow(sampler2DShadow shadowMap, vec4 stc, vec2 pos_screen, float shad_res, float bias)
{
	float recip_shadow_res = 1.0 / shad_res;
	stc.xyz /= stc.w;
	stc.z += bias;
	
	stc.x = floor(stc.x*shad_res + fract(pos_screen.y*0.5)) * recip_shadow_res;
	float cs = shadow2D(shadowMap, stc.xyz).x;
	
	float shadow = cs;
	
	//shadow += shadow2D(shadowMap, stc.xyz+vec3(0.40*recip_shadow_res, 0.35*recip_shadow_res, 0.0)).x;
	//shadow += shadow2D(shadowMap, stc.xyz+vec3(0.72*recip_shadow_res, -0.65*recip_shadow_res, 0.0)).x;
	//shadow += shadow2D(shadowMap, stc.xyz+vec3(-0.60*recip_shadow_res, 0.55*recip_shadow_res, 0.0)).x;
	//shadow += shadow2D(shadowMap, stc.xyz+vec3(-0.92*recip_shadow_res, -0.85*recip_shadow_res, 0.0)).x;
	         
    return shadow;
}

void main() 
{
 vec2 pos_screen = vary_fragcoord.xy;
	vec2 frag = vary_fragcoord.xy/vary_fragcoord.z*0.5+0.5;
	frag *= screen_res;
	
	vec4 pos = vec4(vary_position, 1.0);
	
	float shadow = 1.0;

#if HAS_SHADOW
	vec4 spos = pos;
		
	if (spos.z > -shadow_clip.w)
	{	
		shadow = 0.0;

		vec4 lpos;
		
		vec4 near_split = shadow_clip*-0.75;
		vec4 far_split = shadow_clip*-1.25;
		vec4 transition_domain = near_split-far_split;
		float weight = 0.0;

		if (spos.z < near_split.z)
		{
			lpos = shadow_matrix[3]*spos;
			
			float w = 1.0;
			w -= max(spos.z-far_split.z, 0.0)/transition_domain.z;
			shadow += pcfShadow(shadowMap3, lpos, pos_screen, shadow_res.w, shadow_bias.w)*w;
			weight += w;
			shadow += max((pos.z+shadow_clip.z)/(shadow_clip.z-shadow_clip.w)*2.0-1.0, 0.0);
		}

		if (spos.z < near_split.y && spos.z > far_split.z)
		{
			lpos = shadow_matrix[2]*spos;
			
			float w = 1.0;
			w -= max(spos.z-far_split.y, 0.0)/transition_domain.y;
			w -= max(near_split.z-spos.z, 0.0)/transition_domain.z;
			shadow += pcfShadow(shadowMap2, lpos, pos_screen, shadow_res.z, shadow_bias.z)*w;
			weight += w;
		}

		if (spos.z < near_split.x && spos.z > far_split.y)
		{
			lpos = shadow_matrix[1]*spos;
			
			float w = 1.0;
			w -= max(spos.z-far_split.x, 0.0)/transition_domain.x;
			w -= max(near_split.y-spos.z, 0.0)/transition_domain.y;
			shadow += pcfShadow(shadowMap1, lpos, pos_screen, shadow_res.y, shadow_bias.y)*w;
			weight += w;
		}

		if (spos.z > far_split.x)
		{
			lpos = shadow_matrix[0]*spos;
							
			float w = 1.0;
			w -= max(near_split.x-spos.z, 0.0)/transition_domain.x;
				
			shadow += pcfShadow(shadowMap0, lpos, pos_screen, shadow_res.x, shadow_bias.x)*w;
			weight += w;
		}
		

		shadow /= weight;
	}
	else
	{
		shadow = 1.0;
	}
#endif
=======
    vec3 sun_contrib = min(final_da, shadow) * sunlit;
>>>>>>> 693791f4ffdf5471b16459ba295a50615bbc7762

#if !defined(AMBIENT_KILL)
    color.rgb = amblit;
    color.rgb *= ambient;
#endif

vec3 post_ambient = color.rgb;

#if !defined(SUNLIGHT_KILL)
    color.rgb += sun_contrib;
#endif

vec3 post_sunlight = color.rgb;

    color.rgb *= diffuse_srgb.rgb;

vec3 post_diffuse = color.rgb;

    color.rgb = atmosFragLighting(color.rgb, additive, atten);

vec3 post_atmo = color.rgb;

    vec4 light = vec4(0,0,0,0);
    
    color.rgb = scaleSoftClipFrag(color.rgb);

    //convert to linear before applying local lights
    color.rgb = srgb_to_linear(color.rgb);

   #define LIGHT_LOOP(i) light.rgb += calcPointLightOrSpotLight(light_diffuse[i].rgb, diffuse_linear.rgb, pos.xyz, norm, light_position[i], light_direction[i].xyz, light_attenuation[i].x, light_attenuation[i].y, light_attenuation[i].z, light_attenuation[i].w);

    LIGHT_LOOP(1)
    LIGHT_LOOP(2)
    LIGHT_LOOP(3)
    LIGHT_LOOP(4)
    LIGHT_LOOP(5)
    LIGHT_LOOP(6)
    LIGHT_LOOP(7)

<<<<<<< HEAD
	vec4 light = vec4(0,0,0,0);

	color.rgb = srgb_to_linear(color.rgb);
	
   #define LIGHT_LOOP(i) light.rgb += calcPointLightOrSpotLight(light_diffuse[i].rgb, diff.rgb, pos.xyz, norm, light_position[i], light_direction[i].xyz, light_attenuation[i].x, light_attenuation[i].y, light_attenuation[i].z);

	LIGHT_LOOP(1)
	LIGHT_LOOP(2)
	LIGHT_LOOP(3)
	LIGHT_LOOP(4)
	LIGHT_LOOP(5)
	LIGHT_LOOP(6)
	LIGHT_LOOP(7)

	// keep it linear
	//
 light.rgb *= global_light_strength;
	color.rgb += light.rgb;

	// straight to display gamma, we're post-deferred
	//
	color.rgb = linear_to_srgb(color.rgb);
=======
    // sum local light contrib in linear colorspace
#if !defined(LOCAL_LIGHT_KILL)
    color.rgb += light.rgb;
#endif
    // back to sRGB as we're going directly to the final RT post-deferred gamma correction
    color.rgb = linear_to_srgb(color.rgb);

//color.rgb = amblit;
//color.rgb = vec3(ambient);
//color.rgb = sunlit;
//color.rgb = vec3(final_da);
//color.rgb = post_ambient;
//color.rgb = post_sunlight;
//color.rgb = sun_contrib;
//color.rgb = diffuse_srgb.rgb;
//color.rgb = post_diffuse;
//color.rgb = post_atmo;
>>>>>>> 693791f4ffdf5471b16459ba295a50615bbc7762

#ifdef WATER_FOG
    color = applyWaterFogView(pos.xyz, color);
#endif // WATER_FOG

#endif
    
    frag_color = color;
}

