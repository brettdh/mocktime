LIBRARY := libmocktime.so
CFLAGS := -g -Wall -Werror

all: $(LIBRARY)

OBJS := mocktime.o

$(LIBRARY): $(OBJS)
	gcc -o $@ -shared $^ $(LDFLAGS)

%.o: %.c
	gcc -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(LIBRARY) *.o *~

include deps.mk
