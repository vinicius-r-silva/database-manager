src = $(wildcard *.c)
obj = $(src:.c=.o)

CCFLAGS = -Wall
LDFLAGS = 

all: main

main: $(obj)
	$(CC) $(CCFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) main