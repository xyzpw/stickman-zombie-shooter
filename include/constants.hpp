#pragma once

#include <string>
#include <raylib.h>

constexpr float METERS_TO_PIXELS = 16 / 0.3048f;
constexpr float GRAVITY = 9.8f * METERS_TO_PIXELS;

constexpr Color DAY_COLOR = SKYBLUE;
constexpr Color NIGHT_COLOR{7, 11, 52, 255};
constexpr Color WHITE_SMOKE{245, 245, 245, 255};

constexpr Color MAGAZINE_COLOR_A{255, 255, 0, 255};
constexpr Color MAGAZINE_COLOR_B{255, 0, 0, 255};

constexpr Vector2 PISTOL_CENTER_BARREL_OFFSET{7.f, -3.5f};
constexpr Vector2 PISTOL_CENTER_GRIP_OFFSET{-2.5f, 0};
constexpr Vector2 RIFLE_CENTER_BARREL_OFFSET{29.f, -4.f};
constexpr Vector2 M16_CENTER_BARREL_OFFSET{22.5f, -4};

const std::string PISTOL_SOUND_NAME = "m1911.wav";
const std::string PISTOL_TEXTURE_NAME = "pistol.png";
const std::string RIFLE_SOUND_NAME = "bolt_action.wav";
const std::string RIFLE_RECHAMBER_SOUND_NAME = "rifle_rechamber.wav";
const std::string M1_GARAND_SOUND_NAME = "m1garand.wav";
const std::string M1_GARAND_PING_SOUND_NAME = "m1garand_ping.wav";
const std::string BOLT_ACTION_TEXTURE_NAME = "bolt_action.png";
const std::string M16_SOUND_NAME = "m16.wav";
const std::string M16_TEXTURE_NAME = "m16.png";

// how long the reload key has to be held before checking ammo
constexpr float RELOAD_CHECK_AMMO_THRESHOLD = 0.5f;
