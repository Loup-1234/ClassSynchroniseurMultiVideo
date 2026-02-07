#include "../include/ClassSynchroniseurMultiVideo/SynchroniseurMultiVideo.h"
#include <vector>
#include <string>

using namespace std;

int main() {
    SynchroniseurMultiVideo synchro;

    // Configuration de l'analyse :
    // - Durée d'analyse : 60 secondes
    // - Plage de recherche : 30 secondes
    // - Précision (pas) : 100 (plus petit = plus précis)
    synchro.ConfigurerAnalyse(60.0, 30.0, 100);

    // Option 1 : Utiliser une vidéo comme référence (ancienne méthode)
    /*
    vector<string> mesVideos = {
        "video_ref.mp4",
        "video_angle2.mp4",
        "video_angle3.mp4",
        "video_angle4.mp4",
        "video_angle5.mp4",
        "video_angle6.mp4"
    };
    synchro.GenererVideoSynchronisee(mesVideos, "sortie_synchro.mp4");
    */

    // Option 2 : Utiliser une musique comme référence
    vector<string> mesVideos = {
        "video_angle2.mp4",
        "video_angle3.mp4",
        "video_angle4.mp4",
        "video_angle5.mp4",
        "video_angle6.mp4"
    };

    synchro.GenererVideoSynchronisee("musique_ref.mp3", mesVideos, "sortie_synchro.mp4");

    return 0;
}
