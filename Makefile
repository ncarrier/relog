all:relog librelog.so

relog:relog.c
	gcc $^ -o $@ -g3 -O0 \
		-Wall -Wextra -Werror \
		-DRELOG_LIBRELOG_PATH=\"$(shell pwd)/librelog.so\" \
		-DRELOG_FORCE_DEFAULT_OUTPUT_TO=\"/dev/ulog_main\" \
		-DRELOG_FORCE_DEFAULT_ERROR_TO=\"/dev/ulog_main\"

librelog.so:librelog.c
	gcc $^ -o $@ -fPIC -shared -g3 -O0 \
		-Wall -Wextra -Werror

clean:
	rm -f librelog.so relog

.PHONY: clean