#include "Angel-yjc.h"
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <math.h>

using namespace std;

typedef Angel::vec3  color3;
typedef Angel::vec3  point3;
typedef Angel::vec4  color4;
typedef Angel::vec4  point4;

struct triangle{
  point3 first;
  point3 second;
  point3 third;
};

vector<triangle> sphere_triangles;
int triangle_count;

GLuint Angel::InitShader(const char* vShaderFile, const char* fShaderFile);

GLuint program, program2;      /* shader program object id */
GLuint model_view, model_view2;  // model-view matrix uniform shader variable location
GLuint projection, projection2;  // projection matrix uniform shader variable location

GLuint floor_buffer;  /* vertex buffer object id for floor */
GLuint axis_buffer;
GLuint sphere_buffer;
GLuint sphere_shadow_buffer;
GLuint texName;
GLuint sphere_stripe_texName;
GLuint firework_buffer;

// Projection transformation parameters
GLfloat  fovy = 45.0;  // Field-of-view in Y direction angle (in degrees)
GLfloat  aspect;       // Viewport aspect ratio
GLfloat  zNear = 0.5, zFar = 30.0;

GLfloat angle = 1.0; // rotation angle in degrees

const vec4 vrp(7.0, 3.0, -10.0, 1.0);
const vec4 vpn(-7.0, -3.0 , 10.0, 0.0);
const vec4 vup(0.0, 1.0, 0.0, 0.0);
const vec4 light_position(-14.0, 12.0, -3.0, 1.0);

vec4 eye = vrp;
vec4 at = vrp + vpn; // just (0,0,0,1)
vec4 up = vup;

const int floor_NumVertices = 6; //(1 face)*(2 triangles/face)*(3 vertices/triangle)
point3 floor_points[floor_NumVertices]; // positions for all vertices
color3 floor_colors[floor_NumVertices]; // colors for all vertices

const int axis_NumVertices = 6;
point3 axis_points[axis_NumVertices];
color3 axis_colors[axis_NumVertices];

int sphere_NumVertices;
point3 * sphere_points;
color3 * sphere_colors;
point3 * sphere_shadow_points;
color3 * sphere_shadow_colors;

const color3 green(0.0, 1.0, 0.0); // floor
const color3 red(1.0, 0.0, 0.0); // x axis
const color3 magenta(1.0, 0.0, 1.0); // y axis
const color3 blue(0.0, 0.0, 1.0); // z axis
const color3 golden_yellow(1.0, 0.84, 0.0); // sphere
const color3 dark_gray(0.25, 0.25, 0.25); // shadow

const point3 pointA(-4, 1, 4);
const point3 pointB(-1, 1, -4);
const point3 pointC(3, 1, 5);

const vec3 roll_dirs[3] = {
  normalize(pointB - pointA),
  normalize(pointC - pointB),
  normalize(pointA - pointC)
};

const vec3 y_axis(0,1,0);
const vec3 rot_axes[3] = {
  cross(y_axis, pointB - pointA),
  cross(y_axis, pointC - pointB),
  cross(y_axis, pointA - pointC) // cant use roll_dir[2] bc its too close to 0
};

int curr_dir = 0;

// init
GLfloat trans_x = pointA.x;
const GLfloat trans_y = 1;
GLfloat trans_z = pointA.z;
GLfloat radius = 1; // i think


mat4 acc_matrix = Angel::identity();
mat4 p_to_q(12, 0, 0, 0, 14, 0, 3, -1, 0, 0, 12, 0, 0, 0, 0, 12);

// Some flags - init with wireframe from homework 2
bool rolling_flag = false;
bool began_flag = false;
bool wireframe_flag = false;
bool shadow_flag = true;
bool lighting_flag = true;
bool smooth_shading_flag = true;
bool flat_shading_flag = false;
bool spotlight_flag = true;
bool point_source_flag = false;
bool fog_flag = true;
float fog_type = 1.0; // start with linear
bool blend_flag = true;
bool floor_tex_flag = true;
bool sphere_stripe_flag = true;
bool vertical_stripe_flag = false;
bool eye_stripe_flag = false;
bool fireworks_flag = true;
bool sphere_checker_flag = false;
float t_sub = 0.0;


// for global lighting
const color4 global_ambient(1.0, 1.0, 1.0, 1.0); // global ambient
const color4 light_ambient(0.0, 0.0, 0.0, 1.0); // directional (distant) light source, ground specular color
const color4 light_diffuse(0.8, 0.8, 0.8, 1.0); // diffuse color
const color4 light_specular(0.2,0.2,0.2, 0.0); // specular color, ground diffuse color
const vec4 light_direction(1.0, 0.0, -1.0, 0.0); // in eye frame, already teansformed

GLuint floor_light_buffer;
vec3 floor_light_normals[floor_NumVertices];
point4 floor_light_points[floor_NumVertices];

GLuint sphere_flat_buffer;
point4 * sphere_flat_points;
vec3 * sphere_flat_normals;

GLuint sphere_smooth_buffer;
point4 * sphere_smooth_points;
vec3 * sphere_smooth_normals;

// -----------------------------------------------------------------------------

#define ImageWidth  32
#define ImageHeight 32
GLubyte Image[ImageHeight][ImageWidth][4];

#define	stripeImageWidth 32
GLubyte stripeImage[4*stripeImageWidth];

GLuint floor_tex_buffer;
GLuint sphere_stripe_buffer;

point4 floor_tex_vertices[6] = {
    point4(-5, 0, -4, 1), point4(-5, 0, 8, 1), point4(5, 0, 8, 1),
    point4(5, 0, 8, 1),   point4(5, 0, -4, 1), point4(-5, 0, -4, 1),
};
point3 floor_tex_normals[6] = {
    point3(0, 1, 0), point3(0, 1, 0), point3(0, 1, 0),
    point3(0, 1, 0), point3(0, 1, 0), point3(0, 1, 0),
};
vec2 floor_tex_coord[6] = {
    vec2(0.0, 0.0),         vec2(0.0, 6 ), vec2(5 , 6),
    vec2(5 , 6 ), vec2(5 , 0.0), vec2(0.0, 0.0),
};

