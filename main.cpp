#include <QApplication>

#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/assets/material-symbols-music-cast-rounded.png"));
    MainWindow window;
    window.show();
    return app.exec();
}
