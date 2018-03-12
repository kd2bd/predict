#
# gsat plugins Makefile
#
# Edit the TARGETS line to contain only the plugins you need
#

TARGETS=radio_FT736 radio_ICR10 radio_TMD700 radio_PCR100 radio_print radio_test rotor_print rotor_pictrack

PLUGINDIR=/usr/local/lib/gsat/plugins

all: $(TARGETS)

radio_FT736:
	gcc -fPIC -shared radio_FT736.c -o radio_FT736

radio_ICR10:
	gcc -fPIC -shared radio_ICR10.c -o radio_ICR10

radio_TMD700:
	gcc -fPIC -shared radio_TMD700.c -o radio_TMD700 -lm -lD700

radio_PCR100:
	gcc -fPIC -shared radio_PCR100.c -o radio_PCR100 -lpcr100

radio_print:
	gcc -fPIC -shared radio_print.c -o radio_print

radio_test:
	gcc -fPIC -shared radio_test.c -o radio_test

rotor_print:
	gcc -fPIC -shared rotor_print.c -o rotor_print

rotor_pictrack:
	gcc -fPIC -shared rotor_pictrack.c -o rotor_pictrack

clean:
	rm *.o *~ $(TARGETS)

install:
	cp $(TARGETS) $(PLUGINDIR)
