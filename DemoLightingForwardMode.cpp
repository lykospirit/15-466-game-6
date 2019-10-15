#include "DemoLightingForwardMode.hpp"
#include "DrawLines.hpp"
#include "Load.hpp"
#include "Sprite.hpp"
#include "DrawSprites.hpp"
#include "data_path.hpp"
#include "BasicMaterialForwardProgram.hpp"
#include "gl_errors.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <algorithm>

GLuint spheres_for_basic_material_forward = -1U;
extern Load< MeshBuffer > spheres_meshes;

Load< Scene > spheres_scene_forward(LoadTagLate, []() -> Scene const * {
	spheres_for_basic_material_forward = spheres_meshes->make_vao_for_program(basic_material_forward_program->program);

	return new Scene(data_path("spheres.scene"), [](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = spheres_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable::Pipeline &pipeline = scene.drawables.back().pipeline;
		pipeline = basic_material_forward_program_pipeline;
		pipeline.vao = spheres_for_basic_material_forward;
		pipeline.type = mesh.type;
		pipeline.start = mesh.start;
		pipeline.count = mesh.count;

		float roughness = 1.0f;
		if (transform->name.substr(0, 9) == "Icosphere") {
			roughness = (transform->position.y + 10.0f) / 18.0f;
		}
		pipeline.set_uniforms = [roughness](){
			glUniform1f(basic_material_forward_program->ROUGHNESS_float, roughness);
		};

	});
});


DemoLightingForwardMode::DemoLightingForwardMode() {
}

DemoLightingForwardMode::~DemoLightingForwardMode() {
}

