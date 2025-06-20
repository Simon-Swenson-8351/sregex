CFLAGS := $(CFLAGS) -Iapi -Iinc -Isrc -Itest -Wall -Wextra -Wpedantic -fsanitize=address -g --std=c99

deps := c_collections

all: $(deps)

.PHONY: all $(deps)

$(deps):
	$(MAKE) -C deps/$@
	cp -r deps/$@/dist depsdist
