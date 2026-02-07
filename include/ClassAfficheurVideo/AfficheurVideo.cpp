#define RAYGUI_IMPLEMENTATION
#include <raygui.h>
#include "AfficheurVideo.h"

AfficheurVideo::AfficheurVideo(const char* filePath) {
    InitWindow(800, 450, "Video Player");
    InitAudioDevice();
    video = LoadMedia(filePath);
    duration = IsMediaValid(video) ? static_cast<float>(GetMediaProperties(video).durationSec) : 0.0f;
    if (IsMediaValid(video)) SetWindowSize(video.videoTexture.width, video.videoTexture.height + 50);
    SetTargetFPS(60);
}

AfficheurVideo::~AfficheurVideo() {
    UnloadMedia(&video);
    CloseAudioDevice();
    CloseWindow();
}

void AfficheurVideo::run() {
    bool dragging = false;
    bool wasPlaying = false;
    float sliderValue = 0.0f;
    float seekDelay = 0.0f;

    while (!WindowShouldClose()) {
        UpdateMedia(&video);
        if (seekDelay > 0) seekDelay -= GetFrameTime();

        if (!dragging && seekDelay <= 0 && IsMediaValid(video))
            sliderValue = static_cast<float>(GetMediaPosition(video));

        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (IsMediaValid(video)) {
            DrawTexture(video.videoTexture, 0, 0, WHITE);
            float y = static_cast<float>(video.videoTexture.height) + 10;

            if (GuiButton((Rectangle){10, y, 50, 30}, "Play")) SetMediaState(video, MEDIA_STATE_PLAYING);
            if (GuiButton((Rectangle){70, y, 50, 30}, "Pause")) SetMediaState(video, MEDIA_STATE_PAUSED);

            const Rectangle sliderRect = {130, y, static_cast<float>(GetScreenWidth()) - 140, 30};

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), sliderRect)) {
                dragging = true;
                wasPlaying = (GetMediaState(video) == MEDIA_STATE_PLAYING);
                if (wasPlaying) SetMediaState(video, MEDIA_STATE_PAUSED);
            }

            GuiSlider(sliderRect, "", "", &sliderValue, 0.0f, duration);

            if (dragging && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                dragging = false;
                SetMediaPosition(video, (double)sliderValue);
                if (wasPlaying) SetMediaState(video, MEDIA_STATE_PLAYING);
                seekDelay = 0.5f;
            }
        }
        EndDrawing();
    }
}