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
	install charade.exe /usr/bin
	if [ -f /usr/bin/ssh-agent.exe ]; then \
	    mv /usr/bin/ssh-agent.exe /usr/bin/ssh-agent-orig.exe; \
	    ln -s /usr/bin/charade.exe /usr/bin/ssh-agent.exe; \
	fi

charade.exe: $(OBJFILES)
	$(CC) $(CFLAGS) -Wl,--enable-auto-import -o $@ $+

copyright.c: LICENCE generate-copyright.pl
	./generate-copyright.pl LICENCE > $@

clean:
	rm -f charade.exe
	rm -f *.o *.obj
	rm -f *.d
