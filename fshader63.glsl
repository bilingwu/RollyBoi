/*
File Name: "fshader53.glsl":
           Fragment Shader
*/

#version 130  // YJC: Comment/un-comment this line to resolve compilation errors
                 //      due to different settings of the default GLSL version

in vec4 color;
in float fog_distance;
in float fog_f;
in vec2 texCoord;
in float sphere_tex_coord;
in float stripe_f;

out vec4 fColor;

vec4 fog_color= vec4(0.7, 0.7, 0.7, 0.5);

uniform float floor_tex_flag;
uniform float f_sphere_checker_flag;
uniform float sphere_f;
uniform sampler2D texture_2D;

uniform sampler1D texture_1D;



void main()
{
  vec4 tex_color=color; // color for now

  if (floor_tex_flag== 1.0 && sphere_f == 0.0) {
   tex_color = color * texture(texture_2D,texCoord);
  }
  if (floor_tex_flag== 0.0 && sphere_f == 1.0 && sphere_f>0.0){
    tex_color = color * texture(texture_1D, sphere_tex_coord);
  }
  if(f_sphere_checker_flag==1.0  && floor_tex_flag==0.0){
    vec4 tmp_color=texture(texture_2D,texCoord);
    if(tmp_color[0]==0)
    {tmp_color=vec4(0.9,0.1,0.1,1.0);}
    tex_color=color*tmp_color;
  }

  if (fog_f==1.0){
    float x = clamp(fog_distance, 0.0, 18.0);
    fColor = mix(tex_color, fog_color, x/18);
  }

  else if(fog_f==2.0){
    float x=1/exp(0.09*fog_distance);
    fColor=mix(tex_color,fog_color,1-x);
  }
  else if(fog_f==3.0){
    float x=1/exp(0.09*0.09*fog_distance*fog_distance);
    fColor=mix(tex_color,fog_color,1-x);
  } else{
    fColor = tex_color;
  }

}
