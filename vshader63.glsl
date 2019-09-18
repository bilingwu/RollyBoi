/*
File Name: "vshader53.glsl":
Vertex shader:
  - Per vertex shading for a single point light source;
    distance attenuation is Yet To Be Completed.
  - Entire shading computation is done in the Eye Frame.
*/
#version 130

in  vec4 vPosition;
in  vec3 vNormal;
in  vec3 vColor;
in  vec2 vTexCoord;

out vec4 color;
out float fog_distance;
out float fog_f;
out vec2 texCoord;
out float sphere_tex_coord;
out float stripe_f;


uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
uniform mat4 Projection;
uniform mat3 Normal_Matrix;
uniform vec4 LightPosition;   // Must be in Eye Frame
uniform float Shininess;

uniform float ConstAtt;  // Constant Attenuation
uniform float LinearAtt; // Linear Attenuation
uniform float QuadAtt;   // Quadratic Attenuation

uniform float floor_flag;
uniform float lighting_flag;
uniform float sphere_flag;
uniform float spot_flag;
uniform float point_flag;
uniform float spot_exp;
uniform float spot_ang;
uniform float fog_flag;
uniform float blend_flag;
uniform float eye_flag;

uniform vec4 spot_direction;
uniform vec4 input_material_ambient;
uniform	vec4 input_material_diffuse;
uniform	vec4 input_material_specular;
uniform vec4 input_light_ambient;
uniform	vec4 input_light_diffuse;
uniform	vec4 input_light_specular;


uniform	float sphere_stripe_flag;
uniform	float vertical_stripe_flag;
uniform	float sphere_checker_flag;


void main()
{

  vec4 vPosition4 = vec4(vPosition.x, vPosition.y, vPosition.z, 1.0);
  vec4 vColor4 = vec4(vColor.r, vColor.g, vColor.b, 1.0);



  color = vColor4;

  vec3 pos = (ModelView * vPosition).xyz;
	vec3 tmp_eye_pos=(ModelView*vec4(7.0, 3.0, -10.0,1.0)).xyz;
  fog_f=fog_flag;
	fog_distance=length(pos-tmp_eye_pos);

	vec3 N = normalize(Normal_Matrix * vNormal);
  vec4 global_light=vec4(1.0,1.0,1.0,1);
	vec3 l_distant=vec3(0.1,0.0,-1.0);

	vec4 ini_light_ambient=vec4( 0.0, 0.0, 0.0, 1.0 );
	vec4 ini_light_diffuse=vec4( 0.8, 0.8, 0.8, 1.0 );
	vec4 ini_light_specular=vec4( 0.2, 0.2, 0.2, 1.0 );

	float Shininess=125.0;

	vec4 ini_ambient_product = ini_light_ambient * input_material_ambient;
	vec4 ini_diffuse_product = ini_light_diffuse * input_material_diffuse;
  vec4 ini_specular_product = ini_light_specular * input_material_specular;

	vec3 L_ini = normalize(-l_distant);
	vec3 E_ini = normalize(-pos);
	vec3 H_ini = normalize ( L_ini + E_ini);

	vec4 ini_ambient = ini_ambient_product;

	float ini_d = max( dot(L_ini, N), 0.0 );
	vec4  ini_diffuse = ini_d * ini_diffuse_product;

	float ini_s = pow( max(dot(N, H_ini), 0.0), Shininess );
	vec4  ini_specular = ini_s * ini_specular_product;
	if( dot(L_ini, N) < 0.0 ) {
	ini_specular = vec4(0.0, 0.0, 0.0, 1.0);
	}

	vec4 ini_color =1.0 * (ini_ambient + ini_diffuse + ini_specular);


// Transform vertex  position into eye coordinates


    vec3 L = normalize( LightPosition.xyz - pos );
    vec3 E = normalize( -pos );
    vec3 H = normalize( L + E );

//--- To Do: Compute attenuation ---
    float attenuation = 1.0;
    vec3 tmp_cal_d = LightPosition.xyz - pos;
    float cur_distance=length(tmp_cal_d);
    attenuation=1/(ConstAtt + LinearAtt*cur_distance + QuadAtt*cur_distance*cur_distance);

 // Compute terms in the illumination equation
    vec4 ambient = input_light_ambient*input_material_ambient;
    float d = max( dot(L, N), 0.0 );
    vec4  diffuse = d * input_light_diffuse*input_material_diffuse;

    float s = pow( max(dot(N, H), 0.0), Shininess );
    vec4  specular = s * input_light_specular*input_material_specular;

    if( dot(L, N) < 0.0 ) {
	     specular = vec4(0.0, 0.0, 0.0, 1.0);
    }
    gl_Position = Projection * ModelView * vPosition;

    //--- attenuation below must be computed properly --
   vec4 color_add_point = attenuation * (ambient + diffuse + specular);

   vec3 spot_l = -L;
   vec3 spot_l_f= normalize( spot_direction.xyz-LightPosition.xyz );

   float spot_att=pow(dot(spot_l,spot_l_f),spot_exp);

   vec4 color_add_spot=spot_att*color_add_point;

   if ( dot(spot_l,spot_l_f) < spot_ang){
    color_add_spot=vec4(0,0,0,1);
   }

   if (lighting_flag == 1.0){
     if (spot_flag==1.0){
      color=ini_color+global_light*input_material_ambient+color_add_spot;
     } else {
      color=ini_color+global_light*input_material_ambient+color_add_point;
     }
     if (blend_flag==1.0){
      color[3] = 0.50;
     } else{
      color[3] = 1.0;
     }
   }

  stripe_f = 0.0;
  texCoord = vTexCoord;

  float tmp_a=vPosition[0];
  float tmp_b=vPosition[1];
  float tmp_c=vPosition[2];

  sphere_tex_coord=pos[0]*2.5;
  
  if (sphere_flag == 1.0){
  if (sphere_stripe_flag==1.0){
    stripe_f = 1.0;
     if (vertical_stripe_flag == 1.0){
       sphere_tex_coord=pos[0]*2.5;
     }
     else{
        //if (eye_flag == 0.0){
          //sphere_tex_coord=1.5*(pos[0]+pos[1]+pos[2]);
        //}else {
          sphere_tex_coord=1.5*(tmp_a+tmp_b+tmp_c);
        //}
    }
  } else if (sphere_checker_flag == 1.0){
    //if(eye_flag==1.0){
      if(vertical_stripe_flag==1.0)
        {texCoord[0]=(pos[0]+1)*0.5;texCoord[1]=(pos[1]+1)*0.5;}
      else
        {texCoord[0]=(pos[0]+pos[1]+pos[2])*0.3;texCoord[1]=(pos[0]-pos[1]+pos[2])*0.3;}
    //  }
  }
  }



}
