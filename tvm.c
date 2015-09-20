#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <readline/readline.h>
#include <readline/history.h>

//#define DEBUG
#define PROGRAM_VERSION "0.1"

#define DEFAULT_FILE_EXTENSION ".ins"
#define STACK_SIZE 32
#define PROGRAM_LINES 128
#define PROGRAM_SIZE 256

void print_help(int argc, char *argv[]) {
	printf("tvm version %s - Tiny Virtual Machine - Copyright (C) 2015 Christian Schäl\n", PROGRAM_VERSION);
	printf("Usage: tvm [option]\n");
	printf("       tvm infile\n");
	printf("General options:\n");
	printf("  -h             Show help message\n");
	printf("  -v             Show local version\n");
	printf("  -repl          Enter REPL\n");
	printf("  -docs          Show Documentation\n");
	printf("  -ops           List all instructions\n");
	printf("  -regs          List all available registers\n");
	printf("  -stack         Show maximal stack size\n");
	printf("  -stack [int]   Set maximal stack size\n");
}

enum {
	OP_CODE, OP_VAL, OP_REG
} OP_TYPE;

typedef struct {
	char type;
	int val;
} OP;


OP op_custom(char type, int val) {
	return (OP) {
		.type = type,
		.val = val
	};
}

OP op_code(int v) {
	return op_custom(OP_CODE, v);
}
OP op_val(int v) {
	return op_custom(OP_VAL, v);
}
OP op_reg(int v) {
	return op_custom(OP_REG, v);
}

int is_op_code(OP op) {
	return op.type == OP_CODE;
}
int is_op_val(OP op) {
	return op.type == OP_VAL;
}
int is_op_reg(OP op) {
	return op.type == OP_REG;
}


const char *op_names[] = {
	"PSH", "POP",
	"ADD", "SUB", "MUL", "DIV", "RUT",
	"PRT", "PRC", "STK", "RGS",
	"RDV",
	"SET", "GET", "PUT", "SWP",
	"RPT", "GTI",
	"HLT", "HCF", "RET"
};
enum {
	PSH, POP, 					/* Stack manipulation */
	ADD, SUB, MUL, DIV,	RUT,	/* Maths */
	PRT, PRC, STK, RGS,			/* Stdout */
	RDV,						/* Stdin */
	SET, GET, PUT, SWP,			/* Registers */
	RPT, GTI,					/* Control Flow */
	HLT, HCF, RET,				/* Quit */
	
	OP_CODES_COUNT				
} OP_CODES;

int get_op_code_by_name(const char *name) {
	for (int i = 0; i < OP_CODES_COUNT; ++i) {
		if (!strcmp(name, op_names[i]))
			return i;
	}
	return -1;
}

void print_ops() {
	/*
	 * The last instruction for all op_codes groups 
	 * Effect: Put ops in groups like:
	 *         PSH & POP      -> Stack Instructions
	 *         HLT, HCF & RET -> Exit Instructions
	 */
	const int splits[] = {
		POP,
		RUT,
		RGS,
		RDV,
		SWP,
		GTI,
		RET
	};
	int split_idx = 0;

	puts("  #  | INS ");
	puts("-----+-----");
	for (int i = 0; i < OP_CODES_COUNT; ++i) {
		if (splits[split_idx] == i - 1) {
			++split_idx;
			puts("     |");
		}
		printf(" %-.3x | %s\n", i, op_names[i]);
	}
	printf("\nTotal %d instructions available.\n", OP_CODES_COUNT);
}

/* Registers */
const char *reg_names[] = {
	"eax", "ebx", "ecx",
	"swp"
};
enum {
	REG_EAX, REG_EBX, REG_ECX,		/* General purpose registers */
	REG_SWP,						/* Swap register */
	
	REGISTER_COUNT
} REGISTERS;
int get_reg_by_name(const char *name) {
	for (int i = 0; i < REGISTER_COUNT; ++i) {
		if (!strcmp(name, reg_names[i]))
			return i;
	}
	return -1;
}

void print_regs() {
	puts(" # | REG ");
	puts("---+-----");
	for (int i = 0; i < REGISTER_COUNT; ++i) {
		printf(" %x | %s\n", i, reg_names[i]);
	}
	printf("\nTotal %d registers available.\n", REGISTER_COUNT);
}

typedef struct {
	int ip;
	int sp;
	int *stack;
	int *reg;
} PROGRAM;

PROGRAM program_new(const int stack_size) {
	return (PROGRAM) {
		.ip = 0,
		.sp = -1,
		.stack = malloc(stack_size * sizeof(int)),
		.reg = calloc(REGISTER_COUNT, sizeof(int))
	};
}

void program_delete(PROGRAM *p) {
	free(p->stack);
	free(p->reg);
}

/*
 * Stack Manipulation
 */
