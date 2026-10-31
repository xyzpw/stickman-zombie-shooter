#pragma once

#include <raylib.h>
#include <raymath.h>
#include <string>
#include <cmath>
#include <map>
#include "constants.hpp"
#include "utils.hpp"

constexpr float HEADSHOT_DAMAGE_MULTIPLIER = 3;

constexpr Vector2 CASING_SIZE{3, 2};
constexpr Vector2 CASING_SIZE_RIFLE{5, 2};
constexpr Vector2 CASING_SIZE_SHELL{4, 3};

enum FirearmType {
	FIREARM_PISTOL,
	FIREARM_M1_GARAND,
	FIREARM_BOLT_ACTION,
	FIREARM_M16,
	FIREARM_SHOTGUN,
	_FIREARM_COUNT,
};

enum CartridgeType {
	CARTRIDGE_45ACP,
	CARTRIDGE_30_06,
	CARTRIDGE_556,
	CARTRIDGE_12GA,
	_CARTRIDGE_COUNT,
};

enum FirearmFireType {
	SEMI_AUTO,
	BURST_FIRE,
	FULL_AUTO,
	BOLT_ACTION,
	PUMP_ACTION,
};

const std::map<FirearmType, std::string> FIREARM_NAMES{
	{FIREARM_PISTOL, "M1911"},
	{FIREARM_M1_GARAND, "M1 Garand"},
	{FIREARM_BOLT_ACTION, "Bolt-action rifle"},
	{FIREARM_M16, "M16"},
	{FIREARM_SHOTGUN, "Shotgun"},
};

struct Bullet {
	CartridgeType cartridge;
	Vector2 position{}, position_prev{};
	Vector2 velocity{};
	float speed;
	float damage;
	int perforate_remaining = 3;
	float perforate_chance = 0;
	bool destroyed = false;

	Bullet(CartridgeType cart, Vector2 point = {})
	{
		cartridge = cart;
		position = point;

		switch (cartridge) {
		case CARTRIDGE_45ACP:
			speed = 259 * METERS_TO_PIXELS;
			damage = 34;
			perforate_chance = 0.5f;
			break;
		case CARTRIDGE_30_06:
			speed = 994 * METERS_TO_PIXELS;
			damage = 494;
			perforate_chance = 1;
			break;
		case CARTRIDGE_556:
			speed = 884 * METERS_TO_PIXELS;
			damage = 391;
			perforate_chance = 1;
			break;
		case CARTRIDGE_12GA:
			speed = 400 * METERS_TO_PIXELS;
			damage = 1000;
			perforate_chance = 0.5f;
			break;
		default:
			break;
		}
	}

	void fire(Vector2 start, Vector2 target)
	{
		Vector2 dir = computeDirection(start, target);

		position = start;
		velocity = Vector2Scale(dir, speed);
	}

	/*
	 * return: true if a line from previous to current position hits a circle
	 */
	bool checkHitCircle(Vector2 v, float radius) const
	{
		return CheckCollisionCircleLine(v, radius, position_prev,
						position);
	}

	bool checkHitRec(Rectangle r) const
	{
		return checkCollisionLineRec(position_prev, position, r);
	}

	void processHitEnemy()
	{
		if (destroyed)
			return;

		destroyed |= --perforate_remaining == 0;
		destroyed |= randomFloat(0, 1) > perforate_chance;
		velocity *= 0.9f;
		damage *= 0.81f;
	}
};

struct Magazine {
	int capacity;
	int bullets;
	CartridgeType cartridge;

	void initialize(FirearmType firearm)
	{
		switch (firearm) {
		case FIREARM_PISTOL:
			capacity = 7;
			bullets = 7;
			cartridge = CARTRIDGE_45ACP;
			break;
		case FIREARM_M1_GARAND:
			capacity = 8;
			bullets = 8;
			cartridge = CARTRIDGE_30_06;
			break;
		case FIREARM_BOLT_ACTION:
			capacity = 5;
			bullets = 5;
			cartridge = CARTRIDGE_30_06;
			break;
		case FIREARM_M16:
			capacity = 30;
			bullets = 30;
			cartridge = CARTRIDGE_556;
			break;
		case FIREARM_SHOTGUN:
			capacity = 5;
			bullets = 5;
			cartridge = CARTRIDGE_12GA;
			break;
		default:
			break;
		}
	}

	bool isFull() const {return bullets == capacity;}
	bool isEmpty() const {return bullets == 0;}
};

struct Firearm {
	FirearmType type;
	Vector2 position, barrel_position;
	Vector2 barrel_offset;
	Magazine magazine;
	CartridgeType cartridge;
	bool chambered;
	FirearmFireType fire_type;
	bool trigger_down;
	bool ejects_casings = false;
	float eject_casing_speed = 0;
	float muzzle_flash_size;
	bool manual_rechamber = false;
	bool has_select_fire = false;

	float reload_time;
	float fire_interval;
	float texture_rotation = 0; // rotation (in degrees) for drawtexture
	int burst_remaining = 3;
	std::string name;

