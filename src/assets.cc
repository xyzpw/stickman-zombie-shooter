#include <raylib.h>
#include <filesystem>
#include "assets.hpp"

AssetManager asset_manager{};

void AssetManager::loadAssets()
{
	if (loaded)
		return;

	namespace fs = std::filesystem;

	auto iter = fs::directory_iterator("assets/");
	for (auto &it : iter) {
		if (!it.is_regular_file())
			continue;

		const fs::path &path = it.path();
		const std::string filename_str = path.filename().string();
		const std::string ext_str = path.extension().string();

		if (ext_str == ".wav") {
			Sound snd = LoadSound(path.c_str());
			sounds[filename_str] = snd;
		} else if (ext_str == ".png") {
			Texture2D tex = LoadTexture(path.c_str());
			textures[filename_str] = tex;
		}
	}

	loaded = true;
}

void AssetManager::unloadAssets()
{
	if (!loaded)
		return;

	/* sounds */
	for (auto &[name, snd] : sounds) {
		UnloadSound(snd);
	}
	sounds.clear();

	/* textures */
	for (auto &[name, tex] : textures) {
		UnloadTexture(tex);
	}
	textures.clear();

	loaded = false;
}

void AssetManager::playSound(const std::string &filename)
{
	if (!loaded)
		return;

	PlaySound(sounds.at(filename));
}
