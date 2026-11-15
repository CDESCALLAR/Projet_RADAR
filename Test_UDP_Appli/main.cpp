#include <QCoreApplication>
#include <stdio.h>
#include <conio.h>
#include "radar_client.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    char reponse[256];
    int n;

    if (radar_init("127.0.0.1", 5000) < 0) {
        printf("Erreur : initialisation du reseau impossible\n");
        return 1;
    }

    n = radar_envoyer("DIST\n", reponse, sizeof(reponse));
    if (n < 0) {
        printf("Pas de reponse de la carte\n");
    } else {
        printf("Reponse : %s (%d octets)\n", reponse, n);
    }

    while(!getch());

    radar_fermer();
    return 0;
}