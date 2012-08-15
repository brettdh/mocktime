CFLAGS=-Wall -Werror -g -I.. -m32 -pthread
LDFLAGS=
LIBS=../libmocktime.so

TESTS=test_gettimeofday.o test_sleep.o

all: run_tests

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

run_tests: main.o $(TESTS)
	gcc $(LDFLAGS) main.o $(TESTS) -o $@ $(LIBS)

clean:
	rm -f run_tests *.o *~

include ../deps.mk
