#include "chip8display.h"
#include "chip8machine.h"

#include <QDebug>
#include <QPainter>
#include <QKeyEvent>

Chip8Display::Chip8Display(Chip8Machine *chip8) :
    m_chip8(chip8)
{
    resize(m_chip8->displayResolution() * 8);
    memset(&m_keys, 0, sizeof(m_keys));
}

Chip8Display::~Chip8Display()
{

}

void Chip8Display::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.setBrush(Qt::red);
    p.fillRect(0, 0, size().width(), size().height(), Qt::red);

    p.drawImage(QRectF(QPointF(0, 0), size()), m_chip8->pixels(), (QRectF(QPointF(0, 0), m_chip8->pixels().size())));
}

bool Chip8Display::isPressed(int key)
{
    return m_keys[key];
}

void Chip8Display::keyPressEvent(QKeyEvent *e)
{
    int key = keycodeToKey(e->key());
    if (key != -1)
        m_keys[key] = true;
}


void Chip8Display::keyReleaseEvent(QKeyEvent *e)
{
    int key = keycodeToKey(e->key());
    if (key != -1)
        m_keys[key] = false;
}

int Chip8Display::keycodeToKey(int keycode)
{
    switch(keycode) {
        case Qt::Key_1:
            return 0x01;
        case Qt::Key_2:
            return 0x02;
        case Qt::Key_3:
            return 0x03;
        case Qt::Key_4:
            return 0x0C;
        case Qt::Key_Q:
            return 0x04;
        case Qt::Key_W:
            return 0x05;
        case Qt::Key_E:
            return 0x06;
        case Qt::Key_R:
            return 0x0D;
        case Qt::Key_A:
            return 0x07;
        case Qt::Key_S:
            return 0x08;
        case Qt::Key_D:
            return 0x09;
        case Qt::Key_F:
            return 0x0E;
        case Qt::Key_Z:
            return 0x0A;
        case Qt::Key_X:
            return 0x00;
        case Qt::Key_C:
            return 0x0B;
        case Qt::Key_V:
            return 0x0F;
    }
    return -1;
}
