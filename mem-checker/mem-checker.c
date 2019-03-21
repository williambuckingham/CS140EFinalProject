/*
 * Program to use interrupts for a statistical memory checker
 *	- interrupt code is a replication of ../timer-int/timer.c
 */
#include "rpi.h"
#include "timer-interrupt.h"
#include "mem-checker.h"
#define E mem_corruption_t
#include "Q.h"



/**********************************************************************/
/* 						heap alloc code 					          */
/**********************************************************************/

// ----------------------
// Constants/globals/data
// ----------------------
#define roundup(x,n) (((x)+((n)-1))&(~((n)-1)))

extern char __heap_start__;
extern char __bss_start__;
extern char __bss_end__;
static char* heap_pointer;
static const unsigned magic_number = 8;
static const unsigned shadow_mem_offset = 4000000;

// --------------------------
// Function : shadow_mem_init
// --------------------------
// This function 0 initialises the shadow memory, previously written garbage will certainly
// affect the checkers ability to tell if memory is allocated
// Params : number of buytes to allocate
// Return : void* pointer to allocated memory
void shadow_mem_init(){
	char* temp = &__heap_start__;
	for(int i = 0; i < 4000000; i++){
		temp[i+shadow_mem_offset] = 0;
	}
}

// ---------------
// Function : kfree
// ---------------
// This function is to free as kmalloc is to malloc. All it actually does is mark the 
// Params : void* pointer to be freed
// Return : void
void kfree(void* ptr){
	char* temp = (char*)ptr;
	unsigned index = 0;
	while(1){
		if(temp[index+shadow_mem_offset] == magic_number){
			temp[index+shadow_mem_offset] = 0;
			index++;
		} else {
			break;
		}
	}
}

// ------------------
// Function : kmalloc
// ------------------
// This function returns a pointer to nbytes of heap alllocated memory. It also marks
// that segment of memory as allocated in a shadow memory 4MB away
// Params : number of buytes to allocate
// Return : void* pointer to allocated memory
void *kmalloc(unsigned nbytes) {
	char* malloced_memory = heap_pointer;

	//Mark shadow memory as allocated
	for(int i = 0; i < nbytes; i++){
	 	(malloced_memory + shadow_mem_offset)[i] = magic_number;
	}

	heap_pointer += roundup(nbytes + 100, 8);
	return malloced_memory;
}

// -----------------------
// Function : kmalloc_init
// -----------------------
// This function runs simply initializes the heap_pointer to the heap start
// Params : void
// Return : void
void kmalloc_init(void) {
	heap_pointer = &__heap_start__;
}

/**********************************************************************/
/* 						mem check code 					              */
/**********************************************************************/

// ----------------------
// Constants/globals/data
// ----------------------

static Q_t mem_corruption_log;							//Log of all memory corruptions seen
volatile unsigned* pc_table;							//Table mapping PC to number of times PC seen in int handler

static volatile unsigned cnt;							//Number of times interrupt handler hit
static volatile unsigned num_corruptions;				//Number of times timer interrupt hits code corrupting memory
static volatile unsigned num_distinct_corruptions;		//Number of different PC's seen that corrupt memory
static volatile unsigned period;						//Period between interrupts
unsigned num_instructions;								//Number of instructions in code
volatile unsigned last_fault;							//Most recent memory corruption PC

#define lsi_instruction_type	0b010					//Mask for load/store immediate
#define lsr_instruction_type	0b011					//Mask for load/store register offset 

// -------------------------
// Function : mem_check_init
// -------------------------
// This function runs simply initializes the mem checker, by allocatin enough memory for the pc table
// Params : void
// Return : void
static void mem_check_init(void) {
	num_instructions = ((unsigned)&__bss_start__ - 0x8000)/4;
	pc_table = kmalloc(sizeof(unsigned) * num_instructions);
	printk("Num of instruction is : %u\n", num_instructions);
}


// ----------------------
// Function : get_reg_val
// ----------------------
// This function grabs the value in the register passed into the function
// Params : register whose value is to be looked up
// Return : value in the register
static unsigned get_reg_val(unsigned base_register){
	unsigned value = 0;
	unsigned* stack_start = (unsigned*)(0x9000000 - 14*4);
	if(base_register <= 12){
		value = *(stack_start + base_register);
	} 
	return value;
}

