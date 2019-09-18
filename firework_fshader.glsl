#version 130
in  vec4 color;
out vec4 fColor;
in float y;

void main(){
  if(y<0.1) discard;
  fColor=color;
}
