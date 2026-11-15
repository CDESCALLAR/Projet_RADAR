#include <winsock2.h>
#include <stdio.h>
#include <string.h>
#include "radar_client.h"

#define NB_ESSAIS 3
#define DELAI_MS  1000

// static : ces variables sont partagees par les fonctions du module,
// mais invisibles pour les autres fichiers (main.cpp, dialog.cpp...)
static SOCKET descripteurSocket = INVALID_SOCKET;
static struct sockaddr_in pointDeRencontreDistant;

int radar_init(const char *ip, int port)
{
    WSADATA WSAData;
    DWORD delai = DELAI_MS;

    WSAStartup(MAKEWORD(2,0), &WSAData);

    descripteurSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (descripteurSocket == INVALID_SOCKET) {
        WSACleanup();
        return -1;
    }

    // SO_RCVTIMEO : delai maximum de recvfrom, en millisecondes sous Windows
    // (const char *) : setsockopt attend ce type de pointeur, on convertit
    setsockopt(descripteurSocket, SOL_SOCKET, SO_RCVTIMEO,
               (const char *)&delai, sizeof(delai));

    pointDeRencontreDistant.sin_family = AF_INET;
    pointDeRencontreDistant.sin_addr.s_addr = inet_addr(ip);
    pointDeRencontreDistant.sin_port = htons(port);

    return 0;
}

int radar_envoyer(const char *commande, char *reponse, int taille)
{
    int essai, ecrits;
    int recus = SOCKET_ERROR;

    for (essai = 1; essai <= NB_ESSAIS; essai++) {
        ecrits = sendto(descripteurSocket, commande, strlen(commande), 0,
                        (SOCKADDR *)&pointDeRencontreDistant,
                        sizeof(pointDeRencontreDistant));
        if (ecrits == SOCKET_ERROR) {
            return -1;
        }

        // taille - 1 : on garde une place pour le '\0' de fin de chaine
        recus = recvfrom(descripteurSocket, reponse, taille - 1, 0, NULL, NULL);
        if (recus != SOCKET_ERROR) {
            break;                      // reponse recue
        }
    }

    if (recus == SOCKET_ERROR) {
        return -1;                      // la carte ne repond pas
    }

    reponse[recus] = '\0';              // fin de chaine
    // Retour : -1 = echec (carte muette ou erreur), sinon nombre d'octets recus.
    // Le module n'affiche rien : c'est l'appelant qui decide quoi montrer.
    return recus;                       // nombre d'octets recus
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

    if (radar_envoyer(commande, reponse, sizeof(reponse)) < 0) {
        return -1;                          // pas de reponse
    }
    if (strncmp(reponse, "OK", 2) == 0) {
        return 0;                           // la carte a obeit
    }
    if (strncmp(reponse, "ERR", 3) == 0) {
        return -2;                          // la carte refuse (angle invalide...)
    }
    return -3;                              // reponse inattendue
}

int radar_distance(float *cm)
{
    char reponse[256];

    if (radar_envoyer("DIST\n", reponse, sizeof(reponse)) < 0) {
        return -1;
    }
    if (strncmp(reponse, "ERR", 3) == 0) {
        return -2;
    }
    // sscanf lit la valeur apres "DIST " et la range dans *cm.
    // Elle retourne 1 si elle a bien lu une valeur.
    // cm est deja un pointeur : pas de & devant.
    if (sscanf(reponse, "DIST %f", cm) != 1) {
        return -3;
    }
    return 0;
}

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

    sprintf(commande, "SCAN %d %d %d\n", debut, fin, pas);

    if (radar_envoyer(commande, reponse, sizeof(reponse)) < 0) {
        return -1;
    }
    if (strncmp(reponse, "ERR", 3) == 0) {
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
            break;                  // derniere ligne sans '\n' : on arrête
        }
        ligne = fin_ligne + 1;      // on passe a la ligne suivante
    }

    return -3;                      // on n'a jamais vu "END" : reponse incomplete
}