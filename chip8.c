#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <unistd.h>
#include "chip8.h"

//TODO: add array of all keys, instead of single variable

void draw(unsigned char display[HEIGHT][WIDTH]);
void draw_bits(unsigned char display[HEIGHT][WIDTH]);

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

unsigned char keymap[16]={
    SDLK_x,
    SDLK_1,
    SDLK_2,
    SDLK_3,
    SDLK_q,
    SDLK_w,
    SDLK_e,
    SDLK_a,
    SDLK_s,
    SDLK_d,
    SDLK_z,
    SDLK_c,
    SDLK_4,
    SDLK_r,
    SDLK_f,
    SDLK_v
};

unsigned char shift_flag = 0;
unsigned char jump_flag = 0;
unsigned char debug = 0;
unsigned char spacebar = 0;

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
    memset(chip->key,0,sizeof(chip->key));
    chip->I = 0;
    chip->delay = 0;
    chip->sound = 0;
    chip->PC = 0x200;
    chip->stack_pointer = 0;
    chip->op_counter = 0;
    chip->draw_flag = 0;
    
    for(int i = 0; i<sizeof(font_set); i++){
        chip->memory[i+0x50] = font_set[i];
    }
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(WIDTH*10, HEIGHT*10, 0, &window, &renderer);
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
    unsigned int x_coord;
    unsigned int y_coord;
    unsigned int tmp;
    
    if(debug){
        if(!spacebar){
            chip->PC -=2;
            return;
        }
        printf("Next instr: %X\n",*opcode);
    }

    switch(*instr){
        case 0x0:

            switch(NNN){
                //clear screen
                case 0x0E0:
                    memset(chip->display,0,sizeof(chip->display));
                    chip->draw_flag = 1;
                    //draw(chip->display);
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
            chip->PC = NNN;break;

        // call subroutine
        case 0x2:
            if(chip->stack_pointer > MAX_STACK_SIZE){
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
            chip->V[X] = NN;break;
        
        //add constant
        case 0x7:
            chip->V[X] += NN;break;
        
        //arithmetic and bool ops
        case 0x8:
            switch(N){
                //set reg X to reg Y
                case 0x0:
                    chip->V[X] = chip->V[Y];break;
                
                //binary OR
                case 0x1:
                    chip->V[X] |= chip->V[Y];break;
                
                //binary AND
                case 0x2:
                    chip->V[X] &= chip->V[Y];break;
                
                //binary XOR
                case 0x3:
                    chip->V[X] ^= chip->V[Y];break;

                //add registers
                case 0x4:
                    if(chip->V[X] + chip->V[Y] > 255)
                        chip->V[15] = 1;
                    else chip->V[15]=0;
                    chip->V[X] = (chip->V[X] + chip->V[Y]) & 0xFF;
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
                    chip->V[X] = chip->V[X]<<1;
                    break;

                default:
                    printf("Unknown opcode %X\n", N);
                    exit(1);
            }
            break;
        
        //set I register
        case 0xA:
            chip->I = NNN;break;

        //jump w/ offset
        case 0xB:
            if(jump_flag)
                chip->PC = chip->V[0] + NNN;
            else
                chip->PC = chip->V[X] + NNN;
            break;
        
        //random
        case 0xC:
            chip->V[X] = (rand()%256) & NN;break;
        
        //draw sprite
        case 0xD:
            x_coord = chip->V[X] % WIDTH; //starting point of sprite with wrapping
            y_coord = chip->V[Y] % HEIGHT;
            unsigned short curr_pointer = chip->I; //starting sprite loc in memory
            chip->V[15] = 0; //set VF to 0
            

            for(int i = y_coord; i<y_coord+N; i++){
                if(i>=HEIGHT) break; //vertical clipping

                //get current byte of memory
                unsigned char curr_mem = chip->memory[curr_pointer++];

                for(int j = x_coord; j<x_coord+8; j++){
                    if(j>=WIDTH) break; //horizontal clipping

                    unsigned char next_px = (curr_mem >> (7-j+x_coord)) & 0x1;
                    if(next_px){
                        chip->display[i][j] ^= next_px; //set new pixel value
                        if(!chip->V[15])
                            chip->V[15] = chip->display[i][j] ^ 0x1; //set VF if pixel flipped
                    }
                    //do nothing if next pixel is 0
                }
            }
            chip->draw_flag = 1;
            break;

        //Skip if key
        case 0xE:
            switch(NN){
                //skip to next instr if key in V[X] is currently pressed
                case 0x9E:
                    if(chip->key[chip->V[X]])
                        chip->PC+=2;
                    break;

                //skip to next instr if key in V[X] is not currently pressed
                case 0xA1:
                    if(!chip->key[chip->V[X]])
                        chip->PC+=2;
                    break;
            }
            break;
        
        case 0xF:
            switch(NN){
                //set reg X to delay timer
                case 0x07:
                    chip->V[X] = chip->delay;break;
                
                //set delay timer to reg X
                case 0x15:
                    chip->delay = chip->V[X];break;
                
                //set sound timer to reg X
                case 0x18:
                    chip->sound = chip->V[X];break;

                //add val of reg X to reg I
                case 0x1E:
                    chip->I += chip->V[X];
                    if(chip->I > 0xFFF) chip->V[15] = 1;
                    chip->I %= 0xFFF;
                    break;
                
                //block until key pressed
                case 0x0A:
                    //tmp = input_handler();
                    for(int i = 0; i<16; i++){
                        if(chip->key[i]){
                            chip->V[X] = i;
                            chip->PC+=2;
                            break;
                        }
                    }
                    chip->PC-=2;
                    break;
                
                // set I to address of font char stored in reg X
                case 0x29:
                    chip->I = 0x50 + (5*(chip->V[X] & 0xF));break;
                
                // Binary-coded decimanl conversion
                case 0x33:
                    tmp = chip->V[X];
                    for(int i = chip->I+2; i>=chip->I; i--){
                        chip->memory[i] = tmp % 10;
                        tmp /= 10;
                    }
                    break;
                
                // store to memory
                case 0x55:
                    tmp = chip->I;
                    for(int i = 0; i<=X; i++)
                        chip->memory[tmp+i] = chip->V[i];
                    
                    break;

                // load from memory
                case 0x65:
                    tmp = chip->I;
                    for(int i = 0; i<=X; i++)
                        chip->V[i] = chip->memory[tmp+i];
                    
                    break;

            }
            break;

        default:
            printf("Unknown opcode %X\n",*opcode);

    }

    //decrease timers
    if(++chip->op_counter == 8){
        if(chip->delay>0)
            --chip->delay;
        if(chip->sound>0){
            printf("BEEP\n");
            --chip->sound;
        }
        // emulate 60Hz
        sleep(0.006667);
        chip->op_counter = 0;
    }
    
    
    
}

//prints display bits in terminal
void draw_bits(unsigned char display[HEIGHT][WIDTH]){
    for(int i = 0; i<HEIGHT; i++){
        for(int j = 0; j<WIDTH; j++){
            printf("%X ",display[i][j]);
        }
        printf("\n");
    }
}

//draws rectangles representing each pixel on screen
void draw(unsigned char display[HEIGHT][WIDTH]){
    SDL_SetRenderDrawColor(renderer,0,0,0,0);
    SDL_RenderClear(renderer);
    for (int i = 0; i < HEIGHT; i++){
        for(int j = 0; j < WIDTH; j++){
            //create rectangle to create pixel (each px is a 10x10 px rectangle)
            SDL_Rect r;
            r.x = 10*j;
            r.y = 10*i;
            r.w = 10;
            r.h = 10;
            SDL_SetRenderDrawColor(renderer,0,0,0,0);
            if(display[i][j]==0x1){
                SDL_SetRenderDrawColor(renderer,0,255,0,255);
            }
            
            SDL_RenderFillRect(renderer,&r);
            SDL_RenderDrawRect(renderer,&r);
            //SDL_RenderDrawPoint(renderer, 10*j, i);
        }
    }
    SDL_RenderPresent(renderer);
}

//handles keystrokes
// unsigned char input_handler(){
//     unsigned char k = 0xFF;
//     if(SDL_PollEvent(&event)){
//         switch(event.type){
//             case SDL_KEYDOWN:
//                 switch(event.key.keysym.sym){
//                     case SDLK_1:
//                         k = 0x1;break;
//                     case SDLK_2:
//                         k = 0x2;break;
//                     case SDLK_3:
//                         k = 0x3;break;
//                     case SDLK_4:
//                         k = 0xC;break;
//                     case SDLK_q:
//                         k = 0x4;break;
//                     case SDLK_w:
//                         k = 0x5;break;
//                     case SDLK_e:
//                         k = 0x6;break;
//                     case SDLK_r:
//                         k = 0xD;break;
//                     case SDLK_a:
//                         k = 0x7;break;
//                     case SDLK_s:
//                         k = 0x8;break;
//                     case SDLK_d:
//                         k = 0x9;break;
//                     case SDLK_f:
//                         k = 0xE;break;
//                     case SDLK_z:
//                         k = 0xA;break;
//                     case SDLK_x:
//                         k = 0x0;break;
//                     case SDLK_c:
//                         k = 0xB;break;
//                     case SDLK_v:
//                         k = 0xF;break;
//                     default:
//                         k = 0xFF;break;
//                 }
//             break;
//         }
//     }
//     if(k<0xFF) printf("key pressed: %X\n",k);
//     return k;
// }

void pipeline(chip8 *chip){
    unsigned char tmp1 = 0x0;
    unsigned short tmp2 = 0x0;
    unsigned char *instr = &tmp1;
    unsigned short *opcode = &tmp2;
    unsigned char quit = 0;
    while(!quit){
        
        *instr = 0x0;
        *opcode = 0x0;
        fetch(chip, instr, opcode);
        execute(chip, instr, opcode);
        if(chip->draw_flag){
            draw(chip->display);
            chip->draw_flag = 0;
        }
        if(SDL_PollEvent(&event)){
            switch(event.type){
                case SDL_KEYDOWN:
                    if(event.key.keysym.sym == SDLK_SPACE)
                        spacebar = 1;
                    
                    for(int i = 0; i<16; i++){
                        if(event.key.keysym.sym == keymap[i])
                            chip->key[i] = 1;
                    }
                    
                    break;
                
                case SDL_KEYUP:

                    if(event.key.keysym.sym == SDLK_SPACE)
                        spacebar = 0;

                    for(int i = 0; i<16; i++){
                        if(event.key.keysym.sym == keymap[i])
                            chip->key[i] = 0;
                    }
                    break;

                case SDL_QUIT:
                    quit = 1;
                    break;

            }
        }
        
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char* argv[]){
    if(argc<2) exit(1);

    chip8 chip;
    init(&chip);

    // load CHIP-8 program, exit on unsuccessful load
    if(load_program(chip.memory, argv[1])==-1) exit(1);

    pipeline(&chip);    
}