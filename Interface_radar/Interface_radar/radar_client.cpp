#include <winsock2.h>
#include <stdio.h>
#include <string.h>
#include "radar_client.h"

// ANGLE et DIST : reponse rapide, on peut reessayer sans risque
#define DELAI_MS            1000
#define NB_ESSAIS           3

// SCAN : la duree depend du nombre de mesures
#define SCAN_DEPLACEMENT_MS 600     // aller au premier angle
#define SCAN_PAR_MESURE_MS  300     // servo + capteur + marge, par mesure
#define SCAN_MARGE_MS       1000    // reseau, routeur...

// static : variables partagees par les fonctions de ce fichier,
// invisibles pour les autres fichiers du projet
static SOCKET descripteurSocket = INVALID_SOCKET;
static struct sockaddr_in pointDeRencontreDistant;
static char derniere_erreur[64]; // Dernier message d'erreur renvoye par la carte (ex : "ERR pas")

// Memorise la premiere ligne de la reponse de la carte
static void memoriser_erreur(const char *reponse)
{
    char *fin;

    strncpy(derniere_erreur, reponse, sizeof(derniere_erreur) - 1);
    // strncpy ne garantit pas la fin de chaine : on la force
    derniere_erreur[sizeof(derniere_erreur) - 1] = '\0';

    fin = strchr(derniere_erreur, '\n');
    if (fin != NULL) {
        *fin = '\0';            // on coupe au premier retour à la ligne
    }
}

const char *radar_message_erreur(void)
{
    return derniere_erreur;
}

// Codes de retour (communs a toutes les fonctions) :
//   0 ou nombre de mesures = succes
//  -1 = la carte ne repond pas
//  -2 = la carte a repondu "ERR ..."
//  -3 = reponse illisible ou incomplète
//  -4 = reseau indisponible (creation de la socket impossible)
//  -5 = adresse IP invalide

int radar_init(const char *ip, int port)
{
    WSADATA WSAData;

    WSAStartup(MAKEWORD(2,0), &WSAData);

    descripteurSocket = socket(AF_INET, SOCK_DGRAM, 0);    // SOCK_DGRAM = UDP
    if (descripteurSocket == INVALID_SOCKET) {
        WSACleanup();
        return -4;
    }

    pointDeRencontreDistant.sin_family = AF_INET;
    pointDeRencontreDistant.sin_addr.s_addr = inet_addr(ip);
    if (pointDeRencontreDistant.sin_addr.s_addr == INADDR_NONE) {
        // inet_addr echoue pour "abc", "300.1.1.1"... : on refuse tout de suite
        closesocket(descripteurSocket);
        WSACleanup();
        return -5;
    }
    pointDeRencontreDistant.sin_port = htons(port);        // ordre des octets du reseau

    return 0;
}


// Envoie une commande et attend la reponse.
// delai_ms : temps maximum d'attente de la reponse, pour chaque essai
// nb_essais : nombre d'envois avant d'abandonner
// Retour : nombre d'octets recus, ou -1 si la carte ne repond pas
int radar_envoyer(const char *commande, char *reponse, int taille,
                  int delai_ms, int nb_essais)
{
    int essai, ecrits;
    int recus = SOCKET_ERROR;
    DWORD delai = delai_ms;     // setsockopt attend un DWORD, en millisecondes

    // Le delai change selon la commande : on le regle avant chaque envoi
    // SO_RCVTIMEO = delai maximum de recvfrom
    // (const char *) : type de pointeur attendu par setsockopt
    setsockopt(descripteurSocket, SOL_SOCKET, SO_RCVTIMEO,
               (const char *)&delai, sizeof(delai));

    for (essai = 1; essai <= nb_essais; essai++) {
        ecrits = sendto(descripteurSocket, commande, strlen(commande), 0,
                        (SOCKADDR *)&pointDeRencontreDistant,
                        sizeof(pointDeRencontreDistant));
        if (ecrits == SOCKET_ERROR) {
            return -1;
        }

        // taille - 1 : on garde une place pour le '\0' de fin de chaine
        recus = recvfrom(descripteurSocket, reponse, taille - 1, 0, NULL, NULL);
        if (recus != SOCKET_ERROR) {
            break;              // reponse recue : on sort de la boucle
        }
    }

    if (recus == SOCKET_ERROR) {
        return -1;              // delai depasse a chaque essai (ou erreur reseau)
    }

    reponse[recus] = '\0';      // fin de chaine, pour pouvoir lire le texte
    return recus;
}


