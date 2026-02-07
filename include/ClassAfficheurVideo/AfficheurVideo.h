#pragma once

#include "../raymedia/raymedia.h"

class AfficheurVideo {
public:
    AfficheurVideo(const char *filePath);

    ~AfficheurVideo();

    void run();

private:
    MediaStream video{};

    float duration;
};
