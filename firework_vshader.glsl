#version 130

in  vec3 velocity;
in  vec3 vColor;
out vec4 color;
out float y;

uniform mat4 model_view;
uniform mat4 projection;
uniform float time;

void main() {
 float vx = time * velocity.x * 0.001;
 float vy = 0.1 + 0.001 * velocity.y * time + 0.5 * (-0.00000049)* pow(time ,2);
 float vz = time * velocity.z * 0.001;
 
 vec4 vPosition4 = vec4(vx, vy, vz, 1.0);
 vec4 vColor4 = vec4(vColor.r, vColor.g, vColor.b, 1.0);

 y = vy;
 gl_Position = projection * model_view * vPosition4;
 color = vColor4;
}
