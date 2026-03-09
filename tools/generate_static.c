#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <ctype.h>

#include <md4c-html.h>

#define PATH_MAX_LEN 4096

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} sbuf_t;

typedef struct {
    char *slug;
    char *title;
    char *date;
    char *excerpt;
    char **tags;
    size_t tag_count;
    int reading_minutes;
    char *source_path;
} blog_post_t;

typedef struct {
    char *id;
    char *title;
    char *description;
    char *accent_primary;
    char *accent_secondary;
    int order;
    blog_post_t *posts;
    size_t post_count;
} blog_category_t;

typedef struct {
    blog_category_t *items;
    size_t count;
} blog_catalog_t;

static void sbuf_init(sbuf_t *sb) {
    sb->cap = 4096;
    sb->len = 0;
    sb->data = (char *)malloc(sb->cap);
    if (sb->data) sb->data[0] = '\0';
}

static void sbuf_reserve(sbuf_t *sb, size_t add) {
    if (!sb->data) return;
    if (sb->len + add + 1 <= sb->cap) return;
    size_t new_cap = sb->cap * 2;
    while (sb->len + add + 1 > new_cap) new_cap *= 2;
    char *tmp = (char *)realloc(sb->data, new_cap);
    if (!tmp) return;
    sb->data = tmp;
    sb->cap = new_cap;
}

static void sbuf_append_n(sbuf_t *sb, const char *text, size_t n) {
    if (!sb->data || !text) return;
    sbuf_reserve(sb, n);
    memcpy(sb->data + sb->len, text, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
}

static void sbuf_append(sbuf_t *sb, const char *text) {
    if (!text) return;
    sbuf_append_n(sb, text, strlen(text));
}

static void sbuf_append_fmt(sbuf_t *sb, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    va_list copy;
    va_copy(copy, args);
    int needed = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (needed < 0) {
        va_end(args);
        return;
    }
    sbuf_reserve(sb, (size_t)needed);
    vsnprintf(sb->data + sb->len, sb->cap - sb->len, fmt, args);
    sb->len += (size_t)needed;
    va_end(args);
}

static void sbuf_reset(sbuf_t *sb) {
    if (sb->data) sb->data[0] = '\0';
    sb->len = 0;
}

static void sbuf_free(sbuf_t *sb) {
    free(sb->data);
    sb->data = NULL;
    sb->len = sb->cap = 0;
}

static void html_escape(sbuf_t *sb, const char *text) {
    if (!text) return;
    for (const char *p = text; *p; ++p) {
        switch (*p) {
            case '&': sbuf_append(sb, "&amp;"); break;
            case '<': sbuf_append(sb, "&lt;"); break;
            case '>': sbuf_append(sb, "&gt;"); break;
            case '"': sbuf_append(sb, "&quot;"); break;
            case '\'': sbuf_append(sb, "&#39;"); break;
            default:
                sbuf_append_n(sb, p, 1);
        }
    }
}

static char *strdup_safe(const char *src) {
    return src ? strdup(src) : NULL;
}

static char *read_file(const char *path, size_t *out_len) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (len < 0) {
        fclose(fp);
        return NULL;
    }
    char *buf = (char *)malloc((size_t)len + 1);
    if (!buf) {
        fclose(fp);
        return NULL;
    }
    size_t read = fread(buf, 1, (size_t)len, fp);
    fclose(fp);
    buf[read] = '\0';
    if (out_len) *out_len = read;
    return buf;
}

static bool ensure_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    if (mkdir(path, 0755) == 0) return true;
    if (errno == EEXIST) return true;
    return false;
}

static bool ensure_parents(const char *filepath) {
    char tmp[PATH_MAX_LEN];
    snprintf(tmp, sizeof(tmp), "%s", filepath);
    char *slash = strrchr(tmp, '/');
    if (!slash) return true;
    *slash = '\0';
    if (tmp[0] == '\0') return true;
    for (char *p = tmp + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (!ensure_dir(tmp)) {
                *p = '/';
                return false;
            }
            *p = '/';
        }
    }
    return ensure_dir(tmp);
}

static bool write_file(const char *path, const char *content) {
    if (!ensure_parents(path)) {
        fprintf(stderr, "[bloggen] cannot create directories for %s\n", path);
        return false;
    }
    FILE *fp = fopen(path, "wb");
    if (!fp) {
        fprintf(stderr, "[bloggen] cannot write %s\n", path);
        return false;
    }
    size_t len = content ? strlen(content) : 0;
    if (len > 0) fwrite(content, 1, len, fp);
    fclose(fp);
    return true;
}

