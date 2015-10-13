all:relog librelog.so

CFLAGS += -g3 -O0 \
		-Wall -Wextra -Werror \
		-DRELOG_LIBRELOG_PATH=\"$(shell pwd)/librelog.so\" \
		-DRELOG_FORCE_DEFAULT_OUTPUT_TO=\"/dev/ulog_main\" \
		-DRELOG_FORCE_DEFAULT_ERROR_TO=\"/dev/ulog_main\"

relog:relog.c
	gcc $^ -o $@ $(CFLAGS)

librelog.so:librelog.c popen_noshell.c
	gcc $^ -o $@ -fPIC -shared $(CFLAGS)

clean:
	rm -f librelog.so relog

.PHONY: clean
