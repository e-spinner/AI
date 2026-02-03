#pragma once

#include "the_chariot.hpp"

#include "../actions.hpp"

using namespace the_chariot;

// Stitches input captured by the_magician to the camera service

class CameraController : public System {
public:
  CameraController(float move_speed, float look_sensitivity, float zoom_speed)
      : System("CameraController"), m_move_speed(move_speed),
        m_look_sensitivity(look_sensitivity), m_zoom_speed(zoom_speed) {}

  void update(const Context &ctx) override {
    // TRACE(get<Magician<Actions>>()->actions().raw());
    V3f mov{};
    V3f rot{};

    for (auto [action, active] : get<Magician<Actions>>()->actions()) {
      if (!active) continue;
      switch (action) {
      case Actions::MOVE_FORWARD: mov.z += 1; break;
      case Actions::MOVE_BACK: mov.z -= 1; break;
      case Actions::MOVE_LEFT: mov.x -= 1; break;
      case Actions::MOVE_RIGHT: mov.x += 1; break;
      case Actions::MOVE_UP: mov.y += 1; break;
      case Actions::MOVE_DOWN: mov.y -= 1; break;

      case Actions::LOOK_LEFT: rot.pitch -= 1; break;
      case Actions::LOOK_RIGHT: rot.pitch += 1; break;
      case Actions::LOOK_UP: rot.roll -= 1; break;
      case Actions::LOOK_DOWN: rot.roll += 1; break;

      case Actions::ZOOM_IN: rot.z += 1; break;
      case Actions::ZOOM_OUT: rot.z -= 1; break;

      case Actions::EXIT: return;
      default: continue;
      }
    }

    if (mov.length_squared() > 0) {
      get<camera::Service>()->translate_by(mov.normalize() * m_move_speed *
                                           ctx.delta_time);
    }
    if (rot.length_squared() > 0) {
      get<camera::Service>()->rotate_by(rot.rp() * m_look_sensitivity *
                                        ctx.delta_time);
    }
    if (rot.z != 0) {
      get<camera::Service>()->zoom_by(rot.z * m_zoom_speed * ctx.delta_time);
    }
  }

private:
  float m_move_speed{0.1f};
  float m_look_sensitivity{0.1f};
  float m_zoom_speed{2.0f};
};