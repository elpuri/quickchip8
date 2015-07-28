#include "chip8machine.h"
#include "chip8keyboard.h"

#include <QDebug>
#include <QTimer>

#include <stdlib.h>

unsigned char hexFont[][5] = {
    { 0xF0, 0x90, 0x90, 0x90, 0xF0 }, // 0
    { 0x20, 0x60, 0x20, 0x20, 0x70 }, // 1
    { 0xF0, 0x10, 0xF0, 0x80, 0xF0 }, // 2
    { 0xF0, 0x10, 0xF0, 0x10, 0xF0 }, // 3
    { 0x90, 0x90, 0xF0, 0x10, 0x10 }, // 4
    { 0xF0, 0x80, 0xF0, 0x10, 0xF0 }, // 5
    { 0xF0, 0x80, 0xF0, 0x90, 0xF0 }, // 6
    { 0xF0, 0x10, 0x20, 0x40, 0x40 }, // 7
    { 0xF0, 0x90, 0xF0, 0x90, 0xF0 }, // 8
    { 0xF0, 0x90, 0xF0, 0x10, 0xF0 }, // 9
    { 0xF0, 0x90, 0xF0, 0x90, 0x90 }, // A
    { 0xE0, 0x90, 0xE0, 0x90, 0xE0 }, // B
    { 0xF0, 0x80, 0x80, 0x80, 0xF0 }, // C
    { 0xE0, 0x90, 0x90, 0x90, 0xE0 }, // D
    { 0xF0, 0x80, 0xF0, 0x80, 0xF0 }, // E
    { 0xF0, 0x80, 0xF0, 0x80, 0x80 }  // F
};

Chip8Machine::Chip8Machine()
{
    m_pixels = new QImage(64, 32, QImage::Format_Mono);
    m_pixels->fill(0);
    reset();
}

Chip8Machine::~Chip8Machine()
{
    delete m_pixels;
}

QSize Chip8Machine::displayResolution() const
{
    return QSize(64, 32);
}

void Chip8Machine::reset()
{
    m_pc = 0x200;
    m_sp = 0;
    m_indexReg = 0;
    m_delayTimer = 0;
    m_soundTimer = 0;
    memset(m_memory, 0, sizeof(m_memory));
    memcpy(m_memory, hexFont, sizeof(hexFont));
    memset(&m_regs, 0, 16);
    memset(&m_stack, 0, sizeof(m_stack));
}

void Chip8Machine::loadProgramImage(QByteArray p)
{
    if (p.length() > 4096 - 0x200)
        qFatal("Program image too large");
    memcpy(m_memory + 0x200, p.constData(), p.length());
}

void Chip8Machine::execute(int instructionCount)
{
    for (int i=0; i < instructionCount; i++) {
        if (!doInstruction())
            return;     // An instruction wants to wait for some condition updated by the main timer
    }
}

