#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QThread>
#include <QString>

// Numeros des commandes (plus lisible que 1, 2, 3)
#define CMD_ANGLE 1
#define CMD_DIST  2
#define CMD_SCAN  3

class MyThread : public QThread
{
    Q_OBJECT
public:
    MyThread();
    virtual void run();         // traitement execute en parallele

    // Parametres : remplis par la fenetre AVANT start()
    int commande;               // CMD_ANGLE, CMD_DIST ou CMD_SCAN
    QString ip;
    QString messageCarte;       // raison du refus quand la carte repond "ERR ..."
    int port;
    int angle;
    int debut, fin, pas;

    // Resultats : remplis par run(), lus par la fenetre APRES le signal
    float cm;
    int angles[100];            // 100 = nombre max de mesures d'un SCAN
    float distances[100];

signals:
    void Termine(int);          // code de retour du module (voir tableau plus bas)
};

#endif // MYTHREAD_H