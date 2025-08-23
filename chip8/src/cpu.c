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

void exec_ins() {

    uint16_t ins = memory[pc] << 8 | memory[pc++];
    int opcode   = ins & 0xFF;
    int x   = ins & 0x0F00; 
    int y   = ins & 0x00F0;
    int n   = ins & 0x000F;
    int kk  = ins & 0x00FF;
    int nnn = ins & 0x0FFF;

    printf("[%x]\n", ins);
    return;

    switch (opcode) {
        case 0x0:
            if (kk == 0xE0) {
                for (int r = 0; r < 32; r++) {
                    for (int c = 0; c < 64; c++)
                        display[r][c] = 0;
                }
            }
            else if (kk == 0xEE) {
                pc = stack[sp];
                sp--;
            }
            break;
        case 0x1:
            pc = nnn;
            break;
        case 0x2:
            sp++; stack[sp] = pc; pc = nnn;
            break;
        case 0x3:
            if (V[x] == kk)
                pc += 2;
            pc += 2;
            break;
        case 0x4:
            if (V[x] != kk)
                pc += 2;
            pc += 2;
            break;
        case 0x5:
            if (V[x] == V[y])
                pc += 2;
            pc += 2;
            break;
        case 0x6:
            V[x] = kk;
            break;
        case 0x7:
            V[x] += kk;
            break;
        case 0x8:
            switch(n)
            {
                case 0:
                    V[x] = V[y];
                    break;
                case 1:
                    V[x] = V[x] | V[y];
                    break;
                case 2:
                    V[x] = V[x] & V[y];
                    break;
                case 3:
                    V[x] = V[x] ^ V[y];
                    break;
                case 4:
                    if (V[x] + V[y] > 255)
                    {
                        V[0xF] = 1;
                        V[x] = V[x] & 0xFF;
                    }
                    else 
                    {
                        V[x] += V[y];
                    }
                    break;
                case 5:
                    if (V[x] > V[y])
                    {
                        V[0xF] = 1;
                    }
                    else 
                    { 
                        V[0xF] = 0;
                        V[x] -= V[y];
                    } 
                    break;
                case 6:
                    V[0XF] = V[x] & -V[x];
                    V[x] = V[x] >> 1;
                    break;
                case 7:
                    if (V[y] > V[x])
                    {
                        V[0xF] = 1;
                    }
                    else 
                    { 
                        V[0xF] = 0;
                        V[x] = V[y] - V[x];
                    } 
                    break;
                case 0xE:
                    V[0XF] = V[x] & -V[x];
                    V[x] = V[x] << 1;
                    break;
                default:
                    printf("wtf is [%x]?\n", ins);
                    exit(ins);
                    break;
            }
            pc += 2;
            break;
        case 0x9:
            if (V[x] != V[y]) 
                pc += 2;
            pc +=2;
            break;
        case 0xA:
            I = nnn;
            pc += 2;
            break;
        case 0xB:
            pc = nnn + V[0];
            break;
        case 0xC:
            V[x] = (rand() % 255) & kk;
            pc += 2;
            break;
        case 0xD:
            break;
        case 0xE:
            if (kk == 0x9E)
            {
                if (keyboard[V[x]])
                    pc += 2;
            }
            else //ExA1
            {
                if (!keyboard[V[x]])
                    pc += 2;
            }
            pc += 2;
            break;
        case 0xF:
            switch(kk)
            {
                case 0x07:
                    V[x] = dt;
                    break;
                case 0x0A:
                    while (!keydown) {}
                    for (int i = 0; i < 16; i++)
                    {
                        if (keyboard[i])
                            V[x] = i;
                    }
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
                case 0x29: //0x000 to 0x1FF
                    I = memory[V[x] * 5];
                    break;
                case 0x33:
                    memory[I] = V[x] / 100;
                    memory[I + 1] = (V[x] / 10) % 20;
                    memory[I + 2] = (V[x] % 100) % 10;
                    break;
                case 0x55:
                    for (int i = 0; i < x; i++)
                        memory[I + i] = V[i];
                    break;
                case 0x65:
                    for (int i = 0; i < x; i++)
                        V[i] = memory[I + i];
                    break;
                default:
                    printf("wtf is [%x]?\n", ins);
                    exit(ins);
                    break;
            }
            pc += 2;
            break;
        default:
            printf("wtf is [%x]?\n", ins);
            exit(ins);
            break;
    }
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    int running = 1;
    char *input = argv[1];
    
    if (!load_rom(input)) {
        printf("Read nothing from [%s]...\n", input);
        return 1;
    }

    while (memory[pc] != 0) {
        exec_ins();
    }

    return 0;
}
