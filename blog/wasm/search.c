/*
 * cwist search module — compiled to WebAssembly via Emscripten.
 *
 * Exports a single function:
 *
 *   float cwist_score(query, title, tags, summary, body)
 *
 * Returns a relevance score >= 0.0.  Higher means a better match.
 * Score == 0.0 means no match at all.
 *
 * Weights:  title 3 pts · tags 2 pts · summary 1 pt · body 1 pt
 *
 * Algorithm:
 *   Case-insensitive substring search (ASCII).
 *   Multi-byte UTF-8 sequences (Korean etc.) are compared byte-exact,
 *   which is correct because those scripts do not have case.
 */

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#  define WASM_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#  define WASM_EXPORT
#endif

#include <string.h>
#include <ctype.h>

/*
 * icontains — case-insensitive substring search.
 *
 * For each byte:
 *   - ASCII (< 0x80): fold to lower-case before comparing.
 *   - Non-ASCII (>= 0x80): compare byte-exact (UTF-8 continuation bytes
 *     cannot be confused with ASCII, so this is safe).
 *
 * Returns 1 if needle is found inside haystack, 0 otherwise.
 */
static int icontains(const char *haystack, const char *needle) {
    if (!haystack || !needle || !*needle) return 0;
    size_t nlen = strlen(needle);
    for (; *haystack; ++haystack) {
        size_t i;
        for (i = 0; i < nlen; ++i) {
            if (!haystack[i]) break;
            unsigned char h = (unsigned char)haystack[i];
            unsigned char n = (unsigned char)needle[i];
            if (h < 0x80 && n < 0x80) {
                if ((unsigned char)tolower(h) != (unsigned char)tolower(n)) break;
            } else {
                if (h != n) break;
            }
        }
        if (i == nlen) return 1;
    }
    return 0;
}

/*
 * cwist_score — score a post against a search query.
 *
 * All parameters are UTF-8 C strings.  NULL is treated as empty.
 * tags should be the tag list joined with spaces before calling.
 */
WASM_EXPORT
float cwist_score(const char *query,
                  const char *title,
                  const char *tags,
                  const char *summary,
                  const char *body) {
    if (!query || !*query) return 0.0f;

    float score = 0.0f;
    if (title   && icontains(title,   query)) score += 3.0f;
    if (tags    && icontains(tags,    query)) score += 2.0f;
    if (summary && icontains(summary, query)) score += 1.0f;
    if (body    && icontains(body,    query)) score += 1.0f;
    return score;
}
