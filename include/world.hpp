#pragma once

#include <raylib.h>
#include <vector>
#include "firearm.hpp"

constexpr float GROUND_HEIGHT_RATIO = 0.15;
constexpr float GROUND_HEIGHT_SCREEN_RATIO = 0.15;
constexpr float DAY_TO_NIGHT_TIME = 30;
constexpr float NIGHT_TO_DAY_TIME = 120;

struct WorldObject {
	Rectangle rect;
	float rotation_deg;
	Vector2 velocity;
};

struct World {
	Rectangle ground;
	Color ground_color;
	Color background;

	float time_of_day;

	std::vector<Bullet> active_bullets{};
	std::vector<WorldObject> objects{};

	void initialize();
	void reset();

	void updateTime(float dt);

	void addProjectile(Bullet&, Vector2 direction);
	void addBullet(CartridgeType cartridge, Vector2 point, Vector2 target);
	void addBullet(CartridgeType cartridge, Vector2 point, float angle);
	void addObject(float x, float y, float width, float height,
		       Vector2 velocity, float rotation = 0);
	void addObject(Vector2 position, float width, float height,
		       Vector2 velocity, float rotation = 0);

	void draw() const;
};
