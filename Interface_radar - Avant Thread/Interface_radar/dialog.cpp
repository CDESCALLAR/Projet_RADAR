#include "dialog.h"
#include "ui_dialog.h"
#include "radar_client.h"

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
    ui->twMesures->verticalHeader()->setVisible(false);       // masque la colonne des numeros de lignes
    ui->twMesures->horizontalHeader()->setStretchLastSection(true);   // la derniere colonne prend tout le reste
    ui->twMesures->setColumnWidth(0, ui->twMesures->width() * 40 / 100);   // la colonne 0 = 40 % du tableau

    connect(ui->pbDist, SIGNAL(clicked()), this, SLOT(slotDist()));  // Relie le clic du bouton DIST a la fonction slotDist()
    connect(ui->pbAngle, SIGNAL(clicked()), this, SLOT(slotAngle()));
    connect(ui->pbScan, SIGNAL(clicked()), this, SLOT(slotScan()));
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::slotDist()
{
    float cm;
    int r;

    // 1) Ouvre la communication avec l'IP et le port saisis dans la fenetre.
    // toLatin1().data() : convertit le texte Qt (QString) en char *,
    // le type attendu par le module C.
    r = radar_init(ui->leIp->text().toLatin1().data(), ui->sbPort->value());
    if (r < 0) {
        ui->teJournal->appendPlainText("Erreur : reseau indisponible");
        return;
    }

    // 2) Demande la distance (peut durer jusqu'a 3 s si la carte ne repond pas)
    r = radar_distance(&cm);

    // 3) Referme la communication
    radar_fermer();

    // 4) Affiche le resultat : QString::number(cm, 'f', 1) = nombre avec 1 decimale
    if (r == 0) {
        ui->lbDist->setText(QString::number(cm, 'f', 1) + " cm");
        ui->teJournal->appendPlainText("DIST -> " + QString::number(cm, 'f', 1) + " cm");
    } else {
        ui->lbDist->setText("---");
        ui->teJournal->appendPlainText("DIST -> erreur " + QString::number(r));
    }
}

void Dialog::slotAngle()
{
    int r;
    int angle = ui->sbAngle->value();     // valeur lue dans la boite a fleches

    r = radar_init(ui->leIp->text().toLatin1().data(), ui->sbPort->value());
    if (r < 0) {
        ui->teJournal->appendPlainText("Erreur : reseau indisponible");
        return;
    }

    r = radar_angle(angle);
    radar_fermer();

    if (r == 0) {
        ui->teJournal->appendPlainText("ANGLE " + QString::number(angle) + " -> OK");
    } else {
        ui->teJournal->appendPlainText("ANGLE " + QString::number(angle)
                                       + " -> erreur " + QString::number(r));
    }
}

void Dialog::slotScan()
{
    int angles[100];                      // 100 = nombre max de mesures d'un SCAN
    float distances[100];
    int debut = ui->sbDebut->value();
    int fin = ui->sbFin->value();
    int pas = ui->sbPas->value();
    int n, i, r;
    QString texte;

    r = radar_init(ui->leIp->text().toLatin1().data(), ui->sbPort->value());
    if (r < 0) {
        ui->teJournal->appendPlainText("Erreur : reseau indisponible");
        return;
    }

    // radar_scan remplit les deux tableaux et retourne le nombre de mesures
    n = radar_scan(debut, fin, pas, angles, distances, 100);
    radar_fermer();

    if (n < 0) {
        ui->teJournal->appendPlainText("SCAN -> erreur " + QString::number(n));
        return;                           // on ne touche pas a la table
    }

    // Une ligne par mesure
    ui->twMesures->setRowCount(n);
    for (i = 0; i < n; i++) {
        // new : le tableau devient proprietaire de la case, pas de delete a faire
        ui->twMesures->setItem(i, 0, new QTableWidgetItem(QString::number(angles[i])));

        // distance < 0 : convention du module = pas d'echo (TIMEOUT)
        if (distances[i] < 0) {
            texte = "pas d'echo";
        } else {
            texte = QString::number(distances[i], 'f', 1);
        }
        ui->twMesures->setItem(i, 1, new QTableWidgetItem(texte));
    }

    ui->teJournal->appendPlainText("SCAN -> " + QString::number(n) + " mesures");
}