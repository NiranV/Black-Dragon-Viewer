/***********************************
 * exoPostSpecialF.glsl
 * Provides special post effects.
 * Copyright NiranV Dean, 2014
 ***********************************/
#extension GL_ARB_texture_rectangle : enable

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

uniform sampler2DRect exo_screen;
uniform vec2 screen_res;
VARYING vec2 vary_fragcoord;
uniform vec3 sun_dir;
uniform vec4 sunlight_color_copy;

vec3 flare(vec2 spos, vec2 fpos, vec3 clr)
{
	vec3 color;
	float fade;
    fade = 0.01 / clamp(abs(sun_dir.x / sun_dir.y), 0.01, 0.01);
    if(abs(sun_dir.x) > 0.25 || abs(sun_dir.y) > 0.05)
     fade -= clamp(abs((sun_dir.x * sun_dir.x * 2) + (sun_dir.y * sun_dir.y * 4)), 0.0 , 1.0);
	color = clr * max(0.0, 0.5 - distance(spos, fpos * (-sun_dir.z * 1.25))) * (4 * sunlight_color_copy.a) * fade;
	color += clr * max(0.0, 0.1 / distance(spos, -fpos * (-sun_dir.z * 0.7))) * (1.5 * sunlight_color_copy.a) * fade ;
	color += clr * max(0.0, 0.25 - distance(spos, -fpos * (-sun_dir.z *1.2))) * (24 * sunlight_color_copy.a) * fade;
	color += clr * max(0.0, 0.15 - distance(spos, -fpos * (-sun_dir.z *3.0))) * (22 * sunlight_color_copy.a) * fade;
	
	
	return color;
}

float noise(vec2 pos)
{
	return fract(1111. * sin(111. * dot(pos, vec2(2222., 22.))));	
}

void main ()
{
	vec4 col = texture2DRect(exo_screen, vary_fragcoord.xy);
	
	if(sun_dir.x > -0.75 && sun_dir.x < 0.75
    && sun_dir.y > -0.55 && sun_dir.y < 0.55
    && sun_dir.z < 0.5)
    {
      vec2 position = ( gl_FragCoord.xy / screen_res.xy * 2.0 ) - 1.0;
      position.x *= screen_res.x / screen_res.y;
      vec3 color = flare(position, vec2(sun_dir.xy) * 1.15 , vec3(sunlight_color_copy.rgb));
      col += vec4( color * 0.05, 0.0 );
    }
	
	frag_color = col;
  
}