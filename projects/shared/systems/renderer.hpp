#pragma once

#include "the_chariot.hpp"

#include "../components/renderable.hpp"
#include "../components/transform.hpp"

using namespace the_chariot;

// This one is complicated. it wants a input size of window
// This size shoudlmatch the_world size.

// also wants a entity passed in via set_sun()
// this entity requires the DirectionalLight Component
// this component can be found in subprojects/the_chariot/graphics/light

class Renderer : public System {
public:
  Renderer(int width, int height)
      : System("Renderer"), width(width), height(height) {}

  void on_attach() override {
    // Enable depth testing for 3D rendering
    glEnable(GL_DEPTH_TEST);

    new (&basic) the_chariot::graphics::Shader("../shared/shaders/basic.vert",
                                               "../shared/shaders/basic.frag");

    new (&shadow) the_chariot::graphics::Shader("../shared/shaders/shadow.vert");

    // --- uniform buffers ---
    basic.bindUBO("Matrices", 0);

    glGenBuffers(1, &matrices_UBO);
    glBindBuffer(GL_UNIFORM_BUFFER, matrices_UBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(M4f) * 2, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferRange(GL_UNIFORM_BUFFER, 0, matrices_UBO, 0, sizeof(M4f) * 2);

    init_shadows();
  }

  void update(const Context &ctx) override {
    glEnable(GL_DEPTH_TEST);

    F_ASSERT(sun != INVALID_ENTITY, "failed to init sun");

    // ------------------------------------------------------------------------
    // PASS 1: Shadow Map
    // ------------------------------------------------------------------------
    if (render_shadows) {
      glViewport(0, 0, 2048, 2048); // shadow map resloution
      glBindFramebuffer(GL_FRAMEBUFFER, shadow_FBO);
      glClear(GL_DEPTH_BUFFER_BIT);

      shadow.activate();
      fetch<graphics::DirectionalLight>(sun)->set_light_space_matrix(shadow);

      // Draw everything that casts a shadow
      each<Transform, Renderable>([&](Entity e, Transform &t, Renderable &r) {
        // if (r.casts_shadow) {
        shadow.setM4("model", t.get_model());
        r.model->draw(shadow);
        // }
      });
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // ------------------------------------------------------------------------
    // PASS 2: Main Scene
    // ------------------------------------------------------------------------
    glViewport(0, 0, width, height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    basic.activate();

    // update UBO
    M4f matrices[] = {get<camera::Service>()->get_projection_matrix(),
                      get<camera::Service>()->get_view_matrix()};
    glBindBuffer(GL_UNIFORM_BUFFER, matrices_UBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(matrices), matrices);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, matrices_UBO, 0, sizeof(M4f) * 2);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Set sun / camera uniforms
    fetch<graphics::DirectionalLight>(sun)->set_uniform(basic);
    fetch<graphics::DirectionalLight>(sun)->set_light_space_matrix(basic);
    basic.setV3("view_position", get<camera::Service>()->get_eye());

    // bind shadow map
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadow_map);
    basic.setI("shadow_map", 1);

    // Draw everything with lighting + shadow
    each<Transform, Renderable>([&](Entity e, Transform &t, Renderable &r) {
      r.model->set_material(r.material);
      basic.setM4("model", t.get_model());
      r.model->draw(basic);
    });
  }

  void set_sun(Entity s) {
    sun = s;
    TRACE("Sun Entity Registered");
  }

  bool render_shadows = true;

private:
  int width{0};
  int height{0};
  the_chariot::graphics::Shader basic;
  the_chariot::graphics::Shader shadow;
  GLuint matrices_UBO{0};

  GLuint shadow_FBO, shadow_map;

  void init_shadows() {
    glGenFramebuffers(1, &shadow_FBO);
    glGenTextures(1, &shadow_map);
    glBindTexture(GL_TEXTURE_2D, shadow_map);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 2048, 2048, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // Setup wrapping to white so objects outside the shadow map aren't in shadow
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0, 1.0, 1.0, 1.0};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, shadow_FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           shadow_map, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  Entity sun = INVALID_ENTITY;
};