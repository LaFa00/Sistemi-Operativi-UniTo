#=================================

master.o: master.c
	gcc -std=c89  -Wpedantic -D_GNU_SOURCE -c master.c
master: master.o semaforo.o
	gcc -std=c89  -Wpedantic -D_GNU_SOURCE  -o master master.o semaforo.o -lm 

porto.o: porto.c 
	gcc -std=c89  -Wpedantic -D_GNU_SOURCE -c porto.c
porto: porto.o semaforo.o
	gcc -std=c89  -Wpedantic -D_GNU_SOURCE -o porto porto.o semaforo.o -lm

nave.o: nave.c 
	gcc -std=c89  -Wpedantic -D_GNU_SOURCE -c nave.c
nave: nave.o semaforo.o
	gcc -std=c89  -Wpedantic -D_GNU_SOURCE -o nave nave.o semaforo.o -lm

semaforo.o: semaforo.c
	gcc -std=c89  -Wpedantic -D_GNU_SOURCE -c semaforo.c
#================================================
#======================================
build: master nave porto 

clean:
	clear
	ipcrm -a
	rm -f *.o
	rm master nave porto
	


run: 
	./master
