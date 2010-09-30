export CC := gcc
export CFLAGS := -Wall -Werror -O2

all: charade.exe

# sort to remove duplicates, specifically copyright.c
# (it's auto-generated, but it'll appeare twice on second & subsequent makes: bad)
CFILES := $(sort $(wildcard *.c) copyright.c)
#OBJFILES := $(patsubst %.c,%.o,$(CFILES))
OBJFILES := $(CFILES:.c=.o)

include $(OBJFILES:.o=.d)

%d: %c
	./depend.sh . $< > $@

install: all
	./install.sh

charade.exe: $(OBJFILES)
	$(CC) $(CFLAGS) -Wl,--enable-auto-import -o $@ $+

copyright.c: LICENCE generate-copyright.pl
	./generate-copyright.pl LICENCE > $@

binary: all VERSION
	export VNAME=charade-`cat VERSION`; \
	export DIR=temp/$$VNAME; \
	mkdir -p $$DIR/ \
	&& cp charade.exe $$DIR \
	&& cp install.sh $$DIR \
	&& cp README.md $$DIR \
	&& (cd temp; tar cf - $$VNAME | bzip2 -9) > $$VNAME.tar.bz2 \
	&& echo created $$VNAME.tar.bz2

clean:
	rm -f charade.exe
	rm -f *.o *.obj
	rm -f *.d
	rm -rf temp
	rm -f charade-*.tar.bz2 charade-*.tar.gz
