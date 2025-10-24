#include "MainWindow.hpp"

#include <QtWidgets/QApplication>
#include <qdebug.h>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    MainWindow   window;
    window.show();
    return QApplication::exec();
}
