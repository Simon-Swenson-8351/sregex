deps := c_collections

all: $(deps)

.PHONY: all $(deps)

$(deps):
	$(MAKE) -C deps/$@
	cp -r deps/$@/dist depsdist
