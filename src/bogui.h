#ifndef BOGUI_H
#define BOGUI_H

#include <QMainWindow>
#include "interp.h"

struct textsample
{
    QString option;
    QString sample;
};

namespace Ui {
    class bogui;
}

class bogui : public QMainWindow
{
    Q_OBJECT

public:
    explicit bogui(QWidget *parent = 0);
    ~bogui();

    //the interpreter object
    interp runtime;

private:
    Ui::bogui *ui;

    //these arrays store the values for the code and input samples.
    textsample codesample[10];
    textsample inputsample[10];

private slots:
    void on_inputsamples_currentIndexChanged(int index);
    void on_codesamples_currentIndexChanged(int index);
    void on_bounds_clicked();
    void on_canaries_currentIndexChanged(int index);
    void on_bounds_stateChanged(int );
    void on_btnAppendChar_clicked();
    void on_btnAppendInt_clicked();
    void on_btnStop_clicked();
    void on_btnStep_clicked();
    void on_btnStart_clicked();
    void on_edtCode_textChanged();
    void on_btnCompile_clicked();
    void on_btnRun_clicked();
};

#endif // BOGUI_H
