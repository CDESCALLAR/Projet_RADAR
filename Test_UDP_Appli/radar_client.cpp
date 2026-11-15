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