static char *trim(char *str) {
    if (!str) return str;
    while (isspace((unsigned char)*str)) str++;
    if (*str == '\0') return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) *end-- = '\0';
    return str;
}

static void free_catalog(blog_catalog_t *catalog) {
    if (!catalog || !catalog->items) return;
    for (size_t i = 0; i < catalog->count; ++i) {
        blog_category_t *cat = &catalog->items[i];
        free(cat->id);
        free(cat->title);
        free(cat->description);
        free(cat->accent_primary);
        free(cat->accent_secondary);
        for (size_t j = 0; j < cat->post_count; ++j) {
            blog_post_t *post = &cat->posts[j];
            free(post->slug);
            free(post->title);
            free(post->date);
            free(post->excerpt);
            free(post->source_path);
            for (size_t t = 0; t < post->tag_count; ++t) free(post->tags[t]);
            free(post->tags);
        }
        free(cat->posts);
    }
    free(catalog->items);
    catalog->items = NULL;
    catalog->count = 0;
}

static blog_category_t *add_category(blog_catalog_t *catalog) {
    size_t new_count = catalog->count + 1;
    blog_category_t *tmp = realloc(catalog->items, new_count * sizeof(blog_category_t));
    if (!tmp) return NULL;
    catalog->items = tmp;
    blog_category_t *cat = &catalog->items[catalog->count];
    memset(cat, 0, sizeof(*cat));
    cat->accent_primary = strdup_safe("#ff7a18");
    cat->accent_secondary = strdup_safe("#ffb347");
    cat->order = (int)new_count;
    catalog->count = new_count;
    return cat;
}

static bool load_categories_cfg(const char *path, blog_catalog_t *catalog) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "[bloggen] cannot open %s\n", path);
        return false;
    }
    char line[1024];
    blog_category_t *current = NULL;
    while (fgets(line, sizeof(line), fp)) {
        char *trimmed = trim(line);
        if (*trimmed == '#' || *trimmed == ';' || *trimmed == '\0') continue;
        if (trimmed[0] == '[') {
            char *end = strchr(trimmed, ']');
            if (!end) continue;
            *end = '\0';
            current = add_category(catalog);
            if (!current) break;
            free(current->id);
            current->id = strdup_safe(trim(trimmed + 1));
            continue;
        }
        char *eq = strchr(trimmed, '=');
        if (!eq || !current) continue;
        *eq = '\0';
        char *key = trim(trimmed);
        char *val = trim(eq + 1);
        if (strcmp(key, "title") == 0) {
            free(current->title);
            current->title = strdup_safe(val);
        } else if (strcmp(key, "description") == 0) {
            free(current->description);
            current->description = strdup_safe(val);
        } else if (strcmp(key, "accent_primary") == 0) {
            free(current->accent_primary);
            current->accent_primary = strdup_safe(val);
        } else if (strcmp(key, "accent_secondary") == 0) {
            free(current->accent_secondary);
            current->accent_secondary = strdup_safe(val);
        } else if (strcmp(key, "order") == 0) {
            current->order = atoi(val);
        }
    }
    fclose(fp);
    return catalog->count > 0;
}

static void add_tag(blog_post_t *post, const char *value) {
    size_t new_count = post->tag_count + 1;
    char **tmp = realloc(post->tags, new_count * sizeof(char *));
    if (!tmp) return;
    post->tags = tmp;
    post->tags[post->tag_count] = strdup_safe(value);
    post->tag_count = new_count;
}

static void parse_tags(blog_post_t *post, const char *csv) {
    char *copy = strdup_safe(csv);
    if (!copy) return;
    char *token = strtok(copy, ",");
    while (token) {
        char *trimmed = trim(token);
        if (*trimmed) add_tag(post, trimmed);
        token = strtok(NULL, ",");
    }
    free(copy);
}

