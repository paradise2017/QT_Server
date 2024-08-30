#include "qtqq_server.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QtqqServer w;
    w.show();
    return a.exec();
}
