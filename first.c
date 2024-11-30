#include <stdio.h>
#include <stdint.h>
#include "fake6502.h"

#define cursorXY(x,y) printf("\033[%d;%dH",(x),(y))

uint8_t ram[0xFFFF];

void init_ram() {
    ram[0xFFFC] = 0x00;
    ram[0xFFFD] = 0x02;

    ram[0x0200] = 0x18;
    ram[0x0201] = 0xa9;
    ram[0x0202] = 0x7a;
    ram[0x0203] = 0x69;
    ram[0x0204] = 0x17;
    ram[0x0205] = 0x60;
}

void write6502(uint16_t address, uint8_t value) {
    ram[address] = value;
}

uint8_t read6502(uint16_t address) {
    return ram[address];
}

int step = 0;

void clear_screen() {
    printf("\033[2J");
}
void print_registers() {
    cursorXY(1,1);
    printf("PC:   AC  XR  YR  SR  SP   NV-BDIZC\n");
    printf("%04X  %02X  %02X  %02X  %02X  %02X   ", pc, a, x, y, status, sp);
    for(int mask = 128; mask > 0; mask >>= 1) {
        printf("%c", ((status & mask) == mask) ? '1' : '0');
    }
    printf("\n\n");
    if (step)
        getchar();
}

int main(int argc, char *argv[]) {
    if (argc > 1 && argv[1][0] == 'S') {
        step = 1;
    }
    hookexternal(*print_registers);
    init_ram();
    clear_screen();
    printf("reset…\n");
    reset6502();
    exec6502(5);
    printf("clock ticks: %u\n", clockticks6502);
    return 0;
}

