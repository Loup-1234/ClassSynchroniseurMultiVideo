#include "../include/ClassSynchroniseurMultiVideo/SynchroniseurMultiVideo.h"
#include <vector>
#include <string>

using namespace std;

int main() {
    SynchroniseurMultiVideo synchro;

    vector<string> mesVideos = {
        "video_ref.mp4",
        "video_angle2.mp4",
        "video_angle3.mp4",
        "video_angle4.mp4",
        "video_angle5.mp4",
        "video_angle6.mp4"
    };

    synchro.GenererVideoSynchronisee(mesVideos, "sortie_synchro.mp4");

    return 0;
}
