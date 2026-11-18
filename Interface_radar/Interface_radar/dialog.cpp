#include "dialog.h"
#include "ui_dialog.h"

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
    connect(ui->pbFermer, SIGNAL(clicked()), this, SLOT(close())); // Fermer la fenêtre

    // Creation dynamique du thread
    pRadar = new MyThread;
    connect(pRadar, SIGNAL(Termine(int)), this, SLOT(slotTermine(int)));  // Quand le thread emet Termine(int), la fenetre exécute slotTermine(int)
}

Dialog::~Dialog()
{
    // wait() bloque jusqu'à la fin de run() (retour immediat si le thread est déjà arrêté)
    pRadar->wait();
    delete pRadar;
    delete ui;
}

void Dialog::slotDist()
{
    pRadar->commande = CMD_DIST;
    lancer();
}

void Dialog::slotAngle()
{
    pRadar->commande = CMD_ANGLE;
    pRadar->angle = ui->sbAngle->value();
    lancer();
}

void Dialog::slotScan()
{
    pRadar->commande = CMD_SCAN;
    pRadar->debut = ui->sbDebut->value();
    pRadar->fin = ui->sbFin->value();
    pRadar->pas = ui->sbPas->value();
    lancer();
}

void Dialog::lancer()
{
    pRadar->ip = ui->leIp->text();
    pRadar->port = ui->sbPort->value();

    // On grise les boutons pendant que le thread travaille :
    // sinon un 2e clic modifierait les parametres alors que run() les lit
    ui->pbAngle->setEnabled(false);
    ui->pbDist->setEnabled(false);
    ui->pbScan->setEnabled(false);

    ui->teJournal->appendPlainText("Commande envoyée, attente de la carte...");

    pRadar->start();            // lance run() en parallele : retour immediat
}

// Transforme un code d'erreur en phrase lisible
QString Dialog::texteErreur(int code)
{
    QString texte;

    if (code == -1) {
        texte = "la carte ne répond pas (vérifier IP, port, alimentation, routeur)";
    } else if (code == -2) {
        // messageCarte contient la raison exacte donnee par la carte
        texte = "la carte refuse la commande : " + pRadar->messageCarte;
    } else if (code == -3) {
        texte = "réponse de la carte illisible ou incomplète";
    } else if (code == -4) {
        texte = "réseau indisponible sur ce PC";
    }
     else if (code == -5) {
        texte = "adresse IP invalide";
    }
    else {
        texte = "erreur inconnue";
    }

    return texte + " (code " + QString::number(code) + ")";
}

void Dialog::slotTermine(int r)
{
    int i;
    QString texte;

    // Le thread a fini : on reactive les boutons
    ui->pbAngle->setEnabled(true);
    ui->pbDist->setEnabled(true);
    ui->pbScan->setEnabled(true);

    if (pRadar->commande == CMD_ANGLE) {
        if (r == 0) {
            ui->teJournal->appendPlainText("ANGLE " + QString::number(pRadar->angle) + " -> OK");
        } else {
            ui->teJournal->appendPlainText("ANGLE " + QString::number(pRadar->angle)
                                           + " -> ÉCHEC : " + texteErreur(r));
        }
    }
    else if (pRadar->commande == CMD_DIST) {
        if (r == 0) {
            ui->lbDist->setText(QString::number(pRadar->cm, 'f', 1) + " cm");
            ui->teJournal->appendPlainText("DIST -> " + QString::number(pRadar->cm, 'f', 1) + " cm");
        } else {
            ui->lbDist->setText("---");
            ui->teJournal->appendPlainText("DIST -> ÉCHEC : " + texteErreur(r));
        }
    }
    else {                                  // CMD_SCAN : r = nombre de mesures
        if (r < 0) {
            ui->teJournal->appendPlainText("SCAN -> ÉCHEC : " + texteErreur(r));
            return;                         // on ne touche pas a la table
        }

        ui->twMesures->setRowCount(r);
        for (i = 0; i < r; i++) {
            ui->twMesures->setItem(i, 0, new QTableWidgetItem(QString::number(pRadar->angles[i])));

            if (pRadar->distances[i] < 0) {
                texte = "pas d'écho";
            } else {
                texte = QString::number(pRadar->distances[i], 'f', 1);
            }
            ui->twMesures->setItem(i, 1, new QTableWidgetItem(texte));
        }
        ui->teJournal->appendPlainText("SCAN -> " + QString::number(r) + " mesures");
    }
}