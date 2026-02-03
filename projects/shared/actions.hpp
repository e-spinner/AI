#pragma once

#include "the_chariot.hpp"

using namespace the_chariot;

// Defines actions used by the engine. keybindings need to be defined in
// a config.cfg

BUILD_ACTION_ENUM(Actions, MOVE_FORWARD, MOVE_BACK, MOVE_LEFT, MOVE_RIGHT, MOVE_UP,
                  MOVE_DOWN, CLICK, LOOK_LEFT, LOOK_RIGHT, LOOK_UP, LOOK_DOWN,
                  ZOOM_IN, ZOOM_OUT, EXIT);