void image_set_up(void)
{
 int i, j, c;

 /* --- Generate checkerboard image to the image array ---*/
  for (i = 0; i < ImageHeight; i++)
    for (j = 0; j < ImageWidth; j++)
      {
       c = (((i & 0x8) == 0) ^ ((j & 0x8) == 0));

       if (c == 1) /* white */
	{
         c = 255;
	 Image[i][j][0] = (GLubyte) c;
         Image[i][j][1] = (GLubyte) c;
         Image[i][j][2] = (GLubyte) c;
	}
       else  /* green */
	{
         Image[i][j][0] = (GLubyte) 0;
         Image[i][j][1] = (GLubyte) 150;
         Image[i][j][2] = (GLubyte) 0;
	}

       Image[i][j][3] = (GLubyte) 255;
      }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

/*--- Generate 1D stripe image to array stripeImage[] ---*/
  for (j = 0; j < stripeImageWidth; j++) {
     /* When j <= 4, the color is (255, 0, 0),   i.e., red stripe/line.
        When j > 4,  the color is (255, 255, 0), i.e., yellow remaining texture
      */
    stripeImage[4*j] = (GLubyte)    255;
    stripeImage[4*j+1] = (GLubyte) ((j>4) ? 255 : 0);
    stripeImage[4*j+2] = (GLubyte) 0;
    stripeImage[4*j+3] = (GLubyte) 255;
  }

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
/*----------- End 1D stripe image ----------------*/

/*--- texture mapping set-up is to be done in
      init() (set up texture objects),
      display() (activate the texture object to be used, etc.)
      and in shaders.
 ---*/

}
// -----------------------------------------------------------------------------

void floor(){
  cout << "This is the floor func" << endl;

  floor_colors[0] = green; floor_points[0] = point3(5.0, 0.0, 8.0);
  floor_colors[1] = green; floor_points[1] = point3(5.0, 0.0, -4.0);
  floor_colors[2] = green; floor_points[2] = point3(-5.0, 0.0, 8.0);

  floor_colors[3] = green; floor_points[3] = point3(5.0, 0.0, -4.0);
  floor_colors[4] = green; floor_points[4] = point3(-5.0, 0.0, 8.0);
  floor_colors[5] = green; floor_points[5] = point3(-5.0, 0.0, -4.0);
}

point4 const_floor_vertices[4] = {
    point4(5, 0, 8, 1.0),
    point4(5, 0, -4, 1.0),
    point4(-5, 0, -4, 1.0),
    point4(-5, 0, 8, 1.0),
};

int floor_index = 0;
void floor_light(int a, int b, int c, int d) {
  vec4 u = const_floor_vertices[b] - const_floor_vertices[a];
  vec4 v = const_floor_vertices[d] - const_floor_vertices[a];

  vec3 tmp_normal = normalize(cross(v, u));
  floor_light_normals[floor_index] = tmp_normal; floor_light_points[floor_index] = const_floor_vertices[a];
  floor_index++;
  floor_light_normals[floor_index] = tmp_normal; floor_light_points[floor_index] = const_floor_vertices[b];
  floor_index++;
  floor_light_normals[floor_index] = tmp_normal; floor_light_points[floor_index] = const_floor_vertices[c];
  floor_index++;
  floor_light_normals[floor_index] = tmp_normal; floor_light_points[floor_index] = const_floor_vertices[a];
  floor_index++;
  floor_light_normals[floor_index] = tmp_normal; floor_light_points[floor_index] = const_floor_vertices[c];
  floor_index++;
  floor_light_normals[floor_index] = tmp_normal; floor_light_points[floor_index] = const_floor_vertices[d];
  floor_index++;
}
// ----------------------------------------------------------------------------

void axis(){
  cout << "This is the axis func" << endl;
  axis_colors[0] = red; axis_points[0] = point3(0.0, 0.0, 0.0);
  axis_colors[1] = red; axis_points[1] = point3(10.0, 0.0, 0.0);

  axis_colors[2] = magenta; axis_points[2] = point3(0.0, 0.0, 0.0);
  axis_colors[3] = magenta; axis_points[3] = point3(0.0, 10.0, 0.0);

  axis_colors[4] = blue; axis_points[4] = point3(0.0, 0.0, 0.0);
  axis_colors[5] = blue; axis_points[5] = point3(0.0, 0.0, 10.0);
}

// ----------------------------------------------------------------------------

void sphere(){
  cout << "this is the sphere func" << endl;
  int j = 0;
  for(int i = 0; i < triangle_count; i++){
    sphere_colors[j] = golden_yellow; sphere_points[j] = sphere_triangles[i].first;
    sphere_colors[j+1] = golden_yellow; sphere_points[j+1] = sphere_triangles[i].second;
    sphere_colors[j+2] = golden_yellow; sphere_points[j+2] = sphere_triangles[i].third;
    j = j+3;
  }
}

void sphere_shadow(){
  cout << "this is the sphere shadow calc" << endl;
  int j = 0;
  for(int i = 0; i < triangle_count; i++){
    sphere_shadow_colors[j] = dark_gray; sphere_shadow_points[j] = sphere_triangles[i].first;
    sphere_shadow_colors[j+1] = dark_gray; sphere_shadow_points[j+1] = sphere_triangles[i].second;
    sphere_shadow_colors[j+2] = dark_gray; sphere_shadow_points[j+2] = sphere_triangles[i].third;
    j = j+3;
  }
}

