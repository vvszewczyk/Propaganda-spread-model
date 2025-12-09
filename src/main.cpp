#include "MainWindow.hpp"

#include <QDebug>
#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
    QApplication        app(argc, argv);
    app::ui::MainWindow window;
    window.show();
    return QApplication::exec();
}
