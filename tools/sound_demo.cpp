//=====================================================================
//
// Sound Demo - GameSound.h audio playback demonstration
//
// This demo shows how to use GameSound.h to play WAV files
// with multi-channel support and per-channel volume control.
//
// Controls:
//   Number keys 1-8: Play explosion sounds
//   Number keys 9-0: Play shoot sounds  
//   Q-W-E-R-T: Play spawn sounds
//   A/S/D/F: Adjust volume of last played channel
//   Space: Stop all sounds
//   ESC: Exit
//
// Compile command (waveOut, default on Windows):
//   g++ -o sound_demo.exe sound_demo.cpp -mconsole -lwinmm
//
// Compile with SDL2 backend:
//   g++ -o sound_demo.exe sound_demo.cpp -I<SDL2>/include -L<SDL2>/lib -lSDL2 -lwinmm -DGAMESOUND_USE_SDL=1
//
//=====================================================================
#define GAMESOUND_DEBUG 1

// Uncomment the next line to use SDL2 audio backend instead of waveOut:
#define GAMESOUND_USE_SDL 1

#define GAMESOUND_IMPLEMENTATION
#include "../GameSound.h"

#define GAMELIB_IMPLEMENTATION
#include "../GameLib.h"

#include <stdio.h>
#include <string.h>

#define DEBUG_PRINT(fmt, ...) do { \
    if (GAMESOUND_DEBUG) printf("[DEMO] " fmt "\n", ##__VA_ARGS__); \
} while(0)

// Sound file paths
static const char* g_explosion_sounds[] = {
    "../GeometryWars/assets/explosion-01.wav",
    "../GeometryWars/assets/explosion-02.wav",
    "../GeometryWars/assets/explosion-03.wav",
    "../GeometryWars/assets/explosion-04.wav",
    "../GeometryWars/assets/explosion-05.wav",
    "../GeometryWars/assets/explosion-06.wav",
    "../GeometryWars/assets/explosion-07.wav",
    "../GeometryWars/assets/explosion-08.wav",
};

static const char* g_shoot_sounds[] = {
    "../GeometryWars/assets/shoot-01.wav",
    "../GeometryWars/assets/shoot-02.wav",
    "../GeometryWars/assets/shoot-03.wav",
    "../GeometryWars/assets/shoot-04.wav",
};

static const char* g_spawn_sounds[] = {
    "../GeometryWars/assets/spawn-01.wav",
    "../GeometryWars/assets/spawn-02.wav",
    "../GeometryWars/assets/spawn-03.wav",
    "../GeometryWars/assets/spawn-04.wav",
    "../GeometryWars/assets/spawn-05.wav",
    "../GeometryWars/assets/spawn-06.wav",
    "../GeometryWars/assets/spawn-07.wav",
    "../GeometryWars/assets/spawn-08.wav",
};

static const int g_explosion_count = sizeof(g_explosion_sounds) / sizeof(const char*);
static const int g_shoot_count = sizeof(g_shoot_sounds) / sizeof(const char*);
static const int g_spawn_count = sizeof(g_spawn_sounds) / sizeof(const char*);

// Draw text with background
static void DrawTextBG(GameLib& game, int x, int y, const char* text, int text_color, int bg_color) {
    int len = (int)strlen(text);
    int char_width = 12;
    int padding = 4;
    game.FillRect(x - padding, y - 2, len * char_width + padding * 2, 18, bg_color);
    game.DrawText(x, y, text, text_color);
}