point4 conv(point3 p){
  return point4(p.x, p.y, p.z, 1.0);
}
void sphere_flat(){
  cout << "this is the sphere flat calc" << endl;
  int j = 0;
  for(int i = 0; i < triangle_count; i++){
    triangle t = sphere_triangles[i];
    vec4 u = conv(t.second) - conv(t.first);
    vec4 v = conv(t.third) - conv(t.first);

    vec3 norm = normalize(cross(u, v));

    sphere_flat_normals[j] = norm; sphere_flat_points[j] = conv(t.first);
    sphere_flat_normals[j+1] = norm; sphere_flat_points[j+1] = conv(t.second);
    sphere_flat_normals[j+2] = norm; sphere_flat_points[j+2] = conv(t.third);
    j = j+3;
  }
}

void sphere_smooth(){
  cout << "this is the sphere smooth calc" << endl;
  int j = 0;
  point4 origin(0.0,0.0,0.0,1.0);

  for(int i = 0; i < triangle_count; i++){
    triangle t = sphere_triangles[i];
    sphere_smooth_normals[j] = normalize(t.first); sphere_smooth_points[j] = conv(t.first);
    sphere_smooth_normals[j+1] = normalize(t.second); sphere_smooth_points[j+1] = conv(t.second);
    sphere_smooth_normals[j+2] = normalize(t.third); sphere_smooth_points[j+2] = conv(t.third);
    j = j+3;
  }
}
// ----------------------------------------------------------------------------
point3 firework_vertices[300];
point3 firework_color[300];

void fireworks() {
  for (int i = 0; i < 300; i++) {
    vec3 tmp_v =
        vec3(2.0 * ((rand() % 256) / 256.0 - 0.5), 2.4 * (rand() % 256) / 256.0,
             2.0 * ((rand() % 256) / 256.0 - 0.5));

    firework_vertices[i] = tmp_v;
    vec3 tmp_c = vec3((rand() % 256) / 256.0, (rand() % 256) / 256.0,
                      (rand() % 256) / 256.0);
    firework_color[i] = tmp_c;
  }

  glGenBuffers(1, &firework_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, firework_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(firework_vertices) + sizeof(firework_color), NULL,
               GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(firework_vertices), firework_vertices);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(firework_vertices), sizeof(firework_color),
                  firework_color);
}
// ----------------------------------------------------------------------------

void init(){

  image_set_up();
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  /*--- Create and Initialize a texture object ---*/
  glGenTextures(1, &texName);      // Generate texture obj name(s)

  glActiveTexture( GL_TEXTURE0 );  // Set the active texture unit to be 0
  glBindTexture(GL_TEXTURE_2D, texName); // Bind the texture to this texture unit

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ImageWidth, ImageHeight,
               0, GL_RGBA, GL_UNSIGNED_BYTE, Image);

  glGenBuffers(1, &floor_tex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, floor_tex_buffer);
  glBufferData(GL_ARRAY_BUFFER,
              sizeof(floor_tex_vertices) + sizeof(floor_tex_normals) +
                  sizeof(floor_tex_coord),
              NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(floor_tex_vertices), floor_tex_vertices);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(floor_tex_vertices),
                 sizeof(floor_tex_normals), floor_tex_normals);
  glBufferSubData(GL_ARRAY_BUFFER,
                 sizeof(floor_tex_vertices) + sizeof(floor_tex_normals),
                 sizeof(floor_tex_coord), floor_tex_coord);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


  /*--- Create and Initialize a texture object  or sphere stripe---*/
  glGenTextures(1, &sphere_stripe_texName); // Generate texture obj name(s)

  glActiveTexture(GL_TEXTURE1); // Set the active texture unit to be 0
  glBindTexture(GL_TEXTURE_1D,
                sphere_stripe_texName); // Bind the texture to this texture unit

  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               stripeImage);


  /* -- end init for sphere stripe --*/
  floor();
  // Create and initialize a vertex buffer object for floor, to be used in display()
  glGenBuffers(1, &floor_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, floor_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(floor_points) + sizeof(floor_colors),
    NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(floor_points), floor_points);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(floor_points), sizeof(floor_colors),
    floor_colors);

  axis();
  glGenBuffers(1, &axis_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, axis_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(axis_points) + sizeof(axis_colors),
    NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(axis_points), axis_points);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(axis_points), sizeof(axis_colors),
    axis_colors);

  sphere();
  glGenBuffers(1, &sphere_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, sphere_buffer);
  glBufferData(GL_ARRAY_BUFFER,
  		sizeof(point3)*sphere_NumVertices + sizeof(color3)*sphere_NumVertices,
  		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(point3) * sphere_NumVertices, sphere_points);
	glBufferSubData(GL_ARRAY_BUFFER,
		sizeof(point3) * sphere_NumVertices,
		sizeof(color3) * sphere_NumVertices,
		sphere_colors);

  sphere_shadow();
  glGenBuffers(1, &sphere_shadow_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, sphere_shadow_buffer);
  glBufferData(GL_ARRAY_BUFFER, sphere_NumVertices * sizeof(point3) + sphere_NumVertices * sizeof(color3),
               NULL, GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sphere_NumVertices * sizeof(point3), sphere_shadow_points);
  glBufferSubData(GL_ARRAY_BUFFER, sphere_NumVertices * sizeof(point3), sphere_NumVertices * sizeof(color3),
                  sphere_shadow_colors);

  floor_index = 0;
  floor_light(1, 0, 3, 2);

  glGenBuffers(1, &floor_light_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, floor_light_buffer);
  glBufferData(GL_ARRAY_BUFFER,
               sizeof(floor_light_points) + sizeof(floor_light_normals), NULL,
               GL_STATIC_DRAW);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(floor_light_points),
                  floor_light_points);
  glBufferSubData(GL_ARRAY_BUFFER, sizeof(floor_light_points),
                  sizeof(floor_light_normals), floor_light_normals);

  sphere_flat();
  glGenBuffers(1, &sphere_flat_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_flat_buffer);
	glBufferData(GL_ARRAY_BUFFER, sphere_NumVertices * sizeof(point4) + sphere_NumVertices * sizeof(vec3),
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sphere_NumVertices * sizeof(point4), sphere_flat_points);
	glBufferSubData(GL_ARRAY_BUFFER, sphere_NumVertices * sizeof(point4),
		sphere_NumVertices * sizeof(vec3), sphere_flat_normals);

  sphere_smooth();
  glGenBuffers(1, &sphere_smooth_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, sphere_smooth_buffer);
	glBufferData(GL_ARRAY_BUFFER, sphere_NumVertices * sizeof(point4) + sphere_NumVertices * sizeof(vec3),
		NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sphere_NumVertices * sizeof(point4), sphere_flat_points);
	glBufferSubData(GL_ARRAY_BUFFER, sphere_NumVertices * sizeof(point4),
		sphere_NumVertices * sizeof(vec3), sphere_smooth_normals);

  fireworks();
  // program = InitShader("vshader42.glsl", "fshader42.glsl");
  program2 = InitShader("firework_vshader.glsl", "firework_fshader.glsl");

  //with new shader
  program = InitShader("vshader63.glsl", "fshader63.glsl");
  glEnable(GL_DEPTH_TEST);
  glClearColor(0.529, 0.807, 0.92, 0.0);
}
// -----------------------------------------------------------------------------
void set_spot_light(mat4 mv) {
     float const_att = 2.0;
     float linear_att = 0.01;
     float quad_att = 0.001;

     float spot_exp = 15.0;
     float spot_ang = cos(20.0 * M_PI / 180);

	 vec4 light_position_eyeFrame = mv * light_position;
     glUniform4fv( glGetUniformLocation(program, "LightPosition"),
                   1, light_position_eyeFrame);

     glUniform1f(glGetUniformLocation(program, "ConstAtt"),
                 const_att);
     glUniform1f(glGetUniformLocation(program, "LinearAtt"),
                 linear_att);
     glUniform1f(glGetUniformLocation(program, "QuadAtt"),
                 quad_att);
     glUniform4fv(glGetUniformLocation(program, "input_light_ambient"), 1,
                  light_ambient);
     glUniform4fv(glGetUniformLocation(program, "input_light_diffuse"), 1,
                  light_diffuse);
     glUniform4fv(glGetUniformLocation(program, "input_light_specular"), 1,
                  light_specular);
     glUniform1f(glGetUniformLocation(program, "lighting_flag"), 1.0);
     glUniform1f(glGetUniformLocation(program, "point_flag"), 0.0);
     glUniform1f(glGetUniformLocation(program, "spot_flag"), 1.0);
     point4 spot_direction = mv * vec4(-6.0, 0.0, -4.5, 1.0);
     glUniform4fv(glGetUniformLocation(program, "spot_direction"), 1,
                  spot_direction);
     glUniform1f(glGetUniformLocation(program, "spot_exp"), spot_exp);
     glUniform1f(glGetUniformLocation(program, "spot_ang"), spot_ang);

}

