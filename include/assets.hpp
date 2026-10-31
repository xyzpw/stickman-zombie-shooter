#pragma once

#include <raylib.h>
#include <map>
#include <string>

struct AssetManager {
	void loadAssets();
	void unloadAssets();

	void playSound(const std::string &filename);

	Sound &getSound(const std::string &filename)
	{
		return sounds.at(filename);
	}

	Texture2D &getTexture(const std::string &filename)
	{
		return textures.at(filename);
	}

	private:
	bool loaded;
	std::map<std::string, Sound> sounds;
	std::map<std::string, Texture2D> textures;
};

extern AssetManager asset_manager;