int main() {
    DEBUG_PRINT("=== GameSound Demo Starting ===");
    DEBUG_PRINT("GAMESOUND_DEBUG=%d", GAMESOUND_DEBUG);

    GameLib game;
    game.Open(800, 600, "GameSound Demo", true);

    DEBUG_PRINT("GameLib initialized");

    GameSound sound;

    DEBUG_PRINT("GameSound initialized, IsInitialized=%d", sound.IsInitialized());

    int last_channel = 0;
    int current_volume = 1000;
    int channel_count = 0;

    while (!game.IsClosed()) {
        game.Clear(COLOR_BLACK);

        // Title
        game.DrawText(20, 20, "GameSound Multi-Channel Audio Demo", COLOR_CYAN);
        game.DrawText(20, 45, "Press keys to play sounds:", COLOR_WHITE);

        // Explosion sounds (1-8)
        int y = 80;
        DrawTextBG(game, 40, y, "[1-8] Explosion Sounds", COLOR_YELLOW, 0x333300);
        y += 25;
        for (int i = 0; i < g_explosion_count; i++) {
            char buf[64];
            sprintf(buf, "  %d: explosion-%02d.wav", i + 1, i + 1);
            int color = (i < 8) ? COLOR_LIGHT_GRAY : COLOR_DARK_GRAY;
            game.DrawText(60, y, buf, color);
            y += 20;
        }

        // Shoot sounds (9-0)
        y += 10;
        DrawTextBG(game, 40, y, "[9-0] Shoot Sounds", COLOR_GREEN, 0x003300);
        y += 25;
        for (int i = 0; i < g_shoot_count; i++) {
            char buf[64];
            int key = (i == 0) ? 9 : (i == 1) ? 0 : 0;
            if (i == 0) sprintf(buf, "  9: shoot-01.wav");
            else if (i == 1) sprintf(buf, "  0: shoot-02.wav");
            else if (i == 2) sprintf(buf, "  (shoot-03.wav - no key)");
            else if (i == 3) sprintf(buf, "  (shoot-04.wav - no key)");
            int color = (i < 2) ? COLOR_LIGHT_GRAY : COLOR_DARK_GRAY;
            game.DrawText(60, y, buf, color);
            y += 20;
        }

        // Spawn sounds (Q-T)
        y += 10;
        DrawTextBG(game, 40, y, "[Q-T] Spawn Sounds", COLOR_MAGENTA, 0x330033);
        y += 25;
        const char spawn_keys[] = {'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I'};
        for (int i = 0; i < g_spawn_count && i < 8; i++) {
            char buf[64];
            sprintf(buf, "  %c: spawn-%02d.wav", spawn_keys[i], i + 1);
            game.DrawText(60, y, buf, COLOR_LIGHT_GRAY);
            y += 20;
        }

        // Controls
        y += 15;
        DrawTextBG(game, 40, y, "Controls", COLOR_WHITE, 0x222222);
        y += 25;
        game.DrawText(60, y, "A/S: Volume -/+ (last channel)", COLOR_LIGHT_GRAY);
        y += 20;
        game.DrawText(60, y, "D:   Set volume to 50%", COLOR_LIGHT_GRAY);
        y += 20;
        game.DrawText(60, y, "F:   Set volume to 100%", COLOR_LIGHT_GRAY);
        y += 20;
        game.DrawText(60, y, "Space: Stop all sounds", COLOR_LIGHT_GRAY);
        y += 20;
        game.DrawText(60, y, "ESC: Exit", COLOR_LIGHT_GRAY);

        // Status panel
        int panel_x = 420;
        int panel_y = 80;
        game.FillRect(panel_x - 10, panel_y - 5, 360, 200, 0x111111);
        game.DrawRect(panel_x - 10, panel_y - 5, 360, 200, COLOR_GRAY);

        game.DrawText(panel_x, panel_y, "Status", COLOR_CYAN);
        panel_y += 30;

        char status_buf[128];
        sprintf(status_buf, "Last Channel ID: %d", last_channel);
        game.DrawText(panel_x + 20, panel_y, status_buf, COLOR_LIGHT_GRAY);
        panel_y += 25;

        sprintf(status_buf, "Current Volume: %d / 1000", current_volume);
        game.DrawText(panel_x + 20, panel_y, status_buf, COLOR_LIGHT_GRAY);
        panel_y += 25;

        // Count active channels
        channel_count = 0;
        sprintf(status_buf, "Active Channels: %d", channel_count);
        game.DrawText(panel_x + 20, panel_y, status_buf, COLOR_LIGHT_GRAY);

        // Handle input
        if (game.IsKeyPressed(KEY_ESCAPE)) {
            DEBUG_PRINT("ESC pressed, exiting");
            break;
        }

        // Explosion sounds (1-8)
        for (int i = 0; i < 8; i++) {
            int keys[] = {KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8};
            if (game.IsKeyPressed(keys[i])) {
                DEBUG_PRINT("Playing explosion-%02d.wav", i + 1);
                last_channel = sound.PlayWAV(g_explosion_sounds[i], 1, current_volume);
                DEBUG_PRINT("  -> channel=%d", last_channel);
            }
        }

        // Shoot sounds (9, 0)
        if (game.IsKeyPressed(KEY_9)) {
            DEBUG_PRINT("Playing shoot-01.wav");
            last_channel = sound.PlayWAV(g_shoot_sounds[0], 1, current_volume);
            DEBUG_PRINT("  -> channel=%d", last_channel);
        }
        if (game.IsKeyPressed(KEY_0)) {
            DEBUG_PRINT("Playing shoot-02.wav");
            last_channel = sound.PlayWAV(g_shoot_sounds[1], 1, current_volume);
            DEBUG_PRINT("  -> channel=%d", last_channel);
        }

        // Spawn sounds (Q-T)
        int spawn_keys_code[] = {KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I};
        for (int i = 0; i < g_spawn_count && i < 8; i++) {
            if (game.IsKeyPressed(spawn_keys_code[i])) {
                DEBUG_PRINT("Playing spawn-%02d.wav", i + 1);
                last_channel = sound.PlayWAV(g_spawn_sounds[i], 1, current_volume);
                DEBUG_PRINT("  -> channel=%d", last_channel);
            }
        }

        // Volume controls
        if (game.IsKeyPressed(KEY_A)) {
            current_volume -= 100;
            if (current_volume < 0) current_volume = 0;
            if (last_channel > 0) sound.SetVolume(last_channel, current_volume);
            DEBUG_PRINT("Volume down: %d", current_volume);
        }
        if (game.IsKeyPressed(KEY_S)) {
            current_volume += 100;
            if (current_volume > 1000) current_volume = 1000;
            if (last_channel > 0) sound.SetVolume(last_channel, current_volume);
            DEBUG_PRINT("Volume up: %d", current_volume);
        }
        if (game.IsKeyPressed(KEY_D)) {
            current_volume = 500;
            if (last_channel > 0) sound.SetVolume(last_channel, current_volume);
            DEBUG_PRINT("Volume set to 50%%");
        }
        if (game.IsKeyPressed(KEY_F)) {
            current_volume = 1000;
            if (last_channel > 0) sound.SetVolume(last_channel, current_volume);
            DEBUG_PRINT("Volume set to 100%%");
        }

        // Stop all
        if (game.IsKeyPressed(KEY_SPACE)) {
            DEBUG_PRINT("StopAll called");
            sound.StopAll();
        }

        game.Update();
        game.WaitFrame(60);
    }

    DEBUG_PRINT("Demo exiting");
    return 0;
}
