#ifndef CHIP8_H
#define CHIP8_H

#define MAX_SIZE 4096
#define WIDTH 64
#define HEIGHT 32
#define STACK_SIZE 128
#define FLAG_REGISTER 15
#define PROGRAM_OFFSET 512
#define NUM_REG 16
#define FONT_SET_SIZE 80
#define MAX_PROGRAM_SIZE 3584
#define MAX_STACK_SIZE 12

typedef struct chip8{
    unsigned char memory[MAX_SIZE];
    unsigned char display[HEIGHT][WIDTH];
    unsigned short stack[MAX_STACK_SIZE];
    int stack_pointer;
    int PC;
    int I;
    unsigned char delay;
    unsigned char sound;
    unsigned char V[NUM_REG];
} chip8;

void init(chip8 *chip);
void fetch(chip8 *chip, unsigned char *instr, unsigned short *opcode);
void execute(chip8 *chip, unsigned char *instr, unsigned short *opcode);
void pipeline(chip8 *chip);
void input_handler(chip8 *chip);
int load_program(unsigned char *buf, char *filename);

#endif