void radar_fermer(void)
{
    closesocket(descripteurSocket);
    descripteurSocket = INVALID_SOCKET;
    WSACleanup();
}


int radar_angle(int angle)
{
    char commande[32];
    char reponse[256];

    // sprintf fabrique la commande : "ANGLE 45\n"
    sprintf(commande, "ANGLE %d\n", angle);

    if (radar_envoyer(commande, reponse, sizeof(reponse), DELAI_MS, NB_ESSAIS) < 0) {
        return -1;
    }
    if (strncmp(reponse, "OK", 2) == 0) {
        return 0;
    }
    if (strncmp(reponse, "ERR", 3) == 0) {
        memoriser_erreur(reponse);
        return -2;
    }
    return -3;
}


int radar_distance(float *cm)
{
    char reponse[256];

    if (radar_envoyer("DIST\n", reponse, sizeof(reponse), DELAI_MS, NB_ESSAIS) < 0) {
        return -1;
    }
    if (strncmp(reponse, "ERR", 3) == 0) {
        memoriser_erreur(reponse);
        return -2;
    }

    // sscanf lit la valeur apres "DIST " et la range dans *cm.
    // Elle retourne le nombre de valeurs lues (1 si tout va bien).
    // cm est deja un pointeur : pas de & devant.
    if (sscanf(reponse, "DIST %f", cm) != 1) {
        return -3;
    }
    return 0;
}


// Remplit angles[] et distances[] (au plus "max" mesures).
// Une distance de -1.0 signifie "pas d'echo" (ligne TIMEOUT de la carte).
// Retour : nombre de mesures, ou un code negatif.
int radar_scan(int debut, int fin, int pas, int angles[], float distances[], int max)
{
    char commande[64];
    char reponse[1500];     // un datagramme contient au plus 1472 octets utiles
    char *ligne;            // debut de la ligne en cours de lecture
    char *fin_ligne;        // position du '\n' qui la termine
    int n = 0;              // nombre de mesures deja lues
    int angle;
    float cm;
    int lus;
    int nb_mesures;
    int delai_scan;

    if (pas < 1) {
        return -3;          // evite une division par zero
    }

    // Duree maximale prevue : deplacement + une duree par mesure + marge
    nb_mesures = (fin - debut) / pas + 1;
    if (nb_mesures < 1) {
        nb_mesures = 1;     // fin < debut : la carte refusera tout de suite,
        // mais le delai ne doit jamais etre negatif
    }
    delai_scan = SCAN_DEPLACEMENT_MS + nb_mesures * SCAN_PAR_MESURE_MS + SCAN_MARGE_MS;

    sprintf(commande, "SCAN %d %d %d\n", debut, fin, pas);

    // 1 seul essai : renvoyer un SCAN pendant que la carte travaille
    // donnerait "ERR occupe"
    if (radar_envoyer(commande, reponse, sizeof(reponse), delai_scan, 1) < 0) {
        return -1;
    }
    if (strncmp(reponse, "ERR", 3) == 0) {
        memoriser_erreur(reponse);
        return -2;
    }
    // On decoupe la reponse ligne par ligne
    ligne = reponse;
    while (*ligne != '\0') {
        fin_ligne = strchr(ligne, '\n');
        if (fin_ligne != NULL) {
            *fin_ligne = '\0';      // remplace le '\n' par une fin de chaine :
            // "ligne" devient une chaine independante
        }

        if (strcmp(ligne, "END") == 0) {
            return n;               // fin normale : on rend le nombre de mesures
        }

        // "90 60.5" -> lus = 2 ; "90 TIMEOUT" -> lus = 1 (l'angle seul est lu)
        lus = sscanf(ligne, "%d %f", &angle, &cm);
        if (lus >= 1 && n < max) {
            if (lus == 1) {
                cm = -1.0;          // convention : -1 = pas d'echo
            }
            angles[n] = angle;
            distances[n] = cm;
            n++;
        }

        if (fin_ligne == NULL) {
            break;                  // derniere ligne sans '\n' : on arrete
        }
        ligne = fin_ligne + 1;      // on passe a la ligne suivante
    }

    return -3;                      // "END" jamais vu : reponse incomplete
}