void set_point_light(mat4 mv){
  float const_att = 2.0;
  float linear_att = 0.01;
  float quad_att = 0.001;
  point4 light_position(-14, 12.0, -3, 1.0 );

  vec4 light_position_eyeFrame = mv * light_position;
  glUniform4fv( glGetUniformLocation(program, "LightPosition"),
               1, light_position_eyeFrame);

  glUniform1f(glGetUniformLocation(program, "ConstAtt"),
             const_att);
  glUniform1f(glGetUniformLocation(program, "LinearAtt"),
             linear_att);
  glUniform1f(glGetUniformLocation(program, "QuadAtt"),
             quad_att);
  glUniform4fv(glGetUniformLocation(program, "input_light_ambient"), 1,
              light_ambient);
  glUniform4fv(glGetUniformLocation(program, "input_light_diffuse"), 1,
              light_diffuse);
  glUniform4fv(glGetUniformLocation(program, "input_light_specular"), 1,
              light_specular);
  glUniform1f(glGetUniformLocation(program, "lighting_flag"), 1.0);
  glUniform1f(glGetUniformLocation(program, "point_flag"), 1.0);
  glUniform1f(glGetUniformLocation(program, "spot_flag"), 0.0);
}

void set_sphere_vars(){

  color4 material_diffuse(1.0, 0.84, 0.0, 1.0);
  color4 material_specular(1.0, 0.84, 0.0, 1.0);
  color4 material_ambient(0.2, 0.2, 0.2, 1.0);
  float material_shininess = 125.0;

  glUniform4fv(glGetUniformLocation(program, "input_material_ambient"), 1,
             material_ambient);
  glUniform4fv(glGetUniformLocation(program, "input_material_diffuse"), 1,
             material_diffuse);
  glUniform4fv(glGetUniformLocation(program, "input_material_specular"), 1,
             material_specular);
}

void set_floor_vars(){
  const color4 material_ambient(0.2,0.2,0.2, 1.0);
  const color4 material_diffuse(0.0, 1.0, 0.0, 1.0);
  const color4 material_specular(0.2, 0.2, 0.2, 1.0);
   glUniform4fv(glGetUniformLocation(program, "input_material_ambient"), 1,
                material_ambient);
   glUniform4fv(glGetUniformLocation(program, "input_material_diffuse"), 1,
                material_diffuse);
   glUniform4fv(glGetUniformLocation(program, "input_material_specular"), 1,
                material_specular);
}

