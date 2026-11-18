#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include "mythread.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Dialog;
}
QT_END_NAMESPACE

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog() override;

private:
    Ui::Dialog *ui;
    MyThread *pRadar;           // le thread reseau

private slots:
    void slotDist();
    void slotAngle();
    void slotScan();
    void slotTermine(int);      // appele quand le thread a fini

private:
    void lancer();              // prépare puis démarre le thread
    QString texteErreur(int code);
};
#endif // DIALOG_H
