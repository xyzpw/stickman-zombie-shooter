#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <algorithm>
#include "player.hpp"
#include "world.hpp"
#include "stickman.hpp"
#include "utils.hpp"

using std::atan2f, std::fabsf;

void Player::initialize(World &world)
{
	reset(world);
}

void Player::reset(World &world)
{
	firearms.clear();
	magazines.clear();
	cartridges.clear();

	stickman.loadDefaults();
	death_animation_timeleft = 0;

	aiming_direction = {};
	hip_position = {
		0.5f * GetScreenWidth(),
		world.ground.y - stickman.leg_length
	};

	Firearm fa;
	fa.initialize(FIREARM_PISTOL);
	firearms.push_back(fa);
	giveMagazines(fa.type, 1);

	alive = true;
	aiming = false;
	flipped = false;
	firearm_index = 0;
}

void Player::update()
{
	float dt = GetFrameTime();

	bool aiming_behind = flipped != isArmsFlipped();

	last_fire_elapsed += dt;
	check_ammo_timer = std::max(check_ammo_timer - dt, 0.f);

	if (isReloading()) {
		handleReloading(dt);
	}
	if (isDeathAnimationPlaying()) {
		handleDeathAnimation(dt);
	}

	/* flip legs to match aiming direction */
	if (alive && aiming_behind && !firearm_position_dirty) {
		stickman.pose.flipLegs();
		flipped = !flipped;
	}

	/* update firearm position */
	if (firearm_position_dirty) {
		Firearm &fa = getFirearm();

		fa.position = stickman.getGripPosition(hip_position);
		fa.barrel_position = fa.getBarrelPosition(isArmsFlipped());
		firearm_position_dirty = false;
	}
}

void Player::lateUpdate()
{
	rechambered_frame = false;
	dropped_mag_frame = false;
	fired_frame = false;
}

void Player::walk(float dir, float dt)
{
	if (!alive)
		return;

	float dx = PLAYER_SPEED * dir * dt;
	Vector2 moved = Vector2Add(hip_position, {dx, 0});

	if (moved.x < 0 || moved.x >= GetScreenWidth())
		return;

	hip_position = moved;
	firearm_position_dirty = true;
	flipped = dir < 0;

	Pose &pose = stickman.pose;

	/* animations */
	float tmp = PI * GetTime() * PLAYER_SPEED / METERS_TO_PIXELS;
	float t = (std::sinf(tmp) + 1) * 0.5f;
	pose.lerpLegs(POSE_WALK_A, POSE_WALK_B, t);
	if (flipped)
		pose.flipLegs();

	firearm_position_dirty = true;
}

void Player::aim(Vector2 v)
{
	if (!alive)
		return;

	Vector2 origin = fabsf(v.x - hip_position.x) <= stickman.arm_length * 1.5f
		? stickman.getHeadPosition(hip_position)
		: stickman.getGripPosition(hip_position);
	aiming_direction = computeDirection(origin, v);

	float arm_dir = isArmsFlipped() ? -1 : 1;
	float angle = atan2f(-aiming_direction.y, aiming_direction.x);
	stickman.pose.setArmRotations(angle - toRadians(15) * arm_dir,
				      angle,
				      angle + toRadians(10) * arm_dir,
				      angle);

	Firearm &fa = getFirearm();
	fa.texture_rotation = toDegrees(-angle);

	firearm_position_dirty = true;
}

bool Player::tryShoot()
{
	if (!alive || isCheckingAmmo())
		return false;

	Firearm &fa = getFirearm();

	/* discard reloading */
	if (isReloading()) {
		if (fa.isMagazineInternal())
			reloading_internal = false;
		else
			dropReloadingMag();
		return false;
	}

	/* manual rechamber */
	if (fa.manual_rechamber && fa.isLoaded() && !fa.chambered) {
		if (last_fire_elapsed < fa.fire_interval * 0.5f)
			return false;

		fa.rechamber();
		rechambered_frame = true;
		last_fire_elapsed = 0;
		return false;
	}

	if (last_fire_elapsed < fa.fire_interval)
		return false;

	last_fire_elapsed = 0;

	if (!fa.canFire())
		return false;

	fa.processFire();
	fired_frame = true;
	return true;
}

void Player::reload()
{
	if (!alive || isReloading())
		return;

	check_ammo_timer = 0;

	Firearm &fa = getFirearm();
	if (fa.isFullyLoaded())
		return;
	if (fa.isManuallyLoaded()) {
		if (getCartridgeCount(fa.cartridge) == 0)
			return;
	} else if (getMagazineCount(fa.cartridge) == 0)
		return;

	reload_timer = fa.reload_time;
	/* drop mag if empty */
	if (!fa.isMagazineInternal() && fa.getMagazine().isEmpty())
		dropReloadingMag();

	if (fa.isMagazineInternal())
		reloading_internal = true;
}

