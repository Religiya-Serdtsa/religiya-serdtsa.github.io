CC ?= gcc

STATIC_GENERATOR := bin/bloggen
STATIC_SRCS := \
	tools/generate_static.c \
	lib/md4c/src/md4c.c \
	lib/md4c/src/md4c-html.c \
	lib/md4c/src/entity.c
STATIC_INCLUDES := -Ilib/md4c/src

.PHONY: static-site clean

static-site: $(STATIC_GENERATOR)
	./$(STATIC_GENERATOR) docs/categories.cfg docs/posts assets/styles.css docs

$(STATIC_GENERATOR): $(STATIC_SRCS)
	@mkdir -p $(@D)
	$(CC) -std=c17 -O2 $(STATIC_SRCS) $(STATIC_INCLUDES) -o $@

clean:
	rm -rf bin docs/category docs/post docs/assets
