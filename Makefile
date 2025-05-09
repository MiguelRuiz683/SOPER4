#-----------------------
#-----------------------

CC = gcc  -ansi -pedantic
CFLAGS = -Wall
EXE1 = monitor
EXE2 = miner

all : $(EXE1) $(EXE2)

.PHONY : cleanall

cleanall: clean cleanlogs

clean :
	rm -f *.o core $(EXE1) $(EXE2)

cleanlogs :
	rm -f *.log

$(EXE1) : % : %.o monitor1.o comprobador.o wait.o pow.o
	@echo "#---------------------------"
	@echo "# Generating $@ "
	@echo "# Depepends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -o $@ $@.o comprobador.o monitor1.o wait.o pow.o -lm -lrt

$(EXE2) : % : %.o minero.o pow.o wait.o
	@echo "#---------------------------"
	@echo "# Generating $@ "
	@echo "# Depepends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -o $@ $@.o minero.o pow.o wait.o -lm -lrt

minero.o: minero.c pow.h minero.h monitor1.h utilities.h
	@echo "#---------------------------"
	@echo "# Generating $@ "
	@echo "# Depepends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -c $<

pow.o : pow.c pow.h
	@echo "#---------------------------"
	@echo "# Generating $@ "
	@echo "# Depepends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -c $< -lm

monitor1.o : monitor1.c monitor1.h
	@echo "#---------------------------"
	@echo "# Generating $@ "
	@echo "# Depepends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -c $<


comprobador.o : comprobador.c comprobador.h
	@echo "#---------------------------"
	@echo "# Generating $@ "
	@echo "# Depepends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -c $<


utilities.o : wait.c utilities.h
	@echo "#---------------------------"
	@echo "# Generating $@ "
	@echo "# Depepends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -c $<

example1:
	./monitor & ./miner 4 2 & ./miner 6 10
	wait
	@echo "Todos los procesos han terminado"

example2:
	./monitor 0 & \
	sleep 2; \
	./monitor 500 & \
	sleep 2; \
	./miner 8 100& \
	wait
	@echo "Todos los procesos han terminado"

example3:
	./monitor 50 & \
	sleep 2; \
	./monitor 100 & \
	sleep 2; \
	./miner 10 200& \
	wait
	@echo "Todos los procesos han terminado"

