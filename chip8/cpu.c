#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>

/*
    Memory Map:
    +---------------+= 0xFFF (4095) End of Chip-8 RAM
    |               |
    |               |
    |               |
    |               |
    |               |
    | 0x200 to 0xFFF|
    |     Chip-8    |
    | Program / Data|
    |     Space     |
    |               |
    |               |
    |               |
    +- - - - - - - -+= 0x600 (1536) Start of ETI 660 Chip-8 programs
    |               |
    |               |
    |               |
    +---------------+= 0x200 (512) Start of most Chip-8 programs
    | 0x000 to 0x1FF|
    | Reserved for  |
    |  interpreter  |
    +---------------+= 0x000 (0) Start of Chip-8 RAM
*/

uint8_t memory[4096]    = {0};
uint8_t V[16]           = {0};    // general purpose registers, named "V" to represent that registers are named "Vx" where x is just a hex value
uint8_t st              = 0;      // sound timer, when != 0, decremented at 60Hz
uint8_t dt              = 0;      // delay timer, ^
uint8_t sp              = 0;      // stack pointer, points to the top of the stack
uint16_t pc             = 0x200;  // program counter, points to address of current instruction, programs start at 0x200 (see memory map)
uint16_t stack[16]      = {0};    // stores subroutine return addresses
uint16_t I              = 0;      // special register, generally for memory addresses, usually only uses lowest 12 bits.

// font sprites
uint8_t fontsprites[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
    0x20, 0x60, 0x20, 0x20, 0x70,  // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
    0xF0, 0x80, 0xF0, 0x80, 0x80   // F
};

int load_rom(char *filename) {
    FILE *file = fopen(filename, "rb");

    if (file) {
        return fread(memory + 0x200, 1, sizeof(memory) - 0x200, file);
    } else {
        printf("Could not read file [%s]...\n", filename);
        exit(1);
    }
    
}

int exec_ins() {

    uint16_t ins = memory[pc] << 8 | memory[pc++];
    int x   = ins & 0x0F00; 
    int y   = ins & 0x00F0;
    int kk  = ins & 0x00FF;
    int nnn = ins & 0x0FFF;

    switch (ins & 0xFF ) { // ins & 0xFF extracts the opcode from the instruction
        case 0x0:
            if ((ins & 0x000F) == 0x0) {
                // TODO: clear display
            } 
            else if ((ins & 0x000F) == 0xE) {
                pc = stack[sp];
                sp--;
            }
            break;
        case 0x1:
            // extract the last 12 bits.
            // this works because the & operator will always set the 
            // first byte to 0 since 0x0FFF starts with 0.
            pc = ins & 0x0FFF;
            break;
        case 0x2:
            stack[sp++] = pc;
            pc = ins & 0x0FFF;
            break;
        case 0x3:
            x  = ins & 0x0F00;
            kk = ins & 0x00FF;
            if (V[x] == kk) {
                pc += 2;
            } 
            break;
        case 0x4:
            x  = ins & 0x0F00;
            kk = ins & 0x00FF;
            if (V[x] != kk) {
                pc += 2;
            } 
            break;
        case 0x5:
            x = ins & 0x0F00;
            y = ins & 0x00F0;
            if (V[x] == V[y]) {
                pc += 2;
            }
        case 0x6:
            x  = ins & 0x0F00;
            kk = ins & 0x00FF;
            V[x] = kk;
            break;
        case 0x7:
            x  = ins & 0x0F00;
            kk = ins & 0x00FF;
            V[x] += kk;
            break;   
        case 0x8:
            x = ins & 0x0F00;
            y = ins & 0x00F0;
            switch (ins & 0x000F) {
                case 0x0:
                    V[x] = V[y];
                    break;
                case 0x1:
                    V[x] = V[x] | V[y];
                    break;
                case 0x2:
                    V[x] = V[x] & V[y];
                    break;
                case 0x3:
                    V[x] = V[x] ^ V[y];
                    break;
                case 0x4:
                    if ((V[x] + V[y]) > 255) {
                        V[0xF] = 1;
                        V[x] = (V[x] + V[y]) & 0xFF;
                    } else {
                        V[0xF] = 0;
                        V[x] += V[y];
                    }
                    break;
                case 0x5:
                    V[0xF] = V[x] > V[y];

                    V[x] -= V[y];
                    break;
                case 0x6:
                    V[0xF] = V[x] & 0x1;

                    V[x] /= 2;
                    break;
                case 0x7:
                    V[0xF] = V[y] > V[x];

                    V[x] = V[y] - V[x];
                    break;
                case 0xE:
                    V[0xF] = (V[x] >> 7) & 0x1;

                    V[x] *= 2;
                    break;
                default:
                    break;
            }
            break;
        case 0x9:
            x = ins & 0x0F00;
            y = ins & 0x00F0;
            if (V[x] != V[y])
                pc += 2;
            break;
        case 0xA:
            I = nnn;
            break;
        case 0xB:
            pc = nnn + V[0];
            break;
        case 0xC:
            V[x] = (rand() % 256) & kk;
            break;
        case 0xD:
            // TODO: draw instruction
            break;
        case 0xE:
            // TODO: needs keyboard implementation
            break;
        case 0xF:
            switch(kk) {
                case 0x07:
                    V[x] = dt;
                    break;
                case 0x0A:
                    // TODO: needs keyboard implementation
                    break;
                case 0x15:
                    dt = V[x];
                    break;
                case 0x18:
                    st = V[x];
                    break;
                case 0x1E:
                    I += V[x];
                    break;
                case 0x29:
                    // TODO: huh
                    break;
                case 0x33:
                    memory[I]     = (V[x] / 100) % 10;
                    memory[I + 1] = (V[x] / 10) % 10;
                    memory[I + 2] = V[x] % 10;
                    break;
                case 0x55:
                    for (int i = 0; i < x; i++)
                        memory[I] = V[i];
                    break;
                case 0x65:
                    for (int i = 0; i < x; i++)
                        V[i] = memory[I + i];
                    break;
                default:
                    break;
            }
        default:
            break;
    }
}

int main(int argc, char *argv[]) {
    
    // int running = 1;
    // char *input = argv[1];
    
    // if (!load_rom(input)) {
    //     printf("Read nothing from [%s]...\n", input);
    //     return 1;
    // }

    // while (running) {
    //     exec_ins();
    //     pc++;
    //     if (st > 0)
    //         st--;
    //     if (dt > 0)
    //         dt--;
    //     usleep(1500);
    // }

    int a = 0x2A3D;
    int b = 20;
    uint8_t c = 254;

    printf("[%d] -- [%d] -- [%x] -- [%x]\n", 0x1, 0x0001, a & 0x000F, ((a & 0x000F) == 0x000D));

    return 0;
}
