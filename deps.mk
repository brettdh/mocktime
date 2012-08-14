# Generate header dependency rules
#   see http://stackoverflow.com/questions/204823/
# ---
DEP_SRCS=$(wildcard *.c)
DEPS=$(DEP_SRCS:%.c=.%.dep)

.%.dep: %.c
	$(CC) -MM $(CFLAGS) $< >$@

include $(DEPS)
