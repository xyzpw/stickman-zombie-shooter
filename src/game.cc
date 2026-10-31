#include <raylib.h>
#include <raymath.h>
#include <algorithm>
#include <cmath>
#include "game.hpp"
#include "world.hpp"
#include "constants.hpp"
#include "utils.hpp"
#include "assets.hpp"

using std::powf;

void Game::initialize()
{
	world.initialize();
	player.initialize(world);

	reset();
}

void Game::reset()
{
	zombies.clear();
	world.reset();
	player.reset(world);

	wave = 0;
	points = 0;
	kill_count = 0;
	intermission_timer = 0;
	zombie_spawns_remaining = 0;
	survival_time = 0;
	time_since_death = 0;
	zombie_wave_health = ZOMBIE_HEALTH;
}

void Game::update()
{
	float dt = GetFrameTime();

	zombie_spawn_timer = std::max(zombie_spawn_timer - dt, 0.f);

	if (player.alive)
		survival_time += dt;
	else
		time_since_death += dt;

	if (isIntermission())
		handleIntermission(dt);
	else if (isWaveCleared())
		intermission_timer = WAVE_INTERMISSION;

	if (isZombieSpawnReady())
		spawnZombie();

	handleInput(dt);
	handlePhysics(dt);

	if (player.isReloading())
		processReloading();

	/* update time (day/night cycle) */
	if (!isIntermission() && player.alive && wave > 2)
		world.updateTime(dt);

	player.update();
	updateZombies(dt);

	draw();

	player.lateUpdate();
}

void Game::updateZombies(float dt)
{
	const Firearm &fa = player.getFirearm();

	/* return: true if contact shot */
	auto contactShotFn = [&](Vector2 p, float r) {
		if (!player.fired_frame || fa.type == FIREARM_PISTOL)
			return false;

		return CheckCollisionPointCircle(fa.barrel_position, p, r);
	};

	for (auto &z : zombies) {
		z.update();

		Vector2 head = z.stickman.getHeadPosition(z.hip_position);
		bool alive_prev = z.alive;

		/* destroy head if contact shot */
		if (contactShotFn(head, z.stickman.head_radius)) {
			z.destroyHead();
		}

		/* bullets */
		for (auto &bt : world.active_bullets) {
			if (bt.destroyed)
				continue;

			float dmg = 0;
			if (bt.checkHitCircle(head, z.stickman.head_radius)) {
				dmg = bt.damage * HEADSHOT_DAMAGE_MULTIPLIER;
				points += 5;
			} else if (bt.checkHitRec(z.body)) {
				dmg = bt.damage;
			}
			z.damage(dmg);

			/* destroy bullet if it perforates too many times */
			if (dmg > 0) {
				bt.processHitEnemy();
			}
		}

		/* zombie killed */
		if (!z.alive && alive_prev) {
			points += 10;
			kill_count++;
		}

		if (!z.alive || z.attack_finished)
			continue;

		// move
		z.moveTowards(player.hip_position, dt);
		/* try to attack the player */
		if (player.collides(z.hip_position)) {
			z.playAttackAnimation();
			player.kill(true);
		}
	}
}

void Game::spawnZombie()
{
	Zombie z;
	z.initialize();

	z.health = zombie_wave_health;
	if (wave > 1 && randomFloat(0, 1) < wave * 0.1f)
		z.makeFast();

	z.spawn(world);
	zombies.push_back(z);

	zombie_spawns_remaining--;
	zombie_spawn_timer = randomFloat(0.5f, 1);
}

