#pragma once

#include "../shared/systems/camera_controller.hpp"
#include "../shared/systems/renderer.hpp"
#include "ecs/src/types.hpp"
#include <algorithm>
#include <map>
#include <memory>
#include <queue>
#include <random>

using namespace std;
using namespace the_chariot;

// Components
// ------------------------------------------------------------------------

struct Node {
  vector<Entity> neighbors;
  int row, col;

  bool visited{false};

  static std::string name() { return "Node"; }
};

struct Head {
  Entity current;

  static std::string name() { return "Head"; }
};

// Search Alg  ---  MARK: Search
// ------------------------------------------------------------------------
class Search : public System {
public:
  Search(Entity head) : System("Search"), head(head) {}

  void on_attach() override { move_head_to_node(fetch<Head>(head)->current);
  // Runs once 
    
    auto current  =  fetch<Head>(head)->current;
    auto current_node = fetch<Node>(current); 
    current_node->visited = true;

    q.push(current);
  }

  queue<Entity> q{};

  void update(const Context &ctx) override {
    move_head_to_node(fetch<Node>(fetch<Head>(head)->current)->neighbors[0]);
    // where while loop lives 

    if( !q.empty() ) {
      Entity current = q.front();
      
      q.pop();
      move_head_to_node(current);
     
      auto current_node = fetch<Node>(current); 

      for(Entity n :current_node->neighbors) {
        auto n_node = fetch<Node>(n)
        if (!n_node->visited){
            n_node->visited = true;
            q.push(n);
        }
      }
    }


  }

private:
  Entity head;

  void move_head_to_node(Entity node) {
    auto n                           = fetch<Transform>(node);
    fetch<Transform>(head)->position = {static_cast<float>(n->position.x), 1.0f,
                                        static_cast<float>(n->position.z)};
    fetch<Head>(head)->current       = node;
  }
};

// MARK: Maze
// ------------------------------------------------------------------------

static void generate_maze_recursive(vector<vector<int>> &maze,
                                    vector<vector<bool>> &visited, int row, int col,
                                    int rows, int cols, mt19937 &gen) {
  visited[row][col] = true;
  maze[row][col]    = 1; // Mark as path

  // Directions: North, South, East, West
  vector<pair<int, int>> directions = {{-2, 0}, {2, 0}, {0, -2}, {0, 2}};

  // Shuffle directions for randomness
  shuffle(directions.begin(), directions.end(), gen);

  for (auto [dr, dc] : directions) {
    int new_row = row + dr;
    int new_col = col + dc;

    // Check bounds
    if (new_row < 1 || new_row >= rows - 1 || new_col < 1 || new_col >= cols - 1)
      continue;

    // If not visited, carve path
    if (!visited[new_row][new_col]) {
      // Carve the wall between current and new cell
      maze[row + dr / 2][col + dc / 2] = 1;
      generate_maze_recursive(maze, visited, new_row, new_col, rows, cols, gen);
    }
  }
}

