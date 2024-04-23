#include "../inc/chip8.h"

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

void init_cpu() {

    memcpy(memory, fontsprites, sizeof(fontsprites));
}

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
    int n   = ins & 0x000F;
    int kk  = ins & 0x00FF;
    int nnn = ins & 0x0FFF;

    switch (ins & 0xFF) { // ins & 0xFF extracts the opcode from the instruction
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
        /*
            This instruction was confusing as fuck for me and I had to read code for it here (https://github.com/f0lg0/CHIP-8/blob/main/src/chip8.c)
            In order to cement my own understanding, I'm gonna write out some pesudocode to try to explain the way this instruction works.
            The instruction is in the format Dxyn where:
            D = opcode, duh
            x = V[x] register for the x coordinate of where we're drawing the sprite
            y = V[y] register for the y coordinate of where we're drawing the sprite
            n = the number of bytes the sprite takes up in memory (aka how tall the sprite is, they're ALWAYS 8 bits wide)
            * we start reading the sprite out of memory starting from the address stored in the I register

            To make this happen, we:
            1) Grab the byte out of memory[I]
            2) Scan through each bit in that byte, lighting up display pixels whenever we see a 1
                a) This requires some bitwise magic (which I'm pretty bad at as of writing this).
                   You can walk through a number bit by bit (haha) by:
                        1) AND it with 0x80
                        2) shift 0x80 right by 1 until you get 0
                        * helpful explanation found here (https://stackoverflow.com/questions/4465488/how-to-go-through-each-bit-of-a-byte)
            3) If the pixel we're trying to light up is already lit and being overwritten, we set V[0x0F] to 1 to represent collision
            4) XOR the byte info onto the pixel (only bother if we're lighting something up)
            5) Repeat above steps for memory[I + 1] until we hit memory[I + n]
        */
        uint8_t byte;
        uint8_t sprite_height = n;
        V[0x0F] = 0;

        // iterating over n memory spaces starting at memory[I]
        for (int i = 0; i < n; i++) {

            byte = memory[I + i];

            // iterate over each bit in the byte read at memory[I + i]
            for (int j = 0; j < 8; j++) {

                // check if the current sprite bit is a 1
                if (byte & (0x80 >> j)) {
                    // check if the display bit we're going to change is already on
                    if (display[V[y] + i][V[x] + j] == 1)
                        V[0x0F] = 1; // set collision

                    // XOR our sprite info onto the screen
                    display[V[y] + i][V[x] + j] ^= 1;
                }
            }
        }
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
            I = V[x] * 5; // TODO: fact check
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
        printf("wtf is [%x]...\n", ins);
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

    unsigned char val = 26;  // or whatever

    unsigned int mask;

    for (mask = 0x80; mask != 0; mask >>= 1) {
        printf("[%x] -- [%d] -- [%x] -- [%d]\n", val & mask, val & mask, mask, mask);
        if (val & mask) {
            // bit is 1
        }
        else {
            // bit is 0
        }
    }
    return 0;
}