void set_sphere_stripe_vars(){
  glUniform1f(glGetUniformLocation(program, "sphere_flag"), 1.0);
  glUniform1i(glGetUniformLocation(program, "texture_1D"), 1);
  glUniform1i(glGetUniformLocation(program, "texture_2D"), 0);
  glUniform1f(glGetUniformLocation(program, "sphere_f"), 1.0);
  if (sphere_stripe_flag){
    if (vertical_stripe_flag){
      glUniform1f(glGetUniformLocation(program, "vertical_stripe_flag"), 1.0);
      glUniform1f(glGetUniformLocation(program, "sphere_stripe_flag"), 1.0);
    } else{
      glUniform1f(glGetUniformLocation(program, "vertical_stripe_flag"), 0.0);
      glUniform1f(glGetUniformLocation(program, "sphere_stripe_flag"), 1.0);
    }
  } else{
    glUniform1f(glGetUniformLocation(program, "vertical_stripe_flag"), 0.0);
    glUniform1f(glGetUniformLocation(program, "sphere_stripe_flag"), 0.0);
  }
  if( eye_stripe_flag )  {glUniform1f(glGetUniformLocation(program, "eye_flag"), 1.0);}
  else {glUniform1f(glGetUniformLocation(program, "eye_flag"), 0.0);}

  if (sphere_checker_flag){
    glUniform1f(glGetUniformLocation(program, "sphere_checker_flag"), 1.0);
    glUniform1f(glGetUniformLocation(program, "f_sphere_checker_flag"), 1.0);
  } else{
    glUniform1f(glGetUniformLocation(program, "sphere_checker_flag"), 0.0);
    glUniform1f(glGetUniformLocation(program, "f_sphere_checker_flag"), 0.0);
  }
}
// -----------------------------------------------------------------------------


void drawObj(GLuint buffer, int num_vertices, int obj_type)
{
    //--- Activate the vertex buffer object to be drawn ---//
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    /*----- Set up vertex attribute arrays for each vertex attribute -----*/
    GLuint vPosition = glGetAttribLocation(program, "vPosition");

    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0,
			  BUFFER_OFFSET(0) );


    GLuint vColor = glGetAttribLocation(program, "vColor");

    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0,
			  BUFFER_OFFSET(sizeof(point3) * num_vertices) );
      // the offset is the (total) size of the previous vertex attribute array(s)

    if (obj_type == 0){
           glLineWidth(4.0);
      glDrawArrays(GL_TRIANGLES, 0, num_vertices);
    } else {
      glLineWidth(3.0);
      glDrawArrays(GL_LINES, 0, num_vertices);
    }

    /*--- Disable each vertex attribute array being enabled ---*/
    glDisableVertexAttribArray(vPosition);
    glDisableVertexAttribArray(vColor);
}

void drawObj2(GLuint buffer, int num_vertices)
{
	//--- Activate the vertex buffer object to be drawn ---//
	glBindBuffer(GL_ARRAY_BUFFER, buffer);

	/*----- Set up vertex attribute arrays for each vertex attribute -----*/
	GLuint vPosition = glGetAttribLocation(program, "vPosition");
	glEnableVertexAttribArray(vPosition);
	glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(0));

	GLuint vNormal = glGetAttribLocation(program, "vNormal");
	glEnableVertexAttribArray(vNormal);
	glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
		BUFFER_OFFSET(sizeof(point4) * num_vertices));
	// the offset is the (total) size of the previous vertex attribute array(s)

	/* Draw a sequence of geometric objs (triangles) from the vertex buffer
	 (using the attributes specified in each enabled vertex attribute array) */
	glDrawArrays(GL_TRIANGLES, 0, num_vertices);

	/*--- Disable each vertex attribute array being enabled ---*/
	glDisableVertexAttribArray(vPosition);
	glDisableVertexAttribArray(vNormal);
}

void drawFloorTex(){
  glUniform1i(glGetUniformLocation(program, "texture_2D"), 0);
  glUniform1f(glGetUniformLocation(program, "floor_tex_flag"), 1.0);

  glBindBuffer(GL_ARRAY_BUFFER, floor_tex_buffer);

  GLuint vPosition = glGetAttribLocation(program, "vPosition");
  glEnableVertexAttribArray(vPosition);
  glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(0));

  GLuint vNormal = glGetAttribLocation(program, "vNormal");
  glEnableVertexAttribArray(vNormal);
  glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                        BUFFER_OFFSET(6 * sizeof(point4)));

  GLuint vTexCoord = glGetAttribLocation(program, "vTexCoord");
  glEnableVertexAttribArray(vTexCoord);
  glVertexAttribPointer(
      vTexCoord, 2, GL_FLOAT, GL_FALSE, 0,
      BUFFER_OFFSET(6 * (sizeof(point4) + sizeof(point3))));

  glDrawArrays(GL_TRIANGLES, 0, 6);
  glDisableVertexAttribArray(vTexCoord);

  glDisableVertexAttribArray(vPosition);
  glDisableVertexAttribArray(vNormal);
  glUniform1f(glGetUniformLocation(program, "floor_tex_flag"), 0.0);
}
// -----------------------------------------------------------------------------
void draw_floor(mat4 mv, mat4 p){
  if (lighting_flag){
    glUseProgram(program); // Use the shader program
    model_view = glGetUniformLocation(program, "ModelView");
    projection = glGetUniformLocation(program, "Projection");

    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
    glUniformMatrix4fv(projection, 1, GL_TRUE, p); // GL_TRUE: matrix is row-major

    if (point_source_flag){set_point_light(mv);}
    else{set_spot_light(mv);}
    set_floor_vars();
    mat3 normal_matrix = NormalMatrix(mv, 1);
    glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"),
      1, GL_TRUE, normal_matrix);

    if(floor_tex_flag){drawFloorTex();}
    else{drawObj2(floor_light_buffer, floor_NumVertices);}

  } else{
    glUseProgram(program);
    model_view = glGetUniformLocation(program, "ModelView");
    projection = glGetUniformLocation(program, "Projection");
    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
    glUniformMatrix4fv(projection, 1, GL_TRUE, p); // GL_TRUE: matrix is row-major
    glUniform1f(glGetUniformLocation(program, "lighting_flag"), 0.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    drawObj(floor_buffer, floor_NumVertices, 0);  // draw the floor
  }
}

