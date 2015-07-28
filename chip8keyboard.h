#ifndef CHIP8KEYBOARD
#define CHIP8KEYBOARD

class Chip8Keyboard {
public:
    virtual bool isPressed(int key) = 0;
};

#endif // CHIP8KEYBOARD

