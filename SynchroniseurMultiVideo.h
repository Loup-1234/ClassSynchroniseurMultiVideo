#pragma once

#include <string>
#include <vector>

using namespace std;

class SynchroniseurMultiVideo {
    const int FREQUENCE_ECHANTILLONNAGE = 40000;
    const int HAUTEUR_CIBLE = 480;
    const string TEMP_AUDIO_REF = "temp_ref.raw";
    const string TEMP_AUDIO_CIBLE = "temp_cible.raw";

    struct InfoVideo {
        string chemin;
        double retardSecondes;
    };

    void ExtraireAudio(const string &fichierVideo, const string &fichierAudioSortie) const;

    static vector<float> ChargerAudioBrut(const string &nomFichier);

    double CalculerDecalage(const vector<float> &ref, const vector<float> &cible) const;

public:
    ~SynchroniseurMultiVideo();

    bool GenererVideoSynchronisee(const vector<string> &fichiersEntree, const string &fichierSortie) const;
};