void draw_axis(mat4 mv, mat4 p){
  glUseProgram(program);
  model_view = glGetUniformLocation(program, "ModelView");
  projection = glGetUniformLocation(program, "Projection");
  glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
  glUniformMatrix4fv(projection, 1, GL_TRUE, p); // GL_TRUE: matrix is row-major
  glUniform1f(glGetUniformLocation(program, "lighting_flag"), 0.0);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  drawObj(axis_buffer, axis_NumVertices, 1);  // draw the floor
}

void draw_shadow(mat4 mv, mat4 p){
  if (shadow_flag){
    if (wireframe_flag) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glUseProgram(program);
    mv = LookAt(eye, at, up) *
      p_to_q *
      Translate(trans_x, trans_y, trans_z) *
      acc_matrix;

    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
    glUniformMatrix4fv(projection, 1, GL_TRUE, p); // GL_TRUE: matrix is row-major

    if (blend_flag == 1) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glUniform1f(glGetUniformLocation(program, "blend_flag"), 1);
      drawObj(sphere_shadow_buffer, sphere_NumVertices, 0);
      glDisable(GL_BLEND);
    } else {
      glUniform1f(glGetUniformLocation(program, "blend_flag"), 0);
      drawObj(sphere_shadow_buffer, sphere_NumVertices, 0);
    }

  }
}

void draw_sphere(mat4 mv, mat4 p){
  if (wireframe_flag) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  if (lighting_flag){
    if (flat_shading_flag){
      glUseProgram(program); // Use the shader program
      model_view = glGetUniformLocation(program, "ModelView");
      projection = glGetUniformLocation(program, "Projection");

      glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
      glUniformMatrix4fv(projection, 1, GL_TRUE, p); // GL_TRUE: matrix is row-major

      if (point_source_flag){set_point_light(mv);}
      else{set_spot_light(mv);}
      set_sphere_vars();
      set_sphere_stripe_vars();
      mat3 normal_matrix = NormalMatrix(mv, 1);
      glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"),
        1, GL_TRUE, normal_matrix);
      drawObj2(sphere_flat_buffer, sphere_NumVertices);
      glUniform1f(glGetUniformLocation(program, "sphere_f"), 0.0);
      glUniform1f(glGetUniformLocation(program, "sphere_flag"), 0.0);
    }
    if (smooth_shading_flag){
      glUseProgram(program); // Use the shader program
      model_view = glGetUniformLocation(program, "ModelView");
      projection = glGetUniformLocation(program, "Projection");

      glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
      glUniformMatrix4fv(projection, 1, GL_TRUE, p); // GL_TRUE: matrix is row-major

      if (point_source_flag){set_point_light(mv);}
      else{set_spot_light(mv);}
      set_sphere_vars();
      set_sphere_stripe_vars();
      mat3 normal_matrix = NormalMatrix(mv, 1);
      glUniformMatrix3fv(glGetUniformLocation(program, "Normal_Matrix"),
        1, GL_TRUE, normal_matrix);
      drawObj2(sphere_smooth_buffer, sphere_NumVertices);
      glUniform1f(glGetUniformLocation(program, "sphere_f"), 0.0);
      glUniform1f(glGetUniformLocation(program, "sphere_flag"), 0.0);
    }
  } else{
    glUniformMatrix4fv(model_view, 1, GL_TRUE, mv); // GL_TRUE: matrix is row-major
    glUniformMatrix4fv(projection, 1, GL_TRUE, p); // GL_TRUE: matrix is row-major
    drawObj(sphere_buffer, sphere_NumVertices, 0);
  }
}

void draw_fireworks(mat4 mv, mat4 p){
  glUseProgram(program2);
  model_view2 = glGetUniformLocation(program2, "model_view");
  projection2 = glGetUniformLocation(program2, "projection");
  glUniformMatrix4fv(projection2, 1, GL_TRUE, p);
  glUniformMatrix4fv(model_view2, 1, GL_TRUE, mv);
  glPointSize(3.0);
  glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
  float t = glutGet(GLUT_ELAPSED_TIME);
  int time = int((t - t_sub)) % 5000;
  if (began_flag == true)
    glUniform1f(glGetUniformLocation(program2, "time"), time);
  else
    glUniform1f(glGetUniformLocation(program2, "time"), 0);

  if (fireworks_flag == 1) {

    glBindBuffer(GL_ARRAY_BUFFER, firework_buffer);

    GLuint velocity = glGetAttribLocation(program2, "velocity");
    glEnableVertexAttribArray(velocity);
    glVertexAttribPointer(velocity, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    GLuint vColor = glGetAttribLocation(program2, "vColor");
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 3, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(point3) * 300));
    glDrawArrays(GL_TRIANGLES, 0, 300);
    glDisableVertexAttribArray(velocity);
    glDisableVertexAttribArray(vColor);
  }
}

