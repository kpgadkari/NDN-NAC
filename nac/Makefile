CC = gcc
LD = gcc -pipe

STRIP = strip
CFLAGS = -pipe
#CFLAGS = -pipe -DDEBUG

#CCNX_DIR = /home2/ndn/ccnx-0.4.2
INCLUDES = -I $(CCNX_DIR)/include
LIBS = $(CCNX_DIR)/lib/libccn.a -lcrypto

SRCS_NAC = nac_helper.c ccnnacnode.c nac_gw.c
PROG_NAC = ccnnacnode ccnnacgw

OBJS_NAC = ${SRCS_NAC:.c=.o}

default all: $(PROG_NAC)

$(PROG_NAC): $(LIBS)

#$(PROG_NAC): $(OBJS_NAC)
#	$(LD) -o $@ $(OBJS_NAC) $(LIBS)
#	$(STRIP) $(PROG_NAC)

#nac_helper.o: nac_helper.c nac_help.h
#nacnacnode.o: nac_helper.c ccnnacnode.c nac_help.h
#nac_gw.o: nac_helper.c nac_gw.c nac_help.h

ccnnacnode: nac_helper.o ccnnacnode.o
	$(CC) $(CFLAGS) -o $@ nac_helper.o ccnnacnode.o $(LIBS)

ccnnacgw: nac_helper.o nac_gw.o
	$(CC) $(CFLAGS) -o $@ nac_helper.o nac_gw.o $(LIBS)

rmtmp:
	$(RM) *.o

clean: rmtmp
	$(RM) $(PROG_NAC)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

.PHONY: all rmtmp clean
