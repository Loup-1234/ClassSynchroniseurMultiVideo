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
#include <cmath>

using namespace std;

SynchroniseurMultiVideo::~SynchroniseurMultiVideo() {
    remove(TEMP_AUDIO_REF.c_str());
    remove(TEMP_AUDIO_CIBLE.c_str());
}

void SynchroniseurMultiVideo::configurerAnalyse(double duree, double plage, int pas) {
    if (duree > 0) dureeAnalyse = duree;
    if (plage > 0) plageRechercheMax = plage;
    if (pas > 0) pasDePrecision = pas;
}

void SynchroniseurMultiVideo::extraireAudio(const string &fichierVideo, const string &fichierAudioSortie) const {
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

vector<float> SynchroniseurMultiVideo::chargerAudioBrut(const string &nomFichier) {
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

double SynchroniseurMultiVideo::calculerDecalage(const vector<float> &ref, const vector<float> &cible) const {
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

bool SynchroniseurMultiVideo::genererVideo(const vector<InfoVideo> &listeVideos, const string &fichierSortie, const string &fichierAudioRef) const {
    cout << "[3/3] Generation de la video finale..." << endl;

    stringstream cmd;

    // Commande FFmpeg de base avec options pour écraser la sortie, masquer la bannière et afficher les avertissements
    cmd << "ffmpeg -y -hide_banner -loglevel warning ";

    // Si un fichier audio de référence est fourni, on l'ajoute en premier
    if (!fichierAudioRef.empty()) {
        cmd << "-i \"" << fichierAudioRef << "\" ";
    }

    // Ajoute chaque vidéo d'entrée à la commande FFmpeg
    for (const auto &vid: listeVideos) {
        // Si un décalage est détecté, applique l'option -ss pour démarrer la vidéo à partir de ce décalage
        if (vid.retardSecondes > 0) cmd << "-ss " << vid.retardSecondes << " ";
        // Ajoute le chemin de la vidéo en tant qu'entrée
        cmd << "-i \"" << vid.chemin << "\" ";
    }

    // Début de la chaîne de filtres complexes
    cmd << "-filter_complex \"";

    // Détermine l'index de départ pour les vidéos (si audio ref externe, les vidéos commencent à 1)
    int indexVideoStart = fichierAudioRef.empty() ? 0 : 1;
    int nbVideos = listeVideos.size();

    // Calcul de la grille (lignes x colonnes)
    // On veut une grille aussi carrée que possible ou rectangulaire (ex: 2x2, 3x2, 3x3)
    int cols = ceil(sqrt(nbVideos));
    int rows = ceil((double)nbVideos / cols);

    // Pour chaque vidéo, redimensionne à la taille cible
    for (int i = 0; i < nbVideos; ++i) {
        cmd << "[" << (i + indexVideoStart) << ":v]scale=" << LARGEUR_CIBLE << ":" << HAUTEUR_CIBLE << "[v" << i << "];";
    }

    // Construction de la grille avec xstack
    // xstack permet de positionner les vidéos dans une grille
    // layout=0_0|w0_0|0_h0|w0_h0...
    // Mais une méthode plus simple pour une grille régulière est d'utiliser hstack et vstack combinés,
    // ou xstack avec une configuration de layout automatique si on connait les positions.
    // Pour simplifier et être robuste, on va utiliser xstack avec layout dynamique.

    cmd << "";
    for (int i = 0; i < nbVideos; ++i) {
        cmd << "[v" << i << "]";
    }

    cmd << "xstack=inputs=" << nbVideos << ":fill=black:layout=";

    for (int i = 0; i < nbVideos; ++i) {
        int r = i / cols;
        int c = i % cols;

        if (i > 0) cmd << "|";

        // Position X
        if (c == 0) cmd << "0";
        else {
            for (int k=0; k<c; ++k) {
                if (k > 0) cmd << "+";
                cmd << "w0"; // On suppose toutes les largeurs identiques (w0)
            }
        }

        cmd << "_";

        // Position Y
        if (r == 0) cmd << "0";
        else {
            for (int k=0; k<r; ++k) {
                if (k > 0) cmd << "+";
                cmd << "h0"; // On suppose toutes les hauteurs identiques (h0)
            }
        }
    }

    cmd << "[vout]\" ";

    // Mappe la sortie vidéo du filtre complexe et le flux audio de la première entrée (référence)
    cmd << "-map \"[vout]\" -map 0:a ";
    // Spécifie l'encodeur vidéo (libx264), le format de pixel (yuv420p) pour la compatibilité, le préréglage d'encodage (fast) et le fichier de sortie
    cmd << "-c:v libx264 -profile:v baseline -bf 0 -g 30 -pix_fmt yuv420p -preset fast \"" << fichierSortie << "\"";

    // Exécute la commande FFmpeg
    if (system(cmd.str().c_str()) == 0) {
        cout << "[Succes] Fichier genere : " << fichierSortie << endl;
        return true;
    }

    throw runtime_error("Une erreur est survenue lors de l'encodage FFMPEG.");
}

bool SynchroniseurMultiVideo::genererVideoSynchronisee(const vector<string> &fichiersEntree,
                                                       const string &fichierSortie) const {
    try {
        if (fichiersEntree.size() < 2) {
            throw runtime_error("Il faut fournir au moins 2 fichiers video.");
        }

        cout << "Traitement de " << fichiersEntree.size() << " videos" << endl;

        cout << "[1/3] Analyse de la reference video..." << endl;

        try {
            extraireAudio(fichiersEntree[0], TEMP_AUDIO_REF);
        } catch (const exception &e) {
            throw runtime_error(string("Erreur reference: ") + e.what());
        }

        vector<float> audioRef = chargerAudioBrut(TEMP_AUDIO_REF);

        if (audioRef.empty()) {
            throw runtime_error("Fichier audio reference vide ou illisible.");
        }

        vector<InfoVideo> listeVideos;

        // Ajoute la vidéo de référence à la liste avec un décalage de 0.
        listeVideos.push_back({fichiersEntree[0], 0.0});

        // Boucle sur les vidéos cibles (à partir de la deuxième).
        for (int i = 1; i < fichiersEntree.size(); ++i) {
            cout << "[2/3] Analyse video " << i + 1 << " : " << flush;

            try {
                // Extrait l'audio de la vidéo cible actuelle dans un fichier temporaire.
                extraireAudio(fichiersEntree[i], TEMP_AUDIO_CIBLE);

                vector<float> audioCible = chargerAudioBrut(TEMP_AUDIO_CIBLE);

                double decalage = calculerDecalage(audioRef, audioCible);

                // Ajoute la vidéo à la liste avec son décalage.
                listeVideos.push_back({fichiersEntree[i], decalage});

                cout << "OK (Retard: " << fixed << setprecision(3) << decalage << "s)" << endl;
            } catch (const exception &e) {
                cout << "Echec (" << e.what() << ") - Video ignoree" << endl;
            }
        }

        return genererVideo(listeVideos, fichierSortie);
    } catch (const exception &e) {
        cerr << "[Erreur] " << e.what() << endl;
        return false;
    }
}

bool SynchroniseurMultiVideo::genererVideoSynchronisee(const string &fichierAudioRef,
                                                       const vector<string> &fichiersVideo,
                                                       const string &fichierSortie) const {
    try {
        if (fichiersVideo.empty()) {
            throw runtime_error("Il faut fournir au moins 1 fichier video.");
        }

        cout << "[1/3] Analyse de la reference audio..." << endl;

        try {
            extraireAudio(fichierAudioRef, TEMP_AUDIO_REF);
        } catch (const exception &e) {
            throw runtime_error(string("Erreur reference audio: ") + e.what());
        }

        vector<float> audioRef = chargerAudioBrut(TEMP_AUDIO_REF);

        if (audioRef.empty()) {
            throw runtime_error("Fichier audio reference vide ou illisible.");
        }

        vector<InfoVideo> listeVideos;

        // Boucle sur les vidéos cibles.
        for (int i = 0; i < fichiersVideo.size(); ++i) {
            cout << "[2/3] Analyse video " << i + 1 << " : " << flush;

            try {
                // Extrait l'audio de la vidéo cible actuelle dans un fichier temporaire.
                extraireAudio(fichiersVideo[i], TEMP_AUDIO_CIBLE);

                vector<float> audioCible = chargerAudioBrut(TEMP_AUDIO_CIBLE);

                double decalage = calculerDecalage(audioRef, audioCible);

                // Ajoute la vidéo à la liste avec son décalage.
                listeVideos.push_back({fichiersVideo[i], decalage});

                cout << "OK (Retard: " << fixed << setprecision(3) << decalage << "s)" << endl;
            } catch (const exception &e) {
                cout << "Echec (" << e.what() << ") - Video ignoree" << endl;
            }
        }

        return genererVideo(listeVideos, fichierSortie, fichierAudioRef);
    } catch (const exception &e) {
        cerr << "[Erreur] " << e.what() << endl;
        return false;
    }
}
