#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <emscripten/emscripten.h>

#include <cwist/core/sstring/sstring.h>
#include <cwist/sys/err/cwist_err.h>

#include "md4c-html.h"

#define BLOG_PARSER_FLAGS (MD_FLAG_TABLES | MD_FLAG_STRIKETHROUGH | MD_FLAG_TASKLISTS | MD_FLAG_LATEXMATHSPANS)
#define BLOG_RENDERER_FLAGS (MD_HTML_FLAG_SKIP_UTF8_BOM)

static cwist_sstring g_html;
static bool g_ready = false;
static bool g_has_error = false;
static char g_error_msg[256];

static void blog_reset_error(void) {
    g_has_error = false;
    g_error_msg[0] = '\0';
}

static void ensure_ready(void) {
    if (!g_ready) {
        cwist_sstring_init(&g_html);
        g_ready = true;
    }
}

static bool cwist_result_ok(cwist_error_t err) {
    return err.errtype == CWIST_ERR_INT8 && err.error.err_i8 == ERR_SSTRING_OKAY;
}

static void blog_output_callback(const MD_CHAR *text, MD_SIZE size, void *userdata) {
    (void)userdata;
    if (g_has_error) return;
    cwist_error_t err = cwist_sstring_append_len(&g_html, (const char *)text, (size_t)size);
    if (!cwist_result_ok(err)) {
        g_has_error = true;
        snprintf(g_error_msg, sizeof(g_error_msg), "cwist_append_failed:%d", err.error.err_i8);
    }
}

EMSCRIPTEN_KEEPALIVE
void cwist_blog_init(void) {
    ensure_ready();
    blog_reset_error();
}

EMSCRIPTEN_KEEPALIVE
int cwist_blog_render(const char *markdown, size_t len) {
    ensure_ready();
    blog_reset_error();

    if (!markdown || len == 0) {
        cwist_error_t err = cwist_sstring_assign_len(&g_html, NULL, 0);
        (void)err;
        return 0;
    }

    cwist_error_t reset = cwist_sstring_assign_len(&g_html, NULL, 0);
    if (!cwist_result_ok(reset)) {
        snprintf(g_error_msg, sizeof(g_error_msg), "cwist_reset_failed:%d", reset.error.err_i8);
        g_has_error = true;
        return -1;
    }

    int rc = md_html((const MD_CHAR *)markdown, (MD_SIZE)len, blog_output_callback, NULL, BLOG_PARSER_FLAGS, BLOG_RENDERER_FLAGS);
    if (rc != 0) {
        g_has_error = true;
        snprintf(g_error_msg, sizeof(g_error_msg), "md4c_failed:%d", rc);
        return rc;
    }

    return g_has_error ? -1 : 0;
}

EMSCRIPTEN_KEEPALIVE
const char *cwist_blog_html_ptr(void) {
    ensure_ready();
    return g_html.data ? g_html.data : "";
}

EMSCRIPTEN_KEEPALIVE
size_t cwist_blog_html_len(void) {
    ensure_ready();
    return g_html.size;
}

EMSCRIPTEN_KEEPALIVE
const char *cwist_blog_error(void) {
    if (!g_has_error || g_error_msg[0] == '\0') {
        return "";
    }
    return g_error_msg;
}

EMSCRIPTEN_KEEPALIVE
void cwist_blog_clear(void) {
    if (!g_ready) return;
    cwist_sstring_assign_len(&g_html, NULL, 0);
    blog_reset_error();
}
