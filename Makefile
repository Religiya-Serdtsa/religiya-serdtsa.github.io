CC  ?= gcc
EMCC ?= emcc

STATIC_GENERATOR := bin/bloggen
STATIC_SRCS := \
	tools/generate_static.c \
	tools/scheduler.c \
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

.PHONY: static-site wasm-search clean

static-site: $(STATIC_GENERATOR)
	rm -rf docs
	./$(STATIC_GENERATOR) categories.cfg posts assets/styles.css docs
	cp assets/comments.js  docs/assets/comments.js
	cp assets/search-ui.js docs/assets/search-ui.js
	[ -f assets/search-module.js   ] && cp assets/search-module.js   docs/assets/search-module.js   || true
	[ -f assets/search-module.wasm ] && cp assets/search-module.wasm docs/assets/search-module.wasm || true
	mkdir -p docs/data
	if [ -f data/comments.json ]; then cp data/comments.json docs/data/comments.json; else echo '{}' > docs/data/comments.json; fi
	touch docs/.nojekyll

# Build the CWIST search WASM module (requires Emscripten).
# Outputs: assets/search-module.js  assets/search-module.wasm
wasm-search: blog/wasm/search.c
	@mkdir -p assets
	$(EMCC) -O2 --no-entry \
	    -s WASM=1 \
	    -s MODULARIZE=1 \
	    -s EXPORT_NAME=CwistSearchModule \
	    -s "EXPORTED_FUNCTIONS=[\"_cwist_score\"]" \
	    -s "EXPORTED_RUNTIME_METHODS=[\"ccall\",\"cwrap\"]" \
	    -s FILESYSTEM=0 \
	    -s ENVIRONMENT=web \
	    -s ALLOW_MEMORY_GROWTH=1 \
	    -o assets/search-module.js $<

$(STATIC_GENERATOR): $(STATIC_SRCS)
	@mkdir -p $(@D)
	$(CC) -std=c17 -O2 $(STATIC_SRCS) $(STATIC_INCLUDES) -o $@

clean:
	rm -rf bin docs
