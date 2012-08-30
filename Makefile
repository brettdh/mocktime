LIBRARY := libmocktime.so
CXXFLAGS := -g -Wall -Werror

all: $(LIBRARY)

OBJS := mocktime.o

$(LIBRARY): $(OBJS)
	$(CXX) -o $@ -shared $^ $(LDFLAGS)

%.o: %.cc
	$(CXX) -c -o $@ $< $(CXXFLAGS)

clean:
	rm -f $(LIBRARY) *.o *~

include deps.mk
