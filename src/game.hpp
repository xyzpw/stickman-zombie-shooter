#pragma once

#include <raylib.h>
#include <vector>
#include <map>
#include <utility>
#include "world.hpp"
#include "player.hpp"
#include "zombie.hpp"
#include "firearm.hpp"

constexpr float WAVE_INTERMISSION = 30;

// firearm price map {firearm_cost, ammo_cost}
const std::map<FirearmType, std::pair<int, int>> FIREARM_PRICES{
	{FIREARM_PISTOL, {0, 25}},
	{FIREARM_M1_GARAND, {180, 50}},
	{FIREARM_BOLT_ACTION, {75, 50}},
	{FIREARM_M16, {150, 100}},
	{FIREARM_SHOTGUN, {500, 50}},
};

enum GameState {
	GAME_PAUSED,
	GAME_PLAYING,
};

struct Game {
	GameState state;

	World world;
	Player player;
	std::vector<Zombie> zombies{};

	int zombie_spawns_remaining;
	int wave;
	int points;
	int kill_count;
	float intermission_timer;

	void initialize();
	void reset();

	void update();
	void updateZombies(float dt);

	void spawnZombie();

	int countZombiesAlive() const
	{
		int n = 0;
		for (const auto &z : zombies) {
			if (z.alive)
				n++;
		}
		return n;
	}

	bool isIntermission() const {return intermission_timer > 0;}
	bool isZombieSpawnReady() const
	{
		if (!player.alive)
			return false;

		return zombie_spawns_remaining > 0 &&
			zombie_spawn_timer == 0;
	}
	bool isWaveCleared() const
	{
		return zombie_spawns_remaining == 0 &&
			countZombiesAlive() == 0;
	}

	private:
	void handleInput(float dt);
	void handlePhysics(float dt);
	void handleIntermission(float dt);
	void handleDayNightCycle();
	void processGunshot(Vector2 mouse);
	void processRechamber();
	void processReloading();
	void startNextWave();
	void handleShop();
	void draw() const;
	void drawAmmo() const;
	void drawWave() const;
	void drawGameover() const;

	float zombie_spawn_timer = 1;
	float zombie_wave_health;
	float time_since_death = 0;
	float survival_time = 0;
	float reload_key_time = 0; // how long has reload key been pressed?
};