// ---------------------------------------
// Function : get_addr_from_load_immediate
// ---------------------------------------
// This function takes in an ARM 32 bit instruction and grabs the address from memory being accessed
// Params : struct instruction (should only be a load or store, if not behavious here is undefined)
// Return : address being accessed in the instruction
static unsigned get_addr_from_load_immediate(struct ls_instruction* instr){
	unsigned immediate_val = instr->immediate;
	unsigned base_register = instr->Rn;
	unsigned dest_register = instr->Rd;
	unsigned value_in_register = get_reg_val(base_register);
	
	//Register not handled, could be LR, PC etc.
	if(value_in_register == 0) return 0;
	
	//Strange behaviour, dereferencing pointer?
	if(base_register == dest_register) return 0;

	unsigned base_offset = value_in_register;
	
	//if u bit is 1 then immediate value should be incremented
	if(instr->P == 1 && instr->U == 1){
		base_offset += immediate_val;
	}

	//If U bit is 0 then immediate value should be subtracted
	if(instr->P == 1 && instr->U == 0 ){
		base_offset -= immediate_val;
	}
	unsigned addr = base_offset;
	return addr;
}

// --------------------
// Function : mem_check
// --------------------
// This function defines the main memory check routine
// (1) Cast pc to a load/store instruction struct, defined in mem-checker.h
// (2) Grab address from instruction
// (3) Goto shadow memory and check address is marked as allocated
// (4) If not, produce memory corruption log entry and add to log
// Params : program counter, passed in from interrupt handler
// Return : void
static void mem_check(unsigned pc) {
	//Make sure pc is in expected range
	if(pc < 0x8000 || pc > (unsigned)&__bss_end__){
		printk("pc is %x\n", pc);
		panic("pc not valid");
	}
	
	//Cast pc to a ls struct, makes it much easier to work with bits
	struct ls_instruction* instr = (struct lsi_instruction*)pc;
	
	//Check if instruction is write-back, strange behaviour : TODO : figure out how to handler
	if(instr->W == 1) return;
	
	//Check instruction is a load or store immediate
	if((instr->inst_type) == lsi_instruction_type){
		unsigned address = get_addr_from_load_immediate(instr);
		
		//It means register being referenced was a LR, PC, strange behaviour
		if(address == 0){
			return;
		}

		//Check address is reasonably within the heap
		if((address > &__heap_start__) && (address < (&__heap_start__ + 4000000))){
			
			//Goto shadow memory, to see if not allocated
			if(*(char*)(address + shadow_mem_offset) != magic_number){
				unsigned index = (pc - 0x8000)/sizeof(unsigned);
				pc_table[index]++;
				
				//Only add to log once
				if(pc_table[index] == 1){
					mem_corruption_t* corruption = (mem_corruption_t*)kmalloc(sizeof(mem_corruption_t));
					corruption->pc = pc;
					corruption->bad_addr = address;
					if(instr->L == 1){
						corruption->rw = 0;
					} else {
						corruption->rw = 1;
					}
					Q_append(&mem_corruption_log, corruption);
					num_distinct_corruptions++;
				}
				num_corruptions++;
				last_fault = pc;
			}
		} 	
	}
	cnt++;
}

// -------------------------
// Function : mem_check_dump
// -------------------------
// This function dumps some logging info to console from mem checker, numver of checks down, 
// last faulting PC, how many different faulting PCs
// Params : void
// Return : avoid
static void mem_check_dump() {
	system_disable_interrupts();
	printk("Number of memory checks completed : %u, Last Fault detected at PC : 0x%x and number of distinct corruptions : %u\n", 
 		cnt, last_fault, num_distinct_corruptions);
	system_enable_interrupts();
}



/**********************************************************************/
/* 						interrupt code 					              */
/**********************************************************************/

// ----------------------
// Function : int_handler
// ----------------------
// This function is the interrupt handler defined to handle timer interrupts. It essentially just hands off
// to the mem checker to do most of the work, but does also track period of each interrupt
// Params : void
// Return : void
void int_handler(unsigned pc) {
	unsigned pending = RPI_GetIRQController()->IRQ_basic_pending;

	// if this isn't true, could be a GPU interrupt: just return.
	if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
		return;

    //Clear timer interrupt, the only one we should have enabled
    RPI_GetArmTimer()->IRQClear = 1;
	
    mem_check(pc);

    //Track data on period between interrupts
	static unsigned last_clk = 0;
	unsigned clk = timer_get_time();
	period = last_clk ? clk - last_clk : 0;
	last_clk = clk;
}



/**********************************************************************/
/* 						Test/Driver code 					          */
/**********************************************************************/

// -----------------------------
// Function : check_memory_freed
// -----------------------------
// This function loops thorugh shadow memory and totals up number of bytes not freed and
// prints this to the console
// Params : void
// Return : void
void check_memory_freed(){
	printk("\nSUMMARY OF MEMORY LOSS\n", num_distinct_corruptions);
	unsigned num_bytes_lost = 0;
	char* temp = &__heap_start__;
	for(int i = 0; i < 4000000; i++){
		if(temp[i+shadow_mem_offset] == magic_number){
			num_bytes_lost++;
		}
	}
	printk("\nTotal number of bytes lost is : %u\n", num_bytes_lost);
}