/* Push a new value on the stack */
void stack_push(PROGRAM *prg, int val) {
	prg->stack[++(prg->sp)] = val;
}
/* Pop value of stack */
int stack_pop(PROGRAM *prg) {
	return prg->stack[(prg->sp)--];
}
/* Return the top value of stack */
int stack_top(PROGRAM *prg) {
	return prg->stack[prg->sp];
}

OP fetch(PROGRAM *prg, const OP program[]) {
	return program[(prg->ip)++];
}

int eval(PROGRAM *prg, const OP program[]) {
	const OP instruction = fetch(prg, program);
	
	if (instruction.type != OP_CODE)
		return 0;
	
	switch (instruction.val) {
	case PSH: {
		const OP val = fetch(prg, program);
		stack_push(prg, val.val);
		break;
	}
	case POP:
		stack_pop(prg);
		break;
	case ADD: {
		const int a = stack_pop(prg);
		const int b = stack_pop(prg);
		stack_push(prg, a + b);
		break;
	}
	case SUB: {
		const int a = stack_pop(prg);
		const int b = stack_pop(prg);
		stack_push(prg, a - b);
		break;
	}
	case MUL: {
		const int a = stack_pop(prg);
		const int b = stack_pop(prg);
		stack_push(prg, a * b);
		break;
	}
	case DIV: {
		/* TODO: Add values like INF etc */
		const int a = stack_pop(prg);
		const int b = stack_pop(prg);
		stack_push(prg, a / b);
		break;
	}
	case RUT: {
		const int a = stack_pop(prg);
		stack_push(prg, (int)sqrt(a));
	}
	case PRT: {
		const int val = stack_top(prg);
		printf("-> %d\n", val);
		break;
	}
	case PRC: {
		const int val = stack_pop(prg);
		printf("%c", (char)val);
		break;
	}
	case SET: {
		const OP rg = fetch(prg, program);
		if (!is_op_reg(rg)) {
			printf("!! Instruction %d: Not a register type!\n", prg->ip);
			break;
		}

		const OP val = fetch(prg, program);
		prg->reg[rg.val] = val.val;
		break;
	}
	case PUT: {
		const OP rg = fetch(prg, program);
		if (!is_op_reg(rg)) {
			printf("!! Instruction %d: Not a register type!\n", prg->ip);
			break;
		}
		
		const int val = stack_pop(prg);
		prg->reg[rg.val] = val;
		break;
	}
	case GET: {
		const OP rg = fetch(prg, program);
		if (!is_op_reg(rg)) {
			printf("!! Instruction %d: Not a register type!\n", prg->ip);
			break;
		}
		stack_push(prg, prg->reg[rg.val]);
		break;
	}
	case SWP: {
		const OP rg = fetch(prg, program);
		if (!is_op_reg(rg)) {
			printf("!! Instruction %d: Not a register type!\n", prg->ip);
			break;
		}
		const int swp_val = prg->reg[REG_SWP];
		const int reg_val = prg->reg[rg.val];
		prg->reg[REG_SWP] = reg_val;
		prg->reg[rg.val] = swp_val;
		break;
	}
	case STK: {
		if (prg->sp == -1) {
			puts("[ ]");
			break;
		}
		const int sz = prg->sp + 1;
		printf("[ ");
		for (int i = 0; i < sz; ++i) {
			printf("%d ", prg->stack[i]);
		}
		puts("]");
		break;
	}
	case RGS: {
		for (int i = 0; i < REGISTER_COUNT; ++i) {
			printf("@> %s: %d\n", reg_names[i], prg->reg[i]);
		}
		break;
	}
	case RDV: {
		int a;
		printf("<- ");
		scanf("%d", &a);
		stack_push(prg, a);
		break;
	}
	case HCF: {
		exit(2);
	}
	case RPT: {
		prg->ip = 0;
		break;
	}
	case GTI: {
		const OP line = fetch(prg, program);
		prg->ip = line.val;
		break;
	}
	case HLT: {
		return -1;
	}
	case RET: {
		int ret = stack_pop(prg);
		/* Return 0 means continue */
		if (ret == 0)
			ret = -1;
		return ret;
	}
	};

	return 0;
}

OP *parse(char *prg[], const int lines) {
	int pc = 0;
	OP *program = malloc(sizeof(OP) * PROGRAM_SIZE);

#ifdef DEBUG
	puts(" INSTRUCTION | REGISTER |  VALUE  ");
	puts("-------------+----------+---------");
#endif
	for (int line = 0; line < lines; ++line) {
		const int line_len = strlen(prg[line]);
		
		char instruction[8] = {0};
		char register_id[8] = {0};
		char const_value[24] = {0};

		int str_start = 0;
		for (int i = 0; i < line_len; ++i) {
			if (prg[line][i+1] == ' ' || i+1 == line_len) { 
				/* This will be an instruction */
				if (str_start == 0) {
					strncpy(instruction, prg[line], i + 1);
					str_start = i + 2;
					i += 1;
					
					program[pc++] = op_code(get_op_code_by_name(instruction));
					continue;
				}
				
				/* Next will be a register maybe */
				if (prg[line][str_start] == '@') {
					++str_start;
					strncpy(register_id, prg[line] + str_start, i - str_start + 1);
					str_start = i + 1;
					
					program[pc++] = op_reg(get_reg_by_name(register_id));
					continue;
				} else {
					/* Last case, assume its a value */
					strncpy(const_value, prg[line] + str_start, i - str_start + 1);
					program[pc++] = op_val(atoi(const_value));
				}
				
			}
		}
#ifdef DEBUG
		printf(" %11s |   %-6s | %7s\n", instruction, register_id, const_value);
#endif

	}

	return program;
}

