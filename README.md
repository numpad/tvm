# tvm (1)

```
     ,d                                      
     88                                      
   MM88MM;  8b       d8  88,dPYba,,adPYba,   
     88     `8b     d8'  88P'   "88"    "8a  
     88      `8b   d8'   88      88      88  
     88,      `8b,d8'    88      88      88  
     "Y888      "8"      88      88      88  
```

## NAME
tvm - Tiny C Virtual Machine

## SYNOPSIS
usage: tvm [options] [infile]

## DESCRPITION
tvm is a configurable, small virtual machine with support for different input methods, registers, a good range of instructions and a nice API.

Here are some examples how to use tvm:

`tvm`
  Accept input from stdin until a *newline* is reached. Executes the input afterwards.

`tvm calculator`
  Parse and eval the file "calculator".

`tvm countdown.ins -stack 64`
  Run "countdown.ins" with a stack size of 64 blocks.

To use the API:
    
	/* Instructions (Default size) unparsed */
	char *instructions_input[PROGRAM_SIZE] = {
		"PSH 5",
		"PSH 3",
		"ADD",
		"PRT",
		"MOV @ext",
		"HLT"
	};
	
	/* PROGRAM stores `registers` and `stack`, initialize with default size */
	PROGRAM *program = program_new(STACK_SIZE);
	/* Parse `instructions_input` to get executable code */
	OP *instructions = parse(&program, instructions_input, PROGRAM_SIZE);
	
	/* Evaluate the program until HLT is reached with register @ext != 0 */
	int return_code;
	do {
		return_code = eval(&program, instructions);
	} while (return_code == 0);
	
	/* Work with program... */
	
	/* Clean up memory used by program */
	program_delete(&program);

## OPTIONS
-h			**Show help message**  
-v			**Show local version**  
-repl		**Enter REPL**  
-ops		**List all instructions**  
-regs		**List all available registers**  
-stack		**Show maximal stack size**  
-stack *n*	**Set maximal stack size**  

## AUTHOR
Christian Schäl
