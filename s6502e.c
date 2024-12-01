#include <stdio.h>
#include <sys/ioctl.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdint.h>
#include "fake6502.h"

#define cursorXY(x,y) printf("\033[%d;%dH",(x),(y))

#define RAMSIZE 0x10000
#define SCREENSIZE 0x0400

uint8_t ram[RAMSIZE];

int origin = 0x0600;
int loaded = 0;
int step = 0;

void dump(int start, int count);

unsigned char buffer[RAMSIZE];
unsigned char screen[SCREENSIZE];

void exit_s6502s();

void reset_screen() {
    printf("\033[0m\n");
}

void print_registers() {
    reset_screen();
    printf("PC:   AC  XR  YR  SR  SP   NV-BDIZC\n");
    printf("%04X  %02X  %02X  %02X  %02X  %02X   ", pc, a, x, y, status, sp);
    for(int mask = 128; mask > 0; mask >>= 1) {
        printf("%c", ((status & mask) == mask) ? '1' : '0');
    }
    printf("\n\n");
}

void exit_s6502s() {
    printf("\033[0m\n");
    print_registers();
    printf("clock ticks: %u\n", clockticks6502);
    printf("instructions: %u\n", instructions);
    exit(0);
}

void print_pixel(uint16_t address, uint8_t value) {
    int offset = address - 0x0200;
    int row = offset / 32;
    int col = offset % 32;
    int val = value & 0x0F;
    cursorXY(row + 1, col + 1);
    int color = val < 8 ? val + 30 : val + 82;
    printf("\033[%dm█", color);
    cursorXY(1,40);
}

void write6502(uint16_t address, uint8_t value) {
    ram[address] = value;
    if (address >= 0x0200 && address < 0x0600) {
        print_pixel(address, value);
    }
}

uint8_t random_value() {
    uint8_t value = rand() & 0xFF;
    return value;
}
uint8_t read6502(uint16_t address) {
    switch(address) {
        case 0x00FE:
            return random_value();
        default:
            return ram[address];
    }
}

void clear_screen() {
    printf("\033[2J");
}
int load_binary(char *filename) {
    FILE *file;
    if ((file = fopen(filename, "rb"))) {
        return fread(buffer, 1, sizeof(buffer), file);
    } else {
        fprintf(stderr, "can't read file %s\n", filename);
        return 0;
    }
}

void dump(int start, int count) {
    for(int i=0; i < count; i++) {
        if(i % 8 == 0) {
            printf("\n%04x ", start + i);
        }
        printf(" %02X", ram[start + i]);
    }
    printf("\n");
}

void relocate() {
    printf("relocating %d bytes at %04X\n", loaded, origin);
    for(int i = 0; i<0xFFFF && i<loaded; i++) {
        ram[origin+i] = buffer[i];
    }
    ram[0xFFFC] = origin & 0xFF;
    ram[0xFFFD] = origin >> 8;
    if (step) {
        dump(origin, loaded);
    }
}

void init_exit_trap() {
    ram[0xFFFE] = 0x00;
    ram[0xFFFF] = 0x00;
}

void hook() {
    if (step) {
        print_registers();
    }
    if (pc == 0x0000) {
        exit_s6502s();
    }
}
int main(int argc, char *argv[]) {
    srand (time(NULL));
    int opt;
    struct option long_options[] = {
        { "help", no_argument, NULL, 'h' },
        { "origin", required_argument, NULL, 'o' },
        { "step", no_argument, NULL, 's' },
        { "file", required_argument, NULL, 'f' },
        { NULL, 0 , NULL, 0}
    };

    while ((opt = getopt_long(argc, argv, "ho:sf:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                printf("Help *TO DO*\n");
                return 1;
            case 'f':
                if ((loaded = load_binary(optarg))==0) {
                    return 1;
                }
                break;
            case 'o':
                origin = (int)strtol(optarg, NULL, 16);
                break;
            case 's':
                step = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-h] [-f filename] [-s]\n", argv[0]);
                return 1;
        }
    }
    if (loaded) {
        relocate();
    }
    hookexternal(*hook);
    clear_screen();
    init_exit_trap();
    reset6502();
    exec6502(20000);
    reset_screen();
    return 0;
}
