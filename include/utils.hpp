#pragma once

#include <raylib.h>
#include <string>

Vector2 computeEndpoint(Vector2 start, float length, float rotation);
Vector2 computeDirection(Vector2 start, Vector2 end);
Vector2 angleToDirection(float radians);

Vector2 getScreenSize();

float toRadians(float degrees);
float toDegrees(float radians);
float lerpAngle(float a, float b, float t);

/*
 * return: true if position is inside display
 */
bool isInDisplay(Vector2 v);

bool checkCollisionLineRec(Vector2 start, Vector2 end, Rectangle rec);

float randomFloat(float min, float max);

std::string readableTime(float seconds);

void drawTextCenteredX(const std::string &s, int y, int fontsize, Color color);
