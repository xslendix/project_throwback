#pragma once

#include "FPCamera.h"
#include "common.hpp"

#include "raylib_ex.hpp"

Vector2 const WORLD_SIZE_GEN = {512, 512};
Vector3 const WORLD_SIZE_GEN_V3 = {512, 1, 512};

struct Circle {
  Vector2 position;
  i32 radius;
};

i32 const MIN_CIRCLE_SIZE = 65;
i32 const MAX_CIRCLE_SIZE = 80;
i32 const PATH_THICKNESS = 30;
i32 const WORLD_SCALE = 20;

//f64 const SHOOTING_DISTANCE = 1.35;
f64 const SHOOTING_DISTANCE = 1.20;

// FIXME: Free this shit.
struct World {
  Image image;
  Mesh mesh;
  Model model;

  vector<Circle> circles;
  map<Circle*, Circle*> connections;
  vector<Model> *artifacts;

  auto Unload() {
    UnloadModel(model); // Unloads mesh as well.
    UnloadImage(image);
  }

  auto static Build(Vector2 size, vector<Model> *artifacts, i32 circles) -> World {
    i32 i, j;
    World world;
    world.artifacts = artifacts;

    world.image = GenImageColor(size.x, size.y, WHITE);

    ImageDrawRectangleLines(&world.image, {0, 0, (f64)world.image.width, (f64)world.image.height}, 1, WHITE);

    // 1. Generate circles
    for (i=0; i<circles; i++) {
      Circle circle = {
        {
          (float)GetRandomValue(0, size.x-1),
          (float)GetRandomValue(0, size.y-1),
        },
        GetRandomValue(MIN_CIRCLE_SIZE, MAX_CIRCLE_SIZE),
      };
      if (circle.position.x + circle.radius >= size.x || 
          circle.position.x - circle.radius <= 0 ||
          circle.position.y + circle.radius >= size.y ||
          circle.position.y - circle.radius <= 0) {
      next_circle:
        i--;
        continue;
      }

      for (Circle second : world.circles)
        if (CheckCollisionCircles(circle.position, circle.radius, second.position, second.radius))
          goto next_circle;

      world.circles.push_back(circle);
    }

    // 2. Generate connections
    for (i=0; i<circles; i++) {
      Circle *first = &world.circles.at(i);

      for (j=0; j<circles; j++) {
        if (i == j) continue;
        Circle *second = &world.circles.at(j);

        if (world.connections.count(first) || world.connections.count(second))
          continue;

        TraceLog(LOG_INFO, "LINK: {%f %f} {%f %f}", first->position.x, first->position.y,
            second->position.x, second->position.y);
        world.connections.insert({first, second});
      }
    }

    TraceLog(LOG_INFO, "Filling shit");
    // 3. Generate image
    for (let const& [first, second] : world.connections) {
      ImageDrawCircleV(&world.image, first->position, first->radius, BLACK);
      ImageDrawFlood(&world.image, first->position, BLACK);
      ImageDrawCircleV(&world.image, second->position, second->radius, BLACK);
      ImageDrawFlood(&world.image, second->position, BLACK);

      ImageDrawLineEx(&world.image, first->position, second->position, PATH_THICKNESS, BLACK);
    }
    TraceLog(LOG_INFO, "Donr filling shit");

    // 4. Generate model
    world.mesh = GenMeshHeightmap(world.image, {1,1,1});
    world.model = LoadModelFromMesh(world.mesh);

    return world;
  }

  auto DistanceToObject(FPCamera &cam, i32 index) -> f64 {
     let circle = circles.at(index);
     Vector2 circle_pos_scaled = {
        (circle.position.x * WORLD_SCALE) / WORLD_SIZE_GEN.x,
        (circle.position.y * WORLD_SCALE) / WORLD_SIZE_GEN.y,
     };
     return sqrtf(powf(cam.CameraPosition.x-circle_pos_scaled.x, 2) +
         powf(cam.CameraPosition.z-circle_pos_scaled.y, 2));
  }

  auto CheckObjectInFront(FPCamera &cam) -> i32 {
     size_t i;
     let view_angle = (cam.GetViewAngles().x + 90) * DEG2RAD;

     for (i=0; i<circles.size(); i++) {
       let circle = circles.at(i);
       Vector2 circle_pos_scaled = {
          (circle.position.x * WORLD_SCALE) / WORLD_SIZE_GEN.x,
          (circle.position.y * WORLD_SCALE) / WORLD_SIZE_GEN.y,
       };
       let distance = sqrtf(powf(cam.CameraPosition.x-circle_pos_scaled.x, 2) +
           powf(cam.CameraPosition.z-circle_pos_scaled.y, 2));
       if (distance >= SHOOTING_DISTANCE)
         continue;

       let angle_to_artifact = atan2f(circle_pos_scaled.y - cam.CameraPosition.z,
           circle_pos_scaled.x - cam.CameraPosition.x);
       let diff = view_angle - angle_to_artifact;

       if (diff > PI)
         diff -= 2.0f * PI;
       if (diff < -PI)
         diff += 2.0f * PI;

       if (abs(diff) < (cam.GetFOVX() * DEG2RAD) + .1f)
         return i;
     }

     return -1;
  }
};

