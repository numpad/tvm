#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Uncomment the following line if you want to support windows/distros without gnu/readline */
//#define NO_READLINE

#ifdef NO_READLINE
/* gnu/readline not available? no problem! */

void add_history(const char *str) {
	/* dummy */
}
char *readline(const char *prt) {
	printf("%s", prt);
	char *buf = malloc(256);
	fgets(buf, 256, stdin);
	buf[strlen(buf) - 1] = 0;
	return buf;
}
#else

#include <readline/readline.h>
#include <readline/history.h>

#endif

//#define DEBUG
#define PROGRAM_VERSION "0.1.2"

#define PRT_LOOK " "
#define RDV_LOOK " "
#define DEFAULT_FILE_EXTENSION ".ins"
#define STACK_SIZE 32
#define LABELS_SIZE 16
#define PROGRAM_LINES 256
#define PROGRAM_SIZE 512

void print_help(int argc, char *argv[]) {
	printf("tvm version %s - Tiny Virtual Machine - Copyright (C) 2015 Christian Schäl\n", PROGRAM_VERSION);
	printf("Usage: tvm [option]\n");
	printf("       tvm infile\n");
	printf("General options:\n");
	printf("  -h             Show help message\n");
	printf("  -v             Show local version\n");
	printf("  -repl          Enter REPL\n");
	printf("  -ops           List all instructions\n");
	printf("  -regs          List all available registers\n");
	printf("  -stack         Show maximal stack size\n");
	printf("  -stack [int]   Set maximal stack size\n");
	printf("  -str [str]     Prints 'str' as ASCII codes\n");
}

