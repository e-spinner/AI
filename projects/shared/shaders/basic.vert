#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 tex_coords;

layout(std140) uniform Matrices {
  uniform mat4 projection;
  uniform mat4 view;
};

uniform mat4 model;
uniform float texture_scale = 1.0;

out VERT_OUT {
  vec3 normal;
  vec3 fragment_position;
  vec2 tex_coords;
}
v_out;

void main() {
  gl_Position = projection * view * model * vec4(position, 1.0);

  mat3 normal_matrix = transpose(inverse(mat3(model)));

  v_out.normal            = normalize(normal_matrix * normal);
  v_out.fragment_position = vec3(model * vec4(position, 1.0));
  v_out.tex_coords        = tex_coords * texture_scale;
}