void read_program_stdin(char *sprg[], int *sprg_len) {
	
	for (*sprg_len = 0; *sprg_len < PROGRAM_LINES; ++*sprg_len) {
		sprg[*sprg_len] = readline("| ");
		if (strlen(sprg[*sprg_len]) == 0) {
			printf("\e[1A\e[2D| Done\n\n");
			break;
		}
	}
}

int eval_line(PROGRAM *prg, char *line) {
	prg->ip = 0;
	char *sprg[1];
	sprg[0] = line;
	int sprg_len = 1;
	
	OP *program = parse(sprg, sprg_len);
	return eval(prg, program);
}

void read_program_file(const char *fn, char *sprg[], int *sprg_len) {
	FILE *fp = fopen(fn, "rb");
	/* if file cannot be found, try appending default extension */
	if (fp == NULL) {
		char buf[strlen(fn) + strlen(DEFAULT_FILE_EXTENSION) + 1]; /* + 5 => ".ins" == 4, '\0' == 1 */
		strcpy(buf, fn);
		strcat(buf, DEFAULT_FILE_EXTENSION);
		
		fp = fopen(buf, "rb");

		/* file still cannot be found? */
		if (fp == NULL) {
			printf("File '%s' not found!\n", fn);
			*sprg_len = 0;
			return;
		}
	}

	*sprg_len = 0;
	char buf[64];
	while (fgets(buf, 64, fp)) {
		const int len = strlen(buf);
		buf[len - 1] = 0;
		
		sprg[*sprg_len] = malloc(64);
		strcpy(sprg[*sprg_len], buf);
		++*sprg_len;
	}

	fclose(fp);
}

int main(int argc, char *argv[]) {
	/* Allocate memory for raw program */
	char *sprg[PROGRAM_LINES];
	int sprg_len;
	
	/*
	if (argc == 1)
		read_program_stdin(sprg, &sprg_len);
	} else if (!strcmp(argv[1], "-stack")) {
		if (argc == 2) {
			printf("tvm maximal stack size: %d x 4 bytes\n", STACK_SIZE);
			return 0;
		} else {
			program_delete(&program);
			program = program_new(atoi(argv[2]) * sizeof(int));
		}
	} else read_program_file(argv[1], sprg, &sprg_len);
	
	*/
	/* Store information for program */
	PROGRAM program = program_new(STACK_SIZE);
	/* Use repl instead of stdin/file */
	int repl = 0;
	char *file = NULL;
	int stack_size = STACK_SIZE;
	int arg_stack_idx = -1;
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-ops")) {
			print_ops();
			return 0;
		}
		if (!strcmp(argv[i], "-regs")) {
			print_regs();
			return 0;
		}
		if (!strcmp(argv[i], "-h")) {
			print_help(argc, argv);
			return 0;
		}
		if (!strcmp(argv[i], "-v")) {
			printf("tvm version %s\n", PROGRAM_VERSION);
			return 0;
		}
		
		if (!strcmp(argv[i], "-stack")) {
			arg_stack_idx = i;
			
			if (i == argc - 1 || argv[i+1][0] == '-') {
				/* print */
				printf("tvm maximal stack size: %d x 4 bytes\n", stack_size);
			} else {
				program_delete(&program);
				program = program_new(atoi(argv[i+1]) * sizeof(int));
			}
		}

		if (!strcmp(argv[i], "-repl")) {
			repl = 1;
		}

		if (argv[i][0] != '-' && i != arg_stack_idx + 1) {
			file = argv[i];
		}
	}
	

	if (!repl) {
		if (file)
			read_program_file(file, sprg, &sprg_len);
		else
			read_program_stdin(sprg, &sprg_len);
	}
	/* Check if we even have anything to parse */
	if (sprg_len == 0 && repl == 0) {
		puts("Cannot parse empty program!");
		return 1;
	}

	/* Parse the program */
	OP *instructions;
	if (!repl)
		instructions = parse(sprg, sprg_len);
	
	int ret;
	do {
		if (repl) {
			char *input = readline("<- ");
			if (strlen(input) > 0) {
				ret = eval_line(&program, input);
				add_history(input);
			}
		} else {
			ret = eval(&program, instructions);
		}
	} while (ret == 0);
	
	printf("=> Finished with code %d after %d instructions!\n", ret, program.ip);
	
	return 0;
}

