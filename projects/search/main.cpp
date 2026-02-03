#include <thread>

#include "../shared/systems/camera_controller.hpp"
#include "../shared/systems/renderer.hpp"

using namespace std;
using namespace the_chariot;

// Global Settings that should probably be in the config file --- MARK: Settings
// ------------------------------------------------------------------------

// how long to wait between frames
auto g_sleep_time = chrono::milliseconds(16); // ~60 fps

// Priority order to update systems in
enum Priority {
  Input  = 0,
  Render = 100,
};

// Camera Settings
namespace cam {
V3f initial_position{5.0f, 5.0f, 5.0f};      // [m?]
V2f initial_orientaion{-M_PI / 4.0f, -2.35}; // [rad] { pitch, yaw }
}; // namespace cam

// MARK: Main
// ------------------------------------------------------------------------
int main() {

  // Initialize Engine Objects
  // ------------------------------------------------------------------------
  Config CFG{"config.cfg"};
  auto width  = CFG.get<int>("window", "width", 800);
  auto height = CFG.get<int>("window", "height", 600);

  World the_world{width, height, "AI - projects/search"};

  auto ECS = Coordinator();
  ECS.init();

  ECS.register_components<Transform, Renderable, graphics::DirectionalLight>();

  // Setup Camera
  // ------------------------------------------------------------------------
  auto camera   = ECS.register_service<camera::Service>((float)width / height);
  auto cam_ctrl = ECS.register_system<CameraController>(
      update::Type::FRAME, Priority::Input,
      CFG.get<float>("camera", "move_speed", 0.1f),
      CFG.get<float>("camera", "mouse_sensitivity", 0.1f),
      CFG.get<float>("camera", "zoom_speed", 2.0f));

  camera->setup_camera(cam::initial_position, cam::initial_orientaion,
                       CFG.get<float>("camera", "fov", 60.0f),
                       CFG.get<float>("camera", "near_plane", 0.1f),
                       CFG.get<float>("camera", "far_plane", 200.0f));

  // Setup User Input "Actions"
  // ------------------------------------------------------------------------
  Input::Binding::Array BINDINGS = {{"keyboard", Input::Device::KEYBOARD},
                                    {"mouse", Input::Device::MOUSE},
                                    {"analog", Input::Device::ANALOG}};

  auto magician =
      ECS.register_service<Magician<Actions>>(CFG, BINDINGS, ::from_string);

  // Initialize Window Context and renderer
  // ------------------------------------------------------------------------
  the_world.spin();
  // capture mouse in window context
  SDL_SetWindowRelativeMouseMode(the_world.window(), true);

  auto renderer = ECS.register_system<Renderer, Transform, Renderable>(
      update::Type::FRAME, Priority::Render, width, height);

  Entity sun = ECS.create_entity(graphics::DirectionalLight{});
  renderer->set_sun(sun);

  // Setup Environment
  // ------------------------------------------------------------------------
  vector<Entity> ids;

  // floor
  auto plane = std::make_shared<graphics::Model>("../shared/models", "plane.obj");
  ids.push_back(ECS.create_entity(
      Transform{.scale{100, 1, 100}},
      Renderable{.model = std::move(plane), .casts_shadow = false}));

  // Actual Game Loop  ---  MARK: Loop
  // ------------------------------------------------------------------------
  ECS.start();
  do {
    the_world.poll_events([&](const SDL_Event &e) { magician->process_event(e); });
    magician->update_analog_actions();

    ECS.update();

    the_world.present_frame();

    this_thread::sleep_for(g_sleep_time);

  } while (!the_world.should_close() && !magician->is_active(Actions::EXIT));
}