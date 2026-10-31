#include <raylib.h>
#include "world.hpp"

void World::initialize()
{
	int win_x = GetScreenWidth();
	int win_y = GetScreenHeight();

	ground.width = win_x;
	ground.height = win_y * GROUND_HEIGHT_SCREEN_RATIO;
	ground.x = 0;
	ground.y = win_y - ground.height;

	reset();
}

void World::reset()
{
	active_bullets.clear();
	objects.clear();

	time_of_day = 0;
	background = DAY_COLOR;
	ground_color = BLACK;
}

void World::updateTime(float dt)
{
	time_of_day += dt;
	if (time_of_day > DAY_TO_NIGHT_TIME + NIGHT_TO_DAY_TIME)
		time_of_day = 0;

	if (time_of_day < DAY_TO_NIGHT_TIME) {
		float t = time_of_day / DAY_TO_NIGHT_TIME;
		background = ColorLerp(DAY_COLOR, NIGHT_COLOR, t);
	} else {
		float t = (time_of_day - DAY_TO_NIGHT_TIME) /
			(NIGHT_TO_DAY_TIME);
		background = ColorLerp(NIGHT_COLOR, SKYBLUE, t);
	}
}

void World::addProjectile(Bullet &bullet, Vector2 direction)
{
	active_bullets.push_back(bullet);
}

void World::addBullet(CartridgeType cartridge, Vector2 point,
		      Vector2 target)
{
	Bullet b(cartridge);
	b.fire(point, target);

	active_bullets.push_back(b);
}

void World::addBullet(CartridgeType cartridge, Vector2 point, float angle)
{
	Bullet b(cartridge);
	b.position = point;
	b.velocity = Vector2Scale(angleToDirection(angle), b.speed);

	active_bullets.push_back(b);
}

void World::addObject(float x, float y, float width, float height,
		      Vector2 velocity, float rotation)
{
	WorldObject obj{};
	obj.rect = {x, y, width, height};
	obj.velocity = velocity;

	objects.push_back(obj);
}

void World::addObject(Vector2 position, float width, float height,
		      Vector2 velocity, float rotation)
{
	addObject(position.x, position.y, width, height, velocity);
}

void World::draw() const
{
	DrawRectangleRec(ground, ground_color);

	/* world objects */
	for (const auto &[rec, rot, vel] : objects) {
		DrawRectanglePro(rec, Vector2Zeros, rot, BLACK);
	}
}