bool Player::collides(Vector2 v)
{
	float dx = std::fabs(v.x - hip_position.x);
	return dx < stickman.arm_length;
}

void Player::kill(bool animate)
{
	alive = false;

	if (animate && !isDeathAnimationPlaying())
		playDeathAnimation();
}

void Player::playDeathAnimation()
{
	pose_at_death = stickman.pose;

	death_animation_timeleft = PLAYER_DEATH_ANIMATION_TIME;
}

void Player::draw() const
{
	stickman.draw(hip_position);
}

void Player::drawWeapon() const
{
	const Firearm &fa = getFirearm();
	float rot = fa.texture_rotation;
	const Texture2D &tex = fa.getTexture();

	Rectangle src{
		0, 0,
		(float)tex.width, (float)tex.height
	};
	Rectangle dst{
		fa.position.x, fa.position.y,
		(float)tex.width, (float)tex.height
	};
	Vector2 origin{0.5f * tex.width, 0.5f * tex.height};

	/* flip */
	if (isArmsFlipped()) {
		src.height = -src.height;
	}

	DrawTexturePro(tex, src, dst, origin, rot, WHITE);

}

void Player::handleReloading(float dt)
{
	if (!alive) {
		reload_timer = 0;
		return;
	}

	// animation
	if (reload_timer > 0)
		handleReloadAnimation();

	reload_timer -= dt;
	if (reload_timer <= 0)
		reload_timer = 0;
	else
		return;

	Firearm &firearm = getFirearm();

	/* load bullet into internal magazine */
	if (firearm.isManuallyLoaded()) {
		CartridgeType c = firearm.cartridge;
		Magazine &mag = firearm.getMagazine();

		if (getCartridgeCount(c) == 0)
			return;

		mag.bullets++;
		cartridges.at(c)--;
		firearm.chambered = true;

		if (mag.isFull() || dropping_mag || getCartridgeCount(c) == 0) {
			reloading_internal = false;
			dropping_mag = false;
		}
		if (reloading_internal)
			reload_timer = firearm.reload_time;
		return;
	}

	auto sortMagsPred = [&](const Magazine &a, const Magazine &b) {
		return a.bullets > b.bullets;
	};

	// return if player has no loaded magazines
	if (getMagazineCount(firearm.cartridge) == 0)
		return;

	/* insert next mag */
	for (auto it = magazines.begin(); it != magazines.end(); ++it) {
		if (!firearm.isMagazineAllowed(*it))
			continue;
		else if (it->isEmpty())
			continue;

		Magazine next = *it;
		magazines.erase(it);

		/* save magazine */
		if (!dropping_mag && !firearm.getMagazine().isEmpty())
			magazines.push_back(firearm.magazine);

		firearm.magazine = next;
		break;
	}
	/* sort magazines most to least bullets */
	std::sort(magazines.begin(), magazines.end(), sortMagsPred);

	/* chamber firearm (magazine loses 1) */
	if (!firearm.chambered) {
		firearm.magazine.bullets--;
		firearm.chambered = true;
	}

	dropping_mag = false;
}

void Player::handleDeathAnimation(float dt)
{
	death_animation_timeleft -= dt;
	if (death_animation_timeleft <= 0)
		death_animation_timeleft = 0;

	float t = getDeathAnimationProgress();
	stickman.pose.lerpAll(pose_at_death, PLAYER_POSE_DEAD, t);
}

void Player::handleReloadAnimation()
{
	const Firearm &fa = getFirearm();
	Pose &pose = stickman.pose;

	if (fa.isManuallyLoaded())
		return;

	bool has_mag = reload_timer < 0.5f * fa.reload_time;
	float elapsed = fa.reload_time - reload_timer;
	float progress = 2 * elapsed / fa.reload_time;
	if (has_mag)
		progress -= 1;

	float dir = isArmsFlipped() ? -1 : 1;
	float angle = atan2f(-aiming_direction.y, aiming_direction.x);
	float hi = has_mag ? angle - toRadians(15) * dir : toRadians(-135);
	float lo = has_mag ? angle + toRadians(10) * dir : toRadians(-45);
	float Ahi = has_mag ? toRadians(-135) : angle - toRadians(15) * dir;
	float Alo = has_mag ? toRadians(-45) : angle + toRadians(10) * dir;
	if (!has_mag && dir == -1) {
		hi = PI - hi;
		lo = PI - lo;
	}
	if (has_mag && dir == -1) {
		Ahi = PI - Ahi;
		Alo = PI - Alo;
	}
	pose.upper_left_arm_angle = lerpAngle(Ahi, hi, progress);
	pose.lower_left_arm_angle = lerpAngle(Alo, lo, progress);
}
