#pragma once

#include "the_chariot.hpp"

using namespace the_chariot;

// represents where a entity is in space
// ------------------------------------------------------------------------
struct Transform {
  V3f position;
  Qf rotation{};
  V3f scale{1.0f};

  M4f get_model() const noexcept {
    // Standard model matrix: T * R * S
    // When applied to vertex: T(R(S(v))) - scale first, then rotate, then translate
    // create_rotation() assumes normalized quaternion, so normalize it
    return M4f(1.0f).translate(position).rotate(rotation.normalize()).scale(scale);
  }

  static std::string name() { return "Transform"; }
};