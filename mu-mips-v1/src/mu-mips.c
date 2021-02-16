#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_instruction();
	CURRENT_STATE = NEXT_STATE;
	INSTRUCTION_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			runAll(); 
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* decode and execute instruction                                                                     */ 
/************************************************************/
void handle_instruction()
{
    printf("%X ", mem_read_32(CURRENT_STATE.PC));
	/*IMPLEMENT THIS*/
	/* execute one instruction at a time. Use/update CURRENT_STATE and and NEXT_STATE, as necessary.*/
	uint32_t instruction, opcode, function, rs, rt, rd, sa, immediate, target;
	uint64_t product, p1, p2;
	
	uint32_t addr, data;
	
	int branch_jump = FALSE;
	
	printf("[0x%x]\t", CURRENT_STATE.PC);
	
	instruction = mem_read_32(CURRENT_STATE.PC);
	
	opcode = (instruction & 0xFC000000) >> 26;
	function = instruction & 0x0000003F;
	rs = (instruction & 0x03E00000) >> 21;
	rt = (instruction & 0x001F0000) >> 16;
	rd = (instruction & 0x0000F800) >> 11;
	sa = (instruction & 0x000007C0) >> 6;
	immediate = instruction & 0x0000FFFF;
	target = instruction & 0x03FFFFFF;
	
	if(opcode == 0x00){
		switch(function){
			case 0x00: //SLL
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] << sa;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x02: //SRL
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >> sa;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x03: //SRA 
				if ((CURRENT_STATE.REGS[rt] & 0x80000000) == 1)
				{
					NEXT_STATE.REGS[rd] =  ~(~CURRENT_STATE.REGS[rt] >> sa );
				}
				else{
					NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] >> sa;
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x08: //JR
				NEXT_STATE.PC = CURRENT_STATE.REGS[rs];
				branch_jump = TRUE;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x09: //JALR
				NEXT_STATE.REGS[rd] = CURRENT_STATE.PC + 4;
				NEXT_STATE.PC = CURRENT_STATE.REGS[rs];
				branch_jump = TRUE;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0C: //SYSCALL
				if(CURRENT_STATE.REGS[2] == 0xa){
					RUN_FLAG = FALSE;
					print_instruction(CURRENT_STATE.PC);
				}
				break;
			case 0x10: //MFHI
				NEXT_STATE.REGS[rd] = CURRENT_STATE.HI;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x11: //MTHI
				NEXT_STATE.HI = CURRENT_STATE.REGS[rs];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x12: //MFLO
				NEXT_STATE.REGS[rd] = CURRENT_STATE.LO;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x13: //MTLO
				NEXT_STATE.LO = CURRENT_STATE.REGS[rs];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x18: //MULT
				if ((CURRENT_STATE.REGS[rs] & 0x80000000) == 0x80000000){
					p1 = 0xFFFFFFFF00000000 | CURRENT_STATE.REGS[rs];
				}else{
					p1 = 0x00000000FFFFFFFF & CURRENT_STATE.REGS[rs];
				}
				if ((CURRENT_STATE.REGS[rt] & 0x80000000) == 0x80000000){
					p2 = 0xFFFFFFFF00000000 | CURRENT_STATE.REGS[rt];
				}else{
					p2 = 0x00000000FFFFFFFF & CURRENT_STATE.REGS[rt];
				}
				product = p1 * p2;
				NEXT_STATE.LO = (product & 0X00000000FFFFFFFF);
				NEXT_STATE.HI = (product & 0XFFFFFFFF00000000)>>32;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x19: //MULTU
				product = (uint64_t)CURRENT_STATE.REGS[rs] * (uint64_t)CURRENT_STATE.REGS[rt];
				NEXT_STATE.LO = (product & 0X00000000FFFFFFFF);
				NEXT_STATE.HI = (product & 0XFFFFFFFF00000000)>>32;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x1A: //DIV 
				if(CURRENT_STATE.REGS[rt] != 0)
				{
					NEXT_STATE.LO = (int32_t)CURRENT_STATE.REGS[rs] / (int32_t)CURRENT_STATE.REGS[rt];
					NEXT_STATE.HI = (int32_t)CURRENT_STATE.REGS[rs] % (int32_t)CURRENT_STATE.REGS[rt];
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x1B: //DIVU
				if(CURRENT_STATE.REGS[rt] != 0)
				{
					NEXT_STATE.LO = CURRENT_STATE.REGS[rs] / CURRENT_STATE.REGS[rt];
					NEXT_STATE.HI = CURRENT_STATE.REGS[rs] % CURRENT_STATE.REGS[rt];
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x20: //ADD
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x21: //ADDU 
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rt] + CURRENT_STATE.REGS[rs];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x22: //SUB
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x23: //SUBU
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] - CURRENT_STATE.REGS[rt];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x24: //AND
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] & CURRENT_STATE.REGS[rt];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x25: //OR
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x26: //XOR
				NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] ^ CURRENT_STATE.REGS[rt];
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x27: //NOR
				NEXT_STATE.REGS[rd] = ~(CURRENT_STATE.REGS[rs] | CURRENT_STATE.REGS[rt]);
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x2A: //SLT
				if(CURRENT_STATE.REGS[rs] < CURRENT_STATE.REGS[rt]){
					NEXT_STATE.REGS[rd] = 0x1;
				}
				else{
					NEXT_STATE.REGS[rd] = 0x0;
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			default:
				printf("Instruction at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	}
	else{
		switch(opcode){
			case 0x01:
				if(rt == 0x00000){ //BLTZ
					if((CURRENT_STATE.REGS[rs] & 0x80000000) > 0){
						NEXT_STATE.PC = CURRENT_STATE.PC + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);
						branch_jump = TRUE;
					}
					print_instruction(CURRENT_STATE.PC);
				}
				else if(rt == 0x00001){ //BGEZ
					if((CURRENT_STATE.REGS[rs] & 0x80000000) == 0x0){
						NEXT_STATE.PC = CURRENT_STATE.PC + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);
						branch_jump = TRUE;
					}
					print_instruction(CURRENT_STATE.PC);
				}
				break;
			case 0x02: //J
				NEXT_STATE.PC = (CURRENT_STATE.PC & 0xF0000000) | (target << 2);
				branch_jump = TRUE;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x03: //JAL
				NEXT_STATE.PC = (CURRENT_STATE.PC & 0xF0000000) | (target << 2);
				NEXT_STATE.REGS[31] = CURRENT_STATE.PC + 4;
				branch_jump = TRUE;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x04: //BEQ
				if(CURRENT_STATE.REGS[rs] == CURRENT_STATE.REGS[rt]){
					NEXT_STATE.PC = CURRENT_STATE.PC + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);
					branch_jump = TRUE;
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x05: //BNE
				if(CURRENT_STATE.REGS[rs] != CURRENT_STATE.REGS[rt]){
					NEXT_STATE.PC = CURRENT_STATE.PC + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);
					branch_jump = TRUE;
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x06: //BLEZ
				if((CURRENT_STATE.REGS[rs] & 0x80000000) > 0 || CURRENT_STATE.REGS[rs] == 0){
					NEXT_STATE.PC = CURRENT_STATE.PC +  ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);
					branch_jump = TRUE;
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x07: //BGTZ
				if((CURRENT_STATE.REGS[rs] & 0x80000000) == 0x0 || CURRENT_STATE.REGS[rs] != 0){
					NEXT_STATE.PC = CURRENT_STATE.PC +  ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);
					branch_jump = TRUE;
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x08: //ADDI
				NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x09: //ADDIU
				NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0A: //SLTI
				if ( (  (int32_t)CURRENT_STATE.REGS[rs] - (int32_t)( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF))) < 0){
					NEXT_STATE.REGS[rt] = 0x1;
				}else{
					NEXT_STATE.REGS[rt] = 0x0;
				}
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0C: //ANDI
				NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] & (immediate & 0x0000FFFF);
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0D: //ORI
				NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] | (immediate & 0x0000FFFF);
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0E: //XORI
				NEXT_STATE.REGS[rt] = CURRENT_STATE.REGS[rs] ^ (immediate & 0x0000FFFF);
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x0F: //LUI
				NEXT_STATE.REGS[rt] = immediate << 16;
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x20: //LB
				data = mem_read_32( CURRENT_STATE.REGS[rs] + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF)) );
				NEXT_STATE.REGS[rt] = ((data & 0x000000FF) & 0x80) > 0 ? (data | 0xFFFFFF00) : (data & 0x000000FF);
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x21: //LH
				data = mem_read_32( CURRENT_STATE.REGS[rs] + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF)) );
				NEXT_STATE.REGS[rt] = ((data & 0x0000FFFF) & 0x8000) > 0 ? (data | 0xFFFF0000) : (data & 0x0000FFFF);
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x23: //LW
				NEXT_STATE.REGS[rt] = mem_read_32( CURRENT_STATE.REGS[rs] + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF)) );
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x28: //SB
				addr = CURRENT_STATE.REGS[rs] + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				data = mem_read_32( addr);
				data = (data & 0xFFFFFF00) | (CURRENT_STATE.REGS[rt] & 0x000000FF);
				mem_write_32(addr, data);
				print_instruction(CURRENT_STATE.PC);				
				break;
			case 0x29: //SH
				addr = CURRENT_STATE.REGS[rs] + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				data = mem_read_32( addr);
				data = (data & 0xFFFF0000) | (CURRENT_STATE.REGS[rt] & 0x0000FFFF);
				mem_write_32(addr, data);
				print_instruction(CURRENT_STATE.PC);
				break;
			case 0x2B: //SW
				addr = CURRENT_STATE.REGS[rs] + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				mem_write_32(addr, CURRENT_STATE.REGS[rt]);
				print_instruction(CURRENT_STATE.PC);
				break;
			default:
				// put more things here
				printf("Instruction at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	}
	
	if(!branch_jump){
		NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	}
}

