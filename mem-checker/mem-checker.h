
//Simple struct to define load/store immediate instruction on ARM
struct ls_instruction {
	signed immediate:12;	//Immediate offset
	unsigned Rd:4;			//Destination register
	unsigned Rn:4;			//Base register
	unsigned L:1;			//If 1 then instruction is load
	unsigned W:1;			//If 1 then instruction is writeback
	unsigned B:1;			//If 1 then instruction is ldrb/strb
	unsigned U:1;			//If 1 then increment base by offset, if 0 decrement
	unsigned P:1;			//If 1 then preindex (i.e. add offset BEFORE looking up addr)
	unsigned inst_type:3; 	//Specifies type of instruction (010 for load/stor immediate)
	unsigned cond:4;		//Condition field (I think we can ignore)
};

//Simple struct to define load/store register offset instruction on ARM
struct lsr_instruction {
	unsigned Rm:4;			//Offset register
	unsigned ignore:1;		//Filler
	unsigned shift:2;
	unsigned shift_amount:5;
	unsigned Rd:4;			//Destination register
	unsigned Rn:4;			//Base Register
	unsigned L:1;			//If 1 then instruction is load
	unsigned W:1;			//If 1 then instruction is writeback
	unsigned B:1;			//If 1 then instruction is ldrb/strb
	unsigned U:1;			//If 1 then increment base by offset, if 0 decrement
	unsigned P:1;			//If 1 then preindex (i.e. add offset BEFORE looking up addr)
	unsigned inst_type:3;
	unsigned cond:4;
};

//Simple struct to define load/store multiple instruction on ARM
struct lsm_instruction {
	unsigned reg_list:16;
	unsigned Rn:4;
	unsigned L:1;			//If 1 then instruction is load
	unsigned W:1;			//If 1 then instruction is writeback
	unsigned B:1;			//If 1 then instruction is ldrb/strb
	unsigned U:1;			//If 1 then increment base by offset, if 0 decrement
	unsigned P:1;			//If 1 then preindex (i.e. add offset BEFORE looking up addr)
	unsigned inst_type:3;
	unsigned cond:4;
};

//Struct to define a memory corruption, store basic info that can then be compiled
//into a log at end of mem corruption check run
typedef struct mem_corruption{
	unsigned pc;			//Program counter of where corruption occured
	unsigned bad_addr;		//Address of memory corrupted/attempted access
	unsigned rw;			//If 1 then was a write/store, if 0 was a read/load
	//unsigned num_hits;		//How many times did statistical checker hit this instruction
	//char* bt_func_name;		//If can figure out how to backtrace to function name, add here
	struct mem_corruption* next;
} mem_corruption_t;
