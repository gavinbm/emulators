#include "chip8.h"

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

    int a = 0x2A3D;
    int b = 20;
    uint8_t c = 254;

    printf("[%d] -- [%d] -- [%x] -- [%x]\n", 0x1, 0x0001, a & 0x000F, ((a & 0x000F) == 0x000D));

    return 0;
}
