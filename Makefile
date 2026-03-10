CC ?= gcc

STATIC_GENERATOR := bin/bloggen
STATIC_SRCS := \
	tools/generate_static.c \
	lib/cwist/src/sys/err/error.c \
	lib/cwist/src/core/sstring/sstring.c \
	blog/wasm/cwist_alloc_stub.c \
	lib/cwist/lib/cjson/cJSON.c \
	lib/md4c/src/md4c.c \
	lib/md4c/src/md4c-html.c \
	lib/md4c/src/entity.c

# cwist include paths: real cjson must be found before the wasm stub dir
STATIC_INCLUDES := \
	-Ilib/cwist/include \
	-Ilib/cwist/lib \
	-Iblog/wasm/stubs \
	-Ilib/md4c/src

.PHONY: static-site clean

static-site: $(STATIC_GENERATOR)
	rm -rf docs
	./$(STATIC_GENERATOR) categories.cfg posts assets/styles.css docs
	cp assets/comments.js docs/assets/comments.js
	mkdir -p docs/data
	if [ -f data/comments.json ]; then cp data/comments.json docs/data/comments.json; else echo '{}' > docs/data/comments.json; fi
	touch docs/.nojekyll

$(STATIC_GENERATOR): $(STATIC_SRCS)
	@mkdir -p $(@D)
	$(CC) -std=c17 -O2 $(STATIC_SRCS) $(STATIC_INCLUDES) -o $@

clean:
	rm -rf bin docs
