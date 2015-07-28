#ifndef CHIP8MACHINE_H
#define CHIP8MACHINE_H

#include <QSize>
#include <QImage>
#include <QObject>

class Chip8Keyboard;

class Chip8Machine : public QObject
{
    Q_OBJECT

public:
    Chip8Machine();
    ~Chip8Machine();

    QSize displayResolution() const;
    QImage pixels() { return *m_pixels; }

    void setKeyboardProvider(Chip8Keyboard* keyboard);
    void loadProgramImage(QByteArray p);
    void reset();
    void execute(int instructionCount);
    void delayTimerTick();

private:
    bool doInstruction();
    void exception(QString reason);
    void dumpMachineState();
    u_int16_t currentInstruction();
    QString toHex(u_int16_t v);
    QString toHex(u_int8_t v);

private:
    QImage* m_pixels;
    unsigned char m_memory[4096];
    unsigned char m_regs[16];
    unsigned short m_indexReg;
    int m_delayTimer;
    int m_soundTimer;
    u_int16_t m_pc;
    u_int16_t m_stack[16];
    int m_sp;

    Chip8Keyboard* m_keyboard;
};

#endif // CHIP8MACHINE_H