void DemoLightingForwardMode::draw(glm::uvec2 const &drawable_size) {

	//--- use camera structure to set up scene camera ---
	scene_camera->transform->rotation =
		glm::angleAxis(camera.azimuth, glm::vec3(0.0f, 0.0f, 1.0f))
		* glm::angleAxis(0.5f * 3.1415926f + -camera.elevation, glm::vec3(1.0f, 0.0f, 0.0f))
	;
	scene_camera->transform->position = camera.target + camera.radius * (scene_camera->transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f));
	scene_camera->transform->scale = glm::vec3(1.0f);
	scene_camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	glm::vec3 eye = scene_camera->transform->make_local_to_world()[3];
	glm::mat4 world_to_clip = scene_camera->make_projection() * scene_camera->transform->make_world_to_local();

	//compute light uniforms:
	uint32_t lights = uint32_t(spheres_scene_forward->lights.size());
	//clamp lights to maximum lights allowed by shader:
	lights = std::min< uint32_t >(lights, BasicMaterialForwardProgram::MaxLights);

	std::vector< int32_t > light_type; light_type.reserve(lights);
	std::vector< glm::vec3 > light_location; light_location.reserve(lights);
	std::vector< glm::vec3 > light_direction; light_direction.reserve(lights);
	std::vector< glm::vec3 > light_energy; light_energy.reserve(lights);
	std::vector< float > light_cutoff; light_cutoff.reserve(lights);

  glm::uvec2 player_center = glm::uvec2(drawable_size.x/2, drawable_size.y/2);
  uint32_t player_radius = 8;
  glm::vec4 player_color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

	for (auto const &light : spheres_scene_forward->lights) {
		glm::mat4 light_to_world = light.transform->make_local_to_world();
		//set up lighting information for this light:
		light_location.emplace_back(glm::vec3(light_to_world[3]));
		light_direction.emplace_back(glm::vec3(-light_to_world[2]));
		light_energy.emplace_back(light.energy);

		if (light.type == Scene::Light::Point) {
			light_type.emplace_back(0);
			light_cutoff.emplace_back(1.0f);
		} else if (light.type == Scene::Light::Hemisphere) {
			light_type.emplace_back(1);
			light_cutoff.emplace_back(1.0f);
		} else if (light.type == Scene::Light::Spot) {
			light_type.emplace_back(2);
			light_cutoff.emplace_back(std::cos(0.5f * light.spot_fov));
		} else if (light.type == Scene::Light::Directional) {
			light_type.emplace_back(3);
			light_cutoff.emplace_back(1.0f);
		}

		//skip remaining lights if maximum light count reached:
		if (light_type.size() == lights) break;
	}

	GL_ERRORS();

	//--- actual drawing ---
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	//upload light uniforms:
	glUseProgram(basic_material_forward_program->program);

	glUniform3fv(basic_material_forward_program->EYE_vec3, 1, glm::value_ptr(eye));

	glUniform1ui(basic_material_forward_program->LIGHTS_uint, lights);

	glUniform1iv(basic_material_forward_program->LIGHT_TYPE_int_array, lights, light_type.data());
	glUniform3fv(basic_material_forward_program->LIGHT_LOCATION_vec3_array, lights, glm::value_ptr(light_location[0]));
	glUniform3fv(basic_material_forward_program->LIGHT_DIRECTION_vec3_array, lights, glm::value_ptr(light_direction[0]));
	glUniform3fv(basic_material_forward_program->LIGHT_ENERGY_vec3_array, lights, glm::value_ptr(light_energy[0]));
	glUniform1fv(basic_material_forward_program->LIGHT_CUTOFF_float_array, lights, light_cutoff.data());

  glUniform2uiv(basic_material_forward_program->PLAYER_CENTER_uvec2, 1, glm::value_ptr(player_center));
  glUniform1ui(basic_material_forward_program->PLAYER_RADIUS_uint, player_radius);
  glUniform4fv(basic_material_forward_program->PLAYER_COLOR_vec4, 1, glm::value_ptr(player_color));

	GL_ERRORS();

	//draw scene:
	spheres_scene_forward->draw(world_to_clip);

  //check for failure condition and draw relevant text:
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	DrawSprites draw_sprites(*atlas, view_min, view_max, drawable_size, DrawSprites::AlignPixelPerfect);
  if (camera.game_over) {
    draw_sprites.draw_text("GAME OVER!", glm::vec2(119.0f, 129.0f), 1.0f, glm::u8vec4(0x00, 0x00, 0x00, 0xff));
    draw_sprites.draw_text("GAME OVER!", glm::vec2(120.0f, 130.0f));
    draw_sprites.draw_text("Press R to reset", glm::vec2(127.0f, 109.0f), 0.7f, glm::u8vec4(0x00, 0x00, 0x00, 0xff));
    draw_sprites.draw_text("Press R to reset", glm::vec2(128.0f, 110.0f), 0.7f);
  } else if (camera.win) {
    draw_sprites.draw_text("YOU WIN!", glm::vec2(131.0f, 129.0f), 1.0f, glm::u8vec4(0x00, 0x00, 0x00, 0xff));
    draw_sprites.draw_text("YOU WIN!", glm::vec2(132.0f, 130.0f));
    draw_sprites.draw_text("Press R to reset", glm::vec2(127.0f, 109.0f), 0.7f, glm::u8vec4(0x00, 0x00, 0x00, 0xff));
    draw_sprites.draw_text("Press R to reset", glm::vec2(128.0f, 110.0f), 0.7f);
  } else if (camera.first) {
    draw_sprites.draw_text("WASD to move", glm::vec2(127.0f, 129.0f), 0.8f, glm::u8vec4(0x00, 0x00, 0x00, 0xff));
    draw_sprites.draw_text("WASD to move", glm::vec2(128.0f, 130.0f), 0.8f);
  } else if (camera.is_reset) {
    camera.is_reset = false; // prevent last frame in framebuffer from setting game_over to true
  } else {
    if (camera.tutorial == 0 && camera.target.y <= -23.0f){ camera.tutorial = 1; }
    if (camera.tutorial == 1 && camera.target.y <= -30.0f){ camera.tutorial = 2; }
    if (camera.tutorial == 1){
      draw_sprites.draw_text("Use LMB and scroll to move the camera", glm::vec2(89.0f, 119.0f), 0.7f, glm::u8vec4(0x00, 0x00, 0x00, 0xff));
      draw_sprites.draw_text("Use LMB and scroll to move the camera", glm::vec2(90.0f, 120.0f), 0.7f);
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glReadBuffer(GL_FRONT);
    glm::uvec2 player_tl = player_center - player_radius;
    std::vector< glm::u8vec3 > data(player_radius*2*player_radius*2);
    glReadPixels(player_tl.x, player_tl.y, player_radius*2, player_radius*2, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    uint32_t lose_count = 0, lose_threshold = player_radius*player_radius/3; // ~10% of player
    uint32_t win_count = 0, win_threshold = player_radius*player_radius*3/5; // ~70% of corners outside circle
    glm::u8vec3 red = glm::u8vec3(255,0,0);     // color of part of player that is in darkness
    glm::u8vec3 win = glm::u8vec3(41,60,235);   // approximate color of objective
    for (int i=0; i<data.size(); i++) {
      if (red == data[i]){ lose_count++; }
      if (abs(win.x-data[i].x) + abs(win.y-data[i].y) + abs(win.z-data[i].z) < 100){ win_count++; }
    }
    if (lose_count > lose_threshold) { camera.game_over = true; }
    if (win_count > win_threshold) { camera.win = true; }
  }
}
