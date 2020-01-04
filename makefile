#
# GNU makefile
#
CC       = gcc
INCL     = 
CFLAGS   = -O6 -s -I../include
LINK     = gcc
STRIP    = strip
BIND     = coff2exe
RM       = del
LFLAGS   = -L../lib
LIBS     = -lm -lpdc~1 -lemu
PROGS    = predict.exe

all: $(PROGS)

%.exe : %
	$(STRIP) $<
	$(BIND) $<

% : %.o
	$(LINK) $(LFLAGS) -o $@ $< $(LIBS)

predict.exe : predict

predict : predict.o

clean:
	-$(RM) *.o
	-$(RM) predict
