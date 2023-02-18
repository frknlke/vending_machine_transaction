simulation:	simulation.o
			gcc simulation.o -o simulation -lpthread

simulation.o: simulation.c
			gcc -c simulation.c -o simulation.o

clean:
			rm *.o *.out ./simulation