#version 330 core
in VERT_OUT {
  vec3 normal;
  vec3 fragment_position;
  vec2 tex_coords;
}
f_in;

struct LIGHT {
  vec3 direction;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

struct MATERIAL {
  float shininess;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

uniform LIGHT light;
uniform MATERIAL material;
uniform vec3 view_position;

uniform sampler2D shadow_map;
uniform mat4 light_space;

float in_shadow(vec4 frag_pos_light_space, vec3 normal, vec3 light_dir) {
  vec3 proj_coords = frag_pos_light_space.xyz / frag_pos_light_space.w;
  proj_coords      = proj_coords * 0.5 + 0.5;

  if (proj_coords.z > 1.0) return 0.0;

  float closest_depth = texture(shadow_map, proj_coords.xy).r;
  float current_depth = proj_coords.z;

  float bias = 0.002;
  return current_depth - bias > closest_depth ? 1.0 : 0.0;
}

vec3 blinn_phong(LIGHT l, MATERIAL m, vec3 fragment_position, vec3 normal) {
  // ambient
  vec3 ambient = l.ambient * m.ambient;

  // diffuse
  vec3 light_direction = normalize(-l.direction);
  float diffuse_affect = max(dot(normal, light_direction), 0.0);
  vec3 diffuse         = l.diffuse * m.diffuse * diffuse_affect;

  // specular
  vec3 view_direction    = normalize(view_position - fragment_position);
  vec3 halfway_direction = normalize(light_direction + view_direction);
  float specular_affect = pow(max(dot(normal, halfway_direction), 0.0), m.shininess);
  vec3 specular         = l.specular * m.specular * specular_affect;

  // shadow
  vec4 frag_pos_light_space = light_space * vec4(fragment_position, 1.0);
  float shadow = in_shadow(frag_pos_light_space, normal, light_direction);

  return ambient + (1.0 - shadow) * (diffuse + specular);
}

out vec4 FragColor;

void main() {
  // attenuation
  // float distance    = length(light.position - f_in.fragment_position);
  // float attenuation = 1.0 / (light.attenuation.x + light.attenuation.y * distance
  // +
  //                            light.attenuation.z * (distance * distance));

  vec3 lighting = blinn_phong(light, material, f_in.fragment_position, f_in.normal);

  FragColor = vec4(lighting, 1.0);
}