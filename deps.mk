# Generate header dependency rules
#   see http://stackoverflow.com/questions/204823/
# ---
DEP_C_SRCS=$(wildcard *.c)
DEP_CXX_SRCS=$(wildcard *.cc)
DEPS=$(DEP_C_SRCS:%.c=.%.dep) $(DEP_CXX_SRCS:%.cc=.%.dep)

.%.dep: %.c
	$(CC) -MM $(CFLAGS) $< >$@

.%.dep: %.cc
	$(CXX) -MM $(CXXFLAGS) $< >$@

include $(DEPS)
