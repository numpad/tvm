# tvm Manual

## The Stack
The data structure *stack* used in **tvm** is fixed size which can be modified on startup with the `-stack <n>` argument.
A visualization used in later examples:
`[  ...  b  a  ]`

## The Registers
TODO

## Instructions
**Note:** **tvm** uses an optimistic stack system, that means there is **no guarantee** that the stack won't over-/underflow!

### Ungrouped Instructions
`NOP`
Does completely nothing

### Stack Instructions
`PSH <n>`  
Push *n* on the stack

`POP`
Pop a value from the stack


### Mathematic Operations
`ADD [ ... b a ]`  
*POP*s two values from the stack and *PSH*es their sum.

`SUB [ ... b a ]`  
*POP*s two values from the stack and *PSH*es their difference.  
**NOTE:** *a* represents the *minuend* and *b* the *subtrahend* so the result will be `a - b`

`MUL [ ... b a ]`  
*POP*s two values from the stack and *PSH*es their sum.

`DIV [ ... b a ]`  
*POP*s two values from the stack and *PSH*es their sum.
**NOTE:** *a* represents the *dividend* and *b* the *divisor* so the result will be `a / b`

`RUT [ ... a ]`  
*POP*s a value from the stack and *PSH*es it's square root.
**NOTE:** tvm currently only supports Integral types so the result will be *very* inaccurate

`MOD [ ... b a ]`  
*POP*s two values from the stack and *PSH*es the result of a modulo operatiom.
**NOTE:** the result will be `a % b`

### Bitwise Operations
`AND [ ... b a ]`  
*POP*s two values from the stack and *PSH*es their **bitwise and**.
**NOTE:** Order is: `a & b`

`OR [ ... b a ]`  
*POP*s two values from the stack and *PSH*es their **bitwise or**.
**NOTE:** Order is: `a | b`

`XOR [ ... b a ]`  
*POP*s two values from the stack and *PSH*es their **bitwise exclusive or**.
**NOTE:** Order is: `a ^ b`

`NOT [ ... a ]`  
*POP*s a value from the stack and *PSH*es it's **bitwise not**.


### Shortcuts for increasing/decreasing the stack/a register
`INC [ ... a ]`  
Increases the value of *stack top* by one.

`DEC [ ... a ]`  
Decreases the value of *stack top* by one.

`INCR <REG>`  
Increases the value of *register REG* by one.

`DECR <REG>`  
Decreases the value of *register REG* by one.


### Comparisions
`MIN [ ... b a ]`  
*POP*s two values from the stack and *PSH*es the smaller value.

`MAX [ ... b a ]`  
*POP*s two values from the stack and *PSH*es the bigger value.

`CMP [ ... b a ]`  
*PSH*es **1** if `a > b`, **0** if `a == b` or **-1** if `a < b`


### Standard Output
`PRT [ ... a ]`  
Prints the *stack top*.

`PRC [ ... a ]`  
*POP*s and prints the *stack top* as ASCII character.

`STK`  
Prints all values on the *stack*.

`RGS`  
Prints each *register name* and *value*.

`LBL`  
Prints each *label name* and *position*.


### Standard Input
`RDV`  
Reads a value and *PSH*es it on the stack.


### Register Manipulation
`SET <REG> <VAL>`  
Set *REG* to *VAL*.

`GET <REG>`  
*PSH*es the value of *REG* on the stack.

`PUT <REG> [ ... a ]`  
Set *REG* to *a*.

`MOV <REG> [ ... a ]`
*POP*s and set *REG* to *a*.

`SWP <REG>`
Swaps the values of `@swp` and *REG*.


### Control Flow
`RPT`  
Go to beginning of the program.

`JMP <LBL>`  
Set `pc` to position of *LBL*.

`JLZ [ ... a ]`  
Set `pc` to position of *LBL* if `a < 0`.

`JEZ [ ... a ]`  
Set `pc` to position of *LBL* if `a == 0`.

`JGZ [ ... a ]`  
Set `pc` to position of *LBL* if `a > 0`.

`JNZ [ ... a ]`  
Set `pc` to position of *LBL* if `a != 0`.

### Exit
`HLT`  
Ends the program with *exit code* from `@ext` if `@ext != 0`

`HCF`  
Quit the program immediately *without any cleanup etc*.
**NOTE:** This *never* needs to be used. **Never ever**.



