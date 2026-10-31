#pragma once

#include <raylib.h>
#include "stickman.hpp"
#include "constants.hpp"
#include "utils.hpp"

constexpr float ZOMBIE_HEALTH = 100;
constexpr float ZOMBIE_HEALTH_MAX = 5000;
constexpr float ZOMBIE_SPEED = 0.5f * METERS_TO_PIXELS;
constexpr float ZOMBIE_SPEED_MAX = 2.5f * METERS_TO_PIXELS;
constexpr float ZOMBIE_ATTACK_DURATION = 0.3f;

const Pose ZOMBIE_ATTACK_POSE_A{
	toRadians(90),
	toRadians(-50), toRadians(-45),
	toRadians(-50), toRadians(-45),
	toRadians(-90), toRadians(-90),
	toRadians(-90), toRadians(-90)
};
const Pose ZOMBIE_ATTACK_POSE_B{
	0,
	toRadians(-45), toRadians(-80),
	toRadians(-45), toRadians(-80),
	toRadians(-100), toRadians(-80),
	toRadians(-170), toRadians(-180)
};

struct World;

struct Zombie {
	Vector2 hip_position;
	Stickman stickman;
	Rectangle body;

	float health;
	bool alive;
	float speed = ZOMBIE_SPEED;
	bool flipped;
	bool attack_finished = false;
	bool head_destroyed = false;

	void initialize();
	void reset();

	void spawn(const World &world);
	void update();

	void moveTowards(Vector2 target, float dt);
	void damage(float f);
	void kill();
	void destroyHead();

	void playAttackAnimation();

	/* increase walk speed */
	void makeFast()
	{
		speed = ZOMBIE_SPEED_MAX;
	}

	void validateBodyRect();

	/* return: true if zombie is fast moving */
	bool isFast() const {return speed == ZOMBIE_SPEED_MAX;}

	bool isAttacking() const {return attack_elapsed > 0;}

	void draw() const;

	private:
	void handleAttacking(float dt);

	float alive_elapsed = 0;
	float attack_elapsed = 0;
};