	void initialize(FirearmType t)
	{
		type = t;
		magazine = Magazine{}; magazine.initialize(t);
		trigger_down = false;

		if (t != _FIREARM_COUNT)
			name = FIREARM_NAMES.at(t);

		switch (t) {
		case FIREARM_PISTOL:
			magazine.bullets--;
			cartridge = magazine.cartridge;
			chambered = true;
			ejects_casings = true;
			fire_type = SEMI_AUTO;
			reload_time = 2;
			fire_interval = 0.25f;
			eject_casing_speed = 3 * METERS_TO_PIXELS;
			muzzle_flash_size = 11;
			barrel_offset = PISTOL_CENTER_BARREL_OFFSET;
			break;
		case FIREARM_M1_GARAND:
			cartridge = CARTRIDGE_30_06;
			magazine.bullets--;
			chambered = true;
			ejects_casings = true;
			fire_type = SEMI_AUTO;
			reload_time = 1.5;
			fire_interval = 0.2f;
			eject_casing_speed = 5 * METERS_TO_PIXELS;
			muzzle_flash_size = 21;
			barrel_offset = RIFLE_CENTER_BARREL_OFFSET;
			break;
		case FIREARM_BOLT_ACTION:
			cartridge = CARTRIDGE_30_06;
			chambered = true;
			fire_type = BOLT_ACTION;
			reload_time = 0.8f;
			fire_interval = 1;
			muzzle_flash_size = 21;
			manual_rechamber = true;
			barrel_offset = RIFLE_CENTER_BARREL_OFFSET;
			break;
		case FIREARM_M16:
			cartridge = CARTRIDGE_556;
			magazine.bullets--;
			chambered = true;
			ejects_casings = true;
			fire_type = SEMI_AUTO;
			has_select_fire = true;
			reload_time = 3;
			fire_interval = 0.075f;
			eject_casing_speed = 4 * METERS_TO_PIXELS;
			muzzle_flash_size = 20;
			barrel_offset = M16_CENTER_BARREL_OFFSET;
			break;
		case FIREARM_SHOTGUN:
			cartridge = CARTRIDGE_12GA;
			chambered = true;
			fire_type = PUMP_ACTION;
			reload_time = 1;
			fire_interval = 1;
			muzzle_flash_size = 15;
			manual_rechamber = true;
			barrel_offset = RIFLE_CENTER_BARREL_OFFSET;
			break;
		default:
			break;
		}
	}

	void processFire();
	void rechamber();

	/*
	 * switch between semi, burst, and full-auto fire
	 */
	void tryToggleBurst()
	{
		if (!has_select_fire)
			return;

		if (fire_type == SEMI_AUTO)
			fire_type = BURST_FIRE;
		else if (fire_type == BURST_FIRE)
			fire_type = FULL_AUTO;
		else if (fire_type == FULL_AUTO)
			fire_type = SEMI_AUTO;
	}

	bool canFire() const;

	Magazine &getMagazine()
	{
		return magazine;
	}
	const Magazine &getMagazine() const
	{
		return magazine;
	}

	bool isMagazineInternal() const
	{
		switch (type) {
		case FIREARM_BOLT_ACTION:
		case FIREARM_SHOTGUN:
			return true;
		default:
			return false;
		}
	}
	/*
	 * return: true if bullets are loaded directly into firearm
	 */
	bool isManuallyLoaded() const
	{
		switch (type) {
		case FIREARM_BOLT_ACTION:
		case FIREARM_SHOTGUN:
			return true;
		default:
			return false;
		}
	}

	/*
	 * return: true if firearm accepts specified magazine
	 */
	bool isMagazineAllowed(const Magazine &m) const
	{
		return m.cartridge == cartridge;
	}

	Vector2 getBarrelPosition(bool flip) const
	{
		float rad = toRadians(texture_rotation);
		float c = std::cosf(rad);
		float s = std::sinf(rad);
		float d = flip ? -1 : 1;

		return {
			position.x + barrel_offset.x * c - barrel_offset.y * s * d,
			position.y + barrel_offset.x * s + barrel_offset.y * c * d
		};
	}

	bool isFullyLoaded() const
	{
		if (magazine.isFull())
			return chambered;
		return false;
	}
	bool isLoaded() const
	{
		return !magazine.isEmpty() || chambered;
	}

	bool isAutomatic() const {return fire_type == FULL_AUTO;}
	bool isBurstfire() const { return fire_type == BURST_FIRE; }
	bool isBoltAction() const {return fire_type == BOLT_ACTION;}

	const std::string &getSoundName() const
	{
		if (type == FIREARM_M1_GARAND)
			return M1_GARAND_SOUND_NAME;
		if (type == FIREARM_BOLT_ACTION)
			return RIFLE_SOUND_NAME;
		if (type == FIREARM_M16)
			return M16_SOUND_NAME;
		if (type == FIREARM_SHOTGUN)
			return PISTOL_SOUND_NAME;

		return PISTOL_SOUND_NAME;
	}

	const std::string &getTextureName() const
	{
		switch (type) {
		case FIREARM_BOLT_ACTION:
		case FIREARM_M1_GARAND:
			return BOLT_ACTION_TEXTURE_NAME;
		case FIREARM_M16:
			return M16_TEXTURE_NAME;
		case FIREARM_SHOTGUN:
			return BOLT_ACTION_TEXTURE_NAME;
		default:
			return PISTOL_TEXTURE_NAME;
		}
	}

	std::string getRechamberSoundName() const
	{
		switch (type) {
		case FIREARM_BOLT_ACTION:
		case FIREARM_SHOTGUN:
			return RIFLE_RECHAMBER_SOUND_NAME;
		default:
			return "";
		}
	}

	const Vector2 &getCasingSize() const
	{
		switch (cartridge) {
		case CARTRIDGE_30_06:
		case CARTRIDGE_556:
			return CASING_SIZE_RIFLE;
		case CARTRIDGE_12GA:
			return CASING_SIZE_SHELL;
		default:
			return CASING_SIZE;
		}
	}

	Sound &getSound() const;
	Texture2D &getTexture() const;
};
