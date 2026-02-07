/**
 * @file SynchroniseurMultiVideo.cpp
 * @brief Implémentation de la classe SynchroniseurMultiVideo.
 *
 * Ce fichier contient le code source des méthodes de la classe SynchroniseurMultiVideo,
 * incluant l'extraction audio via FFmpeg, le chargement des données brutes,
 * le calcul de corrélation croisée et la génération de la vidéo finale.
 */

#include "../include/ClassSynchroniseurMultiVideo/SynchroniseurMultiVideo.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

using namespace std;

SynchroniseurMultiVideo::~SynchroniseurMultiVideo() {
    remove(TEMP_AUDIO_REF.c_str());
    remove(TEMP_AUDIO_CIBLE.c_str());
}

void SynchroniseurMultiVideo::ConfigurerAnalyse(double duree, double plage, int pas) {
    if (duree > 0) dureeAnalyse = duree;
    if (plage > 0) plageRechercheMax = plage;
    if (pas > 0) pasDePrecision = pas;
}

void SynchroniseurMultiVideo::ExtraireAudio(const string &fichierVideo, const string &fichierAudioSortie) const {
    stringstream cmd;

    // Construction de la commande FFmpeg pour extraire l'audio.
    // -y: Écrase le fichier de sortie s'il existe déjà.
    // -hide_banner: Supprime l'affichage de la bannière FFmpeg.
    // -loglevel error: N'affiche que les messages d'erreur graves.
    // -i: Spécifie le fichier vidéo d'entrée.
    // -vn: Ignore le flux vidéo (évite le décodage inutile).
    // -f f32le: Définit le format de sortie de l'audio en float 32-bit little-endian.
    // -ac 1: Convertit l'audio en mono.
    // -ar FREQUENCE_ECHANTILLONNAGE: Définit la fréquence d'échantillonnage de l'audio.
    // -t dureeAnalyse: Traite seulement les X premières secondes de la vidéo.

    cmd << "ffmpeg -y -hide_banner -loglevel error "
            << "-i \"" << fichierVideo << "\" "
            << "-vn "
            << "-f f32le -ac 1 -ar " << FREQUENCE_ECHANTILLONNAGE << " -t " << dureeAnalyse << " "
            << "\"" << fichierAudioSortie << "\"";

    // Exécute la commande FFmpeg.
    if (system(cmd.str().c_str()) != 0) throw runtime_error("Impossible d'extraire l'audio de : " + fichierVideo);
}

vector<float> SynchroniseurMultiVideo::ChargerAudioBrut(const string &nomFichier) {
    // Ouvre le fichier audio en mode binaire et se positionne à la fin pour obtenir la taille
    ifstream fichier(nomFichier, ios::binary | ios::ate);

    vector<float> echantillons;

    if (!fichier.is_open()) throw runtime_error("Impossible d'ouvrir le fichier : " + nomFichier);

    // Récupère la taille du fichier
    streamsize taille = fichier.tellg();
    // Repositionne le curseur au début du fichier
    fichier.seekg(0, ios::beg);
    // Calcule le nombre d'éléments que le fichier contient
    int nbElements = taille / sizeof(float);

    // Redimensionne le vecteur pour accueillir tous les échantillons
    echantillons.resize(nbElements);

    // Si le fichier n'est pas vide, lit les données et remplit le vecteur "echantillons"
    if (nbElements > 0) {
        fichier.read(reinterpret_cast<char *>(echantillons.data()), taille);
        if (!fichier) throw runtime_error("Erreur de lecture du fichier : " + nomFichier);
    }

    return echantillons;
}

double SynchroniseurMultiVideo::CalculerDecalage(const vector<float> &ref, const vector<float> &cible) const {
    try {
        if (ref.empty() || cible.empty()) return 0.0;

        // Détermine la taille minimale des deux vecteurs pour éviter les débordements
        const int n = min(ref.size(), cible.size());

        double maxCorr = -1.0;
        int meilleurDecalage = 0;

        // Définit la plage de recherche pour le décalage en échantillons
        const int plageRecherche = static_cast<int>(FREQUENCE_ECHANTILLONNAGE * plageRechercheMax);

        // Boucle de corrélation croisée
        for (int retard = -plageRecherche; retard < plageRecherche; retard += 20) {
            // On ne vérifie pas chaque échantillon, on saute de pasDePrecision en pasDePrecision pour aller plus vitesse
            const int pas = pasDePrecision;

            // On ne compare que le tiers central
            const int debutScan = n / 3;
            const int finScan = 2 * n / 3;

            double corrActuelle = 0;

            for (int i = debutScan; i < finScan; i += pas) {
                // Calcule l'indice correspondant dans le vecteur cible en appliquant le retard
                int j = i + retard;

                if (j >= 0 && j < cible.size()) {
                    // Accumule le produit des échantillons pour la corrélation
                    corrActuelle += ref[i] * cible[j];
                }
            }

            // On garde le meilleur score
            // Plus la corrélation est élevée, mieux c'est
            if (corrActuelle > maxCorr) {
                maxCorr = corrActuelle;
                meilleurDecalage = retard;
            }
        }

        // Conversion échantillons -> secondes
        return static_cast<double>(meilleurDecalage) / FREQUENCE_ECHANTILLONNAGE;
    } catch (const exception &e) {
        cerr << "[Erreur Calcul] " << e.what() << endl;
        return 0.0;
    }
}

