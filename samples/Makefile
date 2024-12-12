CFLAGS = -I ../include -g -O0
LDFLAGS += -L../lib -lm
SRCS = $(wildcard *.c)
TARGETS = $(SRCS:.c=.exe)

all: $(TARGETS)

%.exe: LDFLAGS += -lx
%.exe: %.c
	$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS)
clean:
	$(RM) $(TARGETS)

.PHONY: clean all
