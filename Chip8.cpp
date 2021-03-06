#include "Chip8.hpp"
#include <fstream>
#include <cstring>
#include <random>
#include <chrono>
#include <cstdint>

const unsigned int START_ADDRESS = 0x200; 
const unsigned int FONTSET_START_ADDRESS =0x50; 


const unsigned int FONTSET_SIZE =80; //16 characters at 5 bytes each 
//first four bits (nibble) are used for drawing a number or character
uint8_t fontset[FONTSET_SIZE] =
	{
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

Chip8::Chip8()
	: randGen(std::chrono::system_clock::now().time_since_epoch().count())
{
    pc = START_ADDRESS; //PC starts at 0x200 since first instruction executed

    for(unsigned int i=0; i<FONTSET_SIZE; ++i){
        memory[FONTSET_START_ADDRESS +i] = fontset[i]; 
    }

   	// Initialize RNG
	randByte = std::uniform_int_distribution<uint8_t>(0, 255U);

	// Set up function pointer table
	table[0x0] = &Chip8::Table0;
	table[0x1] = &Chip8::OP_1nnn;
	table[0x2] = &Chip8::OP_2nnn;
	table[0x3] = &Chip8::OP_3xkk;
	table[0x4] = &Chip8::OP_4xkk;
	table[0x5] = &Chip8::OP_5xy0;
	table[0x6] = &Chip8::OP_6xkk;
	table[0x7] = &Chip8::OP_7xkk;
	table[0x8] = &Chip8::Table8;
	table[0x9] = &Chip8::OP_9xy0;
	table[0xA] = &Chip8::OP_Annn;
	table[0xB] = &Chip8::OP_Bnnn;
	table[0xC] = &Chip8::OP_Cxkk;
	table[0xD] = &Chip8::OP_Dxyn;
	table[0xE] = &Chip8::TableE;
	table[0xF] = &Chip8::TableF;

	table0[0x0] = &Chip8::OP_00E0;
	table0[0xE] = &Chip8::OP_00EE;

	table8[0x0] = &Chip8::OP_8xy0;
	table8[0x1] = &Chip8::OP_8xy1;
	table8[0x2] = &Chip8::OP_8xy2;
	table8[0x3] = &Chip8::OP_8xy3;
	table8[0x4] = &Chip8::OP_8xy4;
	table8[0x5] = &Chip8::OP_8xy5;
	table8[0x6] = &Chip8::OP_8xy6;
	table8[0x7] = &Chip8::OP_8xy7;
	table8[0xE] = &Chip8::OP_8xyE;

	tableE[0x1] = &Chip8::OP_ExA1;
	tableE[0xE] = &Chip8::OP_Ex9E;

	tableF[0x07] = &Chip8::OP_Fx07;
	tableF[0x0A] = &Chip8::OP_Fx0A;
	tableF[0x15] = &Chip8::OP_Fx15;
	tableF[0x18] = &Chip8::OP_Fx18;
	tableF[0x1E] = &Chip8::OP_Fx1E;
	tableF[0x29] = &Chip8::OP_Fx29;
	tableF[0x33] = &Chip8::OP_Fx33;
	tableF[0x55] = &Chip8::OP_Fx55;
	tableF[0x65] = &Chip8::OP_Fx65;  
}

//loads the contents of a rom file 
void Chip8::LoadROM(char const* filename){
    
    std::ifstream file(filename, std::ios::binary | std::ios::ate); 

    if(file.is_open()){
        
        //get size of buffer
        std::streampos size = file.tellg();
        //allocate buffer to hold size  
        char* buffer = new char[size]; 

        //go back to start and read file into buffer
        file.seekg(0,std::ios::beg); 
        file.read(buffer,size); 
        file.close(); 

        //starting at 0x200, load ROM contents into Chip8's memory 
        for(long i =0; i<size; ++i){
            memory[START_ADDRESS+i] = buffer[i];
        }

        delete[] buffer; 
    }
}

//fetch decode and execute one cycle of CPU
void Chip8::Cycle(){
    opcode = (memory[pc] << 8u) | memory[pc+1]; 

    pc+=2; 

    ((*this).*(table[(opcode & 0xF000u) >> 12u]))(); 

    if(delayTimer > 0){
        --delayTimer; 
    }

    if(soundTimer >0){
        --soundTimer; 
    }
}

void Chip8::Table0()
{
	((*this).*(table0[opcode & 0x000Fu]))();
}

void Chip8::Table8()
{
	((*this).*(table8[opcode & 0x000Fu]))();
}

void Chip8::TableE()
{
	((*this).*(tableE[opcode & 0x000Fu]))();
}

void Chip8::TableF()
{
	((*this).*(tableF[opcode & 0x00FFu]))();
}

void Chip8::OP_NULL()
{}



//OP Codes

//clear display 
void Chip8::OP_00E0(){
    memset(video,0,sizeof(video)); 
}

//return from subroutine 
//stack has address of one instruction past the one that called subroutine
void Chip8::OP_00EE(){
    --sp; 
    pc= stack[sp]; 
}

//jump to address nnn 
void Chip8::OP_1nnn(){
    uint16_t address = opcode & 0x0FFFu; 
    pc=address; 
}

//call subroutine at nnn
void Chip8::OP_2nnn(){
    uint16_t address = opcode & 0x0FFFu; 

    stack[sp] = pc;
    ++sp; 
    pc = address;  
}

//skip next instruction if Vx=kk
void Chip8::OP_3xkk(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; 
    uint8_t byte = opcode & 0x00FFu;  

    if(V_register[Vx] == byte){
        pc+=2; 
    }
}

//skip next instruction if Vx!=kk
void Chip8::OP_4xkk(){
    uint8_t Vx = (opcode & 0x0F00u) >>8u; 
    uint8_t byte = opcode & 0x00FFu; 

    if(V_register[Vx] != byte){
        pc += 2; 
    }
}

//skip instruction if Vx==Vy
void Chip8::OP_5xy0(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; 
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; 

    if(V_register[Vx] == V_register[Vy]){
        pc +=2; 
    }
}

//sets Vx to kk
void Chip8::OP_6xkk(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; 
    uint8_t byte = opcode & 0x00FFu; 

    V_register[Vx] = byte; 
}

//adds kk to Vx
void Chip8::OP_7xkk(){
    uint8_t Vx = (opcode&0x0F00u) >> 8u; 
    uint8_t byte = opcode & 0x00FFu; 

    V_register[Vx] += byte; 
}

//sets vx to value of vy 
void Chip8::OP_8xy0(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; 
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; 

    V_register[Vx] = V_register[Vy];
}

//sets VX to VX or VY
void Chip8::OP_8xy1(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; 
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; 

    V_register[Vx] |= V_register[Vy];
}

//sets VX to VX & VY
void Chip8::OP_8xy2(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; 
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; 

    V_register[Vx] &= V_register[Vy];
}

//sets VX to VX xor VY
void Chip8::OP_8xy3(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; 
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; 

    V_register[Vx] ^= V_register[Vy];
}

//adds VY to VX
//set VF when there is carry 
void Chip8::OP_8xy4(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; 
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; 

    uint16_t sum = V_register[Vx] + V_register[Vy]; 

    if(sum > 255u){
        V_register[0xF] =1; 
    }
    else{
        V_register[0xF] =0; 
    }

    V_register[Vx] = sum & 0xFFu; 
}

//substracts VY from VX
//set VF when there is a borrow
void Chip8::OP_8xy5(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; 
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; 

    if(V_register[Vx] >  V_register[Vy]){
        V_register[0xF] =1; 
    }
    else{
        V_register[0xF] =0; 
    }

    V_register[Vx] -= V_register[Vy]; 
}

//stores LSB of Vx in Vf 
//shft Vx by 1 
void Chip8::OP_8xy6(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; 

    V_register[0xF] = V_register[Vx] & 0x1u;
    V_register[Vx]  >>= 1; 
}

//substracts Vx from Vy
//set Vf when there is a borrow
void Chip8::OP_8xy7(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; 
    uint8_t Vy = (opcode & 0x00F0u) >> 4u; 

    if(V_register[Vy] >  V_register[Vx]){
        V_register[0xF] =1; 
    }
    else{
        V_register[0xF] =0; 
    }

    V_register[Vx] = V_register[Vy] - V_register[Vx]; 
}

//store MSB in VF and left by 1 
void Chip8::OP_8xyE(){
    uint8_t Vx = (opcode & 0x0F00u) >> 8u; 

    V_register[0xF] = (V_register[Vx] & 0x80u) >> 7u;
    V_register[Vx]  <<= 1; 
}

//skips next instruciton if VX != VY
void Chip8::OP_9xy0()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;

	if (V_register[Vx] != V_register[Vy])
	{
		pc += 2;
	}
}

