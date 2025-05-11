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

$(EXE2) : % : %.o minero.o pow.o wait.o registrador.o
	@echo "#---------------------------"
	@echo "# Generating $@ "
	@echo "# Depepends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -o $@ $@.o minero.o pow.o wait.o registrador.o -lm -lrt

minero.o: minero.c pow.h minero.h utilities.h monitor1.h registrador.h
	@echo "#---------------------------"
	@echo "# Generating $@ "
	@echo "# Depepends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -c $<

pow.o: pow.c pow.h
	@echo "#---------------------------"
	@echo "# Generating $@ "
	@echo "# Depepends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -c $< -lm

monitor1.o: monitor1.c utilities.h pow.h monitor1.h
	@echo "#---------------------------"
	@echo "# Generating $@ "
	@echo "# Depepends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -c $<


comprobador.o: comprobador.c comprobador.h utilities.h pow.h
	@echo "#---------------------------"
	@echo "# Generating $@ "
	@echo "# Depepends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -c $<


utilities.o: wait.c utilities.h pow.h
	@echo "#---------------------------"
	@echo "# Generating $@ "
	@echo "# Depepends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -c $<

registrador.o: registrador.c registrador.h utilities.h pow.h
	@echo "#---------------------------"
	@echo "# Generating $@ "
	@echo "# Depepends on $^"
	@echo "# Has changed $<"
	$(CC) $(CFLAGS) -c $<

example1:
	./monitor & \
	./miner 2 1 & \
	./miner 3 1 & \
	wait
	@echo "Todos los procesos han terminado"

example2:
	./monitor & \
	./miner 2 1 & \
	sleep 1; \
	./miner 1 3 & \
	wait
	@echo "Todos los procesos han terminado"

example3:
	./monitor & \
	./miner 3 1 & \
	sleep 2; \
	./miner 2 1 & \
	wait
	@echo "Todos los procesos han terminado"

