CC = gcc
LD = gcc -pipe

STRIP = strip
CFLAGS = -pipe

SRCS_INFORM = common.c inform.c 
PROG_INFORM = inform

OBJS_INFORM = ${SRCS_INFORM:.c=.o}

default all: $(PROG_INFORM)

$(PROG_INFORM): $(LIBS)

#$(PROG_NAC): $(OBJS_NAC)
#	$(LD) -o $@ $(OBJS_NAC) $(LIBS)
#	$(STRIP) $(PROG_NAC)

#nac_helper.o: nac_helper.c nac_help.h
#nacnacnode.o: nac_helper.c ccnnacnode.c nac_help.h
#nac_gw.o: nac_helper.c nac_gw.c nac_help.h

inform: common.o inform.o
	$(CC) $(CFLAGS) -o $@ inform.o common.o 

rmtmp:
	$(RM) *.o

clean: rmtmp
	$(RM) $(PROG_INFORM)

.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: all rmtmp clean