//set I to address nnn
void Chip8::OP_Annn()
{
	uint16_t address = opcode & 0x0FFFu;

	index = address;
}

//jumps to address NNN plus V0 
void Chip8::OP_Bnnn()
{
	uint16_t address = opcode & 0x0FFFu;

	pc = V_register[0] + address;
}

//set Vx to result and with rand number  
void Chip8::OP_Cxkk()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t byte = opcode & 0x00FFu;

	V_register[Vx] = randByte(randGen) & byte;
}

//draw sprite at Vx,Vy that is 8 pixels wide and N+1 pxiel tall
//VF set to 1 if any screen pixels are flipped 
void Chip8::OP_Dxyn()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (opcode & 0x00F0u) >> 4u;
	uint8_t height = opcode & 0x000Fu;

	// Wrap if going beyond screen boundaries
	uint8_t xPos = V_register[Vx] % VIDEO_WIDTH;
	uint8_t yPos = V_register[Vy] % VIDEO_HEIGHT;

	V_register[0xF] = 0;

	for (unsigned int row = 0; row < height; ++row)
	{
		uint8_t spriteByte = memory[index + row];

		for (unsigned int col = 0; col < 8; ++col)
		{
			uint8_t spritePixel = spriteByte & (0x80u >> col);
			uint32_t* screenPixel = &video[(yPos + row) * VIDEO_WIDTH + (xPos + col)];

			// Sprite pixel is on
			if (spritePixel)
			{
				// Screen pixel also on - collision
				if (*screenPixel == 0xFFFFFFFF)
				{
					V_register[0xF] = 1;
				}

				// Effectively XOR with the sprite pixel
				*screenPixel ^= 0xFFFFFFFF;
			}
		}
	}
}