uint32_t* translate_instruction(uint32_t instruction) {
	uint32_t* newInstruction = malloc(sizeof(uint32_t)*6);
	*newInstruction=instruction >> 26;
	switch(*newInstruction) {
		case 0x0F: case 0x09: case 0x2B: case 0x23: case 0x08:
			newInstruction[1]=instruction>>21&0x1F;
			newInstruction[2]=instruction>>16&0x1F;
			newInstruction[3]=instruction&0xFFFF;
			break;
		case 0x02: //not real opcode J-Type template
			newInstruction[1]=instruction&0x4FFFFFF;
			break;
		case 0x00: //not real opcode R-Type template
			newInstruction[1]=instruction>>21&0x2F;
			newInstruction[2]=instruction>>16&0x1F;
			newInstruction[3]=instruction>>11&0x1F;
			newInstruction[4]=instruction>>6&0x1F;
			newInstruction[5]=instruction>>0&0x2F;
			break;
		default:
			newInstruction[5]=0xFF;
	}
	return newInstruction;
}

/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){
	int i;
	uint32_t addr;
	
	for(i=0; i<PROGRAM_SIZE; i++){
		addr = MEM_TEXT_BEGIN + (i*4);
		printf("[0x%x]\t", addr);
		print_instruction(addr);
	}
}

