#ifndef CHIP8DISPLAY_H
#define CHIP8DISPLAY_H

#include <QRasterWindow>
#include "chip8keyboard.h"

class Chip8Machine;

class Chip8Display : public QRasterWindow, public Chip8Keyboard
{
public:
    Chip8Display(Chip8Machine* chip8);
    ~Chip8Display();

private:
    void paintEvent(QPaintEvent *event);
    bool isPressed(int key) override;
    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);
    int keycodeToKey(int keycode);
private:
    Chip8Machine* m_chip8;
    bool m_keys[16];
};

#endif // CHIP8DISPLAY_H
