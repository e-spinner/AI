#pragma once

#include "the_chariot.hpp"

using namespace the_chariot;

// stores parsed obj model
// ------------------------------------------------------------------------
struct Renderable final {
  std::shared_ptr<graphics::Model> model;
  bool casts_shadow{true};

  static std::string name() { return "Renderable"; }
};