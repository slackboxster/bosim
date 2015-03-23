#include "bogui.h"
#include "ui_bogui.h"
#include <time.h>

bogui::bogui(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::bogui)
{
    ui->setupUi(this);
    runtime.init(ui->edtScreen, ui->edtStack, ui->edtCode, ui->edtInput, ui->tblCodeSeg, ui->tblStack, ui->bounds, ui->rocode, ui->canaries);
    ui->edtCode->setPlainText("select a sample or type code in this box, then compile it.\n"
                              "the possible instructions are:\n"
                              "dostuff: burns up a cpu cycle\n"
                              "call <function name>: calls a function (which must be defined prior to the function call)\n"
                              "var <variable name> <size>: declares a variable. all variables must come before any code in a function. size is number of characters\n"
                              "function <function name>: indicates the start of a function definition\n"
                              "return: indicates the end of a function definition\n"
                              "read <variable name>: reads a line (until first \\n) from the input buffer into the variable specified. Will write length of line unless bounds checking is enabled.\n"
                              "readn <variable name> <n>: reads n characters from the input buffer and writes them into the specified variable. If bounds checking is enabled, it will write either n or the length of the variable, whichever is smaller.\n"
                              "print <variable name>: prints the contents of the given variable until it reaches a null character\n"
                              "exit: terminates the running of the program\n"
                              "crash: this instruction is not recognized by the compiler, but is recognized by the runtime with the opcode " + QString::number(CRASH) + ".\n"
                              );
    ui->edtInput->setPlainText("JJJJJJJJJJ\nThis is where you put the input you want your program to accept.");
    ui->edtStack->setPlainText("compile before executing instructions");
    ui->tblCodeSeg->setColumnWidth(0,40);
    ui->tblCodeSeg->setColumnWidth(1,100);
    ui->tblCodeSeg->setColumnWidth(2,40);
    ui->tblCodeSeg->setColumnWidth(3,40);
    ui->tblStack->setColumnWidth(0,50);//frame
    ui->tblStack->setColumnWidth(1,70);//var name
    ui->tblStack->setColumnWidth(2,70);//var size
    ui->tblStack->setColumnWidth(3,290);//var data
    ui->tblStack->setColumnWidth(4,70);//canary
    ui->tblStack->setColumnWidth(5,70);//prev stackbase
    ui->tblStack->setColumnWidth(6,70);//prev stacktop
    ui->tblStack->setColumnWidth(7,70);//return address

    srand(time(NULL));

    //initialize the code samples and input samples.
    codesample[0].option = "Code samples";
    codesample[0].sample = "Choose another option from the dropdown";
    codesample[1].option = "secure code";
    codesample[1].sample = "var goodbye 10\ndostuff\nfunction hello\ndostuff\nreturn\ncall hello\nreadn goodbye 10\nprint goodbye\nexit\n";
    codesample[2].option = "insecure read";
    codesample[2].sample = "dostuff\nfunction hello\nvar goodbye 10\ndostuff\nread goodbye\nprint goodbye\nreturn\ncall hello\ndostuff\ndostuff\nexit\n";
    codesample[3].option = "insecure readn";
    codesample[3].sample = "dostuff\nfunction hello\nvar goodbye 10\ndostuff\nreadn goodbye 28\nprint goodbye\nreturn\ncall hello\ndostuff\ndostuff\nexit\n";
    codesample[4].option = "extra return";
    codesample[4].sample = "var goodbye 10\ndostuff\nfunction hello\ndostuff\nreturn\ncall hello\nread goodbye\nprint goodbye\nreturn\nexit\n";
    codesample[5].option = "many functions";
    codesample[5].sample = "function one\ndostuff\ndostuff\nreturn\nfunction two\ncall one\ndostuff\nreturn\nfunction three\ndostuff\ncall two\ncall one\ndostuff\nreturn\ndostuff\ncall three\nexit\n";

    inputsample[1].option = "benign input"; //short lines etc.
    inputsample[1].sample = "lines that\nare short \n present \nlittle \nthreat to\nthe stack\n";
    inputsample[2].option = "code smash"; //writes through the code segment
    inputsample[2].sample = "";
    for (int k = 0; k < MAXMEM * 4; k++)
    {
        if ((k%4)==3) inputsample[2].sample += "J";
        else inputsample[2].sample += char(0);
    }
    inputsample[2].sample += "\n";
    inputsample[3].option = "readn smash"; // small and executes within its own  block
    inputsample[3].sample = "";
    inputsample[3].sample += char(0);
    inputsample[3].sample += char(0);
    inputsample[3].sample += char(0);
    inputsample[3].sample += "J";
    inputsample[3].sample += char(0);
    inputsample[3].sample += char(0);
    inputsample[3].sample += char(0);
    inputsample[3].sample += "J";
    inputsample[3].sample += char(0);
    inputsample[3].sample += char(0);
    inputsample[3].sample += char(0);
    inputsample[3].sample += "J";
    inputsample[3].sample += char(0);//null after vars
    inputsample[3].sample += char(0);//null after vars
    inputsample[3].sample += char(0);//null after vars
    inputsample[3].sample += char(0);//null after vars
    inputsample[3].sample += char(0);
    inputsample[3].sample += char(0);
    inputsample[3].sample += "\x03";
    inputsample[3].sample += "\xe8";
    inputsample[3].sample += char(0);
    inputsample[3].sample += char(0);
    inputsample[3].sample += "\x03";
    inputsample[3].sample += "\xe8";
    inputsample[3].sample += char(0);
    inputsample[3].sample += char(0);
    inputsample[3].sample += "\x03";
    inputsample[3].sample += "\xe1";
    //inputsample[3].sample = "\0\0\0J\0\0\0J\0\0\0J\0\0\0\0\0\0\x03\xe8\0\0\x03\xe8\0\0\x03\xe1";
    inputsample[4].option = "interactive smash"; // reads more input as part of the shellcode, calls a function etc.
    //inputsampe[4].sample = "blaa\0\0\x03\xc8\0\0\0B\0\0\0\0\0\0\x03\xe8\0\0\x03\xe4\0\0\x03\xdfbla1bla2bla3bla4bla5bla6bla7bla8bla9\0\0\0J\0\0\x03\xea\0\0\0H\0\0\x03\xea\0\0\0F\0\0\0\x03\0\0\x03\xea\0\0\0C\0\0\0D\ni pwnd u!!!";
    inputsample[4].sample = "blaa";
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += "\x03";
    inputsample[4].sample += "\xc9";
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += "B";
    inputsample[4].sample += char(0);//null after vars
    inputsample[4].sample += char(0);//null after vars
    inputsample[4].sample += char(0);//null after vars
    inputsample[4].sample += char(0);//null after vars
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += "\x03";
    inputsample[4].sample += "\xe8";
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += "\x03";
    inputsample[4].sample += "\xe4";
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += "\x03";
    inputsample[4].sample += "\xdf";
    inputsample[4].sample += "bla1bla2bla3bla4bla5bla6bla7bla8bla9";
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += "J";
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += "\x03";
    inputsample[4].sample += "\xea";
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += "H";
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += "\x03";
    inputsample[4].sample += "\xea";
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += "F";
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += "\x03";
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += "\x03";
    inputsample[4].sample += "\xea";
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += "C";
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += char(0);
    inputsample[4].sample += "D\ni pwnd u!!!";

    //set the options into the combobox...
    ui->codesamples->addItem(codesample[1].option);
    ui->codesamples->addItem(codesample[2].option);
    ui->codesamples->addItem(codesample[3].option);
    ui->codesamples->addItem(codesample[4].option);
    ui->codesamples->addItem(codesample[5].option);
    ui->inputsamples->addItem(inputsample[1].option);
    ui->inputsamples->addItem(inputsample[2].option);
    ui->inputsamples->addItem(inputsample[3].option);
    ui->inputsamples->addItem(inputsample[4].option);
}

