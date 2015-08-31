CCFLAGS+=-std=c99 -g -O0
LDFLAGS+=-lcurses
CC=gcc
LD=gcc
AR=ar rcu
RANLIB=ranlib

all:		vuitest

clean:
		rm -vf *.o libvui.a vuitest

vuitest:	vuitest.o graphviz.o libvui.a
		$(LD) $^ $(LDFLAGS) -o $@

libvui.a:	vui.o translator.o statemachine.o gc.o stack.o string.o utf8.o
		$(AR) $@ $^
		$(RANLIB) $@

%.o:		%.c
		$(CC) $(CCFLAGS) -c $< -o $@

depend:		
		$(CC) $(CCFLAGS) -MM {vuitest,graphviz,vui,translator,statemachine,gc,stack,string,utf8}.c

.PHONY:		all clean

# output of `make depend`

vuitest.o: vuitest.c vui.h string.h translator.h stack.h statemachine.h \
 gc.h graphviz.h
graphviz.o: graphviz.c graphviz.h stack.h statemachine.h
vui.o: vui.c utf8.h gc.h stack.h statemachine.h vui.h string.h \
 translator.h
translator.o: translator.c utf8.h vui.h string.h translator.h stack.h \
 statemachine.h
statemachine.o: statemachine.c utf8.h gc.h stack.h statemachine.h
gc.o: gc.c gc.h stack.h statemachine.h
stack.o: stack.c stack.h
string.o: string.c utf8.h string.h
utf8.o: utf8.c utf8.h