void Game::handleInput(float dt)
{
	if (IsKeyDown(KEY_D))
		player.walk(1, dt);
	if (IsKeyDown(KEY_A))
		player.walk(-1, dt);

	if (!player.alive) {
		bool death_playing = player.isDeathAnimationPlaying();

		/* restart */
		if (IsKeyReleased(KEY_R) && !death_playing) {
			reset();
		}

		return;
	}

	Vector2 mouse = GetMousePosition();
	Vector2 mouse_delta = GetMouseDelta();

	if (mouse_delta != Vector2Zeros) {
		player.aim(mouse);
	}

	if (IsKeyDown(KEY_R)) {
		reload_key_time += dt;
	}
	if (IsKeyReleased(KEY_R)) {
		if (reload_key_time < RELOAD_CHECK_AMMO_THRESHOLD)
			player.reload();
		reload_key_time = 0;
	} else if (reload_key_time > RELOAD_CHECK_AMMO_THRESHOLD) {
		if (!player.isCheckingAmmo() && !player.isReloading())
			player.checkAmmo();
	}

	if (IsKeyPressed(KEY_X)) {
		player.getFirearm().tryToggleBurst();
	}

	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
		if (player.tryShoot())
			processGunshot(mouse);
		else if (player.rechambered_frame)
			processRechamber();
	}

	if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
		Firearm &fa = player.getFirearm();

		fa.trigger_down = false;

		bool burst = fa.isBurstfire();
		if (!burst || (burst && fa.burst_remaining == 0))
			fa.burst_remaining = 3;
	}

	if (IsKeyPressed(KEY_Q) && player.alive) {
		player.cycleFirearm();
	}

	if (isIntermission() && IsKeyDown(KEY_F)) {
		intermission_timer = dt;
	}
}

void Game::handlePhysics(float dt)
{
	/* bullets */
	for (auto &bt : world.active_bullets) {
		if (bt.destroyed)
			continue;

		Vector2 dp = Vector2Scale(bt.velocity, dt);
		bt.position_prev = bt.position;
		bt.position = Vector2Add(bt.position, dp);
	}
	/* world objects */
	for (auto &[rec, rot, vel] : world.objects) {
		vel.y += GRAVITY * dt;

		rec.x += vel.x * dt;
		rec.y += vel.y * dt;
	}

	/* zombies */
	for (auto &z : zombies) {
		Pose &pose = z.stickman.pose;

		if (pose.torso_angle == 0)
			continue;

		float leg_length = z.stickman.leg_length;

		if (z.isAttacking()) {
			float ang = pose.upper_right_leg_angle;
			static float h = computeEndpoint(Vector2Zeros,
							 0.5f * leg_length,
							 ang).y;
			float max_height = world.ground.y - h;

			z.hip_position.y += GRAVITY * dt;
			if (z.hip_position.y > max_height)
				z.hip_position.y = max_height;
		} else if (!z.alive) {
			float h = world.ground.y - z.hip_position.y;
			float t = 1 - h / GRAVITY;
			float rad = z.flipped ? PI : 0;

			z.hip_position.y += GRAVITY * dt;
			if (z.hip_position.y > world.ground.y)
				z.hip_position.y = world.ground.y;
			pose.torso_angle = lerpAngle(pose.torso_angle,
						     rad, t);
		}
	}

	if (player.isDeathAnimationPlaying()) {
		player.hip_position.y += GRAVITY * dt;
		if (player.hip_position.y > world.ground.y)
			player.hip_position.y = world.ground.y;
	}

	/*
	 * return: true if bullet should be removed
	 */
	auto pred_bullet = [&](Bullet &b) {
		if (b.destroyed || b.position.y >= world.ground.y)
			return true;
		return !isInDisplay(b.position);
	};
	auto pred_object = [&](WorldObject &o) {
		if (o.rect.y > world.ground.y)
			return true;
		return !isInDisplay({o.rect.x, o.rect.y});
	};

	/* remove objects outside world */
	auto rmit_bullets = std::remove_if(world.active_bullets.begin(),
					   world.active_bullets.end(),
					   pred_bullet);
	auto rmit_objects = std::remove_if(world.objects.begin(),
					   world.objects.end(),
					   pred_object);
	world.active_bullets.erase(rmit_bullets, world.active_bullets.end());
	world.objects.erase(rmit_objects, world.objects.end());
}

void Game::handleIntermission(float dt)
{
	intermission_timer -= dt;
	if (intermission_timer <= 0 || wave == 0) {
		startNextWave();
		return;
	}
	handleShop();

	const char *s = TextFormat("Intermission: %.1f s",
				   intermission_timer);
	DrawText(s, 0, 0, 20, WHITE);
}