bool SynchroniseurMultiVideo::GenererVideoSynchronisee(const vector<string> &fichiersEntree,
                                                       const string &fichierSortie) const {
    try {
        if (fichiersEntree.size() < 2) {
            throw runtime_error("Il faut fournir au moins 2 fichiers video.");
        }

        cout << "Traitement de " << fichiersEntree.size() << " videos" << endl;

        cout << "[1/3] Analyse de la reference video..." << endl;

        try {
            ExtraireAudio(fichiersEntree[0], TEMP_AUDIO_REF);
        } catch (const exception &e) {
            throw runtime_error(string("Erreur reference: ") + e.what());
        }

        vector<float> audioRef = ChargerAudioBrut(TEMP_AUDIO_REF);

        if (audioRef.empty()) {
            throw runtime_error("Fichier audio reference vide ou illisible.");
        }

        vector<InfoVideo> listeVideos;

        // Ajoute la vidéo de référence à la liste avec un décalage de 0.
        listeVideos.push_back({fichiersEntree[0], 0.0});

        // Boucle sur les vidéos cibles (à partir de la deuxième).
        for (int i = 1; i < fichiersEntree.size(); ++i) {
            cout << "[2/3] Analyse video " << i+1 << " : " << flush;

            try {
                // Extrait l'audio de la vidéo cible actuelle dans un fichier temporaire.
                ExtraireAudio(fichiersEntree[i], TEMP_AUDIO_CIBLE);

                vector<float> audioCible = ChargerAudioBrut(TEMP_AUDIO_CIBLE);

                double decalage = CalculerDecalage(audioRef, audioCible);

                // Ajoute la vidéo à la liste avec son décalage.
                listeVideos.push_back({fichiersEntree[i], decalage});

                cout << "OK (Retard: " << fixed << setprecision(3) << decalage << "s)" << endl;
            } catch (const exception &e) {
                cout << "Echec (" << e.what() << ") - Video ignoree" << endl;
            }
        }

        cout << "[3/3] Generation de la video finale..." << endl;

        stringstream cmd;

        // Commande FFmpeg de base avec options pour écraser la sortie, masquer la bannière et afficher les avertissements
        cmd << "ffmpeg -y -hide_banner -loglevel warning ";

        // Ajoute chaque vidéo d'entrée à la commande FFmpeg
        for (const auto &vid: listeVideos) {
            // Si un décalage est détecté, applique l'option -ss pour démarrer la vidéo à partir de ce décalage
            if (vid.retardSecondes > 0) cmd << "-ss " << vid.retardSecondes << " ";
            // Ajoute le chemin de la vidéo en tant qu'entrée
            cmd << "-i \"" << vid.chemin << "\" ";
        }

        // Début de la chaîne de filtres complexes
        cmd << "-filter_complex \"";

        // Pour chaque vidéo, redimensionne la hauteur à HAUTEUR_CIBLE et ajuste la largeur proportionnellement (-2)
        for (int i = 0; i < listeVideos.size(); ++i)
            cmd << "[" << i << ":v]scale=-2:" << HAUTEUR_CIBLE << "[v" << i << "];";

        // Concatène toutes les vidéos redimensionnées horizontalement (hstack)
        for (int i = 0; i < listeVideos.size(); ++i) cmd << "[v" << i << "]";

        // Applique le filtre hstack avec le nombre d'entrées correspondant au nombre de vidéos
        cmd << "hstack=inputs=" << listeVideos.size() << "[vout]\" ";

        // Mappe la sortie vidéo du filtre complexe et le flux audio de la première vidéo (référence)
        cmd << "-map \"[vout]\" -map 0:a ";
        // Spécifie l'encodeur vidéo (libx264), le format de pixel (yuv420p) pour la compatibilité, le préréglage d'encodage (fast) et le fichier de sortie
        cmd << "-c:v libx264 -pix_fmt yuv420p -preset fast \"" << fichierSortie << "\"";

        // Exécute la commande FFmpeg
        if (system(cmd.str().c_str()) == 0) {
            cout << "[Succes] Fichier genere : " << fichierSortie << endl;
            return true;
        }

        throw runtime_error("Une erreur est survenue lors de l'encodage FFMPEG.");
    } catch (const exception &e) {
        cerr << "[Erreur] " << e.what() << endl;
        return false;
    }
}

