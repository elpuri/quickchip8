quickchip8
==========

I set myself the goal of writing a CHIP8 emulator before going to bed in one sitting.
This is the result. I decided I would be done when the Space Invaders ROM would work.
The initial commit reflects that state. 

Getting there took about three and a half hours of effective work. I did stray a few 
times implementing unnecessary (for Space Invaders) opcodes and tidying up the code, 
but I'm pretty happy about the time I got it done in. 

Performance was not a concern at any point (see the QImage backing store travesty).

Needs Qt5 and a compiler that supports C++11. 
