#include <raylib.h>
#include <raymath.h>
#include "zombie.hpp"
#include "stickman.hpp"
#include "world.hpp"

void Zombie::initialize()
{
	stickman.loadDefaults();

	body.width = stickman.thickness;
	body.height = stickman.torso_length + stickman.neck_length;
	validateBodyRect();

	reset();
}

void Zombie::reset()
{
	stickman.loadDefaults();

	body.width = stickman.thickness;
	body.height = stickman.torso_length + stickman.neck_length;
	validateBodyRect();

	health = ZOMBIE_HEALTH;
	speed = ZOMBIE_SPEED;
	flipped = false;
	alive = true;
	alive_elapsed = 0;
	attack_elapsed = 0;
}

void Zombie::spawn(const World &world)
{
	float x = GetRandomValue(0, 1);
	x *= world.ground.x + world.ground.width;
	float y = world.ground.y - stickman.leg_length;

	hip_position = {x, y};
}

void Zombie::update()
{
	float dt = GetFrameTime();

	if (alive)
		alive_elapsed += dt;

	if (isAttacking())
		handleAttacking(dt);

	validateBodyRect();
}

void Zombie::moveTowards(Vector2 target, float dt)
{
	flipped = target.x - hip_position.x < 0;

	float dx = speed * dt * (flipped ? -1 : 1);
	hip_position.x += dx;

	/* animate legs */
	float tmp = PI * GetTime() * speed / METERS_TO_PIXELS;
	float t = (std::sinf(tmp) + 1) * 0.5f;
	stickman.pose.lerpLegs(POSE_WALK_A, POSE_WALK_B, t);
	if (flipped)
		stickman.pose.flipLegs();

	/* fix arms */
	float rot = flipped ? PI : 0;
	stickman.pose.setArmRotations(rot, rot, rot, rot);
}

void Zombie::damage(float f)
{
	health -= f;
	if (health <= 0)
		kill();
}

void Zombie::kill()
{
	if (!alive)
		return;

	alive = false;
}

void Zombie::destroyHead()
{
	stickman.head_radius *= 0.5f;
	health = 0;

	head_destroyed = true;
}

void Zombie::playAttackAnimation()
{
	handleAttacking(EPSILON);
}

void Zombie::validateBodyRect()
{
	float offset_y = -(stickman.torso_length + stickman.neck_length);

	body.x = hip_position.x - 0.5f * stickman.thickness;
	body.y = hip_position.y + offset_y;
}

void Zombie::draw() const
{
	stickman.draw(hip_position);
}

void Zombie::handleAttacking(float dt)
{
	attack_elapsed += dt;

	float t = Clamp(attack_elapsed / ZOMBIE_ATTACK_DURATION, 0, 1);
	stickman.pose.lerpAll(ZOMBIE_ATTACK_POSE_A, ZOMBIE_ATTACK_POSE_B, t);
	if (flipped)
		stickman.pose.flipAll();

	if (t == 1) {
		attack_finished = true;
		attack_elapsed = 0;
	}
}
