OBJS=main.o App_queue.o App_udp.o cvector.o globals.o mac_process.o package.o LEO_ra_procedure.o ra_procedure.o resource_apply.o timer_queue.o info_process.o scheduling.o
CC=gcc
CFLAGS=-c -Wall -g 

mytool:$(OBJS)
	$(CC) $^ -o mytool -lm -lpthread
%.o:%.c
	$(CC) $^ $(CFLAGS)  -o $@

clean:
	$(RM) *.o mytool -r                            