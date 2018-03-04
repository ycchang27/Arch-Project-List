sim.exe : computer.o sim.o
	gcc -g -Wall -o sim.exe sim.o computer.o

sim.o : computer.h sim.c
	gcc -g -c -Wall sim.c

computer.o : computer.c computer.h
	gcc -g -c -Wall computer.c

clean:
	\rm -rf *.o sim.exe
