#include <QGuiApplication>
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QElapsedTimer>

#include "chip8machine.h"
#include "chip8display.h"

int main(int argc, char *argv[])
{
    QGuiApplication a(argc, argv);

    if (argc < 2)
        qFatal("No rom file defined");

    QFile f(a.arguments().at(1));
    if (!f.open(QFile::ReadOnly)) {
        qFatal(qPrintable(QString("Couldn't open rom file: %1").arg(a.arguments().at(1))));
    }

    QByteArray program = f.readAll();

    Chip8Machine chip8;
    chip8.loadProgramImage(program);

    Chip8Display display(&chip8);
    chip8.setKeyboardProvider(&display);

    display.show();

    QTimer t;
    t.setInterval(16);
    QObject::connect(&t, &QTimer::timeout, [&]() {
        chip8.delayTimerTick();
        chip8.execute(30);   // This seems to be okayish for games that don't use the timer (like they should)
        display.update();
    });
    t.start();

    return a.exec();
}