//skip next instruction if key stored in Vx is pressed
void Chip8::OP_Ex9E()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	uint8_t key = V_register[Vx];

	if (keypad[key])
	{
		pc += 2;
	}
}

//skip next instruction if key stored in Vx is not pressed
void Chip8::OP_ExA1()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	uint8_t key = V_register[Vx];

	if (!keypad[key])
	{
		pc += 2;
	}
}

//set Vx to value of delay timer
void Chip8::OP_Fx07()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	V_register[Vx] = delayTimer;
}

//wait by decrementing PC by 2 whenever keypad value not detected
//so same instruction run 
void Chip8::OP_Fx0A()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	if (keypad[0])
	{
		V_register[Vx] = 0;
	}
	else if (keypad[1])
	{
		V_register[Vx] = 1;
	}
	else if (keypad[2])
	{
		V_register[Vx] = 2;
	}
	else if (keypad[3])
	{
		V_register[Vx] = 3;
	}
	else if (keypad[4])
	{
		V_register[Vx] = 4;
	}
	else if (keypad[5])
	{
		V_register[Vx] = 5;
	}
	else if (keypad[6])
	{
		V_register[Vx] = 6;
	}
	else if (keypad[7])
	{
		V_register[Vx] = 7;
	}
	else if (keypad[8])
	{
		V_register[Vx] = 8;
	}
	else if (keypad[9])
	{
		V_register[Vx] = 9;
	}
	else if (keypad[10])
	{
		V_register[Vx] = 10;
	}
	else if (keypad[11])
	{
		V_register[Vx] = 11;
	}
	else if (keypad[12])
	{
		V_register[Vx] = 12;
	}
	else if (keypad[13])
	{
		V_register[Vx] = 13;
	}
	else if (keypad[14])
	{
		V_register[Vx] = 14;
	}
	else if (keypad[15])
	{
		V_register[Vx] = 15;
	}
	else
	{
		pc -= 2;
	}
}

void Chip8::OP_Fx15()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	delayTimer = V_register[Vx];
}

void Chip8::OP_Fx18()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	soundTimer = V_register[Vx];
}

//add Vx to index, Vf not affected
void Chip8::OP_Fx1E()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	index += V_register[Vx];
}

//set i to location of sprite for digit Vx
void Chip8::OP_Fx29()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t digit = V_register[Vx];

	index = FONTSET_START_ADDRESS + (5 * digit);
}

void Chip8::OP_Fx33()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;
	uint8_t value = V_register[Vx];

	// Ones-place
	memory[index + 2] = value % 10;
	value /= 10;

	// Tens-place
	memory[index + 1] = value % 10;
	value /= 10;

	// Hundreds-place
	memory[index] = value % 10;
}

//store registers V0 to Vx in memory starting at location I
void Chip8::OP_Fx55()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	for (uint8_t i = 0; i <= Vx; ++i)
	{
		memory[index + i] = V_register[i];
	}
}

//read registers V0 to Vx from memory starting at location I
void Chip8::OP_Fx65()
{
	uint8_t Vx = (opcode & 0x0F00u) >> 8u;

	for (uint8_t i = 0; i <= Vx; ++i)
	{
		V_register[i] = memory[index + i];
	}
}