bool Chip8Machine::doInstruction()
{
    u_int16_t instruction = currentInstruction();
    u_int16_t i3, i2, i1, i0;
    i3 = (instruction >> 12) & 0x0f;
    i2 = (instruction >> 8)  & 0x0f;
    i1 = (instruction >> 4) & 0x0f;
    i0 = (instruction >> 0)  & 0x0f;

//    qDebug() << "PC" << toHex(m_pc) << toHex(instruction);

    switch (i3) {
        case 0x0: {
            if (instruction == 0x00E0) {
                m_pixels->fill(0);
                m_pc += 2;
            } else if (instruction == 0x00EE) {
                if (m_sp == 0)
                    exception("Stack underflow");
                m_sp--;
                m_pc = m_stack[m_sp];
            } else {
                exception("Unsupported instruction");
            }
            break;
        }

        case 0x1: {
            // 1nnn - JP addr
            m_pc = instruction & 0xFFF;
            break;
        }

        case 0x2: {
            // 2nnn - CALL addr
            if (m_sp + 1 >= (sizeof(m_stack) / sizeof(u_int16_t)))
                exception("Stack overflow");
            m_stack[m_sp] = m_pc + 2;
            m_sp++;
            m_pc = instruction & 0xFFF;
            break;
        }

        case 0x3: {
            // 3xkk - SE Vx, byte
            if (m_regs[i2] == (instruction & 0xFF))
                m_pc += 4;
            else
                m_pc += 2;
            break;
        }

        case 0x4: {
            // 4xkk - SNE Vx, byte
            if (m_regs[i2] != (instruction & 0xFF))
                m_pc += 4;
            else
                m_pc += 2;
            break;
        }

        case 0x5: {
            // 5xy0 - SE Vx, Vy
            if (m_regs[i2] == m_regs[i1])
                m_pc += 4;
            else
                m_pc += 2;
            break;
        }

        case 0x6: {
            // 6xkk - LD Vx, byte
            m_regs[i2] = instruction & 0xFF;
            m_pc += 2;
            break;
        }

        case 0x7: {
            // 7xkk - ADD Vx, byte
            m_regs[i2] += instruction & 0xFF;
            m_pc += 2;
            break;
        }

        case 0x8: {
            // Reg to reg ALU ops
            switch(i0) {
                case 0x0: {
                    // 8xy0 - LD Vx, Vy
                    m_regs[i2] = m_regs[i1];
                    break;
                }

                case 0x1: {
                    // 8xy1 - OR Vx, Vy
                    m_regs[i2] = m_regs[i2] | m_regs[i1];
                    break;
                }

                case 0x2: {
                    // 8xy3 - AND Vx, Vy
                    m_regs[i2] = m_regs[i2] & m_regs[i1];
                    break;
                }

                case 0x3: {
                    // 8xy3 - XOR Vx, Vy
                    m_regs[i2] = m_regs[i2] ^ m_regs[i1];
                    break;
                }

                case 0x4: {
                    // 8xy4 - ADD Vx, Vy
                    int res = m_regs[i2] + m_regs[i1];
                    if (res > 255)
                        m_regs[15] = 1;
                    else
                        m_regs[15] = 0;
                    m_regs[i2] = (unsigned char) res;
                    break;
                }

                case 0x5: {
                    // 8xy5 - SUB Vx, Vy
                    if (m_regs[i2] > m_regs[i1])
                        m_regs[15] = 1;
                    else
                        m_regs[15] = 0;
                    m_regs[i2] = m_regs[i2] - m_regs[i1];
                    break;
                }

                case 0x6: {
                    // 8xy6 - SHR Vx {, Vy}
                    if (m_regs[i2] & 1)
                        m_regs[15] = 1;
                    else
                        m_regs[15] = 0;
                    m_regs[i2] = m_regs[i2] >> 1;
                    break;
                }

                case 0x7: {
                    // 8xy7 - SUBN Vx, Vy
                    if (m_regs[i2] < m_regs[i1])
                        m_regs[15] = 1;
                    else
                        m_regs[15] = 0;
                    m_regs[i2] = m_regs[i1] - m_regs[i2];
                    break;
                }

                case 0xE: {
                    // 8xyE - SHL Vx {, Vy}
                    if (m_regs[i2] & 0x80)
                        m_regs[15] = 1;
                    else
                        m_regs[15] = 0;

                    m_regs[i2] = m_regs[i2] << 1;
                    break;
                }

                default:
                    exception("Unsupported instruction");
            }
            m_pc += 2;
            break;
        }

        case 0x9: {
            // 9xy0 - SNE Vx, Vy
            if (m_regs[i2] != m_regs[i1])
                m_pc += 4;
            else
                m_pc += 2;
            break;
        }

        case 0xA: {
            // Annn - LD I, addr
            m_indexReg = instruction & 0xFFF;
            m_pc += 2;
            break;
        }

        case 0xC: {
            // Cxkk - RND Vx, byte
            m_regs[i2] = rand() & (instruction & 0xFF);
            m_pc += 2;
            break;
        }

        case 0xD: {
            // Dxyn - DRW Vx, Vy, nibble

            int x = m_regs[i2];
            int y = m_regs[i1];

            // This is a bit pathetic...
            bool collision = false;
            unsigned char* spritePtr = m_memory + m_indexReg;
            for (int line = 0; line < i0; line++) {
                for (int p = 0; p < 8; p++) {
                    if ((x + p < 64) && (y + line < 32)) {
                        char spritePixel = (*spritePtr & (1 << (7 - p))) != 0 ? 1 : 0;
                        char currentPixel = (m_pixels->pixel(x + p, y + line)) == 0xffffffff ? 1 : 0;
                        if (spritePixel != 0 && currentPixel != 0)
                            collision = true;
                        char newPixel = spritePixel ^ currentPixel;
                        m_pixels->setPixel(x + p, y + line, newPixel ? 1 : 0);
                    }
                }
                spritePtr++;
            }
            m_regs[0xF] = collision ? 1 : 0;
            m_pc += 2;
            break;
        }

        case 0xE: {
            switch (instruction & 0xff) {
                case 0x9E: {
                    // Ex9E - SKP Vx
                    if (!m_keyboard->isPressed(m_regs[i2]))
                        m_pc += 2;
                    else
                        m_pc += 4;
                    break;
                }

                case 0xA1: {
                    // ExA1 - SKNP Vx
                    if (m_keyboard->isPressed(m_regs[i2]))
                        m_pc += 2;
                    else
                        m_pc += 4;
                    break;
                }
                default:
                    exception("Unsupported instruction");
            }

            break;
        }

        case 0xF: {
            switch (instruction & 0xFF) {
                case 0x0A: {
                    // Fx0A - LD Vx, K
                    int pressedKey = -1;
                    for (int i = 0; i < 16; i++) {
                        if (m_keyboard->isPressed(i)) {
                            pressedKey = i;
                            break;
                        }
                    }

                    if (pressedKey > 0) {
                        m_regs[i2] = pressedKey;
                        m_pc += 2;
                    } else {
                        return false;
                    }
                    break;
                }

                case 0x07: {
                    // Fx07 - LD Vx, DT
                    m_regs[i2] = m_delayTimer;
                    m_pc += 2;
                    break;
                }

                case 0x15: {
                    // Fx15 - LD DT, Vx
                    m_delayTimer = m_regs[i2];
                    m_pc += 2;
                    break;
                }

                case 0x18: {
                    // Fx18 - LD ST, Vx
                    m_soundTimer = m_regs[i2];
                    m_pc += 2;
                    break;
                }

                case 0x1E: {
                    // Fx1E - ADD I, Vx
                    m_indexReg += m_regs[i2];
                    m_pc += 2;
                    break;
                }

                case 0x29: {
                    // Fx29 - LD F, Vx
                    m_indexReg = m_regs[i2] * 5;
                    m_pc += 2;
                    break;
                }

                case 0x33: {
                    // Fx33 - LD B, Vx
                    int a = m_regs[i2];
                    int hundreds = a / 100;
                    a -= hundreds;
                    m_memory[m_indexReg] = hundreds;
                    int tens = a / 10;
                    m_memory[m_indexReg + 1] = tens;
                    m_memory[m_indexReg + 2] = a % 10;
                    m_pc += 2;
                    break;
                }

                case 0x55: {
                    // Fx55 - LD [I], Vx
                    for (int i = 0; i <= i2; i++) {
                        m_memory[m_indexReg++] = m_regs[i];
                    }
                    m_pc += 2;
                    break;
                }

                case 0x65: {
                    // Fx65 - LD Vx, [I]
                    for (int i = 0; i <= i2; i++) {
                        m_regs[i] = m_memory[m_indexReg++];
                    }
                    m_pc += 2;
                    break;
                }

                default:
                    exception("Unsupported instruction");
            }
            break;
        }

        default:
            exception("Unsupported instruction");
    }
    return true;
}

