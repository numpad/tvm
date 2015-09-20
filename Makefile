CC=gcc
CSTD=c99
CLIB=-lreadline -lm -g
CWARN=-Wall -pedantic

program=tvm

all:
	${CC} -std=${CSTD} ${CLIB} ${CWARN} ${program}.c -o${program} 