void display(void){
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	mat4 eye_frame = LookAt(eye, at, up); // just encase i have to use it
  mat4  p = Perspective(fovy, aspect, zNear, zFar);
  mat4 mv = LookAt(eye, at, up);
  acc_matrix = Rotate(angle, rot_axes[curr_dir].x, rot_axes[curr_dir].y, rot_axes[curr_dir].z) *
                acc_matrix;
  glUniform1f(glGetUniformLocation(program, "fog_flag"), fog_type);
  draw_axis(eye_frame, p);

  glDepthMask(GL_FALSE);
  draw_floor(eye_frame, p);
  glDepthMask(GL_TRUE);

  glDepthMask(GL_FALSE);
  draw_floor(mv, eye_frame);
  mv = LookAt(eye, at, up) *
          Translate(trans_x, trans_y, trans_z) *
          acc_matrix;
  if (eye[1] > 0)
    draw_shadow(mv, p);

  glDepthMask(GL_TRUE);
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  mv = LookAt(eye, at, up);
  draw_floor(mv, p);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  draw_floor(eye_frame, p);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

  mv = LookAt(eye, at, up) *
        Translate(trans_x, trans_y, trans_z) *
        acc_matrix;
  draw_sphere(mv, p);

  if (fireworks_flag == 1) {
    draw_fireworks(eye_frame, p);
  }

  glUseProgram(program);

  glUniform1f(glGetUniformLocation(program, "vertical_stripe_flag"), 0.0);
  glUniform1f(glGetUniformLocation(program, "sphere_stripe_flag"), 0.0);
  glUniform1f(glGetUniformLocation(program, "eye_space_flag"), 0.0);
  glutSwapBuffers();

}

//----------------------------------------------------------------------------
int next_roll_dir(){
  switch (curr_dir){
    case 0: // destination point B
      if(trans_x > pointB.x || trans_z < pointB.z){
        return 1;
      }
      break;
    case 1: // destination point C
      if(trans_x > pointC.x || trans_z > pointC.z) {
        return 2;
      }
      break;
    case 2: // destination point A
      if(trans_x < pointA.x || trans_z < pointA.z){
        return 0;
      }
      break;
  }
  return curr_dir;
}

void idle(void){
  float off = (radius * M_PI) / 180;
  trans_x += roll_dirs[curr_dir].x * off;
  trans_z += roll_dirs[curr_dir].z * off;

  curr_dir = next_roll_dir();

  glutPostRedisplay();
}

//------------------------------------------------------------------------------

void reshape(int width, int height){
    glViewport(0, 0, width, height);
    aspect = (GLfloat) width  / (GLfloat) height;
    glutPostRedisplay();
}

//------------------------------------------------------------------------------
void read_file(){
  string fname;

  cout << "'sphere.8' or 'sphere.128' or 'sphere.256' or 'sphere.1024':" << endl;
  cin >> fname;
  ifstream ifs(fname);

  ifs >> triangle_count;
  cout << "triangle count" << triangle_count << endl;

  sphere_NumVertices = triangle_count * 3;
  sphere_colors = new color3[sphere_NumVertices];
  sphere_points = new point3[sphere_NumVertices];
  sphere_shadow_colors = new color3[sphere_NumVertices];
  sphere_shadow_points = new point3[sphere_NumVertices];
  sphere_flat_points = new point4[sphere_NumVertices];
  sphere_flat_normals = new vec3[sphere_NumVertices];
  sphere_smooth_points = new point4[sphere_NumVertices];
  sphere_smooth_normals = new vec3[sphere_NumVertices];

  int n;
  float a1, a2, a3;
  while(ifs >> n){
    triangle t;
    for(int i = 0; i < n; i++){
      ifs >> a1 >> a2 >> a3;
      // this could be better
      if(i==0){t.first = point3(a1,a2,a3);}
      else if(i==1){t.second = point3(a1,a2,a3);}
      else{t.third = point3(a1,a2,a3);}
    }
    sphere_triangles.push_back(t);
  }
}

//----------------------------------------------------------------------------
void keyboard(unsigned char key, int x, int y){
  switch(key){
    case 'b': case 'B': t_sub = glutGet(GLUT_ELAPSED_TIME);began_flag = true; glutIdleFunc(idle); break;
    case 'f': fog_type = (float)((int)(fog_type + 1.0) % 4); break;
    case 'q': case 'Q': exit(EXIT_SUCCESS); break;
    case 'w': case 'W': wireframe_flag = !wireframe_flag; break;
    case 'v': case 'V': sphere_stripe_flag=true; vertical_stripe_flag=true; sphere_checker_flag=false;break;
    case 's': case 'S': sphere_stripe_flag=true; vertical_stripe_flag=false; sphere_checker_flag=false; break;
    case 'c': case 'C': sphere_stripe_flag=false; vertical_stripe_flag=false; sphere_checker_flag=false;break;
    case 'o': case 'O': eye_stripe_flag=false; break;
    case 'e': case 'E': eye_stripe_flag=true; break;
    case 'x': eye[0] -= 1; break;
    case 'X': eye[0] += 1; break;
    case 'y': eye[1] -= 1; break;
    case 'Y': eye[1] += 1; break;
    case 'z': eye[2] -= 1; break;
    case 'Z': eye[2] += 1; break;
  }
  glutPostRedisplay();
}
//----------------------------------------------------------------------------

void mouse(int button, int state, int x, int y){
  if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN && began_flag) {
    rolling_flag = !rolling_flag;
  }
  if (rolling_flag) {
    glutIdleFunc(idle);
  }
  else {
    glutIdleFunc(NULL);
  }
}

//----------------------------------------------------------------------------

void menu(int id){
  switch(id){
    case 1: eye = vrp; glutIdleFunc(idle); break;
    case 2: exit(EXIT_SUCCESS); break;
  }
  glutPostRedisplay();
}

void shadow_sub_menu(int id){
  switch(id){
    case 3: shadow_flag=true; break;
    case 4: shadow_flag=false; break;
  }
  glutPostRedisplay();
}

void wireframe_sub_menu(int id){
  switch(id){
    case 5: wireframe_flag=true; flat_shading_flag=false; smooth_shading_flag=false; break;
    case 6: wireframe_flag=false; lighting_flag=false; break;
  }
  glutPostRedisplay();
}

