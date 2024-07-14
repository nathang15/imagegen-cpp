#include "MainWindow.h"
#include <QApplication>
#include <QMainWindow>
#include <QWidget>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    winrt::uninit_apartment();
    winrt::init_apartment();
    CoInitialize(nullptr);

    QMainWindow mainWindow;
    QWidget centralWidget(&mainWindow);
    MainWindow w;

    mainWindow.setCentralWidget(&centralWidget);
    centralWidget.setLayout(new QVBoxLayout);
    centralWidget.layout()->addWidget(&w);

    w.resize(1300, 1115);
    mainWindow.resize(1300, 1115);
    mainWindow.show();

    return a.exec();
}