static bool parse_front_matter(const char *content, size_t len,
                               size_t *body_offset,
                               blog_post_t *post) {
    const char *ptr = content;
    const char *end = content + len;
    if (len < 3 || strncmp(ptr, "---", 3) != 0) {
        *body_offset = 0;
        return true;
    }
    ptr = strchr(ptr, '\n');
    if (!ptr) return false;
    ptr++;
    while (ptr < end) {
        const char *line_end = strchr(ptr, '\n');
        size_t line_len = line_end ? (size_t)(line_end - ptr) : (size_t)(end - ptr);
        if (line_len >= 3 && strncmp(ptr, "---", 3) == 0) {
            *body_offset = (size_t)((line_end ? line_end + 1 : end) - content);
            return true;
        }
        char line[1024];
        size_t copy_len = line_len < sizeof(line) - 1 ? line_len : sizeof(line) - 1;
        memcpy(line, ptr, copy_len);
        line[copy_len] = '\0';
        char *eq = strchr(line, ':');
        if (eq) {
            *eq = '\0';
            char *key = trim(line);
            char *val = trim(eq + 1);
            if (strcmp(key, "title") == 0) {
                free(post->title);
                post->title = strdup_safe(val);
            } else if (strcmp(key, "date") == 0) {
                free(post->date);
                post->date = strdup_safe(val);
            } else if (strcmp(key, "excerpt") == 0) {
                free(post->excerpt);
                post->excerpt = strdup_safe(val);
            } else if (strcmp(key, "tags") == 0) {
                parse_tags(post, val);
            } else if (strcmp(key, "reading_minutes") == 0) {
                post->reading_minutes = atoi(val);
            }
        }
        if (!line_end) break;
        ptr = line_end + 1;
    }
    return false;
}

static bool collect_posts_for_category(blog_category_t *cat, const char *posts_root) {
    char dir_path[PATH_MAX_LEN];
    snprintf(dir_path, sizeof(dir_path), "%s/%s", posts_root, cat->id);
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "[bloggen] missing directory %s\n", dir_path);
        return false;
    }
    struct dirent *entry;
    size_t count = 0;
    blog_post_t *posts = NULL;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') continue;
        const char *dot = strrchr(entry->d_name, '.');
        if (!dot || strcmp(dot, ".md") != 0) continue;
        size_t new_count = count + 1;
        blog_post_t *tmp = realloc(posts, new_count * sizeof(blog_post_t));
        if (!tmp) continue;
        posts = tmp;
        blog_post_t *post = &posts[count];
        memset(post, 0, sizeof(*post));
        size_t slug_len = (size_t)(dot - entry->d_name);
        post->slug = (char *)malloc(slug_len + 1);
        memcpy(post->slug, entry->d_name, slug_len);
        post->slug[slug_len] = '\0';

        char file_path[PATH_MAX_LEN];
        snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);
        size_t file_len = 0;
        char *content = read_file(file_path, &file_len);
        if (!content) {
            fprintf(stderr, "[bloggen] failed to read %s\n", file_path);
            free(content);
            continue;
        }
        size_t body_offset = 0;
        if (!parse_front_matter(content, file_len, &body_offset, post)) {
            fprintf(stderr, "[bloggen] invalid front matter in %s\n", file_path);
            free(content);
            continue;
        }
        if (!post->title) {
            post->title = strdup_safe(post->slug);
        }
        if (!post->excerpt) {
            const char *body = content + body_offset;
            size_t excerpt_len = strlen(body);
            if (excerpt_len > 180) excerpt_len = 180;
            char *excerpt = (char *)malloc(excerpt_len + 1);
            memcpy(excerpt, body, excerpt_len);
            excerpt[excerpt_len] = '\0';
            post->excerpt = excerpt;
        }
        post->source_path = strdup_safe(file_path);
        free(content);
        count = new_count;
    }
    closedir(dir);
    cat->posts = posts;
    cat->post_count = count;
    return true;
}

static int compare_categories(const void *a, const void *b) {
    const blog_category_t *ca = (const blog_category_t *)a;
    const blog_category_t *cb = (const blog_category_t *)b;
    return ca->order - cb->order;
}

static void md_callback(const MD_CHAR *text, MD_SIZE size, void *userdata) {
    sbuf_t *sb = (sbuf_t *)userdata;
    sbuf_append_n(sb, (const char *)text, (size_t)size);
}

static bool render_markdown(const char *path, sbuf_t *out) {
    size_t len = 0;
    char *content = read_file(path, &len);
    if (!content) {
        fprintf(stderr, "[bloggen] failed to read markdown %s\n", path);
        return false;
    }
    sbuf_reset(out);
    int rc = md_html((const MD_CHAR *)content, (MD_SIZE)len, md_callback, out,
                     MD_FLAG_TABLES | MD_FLAG_STRIKETHROUGH | MD_FLAG_TASKLISTS,
                     MD_HTML_FLAG_SKIP_UTF8_BOM);
    free(content);
    return rc == 0;
}

static void append_prefixed(sbuf_t *sb, const char *prefix, const char *suffix) {
    if (prefix) sbuf_append(sb, prefix);
    sbuf_append(sb, suffix);
}