void Game::processGunshot(Vector2 mouse)
{
	static Vector2 m1_garand_clip_size{5, 4};

	Firearm &fa = player.getFirearm();
	const Vector2 &casing_size = fa.getCasingSize();
	float tex_rot = fa.texture_rotation; // texture rotation

	float flip = player.isArmsFlipped() ? -1 : 1;
	float angle = toRadians(90 - fa.texture_rotation * flip) * flip;
	float bullet_angle = toRadians(-fa.texture_rotation);
	Vector2 direction = angleToDirection(angle);

	world.addBullet(fa.cartridge, fa.barrel_position, bullet_angle);
	if (fa.type == FIREARM_SHOTGUN) {
		float f = -0.035f;
		for (int i = 0; i < 8; ++i) {
			float ang = bullet_angle + f;
			world.addBullet(fa.cartridge, fa.barrel_position, ang);
			f += 0.07f / 8;
		}
	}

	PlaySound(fa.getSound());
	/* muzzle flash */
	float muzzle_t = randomFloat(0.2f, 0.95f);
	DrawCircleSector(fa.barrel_position, fa.muzzle_flash_size,
			 355 + tex_rot, 365 + tex_rot,
			 2, ColorAlpha(WHITE_SMOKE, muzzle_t));

	/* eject casings when shot */
	if (fa.ejects_casings) {
		Vector2 vel = Vector2Scale(direction, fa.eject_casing_speed);
		world.addObject(fa.position, casing_size.x, casing_size.y,
				vel, angle);
	}

	/* m1 garand ping and clip ejection */
	if (fa.type == FIREARM_M1_GARAND && !fa.isLoaded()) {
		asset_manager.playSound(M1_GARAND_PING_SOUND_NAME);

		Vector2 vel = Vector2Scale(direction, 3.5f * METERS_TO_PIXELS);
		world.addObject(fa.position,
				m1_garand_clip_size.x, m1_garand_clip_size.y,
				vel, angle);
	}
}

void Game::processRechamber()
{
	const Firearm &fa = player.getFirearm();
	const Vector2 &casing_size = fa.getCasingSize();

	const std::string sound_name = fa.getRechamberSoundName();
	if (sound_name.empty())
		return;

	asset_manager.playSound(sound_name);

	float flip = player.isArmsFlipped() ? -1 : 1;
	float angle = toRadians(90 - fa.texture_rotation * flip) * flip;
	Vector2 direction = angleToDirection(angle);

	Vector2 vel = Vector2Scale(direction, 3 * METERS_TO_PIXELS);
	world.addObject(fa.position, casing_size.x, casing_size.y, vel, angle);
}

void Game::processReloading()
{
	static Vector2 mag_size{4, 6};

	if (!player.dropped_mag_frame)
		return;

	const Firearm &fa = player.getFirearm();

	// skip last m1 garand shot because mags are ejected by default
	if (fa.type == FIREARM_M1_GARAND && !fa.isLoaded())
		return;

	world.addObject(fa.position, mag_size.x, mag_size.y,
			Vector2Zeros);
}

void Game::startNextWave()
{
	zombies.clear();
	wave++;
	intermission_timer = 0;

	zombie_spawns_remaining = GetRandomValue(4, 8) * powf(1.5, wave - 1);
	zombie_spawns_remaining = Clamp(zombie_spawns_remaining, 1, 30);
	zombie_wave_health = powf(2, wave - 1) * ZOMBIE_HEALTH;
}

void Game::handleShop()
{
	static int firearm_count = (int)_FIREARM_COUNT;
	static int fontsize = 20;
	int x = 0.25f * GetScreenWidth();
	int y = 0.1f * GetScreenHeight();

	const char *title = TextFormat("Shop (%d points)", points);
	DrawText(title, x, y, fontsize, WHITE);
	y += fontsize * 2;

	for (int i = 0; i < firearm_count; ++i) {
		int item_id = i + 1;

		FirearmType type = (FirearmType)i;
		bool has_firearm = player.hasFirearm(type);

		const auto &name = FIREARM_NAMES.at(type);
		int price = FIREARM_PRICES.at(type).first;
		int ammo_price = FIREARM_PRICES.at(type).second;
		bool affordable = points >= (has_firearm ? ammo_price : price);

		const char *s = TextFormat("%d) %s: cost %d, ammo cost %d",
					   item_id, name.c_str(), price,
					   ammo_price);
		Color col = affordable ? GREEN : RED;
		DrawText(s, x, y, fontsize, col);

		y += fontsize * 1.5f;

		/* buy */
		if (affordable && IsKeyPressed(KEY_ZERO + item_id)) {
			points -= has_firearm ? ammo_price : price;

			if (!has_firearm)
				player.giveFirearm(type);

			switch (type) {
			case FIREARM_BOLT_ACTION:
				player.giveCartridges(CARTRIDGE_30_06, 5);
				break;
			case FIREARM_SHOTGUN:
				player.giveCartridges(CARTRIDGE_12GA, 5);
				break;
			default:
				player.giveMagazines(type, 1);
				break;
			}
		}
	}
}

