#include "MainWindow.hpp"

#include <QtWidgets/QApplication>
#include <qdebug.h>

int main(int argc, char* argv[])
{
    QApplication        app(argc, argv);
    app::ui::MainWindow window;
    window.show();
    return QApplication::exec();
}
