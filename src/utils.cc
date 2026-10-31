#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include "utils.hpp"

using std::cosf, std::sinf, std::remainderf;

Vector2 computeEndpoint(Vector2 start, float length, float rotation)
{
	return {
		start.x + cosf(rotation) * length,
		start.y - sinf(rotation) * length
	};
}

Vector2 computeDirection(Vector2 start, Vector2 end)
{
	Vector2 diff = Vector2Subtract(end, start);
	return Vector2Normalize(diff);
}

Vector2 angleToDirection(float radians)
{
	return {cosf(radians), -sinf(radians)};
}

Vector2 getScreenSize()
{
	return {
		(float)GetScreenWidth(),
		(float)GetScreenHeight()
	};
}

float toRadians(float degrees)
{
	return degrees * DEG2RAD;
}

float toDegrees(float radians)
{
	return radians * RAD2DEG;
}

float lerpAngle(float a, float b, float t)
{
	float delta = remainderf(b - a, PI * 2);
	return a + delta * t;
}

bool isInDisplay(Vector2 v)
{
	bool outside = v.x < 0 && v.x > GetScreenWidth();
	outside |= v.y < 0 || v.y > GetScreenHeight();
	return !outside;
}

bool checkCollisionLineRec(Vector2 start, Vector2 end, Rectangle rec)
{
	if (CheckCollisionPointRec(start, rec) || CheckCollisionPointRec(end, rec))
		return true;

	Vector2 null{};
	auto check = [&](Vector2 a, Vector2 b) {
		return CheckCollisionLines(start, end, a, b, &null);
	};

	Vector2 topA{rec.x, rec.y};
	Vector2 topB{rec.x + rec.width, rec.y};
	Vector2 bottomA{rec.x, rec.y + rec.height};
	Vector2 bottomB{rec.x + rec.width, rec.y + rec.height};

	if (check(topA, topB))
		return true;
	if (check(bottomA, bottomB))
		return true;
	if (check(topA, bottomA))
		return true;
	if (check(topB, bottomB))
		return true;
	return false;
}

float randomFloat(float min, float max)
{
	return 1e-6 * GetRandomValue(1e6 * min, 1e6 * max);
}

std::string readableTime(float seconds)
{
	int m = seconds / 60;
	int s = seconds - m*60.f;

	return TextFormat("%d m, %d s", m, s);
}

void drawTextCenteredX(const std::string &s, int y, int fontsize, Color color)
{
	const char *s2 = s.c_str();

	int len = s.length() * 0.6f * fontsize;
	int x = 0.5f * (GetScreenWidth() - len);

	DrawText(s2, x, y, fontsize, color);
}