// Generate a grid-based maze with nodes at decision points
[[maybe_unused]] static pair<vector<Entity>, Entity>
generate_maze(Coordinator &ecs, int width, int height, float cell_size,
              std::shared_ptr<graphics::Model> cube,
              std::shared_ptr<graphics::Model> plane) {
  // Ensure odd dimensions for proper maze generation
  int cols = (width % 2 == 0) ? width + 1 : width;
  int rows = (height % 2 == 0) ? height + 1 : height;

  // Initialize maze: 0 = wall, 1 = path
  vector<vector<int>> maze(rows, vector<int>(cols, 0));
  vector<vector<bool>> visited(rows, vector<bool>(cols, false));

  random_device rd;
  mt19937 gen(rd());

  // Start from a random odd cell
  uniform_int_distribution<int> row_dist(1, rows - 2);
  uniform_int_distribution<int> col_dist(1, cols - 2);
  int start_row = row_dist(gen);
  int start_col = col_dist(gen);
  // Ensure odd coordinates
  if (start_row % 2 == 0) start_row++;
  if (start_col % 2 == 0) start_col++;

  generate_maze_recursive(maze, visited, start_row, start_col, rows, cols, gen);

  // Ensure entrance and exit
  maze[1][1]               = 1;
  maze[rows - 2][cols - 2] = 1;

  // Create nodes for every path cell in the maze
  vector<Entity> nodes;
  map<pair<int, int>, Entity> cell_to_node;

  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      if (maze[r][c] == 0) continue; // Skip walls

      float x = (c - cols / 2.0f) * cell_size;
      float z = (r - rows / 2.0f) * cell_size;

      Entity node = ecs.create_entity(
          Transform{.position = {x, 0, z}, .scale = V3f{0.5f, 0.5f, 0.5f}},
          Node{.row = r, .col = c},
          Renderable{.model = plane, .material = "green", .casts_shadow = false});

      nodes.push_back(node);
      cell_to_node[{r, c}] = node;
    }
  }

  // Connect nodes to their immediate neighbors
  vector<pair<int, int>> directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
  for (auto [pos, entity] : cell_to_node) {
    auto [r, c] = pos;
    auto *node  = ecs.try_get_component<Node>(entity);
    if (!node) continue;

    for (auto [dr, dc] : directions) {
      int nr = r + dr;
      int nc = c + dc;

      // Check bounds and if it's a path
      if (nr >= 0 && nr < rows && nc >= 0 && nc < cols && maze[nr][nc] == 1) {
        auto neighbor_it = cell_to_node.find({nr, nc});
        if (neighbor_it != cell_to_node.end()) {
          node->neighbors.push_back(neighbor_it->second);
        }
      }
    }
  }

  // Update node rendering based on neighbor count
  for (auto entity : nodes) {
    auto *node       = ecs.try_get_component<Node>(entity);
    auto *renderable = ecs.try_get_component<Renderable>(entity);
    if (!node || !renderable) continue;

    if (node->neighbors.size() > 2) { renderable->material = "red"; }
  }

  // Create start and goal nodes at the grid borders
  pair<int, int> entrance_pos{1, 1};
  pair<int, int> exit_pos{rows - 2, cols - 2};

  int start_node_row = 0;
  int start_node_col = entrance_pos.second;
  float start_x      = (start_node_col - cols / 2.0f) * cell_size;
  float start_z      = (start_node_row - rows / 2.0f) * cell_size;

  Entity start_node = ecs.create_entity(
      Transform{.position = {start_x, 0, start_z}, .scale = V3f{0.5f, 0.5f, 0.5f}},
      Node{.row = start_node_row, .col = start_node_col},
      Renderable{.model = plane, .material = "yellow", .casts_shadow = false});

  int goal_node_row = rows - 1;
  int goal_node_col = exit_pos.second;
  float goal_x      = (goal_node_col - cols / 2.0f) * cell_size;
  float goal_z      = (goal_node_row - rows / 2.0f) * cell_size;

  Entity goal_node = ecs.create_entity(
      Transform{.position = {goal_x, 0, goal_z}, .scale = V3f{0.5f, 0.5f, 0.5f}},
      Node{.row = goal_node_row, .col = goal_node_col},
      Renderable{.model = plane, .material = "yellow", .casts_shadow = false});

  // Connect start to entrance and goal to exit
  auto entrance_it = cell_to_node.find(entrance_pos);
  auto exit_it     = cell_to_node.find(exit_pos);

  if (entrance_it != cell_to_node.end()) {
    auto *start_node_comp = ecs.try_get_component<Node>(start_node);
    auto *entrance_node   = ecs.try_get_component<Node>(entrance_it->second);
    if (start_node_comp && entrance_node) {
      start_node_comp->neighbors.push_back(entrance_it->second);
      entrance_node->neighbors.push_back(start_node);
    }
  }

  if (exit_it != cell_to_node.end()) {
    auto *goal_node_comp = ecs.try_get_component<Node>(goal_node);
    auto *exit_node      = ecs.try_get_component<Node>(exit_it->second);
    if (goal_node_comp && exit_node) {
      goal_node_comp->neighbors.push_back(exit_it->second);
      exit_node->neighbors.push_back(goal_node);
    }
  }

  nodes.push_back(start_node);
  nodes.push_back(goal_node);

  // render maze walls
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      if (maze[r][c] == 0) { // Wall
        if ((r == start_node_row && c == start_node_col) ||
            (r == goal_node_row && c == goal_node_col))
          continue;

        float x = (c - cols / 2.0f) * cell_size;
        float z = (r - rows / 2.0f) * cell_size;

        auto _ = ecs.create_entity(
            Transform{.position = {x, 0.25f, z},
                      .scale    = V3f{cell_size * 0.9f, 0.5f, cell_size * 0.9f}},
            Renderable{.model = cube, .material = "coral", .casts_shadow = true});
      }
    }
  }

  return {nodes, start_node};
}
