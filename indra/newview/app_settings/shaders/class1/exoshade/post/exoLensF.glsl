/***********************************
 * exoPostSpecialF.glsl
 * Provides special post effects.
 * Copyright NiranV Dean, 2014
 ***********************************/
#extension GL_ARB_texture_rectangle : enable

/*[EXTRA_CODE_HERE]*/

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

vec3 lensflare(vec2 uv,vec2 pos)
{
	vec2 main = uv-pos;
	vec2 uvd = uv*(length(uv));
	
	float f1 = max(0.01-pow(length(uv+1.2*pos),1.9),.0)*7.0;

	float f2 = max(1.0/(1.0+32.0*pow(length(uvd+0.8*pos),2.0)),.0)*00.25;
	float f22 = max(1.0/(1.0+32.0*pow(length(uvd+0.85*pos),2.0)),.0)*00.23;
	float f23 = max(1.0/(1.0+32.0*pow(length(uvd+0.9*pos),2.0)),.0)*00.21;
	
	vec2 uvx = mix(uv,uvd,-0.5);
	
	float f4 = max(0.01-pow(length(uvx+0.4*pos),2.4),.0)*6.0;
	float f42 = max(0.01-pow(length(uvx+0.45*pos),2.4),.0)*5.0;
	float f43 = max(0.01-pow(length(uvx+0.5*pos),2.4),.0)*3.0;
	
	uvx = mix(uv,uvd,-.4);
	
	float f5 = max(0.01-pow(length(uvx+0.2*pos),5.5),.0)*2.0;
	float f52 = max(0.01-pow(length(uvx+0.4*pos),5.5),.0)*2.0;
	float f53 = max(0.01-pow(length(uvx+0.6*pos),5.5),.0)*2.0;
	
	uvx = mix(uv,uvd,-0.5);
	
	float f6 = max(0.01-pow(length(uvx-0.3*pos),1.6),.0)*6.0;
	float f62 = max(0.01-pow(length(uvx-0.325*pos),1.6),.0)*3.0;
	float f63 = max(0.01-pow(length(uvx-0.35*pos),1.6),.0)*5.0;
	
	vec3 c = vec3(.0);
	
	c.r+=f2+f4+f5+f6; 
    c.g+=f22+f42+f52+f62; 
    c.b+=f23+f43+f53+f63;
	c = c*1.3 - vec3(length(uvd)*.05);
	
	return c;
}

vec3 cc(vec3 color, float factor,float factor2) // color modifier
{
	float w = color.x+color.y+color.z;
	return mix(color,vec3(w)*factor,w*factor2);
}

void main ()
{
	vec4 col = texture2DRect(exo_screen, vary_fragcoord.xy);
	
	vec2 uv = vary_fragcoord.xy / screen_res.xy - 0.5;
	uv.x *= screen_res.x/screen_res.y; //fix aspect ratio
	
	vec3 color = vec3(1.4,1.2,1.0)*lensflare(uv,sun_dir.xy);
	color = cc(color,.5,.1);
	
	float fade = 0.0;
	if(sun_dir.x > -0.75 && sun_dir.x < 0.75
    && sun_dir.y > -0.55 && sun_dir.y < 0.55
    && sun_dir.z < 0.5)
    {
	  if(sun_dir.z < 0.0)
	  {
		  fade = clamp(0.6 - dot(sun_dir.xy * 1, sun_dir.xy * 1), 0, 1);
	  }
	  color *= fade;
	  col.rgb += color;
    }
	
	frag_color = col;
  
}