void Chip8Machine::dumpMachineState()
{
    qDebug() << "Registers:";
    for (int i = 0; i < 16; i++) {
        qDebug() << QString("V").append(QString::number(i, 16)).toUpper() + " = " + toHex(m_regs[i]);
    }
    qDebug() << QString("PC = ") + toHex(m_pc);
    qDebug() << QString("IX = ") + toHex(m_indexReg);
    qDebug() << "Current instruction: " + toHex(currentInstruction());
}

void Chip8Machine::exception(QString reason)
{
    dumpMachineState();
    m_pixels->scaled(m_pixels->size() * 8).save("screen.png");
    qFatal(qPrintable(reason));
}

u_int16_t Chip8Machine::currentInstruction()
{
    u_int16_t msb = m_memory[m_pc];
    u_int16_t lsb = m_memory[m_pc + 1];
    return msb << 8 | lsb;
}

void Chip8Machine::setKeyboardProvider(Chip8Keyboard *keyboard)
{
    m_keyboard = keyboard;
}

void Chip8Machine::delayTimerTick()
{
    if (m_delayTimer > 0)
        m_delayTimer--;
}

QString Chip8Machine::toHex(u_int16_t v)
{
    return QString("0x").append(QString::number(v, 16).toUpper().rightJustified(4, '0'));
}

QString Chip8Machine::toHex(u_int8_t v)
{
    return QString("0x").append(QString::number(v, 16).toUpper().rightJustified(2, '0'));
}
