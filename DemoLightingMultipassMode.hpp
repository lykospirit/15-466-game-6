#pragma once

#include "Mode.hpp"
#include "Scene.hpp"
#include "Mesh.hpp"
#include "Sprite.hpp"

struct DemoLightingMultipassMode : Mode {
	DemoLightingMultipassMode();
	virtual ~DemoLightingMultipassMode();

	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
  virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	void run_timing();

	//z-up trackball-style camera controls:
	struct {
		float radius = 20.0f;
		float azimuth = 0.5f * 3.1415926f; //angle ccw of -y axis, in radians, [-pi,pi]
		float elevation = 0.5f * 3.1415926f; //angle above ground, in radians, [-pi,pi]
		glm::vec3 target = glm::vec3(0.0f,10.0f,0.0f);
		bool flip_x = false; //flip x inputs when moving? (used to handle situations where camera is upside-down)

    bool move_up = false;
    bool move_down = false;
    bool move_left = false;
    bool move_right = false;
    float move_speed = 0.005f;

    bool first = true;
    bool game_over = false;
    bool win = false;
    bool is_reset = false;
    uint32_t tutorial = 0;
	} camera;

	//mode uses a secondary Scene to hold a camera:
	Scene camera_scene;
	Scene::Camera *scene_camera = nullptr;

  // for help text
  SpriteAtlas const *atlas = nullptr;
	glm::vec2 view_min = glm::vec2(-1.0f, -1.0f);
	glm::vec2 view_max = glm::vec2( 1.0f,  1.0f);
};
