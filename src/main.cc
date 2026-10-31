#include <raylib.h>
#include "game.hpp"
#include "assets.hpp"

int main()
{
	SetTraceLogLevel(LOG_WARNING);
	SetConfigFlags(FLAG_VSYNC_HINT);

	InitWindow(960, 540, "Zombie Shooter");
	InitAudioDevice();
	asset_manager.loadAssets();

	Game game;
	game.initialize();

	while (!WindowShouldClose()) {
		BeginDrawing();

		ClearBackground(game.world.background);
		game.update();

		EndDrawing();
	}

	asset_manager.unloadAssets();
	CloseAudioDevice();
	CloseWindow();
	return 0;
}
