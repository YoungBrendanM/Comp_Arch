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
	uint32_t* instruction = translate_instruction(mem_read_32(CURRENT_STATE.PC));
	switch (*instruction) {
		case 0x0F: { //Load Upper Immediate
			printf("LUI\n");
			NEXT_STATE.REGS[instruction[2]]=instruction[3]<<16;
			break;
		}
		case 0x09: { //Add Immediate Unsigned
			printf("ADDIU\n");
			NEXT_STATE.REGS[instruction[2]]=CURRENT_STATE.REGS[1]+instruction[3];
			break;
		}
		case 0x2B: { //Store Word
			printf("SW\n");
			mem_write_32(CURRENT_STATE.REGS[instruction[1]+instruction[3]], CURRENT_STATE.REGS[instruction[2]]);
			break;
		}
		case 0x23: { // Load Word
			printf("LW\n");
			NEXT_STATE.REGS[instruction[2]] = mem_read_32(CURRENT_STATE.REGS[instruction[1]]+instruction[3]);
			break;
		}
		case 0x08: { // Add Immediate
			printf("ADDI\n");
			NEXT_STATE.REGS[instruction[2]] = ((int) CURRENT_STATE.REGS[instruction[1]]) + ((int) instruction[3]);
			break;
		}
		case 0x0E: { // Xor Immediate
			printf("ORI\n");
			NEXT_STATE.REGS[instruction[2]]=CURRENT_STATE.REGS[1] | instruction[3];
			break;
		}
		case 0x0D: { // Xor Immediate
			printf("XORI\n");
			NEXT_STATE.REGS[instruction[2]]=CURRENT_STATE.REGS[1]^instruction[3];
			break;
		}
		case 0x0C: { // And Immediate
			printf("ANDI\n");
			NEXT_STATE.REGS[instruction[2]]=CURRENT_STATE.REGS[1] & instruction[3];
			break;
		}
		case 0x02: { // Jump
			printf("J\n");
			NEXT_STATE.PC = (CURRENT_STATE.PC & 0xF0000000) | instruction[1] << 2;
			break;
		}
		case 0x00: { //function bits
			switch (instruction[5]) {
				case 0x20: { //Add
					printf("ADD\n");
					NEXT_STATE.REGS[instruction[3]] = ((int) CURRENT_STATE.REGS[instruction[2]]) + ((int) CURRENT_STATE.REGS[instruction[1]]);
					break;
				}
				case 0x21: { //Add Unsigned
					printf("ADDU\n");
					NEXT_STATE.REGS[instruction[3]] = CURRENT_STATE.REGS[instruction[2]] + CURRENT_STATE.REGS[instruction[1]];
					break;
				}
				case 0x25: { //Or
					printf("OR\n");
					NEXT_STATE.REGS[instruction[3]] = CURRENT_STATE.REGS[instruction[2]] | CURRENT_STATE.REGS[instruction[1]];
					break;
				}
				case 0x00: { //Shift Left Logical
					printf("SLL\n");
					NEXT_STATE.REGS[instruction[3]] = CURRENT_STATE.REGS[instruction[2]] << instruction[4];
					break;
				}
				case 0x23: { //Subtract Unsigned
					printf("SUBU\n");
					NEXT_STATE.REGS[instruction[3]] = CURRENT_STATE.REGS[instruction[1]] - CURRENT_STATE.REGS[instruction[2]];
					break;
				}
				case 0x26: { //Exclusive Or
					printf("XOR\n");
					NEXT_STATE.REGS[instruction[3]] = CURRENT_STATE.REGS[instruction[2]] ^ CURRENT_STATE.REGS[instruction[1]];
					break;
				}
				case 0x02: { //Shift Right Logical
					printf("SRL\n");
					NEXT_STATE.REGS[instruction[3]] = CURRENT_STATE.REGS[instruction[2]] >> instruction[4];
					break;
				}
				case 0x03: { //Shift Right Arithmetic
					printf("SRA\n");
					NEXT_STATE.REGS[instruction[3]] = (CURRENT_STATE.REGS[instruction[2]] >> instruction[4]) & 0x80000000;
					break;
				}
				case 0x24: { //And
					printf("AND\n");
					NEXT_STATE.REGS[instruction[3]] = CURRENT_STATE.REGS[instruction[2]] & CURRENT_STATE.REGS[instruction[1]];
					break;
				}
				case 0x22: { //Subtract
					printf("SUB\n");
					NEXT_STATE.REGS[instruction[3]] = ((int) CURRENT_STATE.REGS[instruction[1]]) - ((int) CURRENT_STATE.REGS[instruction[2]]);
					break;
				}
				case 0x18: { //Multiply
					printf("MULT\n");
					long result = ((int) CURRENT_STATE.REGS[instruction[1]])*((int) CURRENT_STATE.REGS[instruction[2]]);
					NEXT_STATE.HI = result>>32;
					NEXT_STATE.LO = result & 0xFFFFFFFF;
					break;
				}
				case 0x19: { //Multiply Unsigned
					printf("MULTU\n");
					uint64_t result = CURRENT_STATE.REGS[instruction[1]]*CURRENT_STATE.REGS[instruction[2]];
					NEXT_STATE.HI = result>>32;
					NEXT_STATE.LO = result & 0xFFFFFFFF;
					break;
				}
				case 0x1A: { //Divide
					printf("DIV\n");
					NEXT_STATE.LO = ((int) CURRENT_STATE.REGS[instruction[1]])/((int) CURRENT_STATE.REGS[instruction[2]]);
					NEXT_STATE.HI = ((int) CURRENT_STATE.REGS[instruction[1]])%((int) CURRENT_STATE.REGS[instruction[2]]);
					break;
				}
				case 0x1B: { //Divide Unsigned
					printf("DIVU\n");
					NEXT_STATE.LO = (CURRENT_STATE.REGS[instruction[1]])/(CURRENT_STATE.REGS[instruction[2]]);
					NEXT_STATE.HI = (CURRENT_STATE.REGS[instruction[1]])%(CURRENT_STATE.REGS[instruction[2]]);
					break;
				}
				case 0x27: { //Nor
					printf("NOR\n");
					NEXT_STATE.REGS[instruction[3]] = ~(CURRENT_STATE.REGS[instruction[2]] | CURRENT_STATE.REGS[instruction[1]]);
					break;
				}
				default: {	
					for(int i=0; i<6;i++) {
						printf("0x%X\n", instruction[i]);
					}
					RUN_FLAG=FALSE;
				}
			}
			break;
		}
		default: {	
			for(int i=0; i<6;i++) {
				printf("0x%X\n", instruction[i]);
			}
			RUN_FLAG=FALSE;
		}
	}
	NEXT_STATE.PC+=4;
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
	//printf("%08x\n", addr); //for debugging
	uint32_t instruction = (mem_read_32(addr));
	unsigned mask = createMask(26,31); //last six bits mask, opcode
	//printf("%08x\n", instruction);
	unsigned opcode = mask & instruction;
	//printf("%x\n", first_six);
	switch(opcode)
	{
		case 0x08: //unsigned add ADDI
		{
			printf("ADDI ");
			unsigned rs_mask = createMask(21,25);
			unsigned rt_mask = createMask(16,20);
			unsigned imm_mask = createMask(0,15);
			unsigned rs = applyMask(rs_mask, instruction);
			unsigned rt = applyMask(rt_mask, instruction);
			unsigned immediate = applyMask(imm_mask, instruction);
			printf("%x, %x, %x\n", rs, rt, immediate); 
			
		}	
		case 0x09: //ADDIU
		{
			printf("ADDIU ");
			unsigned rs_mask = createMask(21,25);
			unsigned rt_mask = createMask(16,20);
			unsigned imm_mask = createMask(0,15);
			unsigned rs = applyMask(rs_mask, instruction);
			unsigned rt = applyMask(rt_mask, instruction);
			unsigned immediate = applyMask(imm_mask, instruction);
			printf("%x, %x, %x\n", rs, rt, immediate); 
		}	
		case 0x0C: //ANDI
		{
			printf("ANDI ");
			unsigned rs_mask = createMask(21,25);
			unsigned rt_mask = createMask(16,20);
			unsigned imm_mask = createMask(0,15);
			unsigned rs = applyMask(rs_mask, instruction);
			unsigned rt = applyMask(rt_mask, instruction);
			unsigned immediate = applyMask(imm_mask, instruction);
			printf("%x, %x, %x\n", rs, rt, immediate); 
		}
		case 0x0D: //ORI
		{
			printf("ORI ");
			unsigned rs_mask = createMask(21,25);
			unsigned rt_mask = createMask(16,20);
			unsigned imm_mask = createMask(0,15);
			unsigned rs = applyMask(rs_mask, instruction);
			unsigned rt = applyMask(rt_mask, instruction);
			unsigned immediate = applyMask(imm_mask, instruction);
			printf("%x, %x, %x\n", rs, rt, immediate); 
		}
		case 0x00: //special case when first six bits are 000000
		{
			unsigned func_mask = createMask(0,5);
			unsigned func = applyMask(func_mask, instruction);
			switch(func)
			{
				case 0x20: //ADD
				{
					printf("ADD ");
					unsigned rs_mask = createMask(21,25);
					unsigned rt_mask = createMask(16,20);
					unsigned rd_mask = createMask(11,15);
					unsigned rs = applyMask(rs_mask, instruction);
					unsigned rt = applyMask(rt_mask, instruction);
					unsigned rd = applyMask(rd_mask, instruction);
					printf("%x, %x, %x\n", rd, rs, rt);
				}
				case 0x21: //ADDU
				{
					printf("ADDU ");
					unsigned rs_mask = createMask(21,25);
					unsigned rt_mask = createMask(16,20);
					unsigned rd_mask = createMask(11,15);
					unsigned rs = applyMask(rs_mask, instruction);
					unsigned rt = applyMask(rt_mask, instruction);
					unsigned rd = applyMask(rd_mask, instruction);
					printf("%x, %x, %x\n", rd, rs, rt);
				}
				case 0x22: //SUB
				{
					printf("SUB ");
					unsigned rs_mask = createMask(21,25);
					unsigned rt_mask = createMask(16,20);
					unsigned rd_mask = createMask(11,15);
					unsigned rs = applyMask(rs_mask, instruction);
					unsigned rt = applyMask(rt_mask, instruction);
					unsigned rd = applyMask(rd_mask, instruction);
					printf("%x, %x, %x\n", rd, rs, rt);
				}
				case 0x23: //SUBU
				{
					printf("SUBU ");
					unsigned rs_mask = createMask(21,25);
					unsigned rt_mask = createMask(16,20);
					unsigned rd_mask = createMask(11,15);
					unsigned rs = applyMask(rs_mask, instruction);
					unsigned rt = applyMask(rt_mask, instruction);
					unsigned rd = applyMask(rd_mask, instruction);
					printf("%x, %x, %x\n", rd, rs, rt);
				}
				case 0x18: //MULT
				{
					printf("MULT ");
					unsigned rs_mask = createMask(21,25);
					unsigned rt_mask = createMask(16,20);
					unsigned rs = applyMask(rs_mask, instruction);
					unsigned rt = applyMask(rt_mask, instruction);
					printf("%x, %x\n", rs, rt); 
				}
				case 0x19: //MULTU
				{
					printf("MULTU ");
					unsigned rs_mask = createMask(21,25);
					unsigned rt_mask = createMask(16,20);
					unsigned rs = applyMask(rs_mask, instruction);
					unsigned rt = applyMask(rt_mask, instruction);
					printf("%x, %x\n", rs, rt); 
				}
				case 0x1A: //DIV
				{
					printf("DIV ");
					unsigned rs_mask = createMask(21,25);
					unsigned rt_mask = createMask(16,20);
					unsigned rs = applyMask(rs_mask, instruction);
					unsigned rt = applyMask(rt_mask, instruction);
					printf("%x, %x\n", rs, rt); 
				}
				case 0x1B: //DIVU
				{
					printf("DIVU ");
					unsigned rs_mask = createMask(21,25);
					unsigned rt_mask = createMask(16,20);
					unsigned rs = applyMask(rs_mask, instruction);
					unsigned rt = applyMask(rt_mask, instruction);
					printf("%x, %x\n", rs, rt); 	
				}
				case 0x24: //AND
				{
					printf("AND ");
					unsigned rs_mask = createMask(21,25);
					unsigned rt_mask = createMask(16,20);
					unsigned rd_mask = createMask(11,15);
					unsigned rs = applyMask(rs_mask, instruction);
					unsigned rt = applyMask(rt_mask, instruction);
					unsigned rd = applyMask(rd_mask, instruction);
					printf("%x, %x, %x\n", rd, rs, rt);
				}
				case 0x25:
				{
					printf("OR ");
					unsigned rs_mask = createMask(21,25);
					unsigned rt_mask = createMask(16,20);
					unsigned rd_mask = createMask(11,15);
					unsigned rs = applyMask(rs_mask, instruction);
					unsigned rt = applyMask(rt_mask, instruction);
					unsigned rd = applyMask(rd_mask, instruction);
					printf("%x, %x, %x\n", rd, rs, rt);
				}
			}
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