enum {
	OP_CODE, OP_VAL, OP_REG, OP_LBL
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
OP op_lbl(int v) {
	return op_custom(OP_LBL, v);
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
int is_op_lbl(OP op) {
	return op.type == OP_LBL;
}


const char *op_names[] = {
	"NOP",
	"PSH", "POP",
	"ADD", "SUB", "MUL", "DIV", "RUT", "MOD",
	"AND", "OR", "XOR", "NOT",
	"INC", "DEC", "INCR", "DECR",
	"MIN", "MAX", "CMP",
	"PRT", "PRC", "STK", "RGS", "LBL",
	"RDV",
	"SET", "GET", "PUT", "MOV", "SWP",
	"RPT", "JMP", "JLZ", "JEZ", "JGZ", "JNZ",
	"HLT", "HCF"
};
enum {
	NOP,
	PSH, POP, 						/* Stack manipulation */
	ADD, SUB, MUL, DIV,	RUT, MOD,	/* Maths */
	AND, OR, XOR, NOT,				/* Bitwise */
	INC, DEC, INCR, DECR,			/* +/- 1 shortcuts */
	MIN, MAX, CMP,					/* Comparision */
	PRT, PRC, STK, RGS, LBL,		/* Stdout */
	RDV,							/* Stdin */
	SET, GET, PUT, MOV, SWP,		/* Registers */
	RPT, JMP, JLZ, JEZ, JGZ, JNZ,	/* Control Flow */
	HLT, HCF,						/* Quit */
	
	OP_CODES_COUNT				
} OP_CODES;

int get_op_code_by_name(const char *name) {
	for (int i = 0; i < OP_CODES_COUNT; ++i) {
		if (!strcmp(name, op_names[i]))
			return i;
	}
	return NOP;
}

void print_ops() {
	/*
	 * The last instruction for all op_codes groups 
	 * Effect: Put ops in groups like:
	 *         PSH & POP      -> Stack Instructions
	 *         HLT, HCF & RET -> Exit Instructions
	 */
	const int splits[] = {
		NOP,
		POP,
		MOD,
		NOT,
		DECR,
		CMP,
		LBL,
		RDV,
		SWP,
		JNZ,
		HCF
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
	"swp",
	"ip", "sp",
	"ext"
};
enum {
	REG_EAX, REG_EBX, REG_ECX,		/* General purpose registers */
	REG_SWP,						/* Swap register */
	REG_IP, REG_SP,
	REG_EXT,
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
	char **labels;
	int *label_idx;
	int *stack;
	int *reg;
} PROGRAM;

PROGRAM program_new(const int stack_size) {
	PROGRAM p = (PROGRAM) {
		.labels = calloc(LABELS_SIZE, sizeof(char*)),
		.label_idx = calloc(LABELS_SIZE, sizeof(int)),
		.stack = malloc(stack_size * sizeof(int)),
		.reg = calloc(REGISTER_COUNT, sizeof(int))
	};
	p.reg[REG_SP] = -1;
	p.reg[REG_EXT] = 1;
	
	return p;
}

void program_delete(PROGRAM *p) {
	free(p->stack);
	free(p->reg);
}

int get_lbl_by_name(const char *name, PROGRAM *prg) {
	for (int i = 0; i < LABELS_SIZE; ++i) {
		if (!strcmp(prg->labels[i], name)) {
			return i;
		}
	}
	return -1;
}

/*
 * Stack Manipulation
 */
/* Push a new value on the stack */
void stack_push(PROGRAM *prg, int val) {
	prg->stack[++(prg->reg[REG_SP])] = val;
}
/* Pop value of stack */
int stack_pop(PROGRAM *prg) {
	return prg->stack[(prg->reg[REG_SP])--];
}
/* Return the top value of stack */
int stack_top(PROGRAM *prg) {
	return prg->stack[prg->reg[REG_SP]];
}

OP fetch(PROGRAM *prg, const OP program[]) {
	return program[(prg->reg[REG_IP])++];
}

int eval(PROGRAM *prg, const OP program[]) {
	const OP instruction = fetch(prg, program);
	
	switch (instruction.type) {
		case OP_CODE:
			break;
		default:
			return 0;
	};
	
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
		const int a = stack_pop(prg);
		const int b = stack_pop(prg);
		stack_push(prg, a / b);
		break;
	}
	case RUT: {
		const int a = stack_pop(prg);
		stack_push(prg, (int)sqrt(a));
		break;
	}
	case MOD: {
		const int a = stack_pop(prg);
		const int b = stack_pop(prg);
		stack_push(prg, a % b);
		break;
	}
	case MIN: {
		const int a = stack_pop(prg);
		const int b = stack_pop(prg);
		stack_push(prg, (a < b) ? a : b);
		break;
	}
	case MAX: {
		const int a = stack_pop(prg);
		const int b = stack_pop(prg);
		stack_push(prg, (a > b) ? a : b);
		break;
	}
	case CMP: {
		const int a = stack_pop(prg);
		const int b = stack_pop(prg);
		stack_push(prg, b);
		stack_push(prg, a);
		
		int val = 0;
		if (a > b)
			val = 1;
		else if (a < b)
			val = -1;

		stack_push(prg, val);
		break;
	}
	case AND: {
		const int a = stack_pop(prg);
		const int b = stack_pop(prg);
		stack_push(prg, a & b);
		break;
	}
	case OR: {
		const int a = stack_pop(prg);
		const int b = stack_pop(prg);
		stack_push(prg, a | b);
		break;
	}
	case XOR: {
		const int a = stack_pop(prg);
		const int b = stack_pop(prg);
		stack_push(prg, a ^ b);
		break;
	}
	case NOT: {
		const int a = stack_pop(prg);
		stack_push(prg, ~a);
		break;
	}
	case PRT: {
		const int val = stack_top(prg);
		printf(PRT_LOOK);
		printf("%d\n", val);
		break;
	}
	case PRC: {
		const int val = stack_pop(prg);
		printf("%c", (char)val);
		break;
	}
	case INC: {
		const int val = stack_pop(prg);
		stack_push(prg, val + 1);
		break;
	}
	case DEC: {
		const int val = stack_pop(prg);
		stack_push(prg, val - 1);
		break;
	}
	case INCR: {
		const OP rg = fetch(prg, program);
		prg->reg[rg.val] += 1;
		break;
	}
	case DECR: {
		const OP rg = fetch(prg, program);
		prg->reg[rg.val] -= 1;
		break;
	}
	case SET: {
		const OP rg = fetch(prg, program);
		if (!is_op_reg(rg)) {
			printf("!! Instruction %d: Not a register type!\n", prg->reg[REG_IP]);
			break;
		}

		const OP val = fetch(prg, program);
		prg->reg[rg.val] = val.val;
		break;
	}
	case MOV: {
		const OP rg = fetch(prg, program);
		if (!is_op_reg(rg)) {
			printf("!! Instruction %d: Not a register type!\n", prg->reg[REG_IP]);
			break;
		}
		
		const int val = stack_pop(prg);
		prg->reg[rg.val] = val;
		break;
	}
	case PUT: {
		const OP rg = fetch(prg, program);
		if (!is_op_reg(rg)) {
			printf("!! Instruction %d: Not a register type!\n", prg->reg[REG_IP]);
			break;
		}
		
		const int val = stack_top(prg);
		prg->reg[rg.val] = val;
		break;
	}
	case GET: {
		const OP rg = fetch(prg, program);
		if (!is_op_reg(rg)) {
			printf("!! Instruction %d: Not a register type!\n", prg->reg[REG_IP]);
			break;
		}
		stack_push(prg, prg->reg[rg.val]);
		break;
	}
	case SWP: {
		const OP rg = fetch(prg, program);
		if (!is_op_reg(rg)) {
			printf("!! Instruction %d: Not a register type!\n", prg->reg[REG_IP]);
			break;
		}
		const int swp_val = prg->reg[REG_SWP];
		const int reg_val = prg->reg[rg.val];
		prg->reg[REG_SWP] = reg_val;
		prg->reg[rg.val] = swp_val;
		break;
	}
	case STK: {
		if (prg->reg[REG_SP] == -1) {
			puts("[ ]");
			break;
		}
		const int sz = prg->reg[REG_SP] + 1;
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
	case LBL: {
		puts(" LABELS | PC ");
		puts("--------+----");
		for (int i = 0; i < LABELS_SIZE; ++i) {
			printf(" %-.10s | %d\n", prg->labels[i], prg->label_idx[i]);
		}
	}
	case RDV: {
		int a;
		printf(RDV_LOOK);
		scanf("%d", &a);
		stack_push(prg, a);
		break;
	}
	case HCF: {
		exit(1);
	}
	case RPT: {
		prg->reg[REG_IP] = 0;
		break;
	}
	case HLT: {
		return prg->reg[REG_EXT];
	}

	case JMP: {
		const OP lbl = fetch(prg, program);
		const int lbl_pc = prg->label_idx[lbl.val];
		prg->reg[REG_IP] = lbl_pc;
		break;
	}
	case JGZ: {
		const OP lbl = fetch(prg, program);
		const int lbl_pc = prg->label_idx[lbl.val];
		const int val = stack_top(prg);
		if (val > 0)
			prg->reg[REG_IP] = lbl_pc;
		
		break;
	}
	case JEZ: {
		const OP lbl = fetch(prg, program);
		const int lbl_pc = prg->label_idx[lbl.val];
		const int val = stack_top(prg);
		if (val == 0)
			prg->reg[REG_IP] = lbl_pc;
		
		break;
	}
	case JLZ: {
		const OP lbl = fetch(prg, program);
		const int lbl_pc = prg->label_idx[lbl.val];
		const int val = stack_top(prg);
		if (val < 0)
			prg->reg[REG_IP] = lbl_pc;
		
		break;
	}
	case JNZ: {
		const OP lbl = fetch(prg, program);
		const int lbl_pc = prg->label_idx[lbl.val];
		const int val = stack_top(prg);
		if (val != 0)
			prg->reg[REG_IP] = lbl_pc;
		
		break;
	}
	};
	
	return 0;
}

OP *parse(PROGRAM *prog, char *prg[], const int lines) {
	int pc = 0;
	OP *program = malloc(sizeof(OP) * PROGRAM_SIZE);
	int label_i = 0;

#ifdef DEBUG
	puts(" INSTRUCTION | REGISTER |  VALUE  | LABEL ");
	puts("-------------+----------+---------+-------");
#endif
	/* Parse all labels */
	for (int line = 0; line < lines; ++line) {
		const int line_len = strlen(prg[line]);
		
		if (prg[line][0] == '.') {
			prog->labels[label_i] = calloc(line_len, 1);
			strcpy(prog->labels[label_i], prg[line] + 1);
			
			prog->label_idx[label_i] = pc;
			++label_i;
			continue;
		}


		int str_start = 0;
		for (int i = 0; i < line_len; ++i) {
			if (prg[line][i+1] == ' ' || i+1 == line_len) { 
				/* This will be an instruction */
				if (str_start == 0) {
					str_start = i + 2;
					++i;
					++pc;
					continue;
				}
				
				/* Next will be a register maybe */
				if (prg[line][str_start] == '@') {
					++str_start;
					str_start = i + 1;
					++pc;
					continue;
				} else if (prg[line][str_start] == '.') {
					/* or a label */
					++str_start;
					str_start = i + 1;
					
					++pc;
					continue;
				} else {
					/* Last case, assume its a value */
					++pc;
				}
				
			}
		}
	}
	
	pc = 0;
	for (int line = 0; line < lines; ++line) {
		const int line_len = strlen(prg[line]);
		
		char instruction[8] = {0};
		char register_id[8] = {0};
		char label_id[16] = {0};
		char const_value[24] = {0};

		int str_start = 0;
		for (int i = 0; i < line_len; ++i) {
			if (prg[line][i+1] == ' ' || i+1 == line_len) { 
				/* This will be an instruction */
				if (str_start == 0) {
					strncpy(instruction, prg[line], i + 1);
					str_start = i + 2;
					i += 1;
					
					const int opcode = get_op_code_by_name(instruction);
					program[pc++] = op_code(opcode);
					
					continue;
				}
				
				/* Next will be a register maybe */
				if (prg[line][str_start] == '@') {
					++str_start;
					strncpy(register_id, prg[line] + str_start, i - str_start + 1);
					str_start = i + 1;
					
					program[pc++] = op_reg(get_reg_by_name(register_id));
					continue;
				} else if (prg[line][str_start] == '.') {
					/* or a label */
					++str_start;
					strncpy(label_id, prg[line] + str_start, i - str_start + 1);
					str_start = i + 1;
					
					program[pc++] = op_lbl(get_lbl_by_name(label_id, prog));
					continue;
				} else {
					/* Last case, assume its a value */
					strncpy(const_value, prg[line] + str_start, i - str_start + 1);
					program[pc++] = op_val(atoi(const_value));
				}
				
			}
		}
#ifdef DEBUG
		printf(" %11s |   %-6s | %7s | %s\n", instruction, register_id, const_value, label_id);
#endif

	}

	return program;
}

void read_program_stdin(char *sprg[], int *sprg_len) {
	for (*sprg_len = 0; *sprg_len < PROGRAM_LINES; ++*sprg_len) {
		sprg[*sprg_len] = readline("| ");
		if (strlen(sprg[*sprg_len]) == 0) {
			//printf("\e[1A\e[2D| Done\n\n");
			break;
		}
	}
}

int eval_line(PROGRAM *prg, char *line) {
	prg->reg[REG_IP] = 0;
	char *sprg[1];
	sprg[0] = line;
	int sprg_len = 1;
	
	OP *program = parse(prg, sprg, sprg_len);
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
		if (!strcmp(argv[i], "-str") && i + 1 < argc) {
			for (int j = strlen(argv[i+1]) - 1; j >= 0; --j) {
				printf("PSH %d\n", argv[i+1][j]);
			}
			printf("\n");
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
		instructions = parse(&program, sprg, sprg_len);
	
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
	
	printf("=> Finished with code %d.\n", ret);
	
	return 0;
}

