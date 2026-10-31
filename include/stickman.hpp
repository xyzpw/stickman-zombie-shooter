#pragma once

#include <raylib.h>
#include <raymath.h>
#include "utils.hpp"
#include "constants.hpp"

struct Pose {
	float torso_angle;
	float upper_left_arm_angle, upper_right_arm_angle,
	     lower_left_arm_angle, lower_right_arm_angle;
	float upper_left_leg_angle, upper_right_leg_angle,
	      lower_left_leg_angle, lower_right_leg_angle;

	void flipAll()
	{
		torso_angle = PI - torso_angle;
		flipArms();
		flipLegs();
	}

	void flipLegs()
	{
		upper_left_leg_angle = PI - upper_left_leg_angle;
		upper_right_leg_angle = PI - upper_right_leg_angle;
		lower_left_leg_angle = PI - lower_left_leg_angle;
		lower_right_leg_angle = PI - lower_right_leg_angle;
	}

	void flipArms()
	{
		upper_left_arm_angle  = PI - upper_left_arm_angle;
		upper_right_arm_angle = PI - upper_right_arm_angle;
		lower_left_arm_angle  = PI - lower_left_arm_angle;
		lower_right_arm_angle = PI - lower_right_arm_angle;
	}

	void lerpAll(const Pose &a, const Pose &b, float t)
	{
		torso_angle = lerpAngle(a.torso_angle, b.torso_angle, t);
		lerpArms(a, b, t);
		lerpLegs(a, b, t);
	}

	void lerpArms(const Pose &a, const Pose &b, float t)
	{
		upper_left_arm_angle = lerpAngle(a.upper_left_arm_angle,
						 b.upper_left_arm_angle, t);
		upper_right_arm_angle = lerpAngle(a.upper_right_arm_angle,
						  b.upper_right_arm_angle, t);
		lower_left_arm_angle = lerpAngle(a.lower_left_arm_angle,
						 b.lower_left_arm_angle, t);
		lower_right_arm_angle = lerpAngle(a.lower_right_arm_angle,
						  b.lower_right_arm_angle, t);
	}

	void lerpArms(float topleft, float topright, float lowerleft,
		      float lowerright, float t)
	{
		upper_left_arm_angle = lerpAngle(upper_left_arm_angle,
						 topleft, t);
		upper_right_arm_angle = lerpAngle(upper_right_arm_angle,
						  topright, t);
		lower_left_arm_angle = lerpAngle(lower_left_arm_angle,
						 lowerleft, t);
		lower_right_arm_angle = lerpAngle(lower_right_arm_angle,
						  lowerright, t);
	}

	void lerpLegs(const Pose &a, const Pose &b, float t)
	{
		upper_left_leg_angle = lerpAngle(a.upper_left_leg_angle,
					    b.upper_left_leg_angle, t);
		upper_right_leg_angle = lerpAngle(a.upper_right_leg_angle,
					     b.upper_right_leg_angle, t);
		lower_left_leg_angle = lerpAngle(a.lower_left_leg_angle,
					    b.lower_left_leg_angle, t);
		lower_right_leg_angle = lerpAngle(a.lower_right_leg_angle,
					     b.lower_right_leg_angle, t);
	}

	void setArmRotations(float topleft, float topright, float lowerleft,
			     float lowerright)
	{
		upper_left_arm_angle = topleft;
		upper_right_arm_angle = topright;
		lower_left_arm_angle = lowerleft;
		lower_right_arm_angle = lowerright;
	}
};

const Pose POSE_IDLE{
	toRadians(90),
	toRadians(-80), toRadians(-75),
	toRadians(-75), toRadians(-75),
	toRadians(-80), toRadians(-100),
	toRadians(-89), toRadians(-90)
};
const Pose POSE_WALK_A{
	toRadians(90),
	toRadians(-40), toRadians(-160),
	toRadians(45), toRadians(-60),
	-1.7f, -1.2f,
	-2.f, -1.4f
};
const Pose POSE_WALK_B{
	toRadians(90),
	toRadians(-145), toRadians(-70),
	toRadians(-50), toRadians(70),
	-1.2f, -1.7f,
	-1.4f, -2.f
};