void lighting_sub_menu(int id){
  switch(id){
    case 7: lighting_flag=true; break;
    case 8: lighting_flag=false; break;
  }
  glutPostRedisplay();
}

void shading_sub_menu(int id){
  switch(id){
    case 9: flat_shading_flag=true; smooth_shading_flag=false; wireframe_flag=false; break;
    case 10: flat_shading_flag=false; smooth_shading_flag=true; wireframe_flag=false; break;
  }
  glutPostRedisplay();
}

void lightsource_sub_menu(int id){
  switch(id){
    case 11: spotlight_flag=true; point_source_flag=false;break;
    case 12: spotlight_flag=false; point_source_flag=true;break;
  }
  glutPostRedisplay();
}

void fog_sub_menu(int id){
  switch(id){
    case 13: fog_flag=false; fog_type=0.0;break;
    case 14: fog_flag=true; fog_type=1.0;break;
    case 15: fog_flag=true; fog_type=2.0;break;
    case 16: fog_flag=true; fog_type=3.0;break;
  }
  glutPostRedisplay();
}
void blend_sub_menu(int id){
  switch(id){
    case 17: blend_flag=true; break;
    case 18: blend_flag=false; break;
  }
  glutPostRedisplay();
}
void floor_texture_sub_menu(int id){
  switch(id){
    case 19: floor_tex_flag=true; break;
    case 20: floor_tex_flag=false; break;
  }
  glutPostRedisplay();
}
void stripe_texture_sub_menu(int id){
  switch(id){
    case 21: sphere_stripe_flag=false; vertical_stripe_flag=false; sphere_checker_flag=false;break;
    case 22: sphere_stripe_flag=true; vertical_stripe_flag=true; break;
    case 23: sphere_stripe_flag=false; vertical_stripe_flag=false; sphere_checker_flag=true;break;
  }
  glutPostRedisplay();
}
void fireworks_sub_menu(int id){
  switch(id){
    case 24: fireworks_flag=true; break;
    case 25: fireworks_flag=false; break;
  }
  glutPostRedisplay();
}

//----------------------------------------------------------------------------
#define DIM 2000
int main(int argc, char **argv){

  read_file();

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(DIM, DIM);
  glutCreateWindow("HW 2");

  int err = glewInit();
  if (GLEW_OK != err){
    printf("Error: glewInit failed: %s\n", (char*) glewGetErrorString(err));
    exit(1);
  }
  // Get info of GPU and supported OpenGL version
  printf("Renderer: %s\n", glGetString(GL_RENDERER));
  printf("OpenGL version supported %s\n", glGetString(GL_VERSION));

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);

  int shadow_sub = glutCreateMenu(shadow_sub_menu);
  glutAddMenuEntry("Yes", 3);
  glutAddMenuEntry("No", 4);
  int wireframe_sub = glutCreateMenu(wireframe_sub_menu);
  glutAddMenuEntry("Yes", 5);
  glutAddMenuEntry("No", 6);
  int lighting_sub = glutCreateMenu(lighting_sub_menu);
  glutAddMenuEntry("Yes", 7);
  glutAddMenuEntry("No", 8);
  int shading_sub = glutCreateMenu(shading_sub_menu);
  glutAddMenuEntry("Flat", 9);
  glutAddMenuEntry("Smooth", 10);
  int lightsource_sub = glutCreateMenu(lightsource_sub_menu);
  glutAddMenuEntry("SpotLight", 11);
  glutAddMenuEntry("Point", 12);
  int fog_sub = glutCreateMenu(fog_sub_menu);
  glutAddMenuEntry("No Fog", 13);
  glutAddMenuEntry("Linear", 14);
  glutAddMenuEntry("Exponential", 15);
  glutAddMenuEntry("Exponential Square", 16);
  int blend_sub = glutCreateMenu(blend_sub_menu);
  glutAddMenuEntry("Yes", 17);
  glutAddMenuEntry("No", 18);
  int floor_texture_sub = glutCreateMenu(floor_texture_sub_menu);
  glutAddMenuEntry("Yes", 19);
  glutAddMenuEntry("No", 20);
  int stripe_texture_sub = glutCreateMenu(stripe_texture_sub_menu);
  glutAddMenuEntry("None", 21);
  glutAddMenuEntry("Yes-Contour", 22);
  glutAddMenuEntry("Yes-Checker", 23);
  int fireworks_sub = glutCreateMenu(fireworks_sub_menu);
  glutAddMenuEntry("Yes", 24);
  glutAddMenuEntry("No", 25);

  glutCreateMenu(menu);
  glutAddMenuEntry("Default View Point", 1);
  glutAddSubMenu("Shadow", shadow_sub);
  glutAddSubMenu("Wireframe", wireframe_sub);
  glutAddSubMenu("Lighting", lighting_sub);
  glutAddSubMenu("Shading", shading_sub);
  glutAddSubMenu("LightSource", lightsource_sub);
  glutAddSubMenu("FogOptions", fog_sub);
  glutAddSubMenu("Blend", blend_sub);
  glutAddSubMenu("Floor Texture", floor_texture_sub);
  glutAddSubMenu("Ball Texture", stripe_texture_sub);
  glutAddSubMenu("FireWorks ", fireworks_sub);

  glutAddMenuEntry("Quit", 2);
  glutAttachMenu(GLUT_LEFT_BUTTON);

  init();
  glutMainLoop();

  // remove
  delete[] sphere_points;
  delete[] sphere_colors;
  delete[] sphere_shadow_colors;
  delete[] sphere_shadow_points;
  delete[] sphere_flat_points;
  delete[] sphere_flat_normals;
  delete[] sphere_smooth_points;
  delete[] sphere_smooth_normals;
  return 0;
}