// --------------------
// Function : print_log
// --------------------
// This function simply prints out the log of memory corruptions to the terminal. Log is
// stored as a queue.
// Params : void
// Return : void
void print_log(){
	if(Q_size(&mem_corruption_log) == 0){
		return;
	}
	printk("\nSUMMARY OF %u MEMORY CORRUPTIONS FOUND \n", num_distinct_corruptions);
	while(Q_size(&mem_corruption_log) != 0){
     	mem_corruption_t* corruption = Q_pop(&mem_corruption_log);
     	unsigned index = (corruption->pc - 0x8000)/sizeof(unsigned);
    	
    	char* read_or_write = "read";
    	if(corruption->rw == 1){
    		read_or_write = "write";
    	}
    	printk("Corruption at pc : 0x%x, bad address : 0x%x, instruction was a %s, and number of times pc hit : %u\n", 
     		corruption->pc, corruption->bad_addr, read_or_write, pc_table[index]);
    	kfree(corruption);
    }
    kfree(pc_table);
    check_memory_freed();
}

// ----------------------
// Function : example_one
// ----------------------
// This function runs a simple example of the mem checker. It is a tight loop
// around a dereference of non-allocated memory, to show that we can pick up a single fault
// Params : void
// Return : void
void example_one(){
	system_enable_interrupts();
	num_distinct_corruptions = 0;
	printk("\n----RUNNING EXAMPLE ONE----\n");
	printk("Simple example to show fault caught when dereferencing beyond alloced memory\n\n");
	
	//Tight loop around dereference of non-allocated memory
	volatile char* p = kmalloc(1);
	for(int i = 0; i < 2000; i++){
		*(p+5) += 1;
        if(i % 100 == 0){
        	mem_check_dump();
        }
    }
    //Print out mem corruption log
    system_disable_interrupts();
    print_log();
}

// ----------------------
// Function : example_two
// ----------------------
// This function runs a simple example of the mem checker. It is a tight loop
// around a dereference of non-allocated memory, to show that we can pick up multiple faults
// Params : void
// Return : void
void example_two(){
	system_enable_interrupts();
	num_distinct_corruptions = 0;
	printk("\n----RUNNING EXAMPLE TWO----\n");
	printk("Simple example to show we can catch multiple faults when dereferencing beyond alloced memory\n\n");
	
	//Initialise several pointers
	volatile char* p1 = kmalloc(1);
	volatile char* p2 = kmalloc(1);
	volatile char* p3 = kmalloc(1);
	volatile char* p4 = kmalloc(1);
	volatile char* p5 = kmalloc(1);
	volatile char* p6 = kmalloc(1);

	//Tight loop around derefernce to non-allocated memory
	for(int i = 0; i < 4000; i++){
		delay_us(i);
		*(p1+10) += 1;
		delay_us(i);					//Without the delay, the mem-checker stuggles to pick up more than one fault
		*(p2+10) += 1;
		delay_us(i);
		*(p3+10) += 1;
		delay_us(i);
		*(p4+10) += 1;
		delay_us(i);
		*(p5+10) += 1;
		delay_us(i);
		*(p6+10) += 1;
		delay_us(i);
		if(i % 100 == 0){
			mem_check_dump();
		}
	}
	system_disable_interrupts();
	print_log();
}

// ------------------------
// Function : example_three
// ------------------------
// This function runs a simple example of the mem checker. It allocates two pointers
// but only frees one of them, good check that we are tracking non-freed memory
// Params : void
// Return : void
void example_three(){
	system_enable_interrupts();
	num_distinct_corruptions = 0;
	printk("\n----RUNNING EXAMPLE THREE----\n");
	printk("Simple example to show we can catch non freed allocated memory\n\n");
	volatile char*p1 = kmalloc(100);
	volatile char*p2 = kmalloc(200);
	kfree(p1);
	system_disable_interrupts();
	print_log();
}

// ------------------
// Function : notmain
// ------------------
// This function defines entry point to mem-checker program, handles setting up uart, interrupt handlers, heap allocator
// and then run several examples that show how the mem checker can detect memory corruption
// Params : void
// Return : void
void notmain() {
	
	/****INITIALIZE****/

	//Initialize uart
	uart_init();

	//Set up interrupt handlers
	install_int_handlers();

	//Initialize time interrupts
	timer_interrupt_init(0x4);

	//Initialize shadow memory
	shadow_mem_init();

	//Initialize the heap allocator
    kmalloc_init();

    //Initialize data structures use in mem checker
    mem_check_init();


  	//Used for debugging, can check bad addresses are reasonable given where hap starts
  	printk("After allocating mem check data, heap pointer is  %x\n", heap_pointer);

    
  	/****RUN EXAMPLES****/
    //Run test examples
    example_one();
    
	//Re-Initialize shadow memory
	shadow_mem_init();
    example_two();

    //Re-Initialize shadow memory
	shadow_mem_init();
    example_three();

	clean_reboot();
}
