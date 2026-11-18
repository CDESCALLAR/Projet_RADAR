#include "mythread.h"
#include "radar_client.h"

MyThread::MyThread() : QThread()
{
}

void MyThread::run()
{
    int r;

    // Ouvre la communication. Les champs viennent de la fenetre.
    // toLatin1().data() : QString -> char * pour le module C
    r = radar_init(ip.toLatin1().data(), port);
    if (r < 0) {
        emit Termine(r);        // -4 ou -5, le code est déjà le bon
        return;
    }

    // Une seule des trois commandes, selon ce que la fenetre a demande
    if (commande == CMD_ANGLE) {
        r = radar_angle(angle);
    } else if (commande == CMD_DIST) {
        r = radar_distance(&cm);
    } else {
        r = radar_scan(debut, fin, pas, angles, distances, 100);
    }

    radar_fermer();

    // Previent la fenetre : "c'est fini, voici le code"
    messageCarte = QString(radar_message_erreur());
    emit Termine(r);
}