#define SDL_MAIN_USE_CALLBACKS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>

#include "include/color.h"

#include "include/snes.h"
#include "include/cart.h"

#define CYCLES_PER_FRAME 357368u
#define FRAME_NS ((int64_t)(1000000000.0 / 60.0988))

// requestAnimationFrame can fire slightly early (jitter). Treat a frame as due
// if we're within this much of FRAME_NS so we don't drop a visible frame on a
// ~60 Hz display. The accumulator self-corrects on the next tick.
#define FRAME_TOLERANCE_NS ((int64_t)1500000)

// After a hidden tab / long stall, never emulate more than this many frames
// worth of time in a single tick.
#define MAX_CATCHUP_NS (FRAME_NS * 2)

static const char title[] = "HAPPY SNES!";

typedef struct app_state {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    cart* cart;
    snes* snes;

    uint64_t last_time;
    int64_t elapsed;
} app_state;

static inline void upload_frame(app_state* state, const uint32_t* buffer) {
    SDL_UpdateTexture(state->texture, NULL, buffer, SNES_SCREEN_WIDTH * (int)sizeof(uint32_t));
}

static inline void run_one_frame(snes* s) {
    unsigned int frame_time = 0;

    while (frame_time < CYCLES_PER_FRAME) {
        unsigned int cycles_elapsed = cycle_cpu(s->cpu);
        snes_run_for(s, cycles_elapsed);
        frame_time += cycles_elapsed;
    }
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
    app_state* state = calloc(1, sizeof(app_state));

    if (state == NULL) {
        return SDL_APP_FAILURE;
    }

    *appstate = state;

    if (argc < 2) {
        printf(BRED "Requires ROM.\n" COLOR_RESET);
        return SDL_APP_FAILURE;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer(title, SNES_SCREEN_WIDTH * 3, SNES_SCREEN_HEIGHT * 3, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, &state->window, &state->renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetDefaultTextureScaleMode(state->renderer, SDL_SCALEMODE_NEAREST);

    state->texture = SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, SNES_SCREEN_WIDTH, SNES_SCREEN_HEIGHT);
    if (state->texture == NULL) {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_SetTextureScaleMode(state->texture, SDL_SCALEMODE_NEAREST);

    // The browser canvas doesn't enforce a window aspect ratio, so letterbox
    // in the renderer instead. This is free and keeps pixels undistorted.
    SDL_SetRenderLogicalPresentation(state->renderer, SNES_SCREEN_WIDTH, SNES_SCREEN_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    FILE* rom_file = fopen(argv[1], "rb");

    if (rom_file == NULL) {
        printf(BRED "Failed to open ROM file.\n" COLOR_RESET);
        return SDL_APP_FAILURE;
    }

    fseek(rom_file, 0, SEEK_END);
    long rom_size = ftell(rom_file);
    fseek(rom_file, 0, SEEK_SET);

    if (rom_size <= 0) {
        printf(BRED "ROM file is empty.\n" COLOR_RESET);
        fclose(rom_file);
        return SDL_APP_FAILURE;
    }

    uint8_t* rom_buffer = malloc((size_t)rom_size);

    if (rom_buffer == NULL) {
        printf(BRED "Failed to allocate ROM memory.\n" COLOR_RESET);
        fclose(rom_file);
        return SDL_APP_FAILURE;
    }

    size_t read = fread(rom_buffer, 1, (size_t)rom_size, rom_file);
    fclose(rom_file);

    if (read != (size_t)rom_size) {
        printf(BRED "Failed to read ROM file.\n" COLOR_RESET);
        free(rom_buffer);
        return SDL_APP_FAILURE;
    }

    state->cart = new_cart(rom_buffer);
    free(rom_buffer);

    if (state->cart == NULL) {
        printf(BRED "Failed to start emulator.\n" COLOR_RESET);
        return SDL_APP_FAILURE;
    }

    state->snes = new_snes(state->cart);
    if (state->snes == NULL) {
        printf(BRED "Failed to start emulator.\n" COLOR_RESET);
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {
    app_state* state = (app_state*) appstate;

    // Emscripten calls this from requestAnimationFrame, which the browser
    // already paces to the display refresh rate and pauses in hidden tabs.
    // So: no sleeping here, just do work only when a frame is due.
    uint64_t now = SDL_GetTicksNS();

    if (state->last_time == 0) {
        state->last_time = now;
    }

    state->elapsed += (int64_t)(now - state->last_time);
    state->last_time = now;

    if (state->elapsed > MAX_CATCHUP_NS) {
        state->elapsed = MAX_CATCHUP_NS;
    }

    bool new_frame = false;

    while (state->elapsed + FRAME_TOLERANCE_NS >= FRAME_NS) {
        state->elapsed -= FRAME_NS;
        run_one_frame(state->snes);
        new_frame = true;
    }

    // Only the last emulated frame is ever visible, so upload and present once.
    if (new_frame) {
        upload_frame(state, state->snes->ppu->framebuffer);
        SDL_RenderClear(state->renderer);
        SDL_RenderTexture(state->renderer, state->texture, NULL, NULL);
        SDL_RenderPresent(state->renderer);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
        case SDL_EVENT_WINDOW_SHOWN: {
            // Don't try to "catch up" on time spent hidden/unfocused.
            app_state* state = (app_state*) appstate;
            state->last_time = 0;
            state->elapsed = 0;
            break;
        }
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    app_state* state = (app_state*) appstate;

    if (state == NULL) {
        return;
    }

    if (state->snes) {
        free_snes(state->snes);
    }

    if (state->cart) {
        free_cart(state->cart);
    }

    if (state->texture) {
        SDL_DestroyTexture(state->texture);
    }

    if (state->renderer) {
        SDL_DestroyRenderer(state->renderer);
    }

    if (state->window) {
        SDL_DestroyWindow(state->window);
    }

    free(state);
}