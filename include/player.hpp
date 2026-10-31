#pragma once

#include <raylib.h>
#include <vector>
#include <map>
#include "stickman.hpp"
#include "constants.hpp"
#include "firearm.hpp"
#include "utils.hpp"

constexpr float PLAYER_SPEED = 2.25f * METERS_TO_PIXELS;
constexpr float PLAYER_DEATH_ANIMATION_TIME = 0.5f;
constexpr float CHECK_AMMO_DURATION = 1.5f;

struct World;

const Pose PLAYER_POSE_DEAD{
	toRadians(-180),
	toRadians(-180), toRadians(-180),
	toRadians(-180), toRadians(-180),
	toRadians(20), toRadians(10),
	toRadians(-20), toRadians(-10)
};

struct Player {
	Vector2 hip_position;
	Stickman stickman;
	Vector2 aiming_direction;

	std::vector<Firearm> firearms;
	std::vector<Magazine> magazines;
	std::map<CartridgeType, int> cartridges;

	bool alive;
	bool aiming;
	bool flipped;

	unsigned int firearm_index = 0;
	// was firearm rechambered this frame?
	bool rechambered_frame = false;
	// was the firearms magazine dropped this frame?
	bool dropped_mag_frame = false;
	// was the equipped firearm fired this frame?
	bool fired_frame = false;

	void initialize(World &world);
	void reset(World &world);

	void update();
	void lateUpdate();

	void walk(float dir, float dt);
	void aim(Vector2 v);
	/*
	 * return: true on success
	 */
	bool tryShoot();
	void reload();
	/*
	 * drop the current magazine if reloading
	 */
	void dropReloadingMag()
	{
		if (dropping_mag)
			return;

		if (reloading_internal) {
			reloading_internal = false;
			return;
		}

		const Firearm &f = getFirearm();
		if (!isReloading())
			return;

		dropped_mag_frame = true;
		dropping_mag = true;
		if (reload_timer >= 0.5f * f.reload_time)
			reload_timer *= 0.5f;
	}

	void cycleFirearm()
	{
		if (isReloading() || isCheckingAmmo())
			return;

		unsigned int size = firearms.size();
		if (size < 2)
			return;

		/* return: true if the specified firearm is loaded or player has extra ammo */
		static auto isNextValidFn = [&](const Firearm &f) {
			int count = f.isMagazineInternal()
				? getCartridgeCount(f.cartridge)
				: getMagazineCount(f.cartridge);
			return f.isLoaded() || count > 0;
		};
		/* swap firearm index to specified value */
		auto cycleFn = [&](int i) {
			firearm_index = i;
			firearm_position_dirty = true;
		};

		unsigned int next = firearm_index + 1;
		if (next == size)
			next = 0;

		/* select next firearm with available ammo */
		bool looped = false;
		for (unsigned int i = next; i < size; ++i) {
			if (i == firearm_index)
				break;

			if (isNextValidFn(firearms.at(i))) {
				cycleFn(i);
				break;
			} else if (i + 1 == size) {
				if (looped)
					return;
				looped = true;
				i = 0;
			}
		}
	}

	Firearm &getFirearm()
	{
		return firearms.at(firearm_index);
	}
	const Firearm &getFirearm() const
	{
		return firearms.at(firearm_index);
	}

	/*
	 * return: number of cartridges player has for reloading
	 */
	int getCartridgeCount(CartridgeType cart) const
	{
		return cartridges.count(cart) ? cartridges.at(cart) : 0;
	}
	/*
	 * return: number of loaded magazines for a specified cartridge
	 */
	int getMagazineCount(CartridgeType cart) const
	{
		int n = 0;
		for (const auto &m : magazines) {
			if (m.cartridge == cart && !m.isEmpty())
				n++;
		}
		return n;
	}

	bool hasFirearm(FirearmType type)
	{
		for (const auto &f : firearms) {
			if (f.type == type)
				return true;
		}
		return false;
	}

	void checkAmmo() {check_ammo_timer = CHECK_AMMO_DURATION;}
	void giveMagazines(FirearmType type, int count)
	{
		Magazine m;
		m.initialize(type);
		for (int i = 0; i < count; ++i) {
			magazines.push_back(m);
		}
	}
	void giveCartridges(CartridgeType type, int count)
	{
		if (cartridges.count(type))
			cartridges.at(type) += count;
		else
			cartridges[type] = count;
	}
	bool giveFirearm(FirearmType type)
	{
		if (hasFirearm(type))
			return false;

		Firearm fa;
		fa.initialize(type);
		firearms.push_back(fa);
		return true;
	}

	/*
	 * return: true if point collides with hitbox
	 */
	bool collides(Vector2 v);

	void kill(bool animate = false);
	void playDeathAnimation();

	Vector2 getHeadPosition() const
	{
		return stickman.getHeadPosition(hip_position);
	}

	bool isReloading() const {return reload_timer > 0;}
	bool isRechambering() const {return rechamber_timer > 0;}
	bool isCheckingAmmo() const {return check_ammo_timer > 0;}

	bool isDeathAnimationPlaying() const
	{
		return death_animation_timeleft > 0;
	}
	float getDeathAnimationProgress() const
	{
		return (PLAYER_DEATH_ANIMATION_TIME - death_animation_timeleft)
			/ PLAYER_DEATH_ANIMATION_TIME;
	}

	bool isArmsFlipped() const
	{
		return getFirearm().position.x < hip_position.x;
	}

	void draw() const;
	void drawWeapon() const;

	private:
	bool firearm_position_dirty = false;

	// is currently holding active firearm trigger?
	bool shoot_down = false;
	// should discard current mag when reloading?
	bool dropping_mag = false;
	// is reloading a firearm with an internal magazine?
	bool reloading_internal = false;

	float reload_timer = 0;
	// how long until rechamber is finished
	float rechamber_timer = 0;
	float last_fire_elapsed = 0;
	float check_ammo_timer = 0;

	float death_animation_timeleft = 0;
	Pose pose_at_death;

	void handleReloading(float dt);
	void handleDeathAnimation(float dt);
	void handleReloadAnimation();
};