bool SynchroniseurMultiVideo::GenererVideoSynchronisee(const string &fichierAudioRef,
                                                       const vector<string> &fichiersVideo,
                                                       const string &fichierSortie) const {
    try {
        if (fichiersVideo.empty()) {
            throw runtime_error("Il faut fournir au moins 1 fichier video.");
        }

        cout << "[1/3] Analyse de la reference audio..." << endl;

        try {
            ExtraireAudio(fichierAudioRef, TEMP_AUDIO_REF);
        } catch (const exception &e) {
            throw runtime_error(string("Erreur reference audio: ") + e.what());
        }

        vector<float> audioRef = ChargerAudioBrut(TEMP_AUDIO_REF);

        if (audioRef.empty()) {
            throw runtime_error("Fichier audio reference vide ou illisible.");
        }

        vector<InfoVideo> listeVideos;

        // Boucle sur les vidéos cibles.
        for (int i = 0; i < fichiersVideo.size(); ++i) {
            cout << "[2/3] Analyse video " << i+1 << " : " << flush;

            try {
                // Extrait l'audio de la vidéo cible actuelle dans un fichier temporaire.
                ExtraireAudio(fichiersVideo[i], TEMP_AUDIO_CIBLE);

                vector<float> audioCible = ChargerAudioBrut(TEMP_AUDIO_CIBLE);

                double decalage = CalculerDecalage(audioRef, audioCible);

                // Ajoute la vidéo à la liste avec son décalage.
                listeVideos.push_back({fichiersVideo[i], decalage});

                cout << "OK (Retard: " << fixed << setprecision(3) << decalage << "s)" << endl;
            } catch (const exception &e) {
                cout << "Echec (" << e.what() << ") - Video ignoree" << endl;
            }
        }

        cout << "[3/3] Generation de la video finale..." << endl;

        stringstream cmd;

        // Commande FFmpeg de base avec options pour écraser la sortie, masquer la bannière et afficher les avertissements
        cmd << "ffmpeg -y -hide_banner -loglevel warning ";

        // Input 0: Audio Ref
        cmd << "-i \"" << fichierAudioRef << "\" ";

        // Inputs 1..N: Videos
        for (const auto &vid: listeVideos) {
            if (vid.retardSecondes > 0) cmd << "-ss " << vid.retardSecondes << " ";
            cmd << "-i \"" << vid.chemin << "\" ";
        }

        // Début de la chaîne de filtres complexes
        cmd << "-filter_complex \"";

        // Pour chaque vidéo, redimensionne la hauteur à HAUTEUR_CIBLE et ajuste la largeur proportionnellement (-2)
        // Les index des vidéos commencent maintenant à 1 (car 0 est l'audio)
        for (int i = 0; i < listeVideos.size(); ++i)
            cmd << "[" << (i + 1) << ":v]scale=-2:" << HAUTEUR_CIBLE << "[v" << i << "];";

        // Concatène toutes les vidéos redimensionnées horizontalement (hstack)
        for (int i = 0; i < listeVideos.size(); ++i) cmd << "[v" << i << "]";

        // Applique le filtre hstack avec le nombre d'entrées correspondant au nombre de vidéos
        cmd << "hstack=inputs=" << listeVideos.size() << "[vout]\" ";

        // Mappe la sortie vidéo du filtre complexe et le flux audio de la première entrée (référence audio)
        cmd << "-map \"[vout]\" -map 0:a ";
        // Spécifie l'encodeur vidéo (libx264), le format de pixel (yuv420p) pour la compatibilité, le préréglage d'encodage (fast) et le fichier de sortie
        cmd << "-c:v libx264 -pix_fmt yuv420p -preset fast \"" << fichierSortie << "\"";

        // Exécute la commande FFmpeg
        if (system(cmd.str().c_str()) == 0) {
            cout << "[Succes] Fichier genere : " << fichierSortie << endl;
            return true;
        }

        throw runtime_error("Une erreur est survenue lors de l'encodage FFMPEG.");
    } catch (const exception &e) {
        cerr << "[Erreur] " << e.what() << endl;
        return false;
    }
}