void Game::draw() const
{
	world.draw();
	player.draw();
	if (player.alive)
		player.drawWeapon();
	else
		drawGameover();

	/* zombies */
	for (const auto &z : zombies) {
		z.draw();
	}

	drawAmmo();
	drawWave();
}

void Game::drawAmmo() const
{
	static int fontsize = 20;
	static int mag_height = 16;
	int x = 5, y = GetScreenHeight() - fontsize * 2;

	const Firearm &firearm = player.getFirearm();
	const CartridgeType cartridge = firearm.cartridge;
	const bool manual_load = firearm.isManuallyLoaded();

	/* draw weapon name */
	std::string label = firearm.name;
	if (player.isCheckingAmmo()) {
		const Magazine &mag = firearm.magazine;
		label += TextFormat(" (ammo %d/%d)", mag.bullets, mag.capacity);
	} else if (player.isReloading()) {
		label += " (reloading)";
	} else if (!firearm.isLoaded()) {
		int n = firearm.isManuallyLoaded()
			? player.getCartridgeCount(cartridge)
			: player.getMagazineCount(cartridge);
		label += n == 0 ? " (no ammo)" : " (empty)";
	} else if (firearm.has_select_fire) {
		switch (firearm.fire_type) {
		case SEMI_AUTO:
			label += " (semi-auto)";
			break;
		case BURST_FIRE:
			label += " (burst fire)";
			break;
		case FULL_AUTO:
			label += " (full-auto)";
			break;
		default:
			break;
		}
	}
	DrawText(label.c_str(), x, y, fontsize, WHITE);

	/*
	 * return: color for magazine
	 */
	static auto colorFn = [&](const Magazine &m) {
		float t = (float)m.bullets / m.capacity;
		if (t == 1)
			return WHITE;
		return ColorLerp(MAGAZINE_COLOR_A, MAGAZINE_COLOR_B, 1 - t);
	};

	y += fontsize;

	/* draw magazine in proper position and color */
	auto magFn = [&](const Magazine &m) {
		if (m.cartridge != firearm.cartridge)
			return;

		int w = manual_load ? 5 : 10;
		Color col = manual_load ? WHITE : colorFn(m);

		DrawRectangle(x, y, w, mag_height, col);
		x += w + 1;
	};

	/* draw magazines */
	int size = manual_load
		? player.getCartridgeCount(cartridge)
		: player.magazines.size();
	for (int i = 0; i < size; ++i) {
		const Magazine &m = manual_load
			? firearm.getMagazine()
			: player.magazines.at(i);
		magFn(m);
	}
}

void Game::drawWave() const
{
	static const float offset_x = -48;

	int x = GetScreenWidth() * 0.5 + offset_x;
	const char *s = TextFormat("Wave %d", wave);
	DrawText(s, x, 0, 20, BLACK);
}

void Game::drawGameover() const
{
	static float fontsize = 20;
	static float fontsize_large = 30;

	float y = 0.25f * GetScreenHeight();

	auto drawFn = [&](const std::string &s) {
		drawTextCenteredX(s, y, fontsize, WHITE);
		y += fontsize;
	};

	drawTextCenteredX("Game over", y, fontsize_large, RED);
	y += fontsize_large;

	const std::string time_str = readableTime(survival_time);

	drawFn(TextFormat("You survived to wave %d", wave));
	drawFn(TextFormat("Kills: %d", kill_count));
	drawFn(TextFormat("Time: %s", time_str.c_str()));
}
