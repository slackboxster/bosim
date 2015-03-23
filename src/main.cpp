#include <QtGui/QApplication>
#include "bogui.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    bogui w;
    w.show();

    return a.exec();
}
