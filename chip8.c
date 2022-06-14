#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <unistd.h>
#include "chip8.h"

void draw(unsigned char display[HEIGHT][WIDTH]);

unsigned char font_set[FONT_SET_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

unsigned char shift_flag = 0;
unsigned char jump_flag = 0;

//SDL display variables
SDL_Event event;
SDL_Renderer *renderer;
SDL_Window *window;

/*
    Initialize CHIP-8 specifications
*/
void init(chip8 *chip){
    memset(chip->memory,0,sizeof(chip->memory));
    memset(chip->display,0,sizeof(chip->display));
    memset(chip->stack,0,sizeof(chip->stack));
    memset(chip->V,0,sizeof(chip->V));

    chip->I = 0;
    chip->delay = 0;
    chip->sound = 0;
    chip->PC = 0x200;
    chip->stack_pointer = 0;

    for(int i = 0; i<sizeof(font_set); i++){
        chip->memory[i+0x50] = font_set[i];
    }
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window, &renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
}

int load_program(unsigned char* buf, char* filename){
    FILE *fileptr;
    short filelen;
    
    fileptr = fopen(filename, "r");

    fseek(fileptr,0,SEEK_END);
    filelen = ftell(fileptr);
    printf("filelen: %d\n",filelen);
    if(filelen>MAX_PROGRAM_SIZE) return -1;
    rewind(fileptr);

    for(int i = PROGRAM_OFFSET; i<(filelen+PROGRAM_OFFSET); i++){
        buf[i] = (unsigned char) fgetc(fileptr);
        //printf("%X\n",buf[i]);
    }
    
    return 1;
}

void fetch(chip8 *chip, unsigned char *instr, unsigned short *opcode){
    *instr = (chip->memory[chip->PC]>>4) & 0xF;
    *opcode = (chip->memory[chip->PC]<<8) | chip->memory[chip->PC+1];
    chip->PC += 2;
}

void execute(chip8 *chip, unsigned char *instr, unsigned short *opcode){
    unsigned char X = (*opcode >> 8) & 0xF;
    unsigned char Y = (*opcode >> 4) & 0xF;
    unsigned char N = *opcode & 0xF;
    unsigned char NN = *opcode & 0xFF;
    unsigned short NNN = *opcode & 0xFFF;
    int x_coord;
    int y_coord;
    switch(*instr){
        case 0x0:
            switch(NNN){
                //clear screen
                case 0x0E0:
                    memset(chip->display,0,sizeof(chip->display));
                    draw(chip->display);
                    break;
                //return from subroutine
                case 0x0EE:
                    chip->PC = chip->stack[--chip->stack_pointer];
                    chip->stack[chip->stack_pointer] = 0;
                    break;

                default:
                    printf("Unknown opcode clear %i\n",(int) NNN);
                    exit(1);
            }
            break;
        
        //jump to NNN
        case 0x1:
            chip->PC = NNN;
            break;

        // call subroutine
        case 0x2:
            if(chip->stack_pointer > 1){
                printf("Stack Overflow");
                exit(1);
            }
            chip->stack[chip->stack_pointer++] = chip->PC;
            chip->PC = NNN;
            break;
        
        //skip instruction if V[X] equal to NN
        case 0x3:
            if(chip->V[X] == NN)
                chip->PC+=2;
            break;
        
        //skip instruction if V[X] not equal to NN
        case 0x4:
            if(chip->V[X] != NN)
                chip->PC+=2;
            break;
        
        //skip instruction if V[X] == V[Y]
        case 0x5:
            if(chip->V[X] == chip->V[Y])
                chip->PC+=2;
            break;
        
        //skip instruction if V[X] != V[Y]
        case 0x9:
            if(chip->V[X] != chip->V[Y])
                chip->PC+=2;
            break;

        //set
        case 0x6:
            chip->V[X] = NN;
            break;
        
        //add constant
        case 0x7:
            chip->V[X] += NN;
            break;
        
        case 0x8:
            switch(N){
                //set reg X to reg Y
                case 0x0:
                    chip->V[X] = chip->V[Y];
                    break;
                
                //binary OR
                case 0x1:
                    chip->V[X] |= chip->V[Y];
                    break;
                
                //binary AND
                case 0x2:
                    chip->V[X] &= chip->V[Y];
                    break;
                
                //binary XOR
                case 0x3:
                    chip->V[X] ^= chip->V[Y];
                    break;

                //add registers
                case 0x4:
                    if(chip->V[X] + chip->V[Y] > 255)
                        chip->V[15] = 1;
                    chip->V[X] += chip->V[Y];
                    break;
                
                //subtract register X - Y, store in X
                case 0x5:
                    chip->V[15] = 0;
                    if(chip->V[X] > chip->V[Y]) chip->V[15] = 1;
                    chip->V[X] -= chip->V[Y];
                    break;
                
                //subtract register Y - X, store in X
                case 0x7:
                    chip->V[15] = 0;
                    if(chip->V[Y] > chip->V[X]) chip->V[15] = 1;
                    chip->V[X] = chip->V[Y] - chip->V[X];
                    break;
                
                //shift right
                case 0x6:
                    if(shift_flag) chip->V[X] = chip->V[Y];
                    chip->V[15] = chip->V[X] & 0x1;
                    chip->V[X] = chip->V[X]>>1;
                    break;
                
                //shift left
                case 0xE:
                    if(shift_flag) chip->V[X] = chip->V[Y];
                    chip->V[15] = (chip->V[X]>>7) & 0x1;
                    break;

                default:
                    printf("Unknown opcode arith %x\n",(int) N);
                    exit(1);
            }
            break;
        
        //set I register
        case 0xA:
            chip->I = NNN;
            break;

        //jump w/ offset
        case 0xB:
            if(jump_flag)
                chip->PC = chip->V[0] + NNN;
            else
                chip->PC = chip->V[X] + NNN;
            break;
        
        //random
        case 0xC:
            chip->V[X] = (rand()%256) + NN;
            break;
        
        //display
        case 0xD:
            x_coord = chip->V[X] % WIDTH;
            y_coord = chip->V[Y] % HEIGHT;
            chip->V[15] = 0;
            int curr_pointer = chip->I;
            for(int i = y_coord; i<N; i++){
                if(i>=HEIGHT) break;
                unsigned char curr_mem = chip->memory[curr_pointer++];
                for(int j = x_coord; j<8; j++){
                    if(j>=WIDTH) break;
                    unsigned char next_px = (curr_mem >> (7-j)) & 0x1;
                    if(next_px){
                        if(chip->display[i][j]) chip->V[15]=1;
                        chip->display[i][j] ^= next_px;
                    }
                }
            }
            draw(chip->display);
            break;

        default:
            printf("Unknown opcode %i\n",(int)*instr);

    }
    if(chip->delay>0)
        --chip->delay;
    if(chip->sound>0){
        printf("BEEP\n");
        --chip->sound;
    }
    sleep(0.016667);
}

void draw(unsigned char display[HEIGHT][WIDTH]){
    for (int i = 0; i < HEIGHT; ++i){
        for(int j = 0; j < WIDTH; j++){
            SDL_SetRenderDrawColor(renderer,0,0,0,0);
            if(display[i][j])
                SDL_SetRenderDrawColor(renderer,255,255,255,255);
            SDL_RenderDrawPoint(renderer, j, i);
        }
    }
    SDL_RenderPresent(renderer);
}


void pipeline(chip8 *chip){
    unsigned char tmp1 = 0x0;
    unsigned short tmp2 = 0x0;
    unsigned char *instr = &tmp1;
    unsigned short *opcode = &tmp2;

    for(;;){
        *instr = 0x0;
        *opcode = 0x0;
        fetch(chip, instr, opcode);
        execute(chip, instr, opcode);
        
    }
}

int main(int argc, char* argv[]){
    if(argc<2) exit(1);

    chip8 chip;
    init(&chip);

    // load CHIP-8 program, exit on unsuccessful load
    if(load_program(chip.memory, argv[1])==-1) exit(1);

    pipeline(&chip);

    while (1) {
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
            break;
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
    
}