/************************************************************/
/* Print the instruction at given memory address (in MIPS assembly format)    */
/************************************************************/
void print_instruction(uint32_t addr){
	/*IMPLEMENT THIS*/
	//printf("%x
	uint32_t instruction = (mem_read_32(addr)); //reading in address from mem
	//printf("address = 0x%x\n", instruction);
	
	//creating bit massk
	unsigned opcode_mask = createMask(26,31); //last six bits mask, opcode

	unsigned rs_mask = createMask(21,25);
	unsigned rt_mask = createMask(16,20);
	unsigned imm_mask = createMask(0,15);	
	unsigned base_mask = createMask(21,25);
	unsigned offset_mask = createMask(0,15);
	unsigned target_mask = createMask(0,26);	
	unsigned sa_mask = createMask(6,10);
	unsigned branch_mask = createMask(16,20);	
	unsigned func_mask = createMask(0,5);
	unsigned rd_mask = createMask(11,15);
	

	//applying masks to get parts of command	
	unsigned opcode = applyMask(opcode_mask, instruction);
	unsigned rs = applyMask(rs_mask, instruction);
	unsigned rt = applyMask(rt_mask, instruction);
	unsigned immediate = applyMask(imm_mask, instruction);
	unsigned base = applyMask(base_mask, instruction);
	unsigned offset = applyMask(offset_mask, instruction);
	unsigned target = applyMask(target_mask, instruction);
	unsigned sa = applyMask(sa_mask, instruction);
	unsigned branch = applyMask(branch_mask, instruction);
	unsigned func = applyMask(func_mask, instruction);
	unsigned rd = applyMask(rd_mask, instruction);
	
	switch(opcode)
	{
		case 0x21000000: //add ADDI
		{
			printf("ADDI ");
			printf("$%x $%x 0x%x\n", rs, rt, immediate); 
			break;
		}	
		case 0x24000000: //ADDIU
		{
			printf("ADDIU ");
			printf("$%x $%x 0x%04x\n", rs, rt, immediate); 
			break;
		}	
		case 0x30000000: //ANDI
		{
			printf("ANDI ");
			printf("$%x $%x 0x%x\n", rs, rt, immediate); 
			break;
		}
		case 0x34000000: //ORI
		{
			printf("ORI ");
			printf("$%x $%x, 0x%04x\n", rs, rt, immediate); 
			break;
		}
		case 0x38000000: //XORI
		{
			printf("XORI ");
			printf("$%x $%x 0x%x\n", rs, rt, immediate);
			break; 
		}
		case 0x0A000000: //STLI set on less than immediate
		{
			printf("STLI ");
			printf("$%x $%x 0x%x\n", rs, rt, immediate); 
			break;
		}
		case 0x8C000000: //Load Word - for now on is load/store instructions mostly
		{
			printf("LW ");
			printf("$%x 0x%x $%x\n", rt, offset, base); 
			break;
		}
		case 0x20000000: //Load Byte LB
		{
			printf("LB ");
			printf("$%x, 0x%x $%x)\n", rt, offset, base); 
			break;
		}
		case 0x85000000: //Load halfword
		{
			printf("LH ");
			printf("$%x 0x%x $%x)\n", rt, offset, base); 
			break;
		}
		case 0x3C000000: //LUI Load Upper Immediate, was 0F
		{
			printf("LUI ");
			printf("$%x 0x%x\n", rt, immediate); 
			break;
		}
		case 0xAC000000: //SW Store Word
		{
			printf("SW ");
			printf("$%x 0x%x $%x\n", rt, offset, base); 
			break;
		}
		case 0x28000000: //SB Store Byte CHECK FORMAT
		{
			printf("SB ");
			printf("$%x 0x%x $%x\n", rt, offset, base); 
			break;
		}
		case 0x29000000: //SH Store Halfword CHECK FORMAT
		{
			printf("SH ");
			printf("$%x 0x%x $%x)\n", rt, offset, base); 
			break;
		}
		case 0x10000000: //BEQ Branch if equal - start of branching instructions
		{
			printf("BEQ ");
			printf("$%x $%x 0x%04x\n", rs, rt, offset); 
			break;
		}
		case 0x14000000: //BNE Branch on Not Equal
		{
			printf("BNE ");
			printf("$%x $%x 0x%04x\n", rs, rt, offset); 
			break;
		}
		case 0x06000000: //BLEZ Brnach on Less than or equal to zero 
		{
			printf("BLEZ ");
			printf("$%x 0x%x\n", rs, offset); 
			break;
		}
		case 0x01000000: //special branch cases
		{		
			switch(branch)
			{
				case 0x00: //BLTZ Brnach on Less than zero
				{
					printf("BLTZ ");
					printf("#%x 0x%x\n", rs, offset); 
					break;
				}
				case 0x01: // BGEZ Branch on greater than or equal zero
				{
					printf("BGEZ ");
					printf("$%x 0x%x\n", rs, offset); 
					break;
				}
			}
					
			
		}
		case 0x07000000: //BGTZ Branch on Greater than Zero
		{
			printf("BLEZ ");
			printf("$%x 0x%x\n", rs, offset); 
			break;
		}
		case 0x08000000: //Jump J (bum bum bummmm bum, RIP Eddie VanHalen)
		{
			printf("J ");
			printf("0x%x\n", (addr & 0xF0000000) | (target));
			break;
		}
		case 0x03000000: //JAL Jump and Link
		{
			printf("JAL ");
			printf("0x%x\n", (addr & 0xF0000000) | (target));
			break;
		}
		case 0x00000000: //special case when first six bits are 000000, function operations
		{
			switch(func)
			{
				case 0x20: //ADD
				{
					printf("ADD ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x21: //ADDU
				{
					printf("ADDU ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x22: //SUB
				{
					printf("SUB ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x23: //SUBU
				{
					printf("SUBU ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x18: //MULT
				{
					printf("MULT ");
					printf("$%x $%x\n", rs, rt); 
					break;
				}
				case 0x19: //MULTU
				{
					printf("MULTU ");
					printf("$%x $%x\n", rs, rt); 
					break;
				}
				case 0x1A: //DIV
				{
					printf("DIV ");
					printf("$%x $%x\n", rs, rt); 
					break;
				}
				case 0x1B: //DIVU
				{
					printf("DIVU ");
					printf("$%x $%x\n", rs, rt); 
					break;	
				}
				case 0x24: //AND
				{
					printf("AND ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x25: //OR
				{
					printf("OR ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x26: //XOR
				{
					printf("XOR ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x27: //NOR
				{
					printf("NOR ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x2A: //SLT Set on less than
				{
					printf("SLT ");
					printf("$%x $%x $%x\n", rd, rs, rt);
					break;
				}
				case 0x00: //SLL Shift Left Logical NEED TO CHECK FORMAT
				{
					printf("SLL ");
					printf("$%x $%x %x\n", rd, rt, sa); //different from the rest with the sa thingy
					break;
				}
				case 0x02: //SRL Shift Right Logical CHECK FORMAT
				{
					printf("SRL ");
					printf("$%x $%x %x\n", rd, rt, sa); //different from the rest with the sa thingy
					break;
				}
				case 0x03: //SRA Shift Right Arithmetic
				{
					printf("SRA ");
					printf("$%x $%x %x\n", rd, rt, sa); //different from the rest with the sa thingy
					break;
				} 
				case 0x10: //MFHI Move from HI
				{
					printf("MFHI ");
					printf("$%x\n", rd); //only the rd register
					break;
				}
				case 0x12: //MFLO Move from LO
				{
					printf("MFHI ");
					printf("$%x\n", rd); //only the rd register
					break;
				}
				case 0x11: //MTHI Move to HI
				{
					printf("MFHI ");
					printf("$%x\n", rs); //only the rs register
					break;
				}
				case 0x13: //MTLO Move to LO
				{
					printf("MFHI ");
					printf("$%x\n", rs); //only the rs register
					break;
				}
				case 0x08: //JR Jump Register
				{
					printf("JR ");
					printf("%x\n", rs); //only the rs register
					break;
				}
				case 0x09: //JALR Jump and Link Register CHECK FORMAT
				{
					printf("JR ");
					if(rd == 0x1F) //if rd is all one's (or 31) then not given
					{
						printf("$%x\n", rs);
					}
					else //rd is given
						printf("$%x $%x\n", rd, rs);
					break;
				}
				case 0x0C: //SYSCALL
				{
					printf("SYSCALL\n");
					break;
				}
			}

			
			break;
		}			
		default:
		{
			printf("Command not found...\n");
			break;
		}
			
	}	
	
}

unsigned createMask(unsigned a, unsigned b)
{
   unsigned r = 0;
   for (unsigned i=a; i<=b; i++)
       r |= 1 << i;

   return r;
}

unsigned applyMask(unsigned mask, uint32_t instruction)
{
	return mask & instruction;
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