static void render_layout(blog_catalog_t *catalog, const char *page_title,
                          const char *accent_primary, const char *accent_secondary,
                          const char *active_category,
                          const char *main_html,
                          const char *root_prefix,
                          sbuf_t *out) {
    sbuf_reset(out);
    sbuf_append(out, "<!DOCTYPE html><html lang=\"ko\"><head><meta charset=\"utf-8\">");
    sbuf_append(out, "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
    sbuf_append(out, "<title>");
    html_escape(out, page_title ? page_title : "cwist Blog");
    sbuf_append(out, "</title><link rel=\"stylesheet\" href=\"");
    append_prefixed(out, root_prefix, "assets/styles.css\">");
    if (accent_primary && accent_secondary) {
        sbuf_append(out, "<style>:root{--accent-primary:");
        sbuf_append(out, accent_primary);
        sbuf_append(out, ";--accent-secondary:");
        sbuf_append(out, accent_secondary);
        sbuf_append(out, ";}</style>");
    }
    sbuf_append(out, "</head><body><div class=\"page-shell\"><nav><div class=\"brand\">cwist Material Blog</div><div class=\"nav-links\">");
    for (size_t i = 0; i < catalog->count; ++i) {
        blog_category_t *cat = &catalog->items[i];
        if (!cat->id || !cat->title) continue;
        sbuf_append(out, "<a ");
        if (active_category && strcmp(active_category, cat->id) == 0) sbuf_append(out, "class=\"active\" ");
        sbuf_append(out, "href=\"");
        append_prefixed(out, root_prefix, "category/");
        sbuf_append(out, cat->id);
        sbuf_append(out, "/\">");
        html_escape(out, cat->title);
        sbuf_append(out, "</a>");
    }
    sbuf_append(out, "</div></nav><main>");
    sbuf_append(out, main_html ? main_html : "");
    sbuf_append(out, "</main><footer>GitHub Pages static build</footer></div></body></html>");
}

static void build_home(blog_catalog_t *catalog, const char *out_dir) {
    sbuf_t section, page;
    sbuf_init(&section);
    sbuf_init(&page);

    sbuf_append(&section, "<section class=\"card\"><h1>Categories</h1><div class=\"category-list\">");
    for (size_t i = 0; i < catalog->count; ++i) {
        blog_category_t *cat = &catalog->items[i];
        if (!cat->id || !cat->title) continue;
        sbuf_append(&section, "<a class=\"category-card\" href=\"");
        append_prefixed(&section, "", "category/");
        sbuf_append(&section, cat->id);
        sbuf_append(&section, "/\"><div class=\"accent-pill\"><span class=\"accent-dot\" style=\"background:");
        sbuf_append(&section, cat->accent_primary);
        sbuf_append(&section, ";\"></span>");
        html_escape(&section, cat->title);
        sbuf_append(&section, "</div><p>");
        html_escape(&section, cat->description ? cat->description : "");
        sbuf_append(&section, "</p></a>");
    }
    sbuf_append(&section, "</div></section>");

    render_layout(catalog, "cwist Blog", NULL, NULL, NULL, section.data, "", &page);

    char path[PATH_MAX_LEN];
    snprintf(path, sizeof(path), "%s/index.html", out_dir);
    write_file(path, page.data);

    sbuf_free(&section);
    sbuf_free(&page);
}

static void build_category_page(blog_catalog_t *catalog, blog_category_t *cat, const char *out_dir) {
    sbuf_t section, page;
    sbuf_init(&section);
    sbuf_init(&page);
    const char *root_prefix = "../../";

    sbuf_append(&section, "<section class=\"card\"><h1>");
    html_escape(&section, cat->title);
    sbuf_append(&section, "</h1><p>");
    html_escape(&section, cat->description ? cat->description : "");
    sbuf_append(&section, "</p><div class=\"post-list\">");
    for (size_t i = 0; i < cat->post_count; ++i) {
        blog_post_t *post = &cat->posts[i];
        if (!post->slug || !post->title) continue;
        sbuf_append(&section, "<a class=\"post-card\" href=\"");
        append_prefixed(&section, root_prefix, "post/");
        sbuf_append(&section, cat->id);
        sbuf_append(&section, "/");
        sbuf_append(&section, post->slug);
        sbuf_append(&section, "/\"><h3>");
        html_escape(&section, post->title);
        sbuf_append(&section, "</h3><p>");
        html_escape(&section, post->excerpt ? post->excerpt : "");
        sbuf_append(&section, "</p><div class=\"tags\">");
        for (size_t t = 0; t < post->tag_count; ++t) {
            sbuf_append(&section, "<span class=\"tag\">");
            html_escape(&section, post->tags[t]);
            sbuf_append(&section, "</span>");
        }
        sbuf_append(&section, "</div></a>");
    }
    sbuf_append(&section, "</div></section>");

    render_layout(catalog, cat->title, cat->accent_primary, cat->accent_secondary, cat->id, section.data, root_prefix, &page);

    char path[PATH_MAX_LEN];
    snprintf(path, sizeof(path), "%s/category/%s/index.html", out_dir, cat->id);
    write_file(path, page.data);

    sbuf_free(&section);
    sbuf_free(&page);
}

static void build_post_page(blog_catalog_t *catalog, blog_category_t *cat, blog_post_t *post, const char *out_dir) {
    sbuf_t md_html, section, page;
    sbuf_init(&md_html);
    sbuf_init(&section);
    sbuf_init(&page);
    const char *root_prefix = "../../../";

    if (!render_markdown(post->source_path, &md_html)) {
        fprintf(stderr, "[bloggen] failed to render %s\n", post->source_path);
        goto cleanup;
    }

    sbuf_append(&section, "<section class=\"card\"><a href=\"");
    append_prefixed(&section, root_prefix, "category/");
    sbuf_append(&section, cat->id);
    sbuf_append(&section, "/\">&larr; Back to ");
    html_escape(&section, cat->title);
    sbuf_append(&section, "</a><h1>");
    html_escape(&section, post->title);
    sbuf_append(&section, "</h1><p>");
    html_escape(&section, post->date ? post->date : "");
    if (post->reading_minutes > 0) {
        sbuf_append(&section, " &middot; ");
        sbuf_append_fmt(&section, "%d min read", post->reading_minutes);
    }
    sbuf_append(&section, "</p><div class=\"post-body\">");
    sbuf_append(&section, md_html.data);
    sbuf_append(&section, "</div></section>");

    render_layout(catalog, post->title, cat->accent_primary, cat->accent_secondary, cat->id, section.data, root_prefix, &page);

    char path[PATH_MAX_LEN];
    snprintf(path, sizeof(path), "%s/post/%s/%s/index.html", out_dir, cat->id, post->slug);
    write_file(path, page.data);

cleanup:
    sbuf_free(&md_html);
    sbuf_free(&section);
    sbuf_free(&page);
}

static void copy_assets(const char *src_css, const char *out_dir) {
    char dst_path[PATH_MAX_LEN];
    snprintf(dst_path, sizeof(dst_path), "%s/assets/styles.css", out_dir);
    if (!ensure_parents(dst_path)) {
        fprintf(stderr, "[bloggen] failed to create assets directory\n");
        return;
    }
    FILE *src = fopen(src_css, "rb");
    if (!src) {
        fprintf(stderr, "[bloggen] missing %s\n", src_css);
        return;
    }
    FILE *dst = fopen(dst_path, "wb");
    if (!dst) {
        fclose(src);
        fprintf(stderr, "[bloggen] cannot write %s\n", dst_path);
        return;
    }
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), src)) > 0) {
        fwrite(buf, 1, n, dst);
    }
    fclose(src);
    fclose(dst);
}

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <categories.cfg> <posts_dir> <assets_css> <out_dir>\n", argv[0]);
        return 1;
    }
    const char *categories_cfg = argv[1];
    const char *posts_dir = argv[2];
    const char *assets_css = argv[3];
    const char *out_dir = argv[4];

    blog_catalog_t catalog = {0};
    if (!load_categories_cfg(categories_cfg, &catalog)) {
        fprintf(stderr, "[bloggen] no categories defined\n");
        free_catalog(&catalog);
        return 1;
    }

    qsort(catalog.items, catalog.count, sizeof(blog_category_t), compare_categories);
    for (size_t i = 0; i < catalog.count; ++i) {
        if (!catalog.items[i].id) continue;
        collect_posts_for_category(&catalog.items[i], posts_dir);
    }

    copy_assets(assets_css, out_dir);

    build_home(&catalog, out_dir);
    for (size_t i = 0; i < catalog.count; ++i) {
        blog_category_t *cat = &catalog.items[i];
        if (!cat->id) continue;
        build_category_page(&catalog, cat, out_dir);
        for (size_t j = 0; j < cat->post_count; ++j) {
            build_post_page(&catalog, cat, &cat->posts[j], out_dir);
        }
    }

    free_catalog(&catalog);
    return 0;
}
