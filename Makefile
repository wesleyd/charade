CC := gcc
CFLAGS := -Wall -Werror -O2

export CC CFLAGS

all: charade.exe

CFILES := $(wildcard *.c)
#OBJFILES := $(patsubst %.c,%.o,$(CFILES))
OBJFILES := $(CFILES:.c=.o)

include $(OBJFILES:.o=.d)

%d: %c
	./depend.sh . $<

install: all
	install charade.exe /usr/local/bin

charade.exe: $(OBJFILES)
	$(CC) $(CFLAGS) -o $@ $+

clean:
	rm -f charade.exe
	rm -f *.o *.obj
	rm -f *.d