bogui::~bogui()
{
    delete ui;
}

void bogui::on_btnRun_clicked()
{
    runtime.execute();
}

void bogui::on_btnCompile_clicked()
{
    runtime.compile();
    ui->btnStart->setEnabled(true);
    ui->btnCompile->setEnabled(false);
}

void bogui::on_edtCode_textChanged()
{
    runtime.codeChanged();
    ui->btnStart->setEnabled(false);
    ui->btnCompile->setEnabled(true);
}

void bogui::on_btnStart_clicked()
{
    runtime.start();
    ui->btnStart->setEnabled(false);
    ui->btnStep->setEnabled(true);
    ui->btnStop->setEnabled(true);
    ui->btnRun->setEnabled(false);
}

void bogui::on_btnStep_clicked()
{
    if (!runtime.step())
    {
        ui->btnStart->setEnabled(true);
        ui->btnStep->setEnabled(false);
        ui->btnStop->setEnabled(false);
        ui->btnRun->setEnabled(true);
    }
}

void bogui::on_btnStop_clicked()
{
    runtime.stop();
    ui->btnStart->setEnabled(true);
    ui->btnStep->setEnabled(false);
    ui->btnStop->setEnabled(false);
    ui->btnRun->setEnabled(true);
}

void bogui::on_btnAppendInt_clicked()
{
    string int_chars = "";
    //turn the integer into 4 characters:
    for (int k = 0; k < 4; k++)
    {
        unsigned int temp = ui->spinChar->value();
        temp=temp<<(k%4)*8; // left shift by 0,8,16,or 24 bits to cut the unnecessary data off the front of the int.
        temp=temp>>24; // shifting back by 24 bits means only the first 8 bits
        int_chars += temp; // do this more for the loop terminating condition than anything else.
    }

    string value = ui->edtInput->toPlainText().toStdString();
    value += int_chars;//ui->spinChar->value();
    ui->edtInput->setPlainText(QString::fromStdString(value));
}

void bogui::on_btnAppendChar_clicked()
{
    string value = ui->edtInput->toPlainText().toStdString();
    value += ui->spinChar->value();
    ui->edtInput->setPlainText(QString::fromStdString(value));
}

void bogui::on_bounds_stateChanged(int )
{
    //re-enable compile and disenable start.
    ui->btnCompile->setEnabled(true);
    ui->btnStart->setEnabled(false);
}

void bogui::on_canaries_currentIndexChanged(int index)
{
    //re-enable compile and disenable start.
    ui->btnCompile->setEnabled(true);
    ui->btnStart->setEnabled(false);
}

void bogui::on_bounds_clicked()
{
    //re-enable compile and disenable start.
    ui->btnCompile->setEnabled(true);
    ui->btnStart->setEnabled(false);
}

void bogui::on_codesamples_currentIndexChanged(int index)
{
    ui->edtCode->setPlainText(codesample[index].sample);
}

void bogui::on_inputsamples_currentIndexChanged(int index)
{
    ui->edtInput->setPlainText(inputsample[index].sample);
}
