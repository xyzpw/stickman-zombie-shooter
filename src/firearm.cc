#include <raylib.h>
#include "firearm.hpp"
#include "assets.hpp"

void Firearm::processFire()
{
	trigger_down = true;
	chambered = false;
	burst_remaining--;

	switch (fire_type) {
	case SEMI_AUTO:
	case BURST_FIRE:
	case FULL_AUTO:
		if (!magazine.isEmpty())
			rechamber();
		break;
	case BOLT_ACTION:
	case PUMP_ACTION:
		magazine.bullets--;
		break;
	}

	if (!chambered && has_select_fire)
		burst_remaining = 3;
}

void Firearm::rechamber()
{
	switch (fire_type) {
	case BOLT_ACTION:
	case PUMP_ACTION:
		break;
	default:
		magazine.bullets--;
	}

	chambered = true;
}

bool Firearm::canFire() const
{
	if (!isLoaded())
		return false;
	if (trigger_down && isBurstfire() && burst_remaining == 0)
		return false;
	if (trigger_down && !isAutomatic() && !isBurstfire())
		return false;

	return chambered;
}

Sound &Firearm::getSound() const
{
	return asset_manager.getSound(getSoundName());
}

Texture2D &Firearm::getTexture() const
{
	return asset_manager.getTexture(getTextureName());
}
