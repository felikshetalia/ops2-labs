override CFLAGS=-Wall -Wextra -fanalyzer -g -O0 -fsanitize=address,undefined -lm

ifdef CI
override CFLAGS=-Wall -Wextra -Werror -lm
endif

.PHONY: all clean clean-resources

all: sop-railway sop-train

railway.o : railway.c railway.h
	gcc $(CFLAGS) -c -o $@ $<

sop-railway: sop-railway.c macros.h railway.o
	gcc $(CFLAGS) -o $@ $< railway.o

sop-train: sop-train.c macros.h railway.o
	gcc $(CFLAGS) -o $@ $< railway.o

clean:
	rm -f sop-railway sop-train *.o

clean-resources:
	rm -f /dev/shm/sop-shmem