struct Stickman {
	float head_radius;
	float torso_length,
	      neck_length,
	      arm_length,
	      leg_length;
	Pose pose;
	float thickness;

	void loadDefaults()
	{
		head_radius = 0.15 * METERS_TO_PIXELS;
		torso_length = 0.6 * METERS_TO_PIXELS;
		neck_length = 0.1 * METERS_TO_PIXELS;
		arm_length = 0.8 * METERS_TO_PIXELS;
		leg_length = 0.96 * METERS_TO_PIXELS;

		pose = POSE_IDLE;

		thickness = 3;
	}

	/*
	 * find position of head
	 * @hip: position at stickmans hip
	 *
	 * return: position at center of head
	 */
	Vector2 getHeadPosition(Vector2 hip) const
	{
		float deg = RAD2DEG * pose.torso_angle;
		float length = torso_length + neck_length + head_radius;

		if (FloatEquals(deg, 0))
			return {hip.x + length, hip.y};
		if (FloatEquals(deg, 90))
			return {hip.x, hip.y - length};
		if (FloatEquals(deg, 180))
			return {hip.x - length, hip.y};
		if (FloatEquals(deg, 270))
			return {hip.x, hip.y + length};
		return computeEndpoint(hip, length, pose.torso_angle);
	}

	Vector2 getGripPosition(Vector2 hip) const
	{
		Vector2 neck = computeEndpoint(hip, torso_length,
					       pose.torso_angle);
		Vector2 hi_arm = computeEndpoint(neck, 0.5f * arm_length,
						 pose.upper_right_arm_angle);
		Vector2 lo_arm = computeEndpoint(hi_arm, 0.5f * arm_length,
						 pose.lower_right_arm_angle);
		return lo_arm;
	}

	void draw(Vector2 pos) const
	{
		/* function alias for readability */
		using Alias = Vector2(*)(Vector2 start,
					 float length,
					 float rotation);
		static Alias end = computeEndpoint;

		static auto draw_fn = [&](Vector2 A, Vector2 B) {
			DrawLineEx(A, B, thickness, BLACK);
		};

		float neck_to_head_len = neck_length + head_radius;
		float arm_half_len = 0.5f * arm_length;
		float leg_half_len = 0.5f * leg_length;
		Vector2 neck = end(pos, torso_length, pose.torso_angle);
		Vector2 head = end(neck, neck_to_head_len, pose.torso_angle);
		Vector2 ul_arm = end(neck, arm_half_len,
				     pose.upper_left_arm_angle);
		Vector2 ur_arm = end(neck, arm_half_len,
				     pose.upper_right_arm_angle);
		Vector2 ll_arm = end(ul_arm, arm_half_len,
				     pose.lower_left_arm_angle);
		Vector2 lr_arm = end(ur_arm, arm_half_len,
				     pose.lower_right_arm_angle);
		Vector2 ul_leg = end(pos, leg_half_len,
				     pose.upper_left_leg_angle);
		Vector2 ur_leg = end(pos, leg_half_len,
				     pose.upper_right_leg_angle);
		Vector2 ll_leg = end(ul_leg, leg_half_len,
				     pose.lower_left_leg_angle);
		Vector2 lr_leg = end(ur_leg, leg_half_len,
				     pose.lower_right_leg_angle);

		draw_fn(pos, neck);
		DrawCircleV(head, head_radius, BLACK);
		draw_fn(neck, head);
		draw_fn(neck, ul_arm);
		draw_fn(neck, ur_arm);
		draw_fn(ul_arm, ll_arm);
		draw_fn(ur_arm, lr_arm);
		draw_fn(pos, ul_leg);
		draw_fn(pos, ur_leg);
		draw_fn(ul_leg, ll_leg);
		draw_fn(ur_leg, lr_leg);
	}
};
