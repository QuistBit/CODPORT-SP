/* R78: project the authored starting fog from a map's original GSC source.
 * This deliberately does not execute unrelated mission statements/helpers.
 * Full source is lexed to identify real main, with bounded structured walks
 * inside main. Only constant fog arguments and pure cvar equality predicates
 * are admitted; the existing VM/host remains responsible for actual effects. */
#include "cod1_author_fog.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define AF_MAX_TOKENS 131072u
#define AF_MAX_DEPTH 48u
#define AF_MAX_EXPR_TOKENS 512u
#define AF_MAX_CVARS 32u
#define AF_MAX_WRITES 32u
#define AF_NONE ((size_t)-1)

enum { AF_ID = 256, AF_NUMBER, AF_STRING, AF_EQ, AF_NE,
       AF_DEV_OPEN, AF_DEV_CLOSE };

typedef struct {
    int kind;
    size_t offset, length, match;
    unsigned line, column;
} AfToken;

typedef struct {
    const char *source;
    size_t size, count, capacity;
    AfToken *tokens;
    Cod1AuthorFogReadCvar read_cvar;
    void *user;
    char *error;
    size_t error_size;
    int failed;
    unsigned cvar_count;
    struct { char name[COD1_AUTHOR_FOG_CVAR_CAP];
             char value[COD1_AUTHOR_FOG_CVAR_CAP]; } cvars[AF_MAX_CVARS];
} AfParser;

/* flow: 0 = configuration continues; 1 = definite time/return boundary;
 * 2 = an opaque earlier conditional/loop may cross such a boundary. */
typedef struct {
    Cod1AuthorFogPlan plan;
    int flow, unknown_write;
    unsigned write_count;
    char writes[AF_MAX_WRITES][COD1_AUTHOR_FOG_CVAR_CAP];
} AfContext;

enum { AF_SIMPLE, AF_BLOCK, AF_IF, AF_OPAQUE };
typedef struct {
    int kind;
    size_t start, end, condition_start, condition_end;
    size_t first, second;
} AfStatement;

typedef struct { float number; long long integer; int is_integer; } AfNumber;
typedef struct { AfParser *parser; size_t next, end; unsigned depth; } AfExpression;

static int af_fail(AfParser *p, size_t token, const char *format, ...) {
    va_list args;
    size_t prefix = 0;
    if (p->failed) return 0;
    p->failed = 1;
    if (p->error && p->error_size) {
        if (token < p->count) {
            int n = snprintf(p->error, p->error_size, "line %u:%u: ",
                             p->tokens[token].line, p->tokens[token].column);
            if (n > 0) prefix = (size_t)n;
            if (prefix >= p->error_size) prefix = p->error_size - 1;
        }
        va_start(args, format);
        vsnprintf(p->error + prefix, p->error_size - prefix, format, args);
        va_end(args);
        p->error[p->error_size - 1] = 0;
    }
    return 0;
}

static int af_word(const AfParser *p, size_t i, const char *word) {
    size_t n = strlen(word);
    return i < p->count && p->tokens[i].kind == AF_ID &&
           p->tokens[i].length == n &&
           !memcmp(p->source + p->tokens[i].offset, word, n);
}

static int af_word_ci(const AfParser *p, size_t i, const char *word) {
    size_t j, n = strlen(word);
    if (i >= p->count || p->tokens[i].kind != AF_ID ||
        p->tokens[i].length != n) return 0;
    for (j = 0; j < n; ++j)
        if (tolower((unsigned char)p->source[p->tokens[i].offset + j]) !=
            tolower((unsigned char)word[j])) return 0;
    return 1;
}

static int af_same_name(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a++) != tolower((unsigned char)*b++)) return 0;
    }
    return *a == *b;
}

static int af_kind(const AfParser *p, size_t i, int kind) {
    return i < p->count && p->tokens[i].kind == kind;
}

static int af_open(int kind) {
    return kind == '(' || kind == '[' || kind == '{' || kind == AF_DEV_OPEN;
}

static int af_close(int kind) {
    return kind == ')' || kind == ']' || kind == '}' || kind == AF_DEV_CLOSE;
}

static int af_pair(int a, int b) {
    return (a == '(' && b == ')') || (a == '[' && b == ']') ||
           (a == '{' && b == '}') || (a == AF_DEV_OPEN && b == AF_DEV_CLOSE);
}

static void af_advance(const char *source, size_t *position, unsigned *line,
                       unsigned *column) {
    if (source[(*position)++] == '\n') { ++*line; *column = 1; }
    else ++*column;
}

static int af_add(AfParser *p, int kind, size_t start, size_t end,
                  unsigned line, unsigned column) {
    AfToken *next;
    size_t capacity;
    if (p->count == AF_MAX_TOKENS) return af_fail(p, AF_NONE, "token limit");
    if (p->count == p->capacity) {
        capacity = p->capacity ? p->capacity * 2 : 1024;
        if (capacity > AF_MAX_TOKENS) capacity = AF_MAX_TOKENS;
        next = (AfToken *)realloc(p->tokens, capacity * sizeof(*next));
        if (!next) return af_fail(p, AF_NONE, "token allocation failed");
        p->tokens = next; p->capacity = capacity;
    }
    next = &p->tokens[p->count++];
    next->kind = kind; next->offset = start; next->length = end - start;
    next->line = line; next->column = column; next->match = AF_NONE;
    return 1;
}

static int af_lex(AfParser *p) {
    size_t pos = 0, start, stack[AF_MAX_DEPTH], depth = 0, i;
    unsigned line = 1, column = 1, token_line, token_column;
    int kind;
    while (pos < p->size) {
        unsigned char c = (unsigned char)p->source[pos];
        if (isspace(c)) { af_advance(p->source, &pos, &line, &column); continue; }
        if (c == '/' && pos + 1 < p->size && p->source[pos + 1] == '/') {
            while (pos < p->size && p->source[pos] != '\n')
                af_advance(p->source, &pos, &line, &column);
            continue;
        }
        if (c == '/' && pos + 1 < p->size && p->source[pos + 1] == '*') {
            af_advance(p->source, &pos, &line, &column);
            af_advance(p->source, &pos, &line, &column);
            while (pos + 1 < p->size &&
                   !(p->source[pos] == '*' && p->source[pos + 1] == '/'))
                af_advance(p->source, &pos, &line, &column);
            if (pos + 1 >= p->size) return af_fail(p, AF_NONE, "unterminated comment");
            af_advance(p->source, &pos, &line, &column);
            af_advance(p->source, &pos, &line, &column);
            continue;
        }
        start = pos; token_line = line; token_column = column; kind = c;
        if (isalpha(c) || c == '_') {
            kind = AF_ID;
            do { af_advance(p->source, &pos, &line, &column); }
            while (pos < p->size && (isalnum((unsigned char)p->source[pos]) ||
                                     p->source[pos] == '_'));
        } else if (isdigit(c) || (c == '.' && pos + 1 < p->size &&
                                  isdigit((unsigned char)p->source[pos + 1]))) {
            int dot = c == '.';
            kind = AF_NUMBER;
            af_advance(p->source, &pos, &line, &column);
            while (pos < p->size && isdigit((unsigned char)p->source[pos]))
                af_advance(p->source, &pos, &line, &column);
            if (pos < p->size && p->source[pos] == '.' && !dot) {
                af_advance(p->source, &pos, &line, &column);
                while (pos < p->size && isdigit((unsigned char)p->source[pos]))
                    af_advance(p->source, &pos, &line, &column);
            }
            if (pos < p->size && (p->source[pos] == 'e' || p->source[pos] == 'E')) {
                af_advance(p->source, &pos, &line, &column);
                if (pos < p->size && (p->source[pos] == '+' || p->source[pos] == '-'))
                    af_advance(p->source, &pos, &line, &column);
                if (pos == p->size || !isdigit((unsigned char)p->source[pos]))
                    return af_fail(p, AF_NONE, "malformed numeric exponent");
                while (pos < p->size && isdigit((unsigned char)p->source[pos]))
                    af_advance(p->source, &pos, &line, &column);
            }
        } else if (c == '"') {
            int closed = 0;
            kind = AF_STRING;
            af_advance(p->source, &pos, &line, &column);
            while (pos < p->size) {
                c = (unsigned char)p->source[pos];
                if (c == '\n' || c == '\r') return af_fail(p, AF_NONE, "newline in string");
                af_advance(p->source, &pos, &line, &column);
                if (c == '"') { closed = 1; break; }
                if (c == '\\') {
                    if (pos == p->size || p->source[pos] == '\n' || p->source[pos] == '\r')
                        return af_fail(p, AF_NONE, "unterminated string escape");
                    af_advance(p->source, &pos, &line, &column);
                }
            }
            if (!closed) return af_fail(p, AF_NONE, "unterminated string");
        } else {
            af_advance(p->source, &pos, &line, &column);
            if (pos < p->size) {
                char n = p->source[pos];
                if ((c == '=' && n == '=') || (c == '!' && n == '=') ||
                    (c == '/' && n == '#') || (c == '#' && n == '/')) {
                    kind = c == '=' ? AF_EQ : c == '!' ? AF_NE :
                           c == '/' ? AF_DEV_OPEN : AF_DEV_CLOSE;
                    af_advance(p->source, &pos, &line, &column);
                }
            }
        }
        if (!af_add(p, kind, start, pos, token_line, token_column)) return 0;
    }
    for (i = 0; i < p->count; ++i) {
        kind = p->tokens[i].kind;
        if (af_open(kind)) {
            if (depth == AF_MAX_DEPTH) return af_fail(p, i, "delimiter nesting limit");
            stack[depth++] = i;
        } else if (af_close(kind)) {
            if (!depth || !af_pair(p->tokens[stack[depth - 1]].kind, kind))
                return af_fail(p, i, "mismatched delimiter");
            start = stack[--depth]; p->tokens[start].match = i; p->tokens[i].match = start;
        }
    }
    if (depth) return af_fail(p, stack[depth - 1], "unclosed delimiter");
    return 1;
}

static int af_string(AfParser *p, size_t token, char *out, size_t cap) {
    const AfToken *t;
    size_t i, n = 0;
    if (!af_kind(p, token, AF_STRING) || !out || !cap)
        return af_fail(p, token, "ordinary string required");
    t = &p->tokens[token];
    for (i = 1; i + 1 < t->length; ++i) {
        char c = p->source[t->offset + i];
        if (c == '\\') {
            c = p->source[t->offset + ++i];
            if (c == 'n') c = '\n';
            else if (c == 'r') c = '\r';
            else if (c == 't') c = '\t';
            else if (c != '"' && c != '\\')
                return af_fail(p, token, "unsupported profile string escape");
        }
        if (n + 1 >= cap) return af_fail(p, token, "profile string limit");
        out[n++] = c;
    }
    out[n] = 0; return 1;
}

static int af_number_expression(AfExpression *e, int precedence, AfNumber *out);

static int af_number_primary(AfExpression *e, AfNumber *out) {
    AfParser *p = e->parser;
    size_t token = e->next, close;
    char text[128], *end;
    int sign;
    if (token >= e->end) return af_fail(p, token, "missing numeric expression");
    if (++e->depth > AF_MAX_DEPTH) return af_fail(p, token, "expression nesting limit");
    if (af_kind(p, token, '+') || af_kind(p, token, '-')) {
        sign = af_kind(p, token, '-') ? -1 : 1; ++e->next;
        if (!af_number_primary(e, out)) return 0;
        if (out->is_integer) {
            if (sign < 0 && out->integer == INT_MIN)
                return af_fail(p, token, "integer negation overflow");
            out->integer *= sign; out->number = (float)out->integer;
        } else if (sign < 0) out->number = -out->number;
    } else if (af_kind(p, token, '(')) {
        close = p->tokens[token].match;
        if (close >= e->end) return af_fail(p, token, "numeric parenthesis range");
        if (close == token + 2 && af_word(p, token + 1, "float")) {
            e->next = close + 1;
            if (!af_number_primary(e, out)) return 0;
            out->number = out->is_integer ? (float)out->integer : out->number;
            out->is_integer = 0;
        } else {
            ++e->next;
            if (!af_number_expression(e, 0, out)) return 0;
            if (e->next != close) return af_fail(p, e->next, "unsupported numeric expression");
            ++e->next;
        }
    } else if (af_kind(p, token, AF_NUMBER)) {
        const AfToken *t = &p->tokens[token];
        if (t->length >= sizeof(text)) return af_fail(p, token, "numeric literal limit");
        memcpy(text, p->source + t->offset, t->length); text[t->length] = 0;
        errno = 0; out->number = strtof(text, &end);
        if (errno || *end || !isfinite(out->number))
            return af_fail(p, token, "numeric literal outside float32 range");
        out->is_integer = !strchr(text, '.') && !strchr(text, 'e') && !strchr(text, 'E');
        out->integer = 0;
        if (out->is_integer) {
            long integer;
            errno = 0; integer = strtol(text, &end, 10);
            if (errno || *end || integer < INT_MIN || integer > INT_MAX)
                return af_fail(p, token, "integer literal outside int32 range");
            out->integer = integer;
        }
        ++e->next;
    } else return af_fail(p, token, "only constant numeric fog expressions are supported");
    --e->depth;
    return 1;
}

static int af_precedence(int kind) {
    if (kind == '+' || kind == '-') return 1;
    if (kind == '*' || kind == '/') return 2;
    return 0;
}

static int af_number_expression(AfExpression *e, int precedence, AfNumber *out) {
    AfNumber right;
    AfParser *p = e->parser;
    int op, next_precedence;
    size_t token;
    if (!af_number_primary(e, out)) return 0;
    while (e->next < e->end) {
        op = p->tokens[e->next].kind; next_precedence = af_precedence(op);
        if (!next_precedence || next_precedence <= precedence) break;
        token = e->next++;
        if (!af_number_expression(e, next_precedence, &right)) return 0;
        if (op == '/' && (right.is_integer ? right.integer == 0 : right.number == 0.0f))
            return af_fail(p, token, "division by zero");
        if (out->is_integer && right.is_integer) {
            long long value;
            if (op == '+') value = out->integer + right.integer;
            else if (op == '-') value = out->integer - right.integer;
            else if (op == '*') value = out->integer * right.integer;
            else value = out->integer / right.integer;
            if (value < INT_MIN || value > INT_MAX) return af_fail(p, token, "integer expression overflow");
            out->integer = value; out->number = (float)value;
        } else {
            float a = out->is_integer ? (float)out->integer : out->number;
            float b = right.is_integer ? (float)right.integer : right.number;
            if (op == '+') out->number = a + b;
            else if (op == '-') out->number = a - b;
            else if (op == '*') out->number = a * b;
            else out->number = a / b;
            out->is_integer = 0;
            if (!isfinite(out->number)) return af_fail(p, token, "nonfinite numeric expression");
        }
    }
    return 1;
}

static int af_fog_kind(const AfParser *p, size_t token) {
    return af_word_ci(p, token, "setExpFog") ? COD1_AUTHOR_FOG_EXP :
           af_word_ci(p, token, "setCullFog") ? COD1_AUTHOR_FOG_CULL : 0;
}

static int af_contains_fog(const AfParser *p, size_t start, size_t end) {
    size_t i;
    for (i = start; i < end; ++i)
        if (af_fog_kind(p, i) && af_kind(p, i + 1, '(')) return 1;
    return 0;
}

static int af_contains_boundary(const AfParser *p, size_t start, size_t end) {
    size_t i;
    for (i = start; i < end; ++i) {
        if (af_word(p, i, "return") || af_word(p, i, "wait") ||
            af_word(p, i, "waittill") || af_word(p, i, "waittillmatch") ||
            af_word(p, i, "waittillframeend")) return 1;
    }
    return 0;
}

/* Opaque entity-array for loops occur before authored fog. They remain
 * projected mission work, while explicit unbounded guards must not produce
 * a profile at an unreachable later statement. No loop is executed here. */
static int af_unbounded_control(const AfParser *p, size_t token) {
    size_t i, first = AF_NONE, second = AF_NONE, start, end;
    char text[128], *after;
    double literal;
    if (af_word(p, token, "while") || af_word(p, token, "do")) return 1;
    if (!af_word(p, token, "for") || !af_kind(p, token + 1, '(')) return 0;
    end = p->tokens[token + 1].match;
    for (i = token + 2; i < end; ++i) {
        if (af_open(p->tokens[i].kind)) { i = p->tokens[i].match; continue; }
        if (af_kind(p, i, ';')) {
            if (first == AF_NONE) first = i;
            else { second = i; break; }
        }
    }
    if (first == AF_NONE || second == AF_NONE) return 1;
    start = first + 1; end = second;
    if (start == end) return 1;
    while (af_kind(p, start, '(') && p->tokens[start].match + 1 == end) { ++start; --end; }
    if (end == start + 1 && af_kind(p, start, AF_NUMBER)) {
        if (p->tokens[start].length >= sizeof(text)) return 1;
        memcpy(text, p->source + p->tokens[start].offset, p->tokens[start].length);
        text[p->tokens[start].length] = 0; errno = 0; literal = strtod(text, &after);
        return errno || *after || literal != 0.0;
    }
    if (end == start + 1 && af_word(p, start, "false")) return 0;
    /* Any entirely constant guard beyond the literal case is conservatively
     * opaque; do not add a second mission-condition evaluator to this reader. */
    for (i = start; i < end; ++i)
        if (p->tokens[i].kind == AF_ID && !af_word(p, i, "true") &&
            !af_word(p, i, "false") && !af_word(p, i, "float")) return 0;
    return 1;
}

static int af_contains_unbounded_control(const AfParser *p, size_t start, size_t end) {
    size_t i;
    for (i = start; i < end; ++i)
        if ((af_word(p, i, "while") || af_word(p, i, "do") || af_word(p, i, "for")) &&
            af_unbounded_control(p, i)) return 1;
    return 0;
}

static int af_statement(AfParser *p, size_t start, size_t end,
                        unsigned depth, AfStatement *out) {
    AfStatement child;
    size_t i, close;
    memset(out, 0, sizeof(*out)); out->start = start; out->second = AF_NONE;
    if (depth > AF_MAX_DEPTH) return af_fail(p, start, "statement nesting limit");
    if (start >= end) return af_fail(p, start, "missing statement");
    if (af_kind(p, start, '{')) {
        out->kind = AF_BLOCK; out->first = start + 1;
        out->end = p->tokens[start].match + 1;
        return out->end <= end || af_fail(p, start, "block range");
    }
    if (af_kind(p, start, AF_DEV_OPEN)) {
        out->kind = AF_OPAQUE; out->end = p->tokens[start].match + 1;
        return out->end <= end || af_fail(p, start, "developer region range");
    }
    if (af_word(p, start, "if")) {
        if (!af_kind(p, start + 1, '(')) return af_fail(p, start, "if condition required");
        close = p->tokens[start + 1].match;
        if (close >= end) return af_fail(p, start, "if condition range");
        out->kind = AF_IF; out->condition_start = start + 2;
        out->condition_end = close; out->first = close + 1;
        if (!af_statement(p, out->first, end, depth + 1, &child)) return 0;
        out->end = child.end;
        if (af_word(p, out->end, "else")) {
            out->second = out->end + 1;
            if (!af_statement(p, out->second, end, depth + 1, &child)) return 0;
            out->end = child.end;
        }
        return 1;
    }
    if (af_word(p, start, "for") || af_word(p, start, "while") || af_word(p, start, "switch")) {
        if (!af_kind(p, start + 1, '(')) return af_fail(p, start, "control condition required");
        close = p->tokens[start + 1].match;
        if (close >= end) return af_fail(p, start, "control condition range");
        if (!af_statement(p, close + 1, end, depth + 1, &child)) return 0;
        out->kind = AF_OPAQUE; out->end = child.end; return 1;
    }
    if (af_word(p, start, "do")) {
        if (!af_statement(p, start + 1, end, depth + 1, &child)) return 0;
        i = child.end;
        if (!af_word(p, i, "while") || !af_kind(p, i + 1, '('))
            return af_fail(p, i, "do while condition required");
        close = p->tokens[i + 1].match;
        if (close + 1 >= end || !af_kind(p, close + 1, ';'))
            return af_fail(p, i, "do while terminator required");
        out->kind = AF_OPAQUE; out->end = close + 2; return 1;
    }
    if (af_word(p, start, "else")) return af_fail(p, start, "orphan else");
    out->kind = AF_SIMPLE;
    for (i = start; i < end; ++i) {
        if (af_kind(p, i, ';')) { out->end = i + 1; return 1; }
        if (af_open(p->tokens[i].kind)) {
            close = p->tokens[i].match;
            if (close >= end) return af_fail(p, i, "statement delimiter range");
            i = close;
        } else if (af_close(p->tokens[i].kind)) return af_fail(p, i, "unexpected statement delimiter");
    }
    return af_fail(p, start, "statement terminator required");
}

static int af_record_writes(AfParser *p, AfContext *ctx, size_t start, size_t end) {
    size_t i;
    unsigned j;
    char name[COD1_AUTHOR_FOG_CVAR_CAP];
    for (i = start; i < end; ++i) {
        if (!af_word_ci(p, i, "setcvar") || !af_kind(p, i + 1, '(')) continue;
        if (!af_kind(p, i + 2, AF_STRING)) { ctx->unknown_write = 1; continue; }
        if (!af_string(p, i + 2, name, sizeof(name))) return 0;
        for (j = 0; j < ctx->write_count; ++j) if (af_same_name(ctx->writes[j], name)) break;
        if (j != ctx->write_count) continue;
        if (ctx->write_count == AF_MAX_WRITES) return af_fail(p, i, "cvar write tracking limit");
        memcpy(ctx->writes[ctx->write_count++], name, strlen(name) + 1);
    }
    return 1;
}

static int af_cvar_value(AfParser *p, AfContext *ctx, size_t token,
                         const char *name, const char **value) {
    unsigned i;
    if (!*name) return af_fail(p, token, "empty profile cvar name");
    if (ctx->unknown_write) return af_fail(p, token, "earlier dynamic cvar write makes profile selection ambiguous");
    for (i = 0; i < ctx->write_count; ++i)
        if (af_same_name(ctx->writes[i], name))
            return af_fail(p, token, "profile cvar was written by earlier mission statements");
    for (i = 0; i < p->cvar_count; ++i)
        if (af_same_name(p->cvars[i].name, name)) { *value = p->cvars[i].value; return 1; }
    if (p->cvar_count == AF_MAX_CVARS) return af_fail(p, token, "profile cvar snapshot limit");
    if (!p->read_cvar) return af_fail(p, token, "profile cvar reader required");
    i = p->cvar_count;
    memcpy(p->cvars[i].name, name, strlen(name) + 1);
    memset(p->cvars[i].value, 0xff, sizeof(p->cvars[i].value));
    if (!p->read_cvar(p->user, name, p->cvars[i].value, sizeof(p->cvars[i].value)) ||
        !memchr(p->cvars[i].value, 0, sizeof(p->cvars[i].value)))
        return af_fail(p, token, "profile cvar read failed or lacked a terminator");
    ++p->cvar_count; *value = p->cvars[i].value; return 1;
}

static int af_condition(AfParser *p, AfContext *ctx, size_t start,
                        size_t end, int *selected) {
    size_t i, op = AF_NONE, left_start, left_end, right_start, right_end;
    char name[COD1_AUTHOR_FOG_CVAR_CAP], expected[COD1_AUTHOR_FOG_CVAR_CAP];
    const char *value = NULL;
    while (af_kind(p, start, '(') && p->tokens[start].match + 1 == end) { ++start; --end; }
    for (i = start; i < end; ++i) {
        if (af_open(p->tokens[i].kind)) { i = p->tokens[i].match; continue; }
        if (af_kind(p, i, AF_EQ) || af_kind(p, i, AF_NE)) {
            if (op != AF_NONE) return af_fail(p, i, "unsupported fog condition");
            op = i;
        }
    }
    if (op == AF_NONE) return af_fail(p, start, "fog requires a pure cvar equality condition");
    left_start = start; left_end = op; right_start = op + 1; right_end = end;
    while (af_kind(p, left_start, '(') && p->tokens[left_start].match + 1 == left_end)
        { ++left_start; --left_end; }
    while (af_kind(p, right_start, '(') && p->tokens[right_start].match + 1 == right_end)
        { ++right_start; --right_end; }
    if (left_end != left_start + 4 || !af_word_ci(p, left_start, "getcvar") ||
        !af_kind(p, left_start + 1, '(') || !af_kind(p, left_start + 2, AF_STRING) ||
        !af_kind(p, left_start + 3, ')') || right_end != right_start + 1 ||
        !af_kind(p, right_start, AF_STRING))
        return af_fail(p, start, "fog requires getcvar(string) ==/!= string");
    if (!af_string(p, left_start + 2, name, sizeof(name)) ||
        !af_string(p, right_start, expected, sizeof(expected)) ||
        !af_cvar_value(p, ctx, left_start, name, &value)) return 0;
    *selected = !strcmp(value, expected);
    if (af_kind(p, op, AF_NE)) *selected = !*selected;
    return 1;
}

static int af_fog_call(AfParser *p, AfContext *ctx, const AfStatement *statement) {
    Cod1AuthorFogCall call;
    AfExpression expression;
    AfNumber value;
    size_t start = statement->start, close, i, argument_start, argument_end;
    unsigned argc, n;
    double duration;
    memset(&call, 0, sizeof(call)); call.kind = af_fog_kind(p, start);
    if (!af_word(p, start, "setExpFog") && !af_word(p, start, "setCullFog"))
        return af_fail(p, start, "unsupported fog builtin spelling");
    argc = call.kind == COD1_AUTHOR_FOG_EXP ? 5u : 6u;
    if (!af_kind(p, start + 1, '(')) return af_fail(p, start, "fog call argument list required");
    close = p->tokens[start + 1].match;
    if (close + 2 != statement->end || !af_kind(p, close + 1, ';'))
        return af_fail(p, start, "fog must be a direct global statement");
    argument_start = start + 2;
    for (n = 0; n < argc; ++n) {
        argument_end = close;
        for (i = argument_start; i < close; ++i) {
            if (af_open(p->tokens[i].kind)) { i = p->tokens[i].match; continue; }
            if (af_kind(p, i, ',')) { argument_end = i; break; }
        }
        if (argument_end == argument_start || argument_end - argument_start > AF_MAX_EXPR_TOKENS)
            return af_fail(p, argument_start, "fog numeric argument token limit or empty argument");
        memset(&expression, 0, sizeof(expression)); expression.parser = p;
        expression.next = argument_start; expression.end = argument_end;
        if (!af_number_expression(&expression, 0, &value)) return 0;
        if (expression.next != argument_end) return af_fail(p, expression.next, "unsupported fog argument expression");
        call.args[n] = value.is_integer ? (float)value.integer : value.number;
        if (!isfinite(call.args[n])) return af_fail(p, start, "nonfinite fog argument");
        if (n + 1 < argc && argument_end == close) return af_fail(p, start, "fog call arity mismatch");
        if (n + 1 == argc && argument_end != close) return af_fail(p, start, "fog call arity mismatch");
        argument_start = argument_end + 1;
    }
    if (call.kind == COD1_AUTHOR_FOG_EXP) {
        if (!(call.args[0] > 0.0f && call.args[0] < 1.0f))
            return af_fail(p, start, "setExpFog density must be within (0,1)");
        i = 1;
    } else {
        if (!(call.args[0] >= 0.0f && call.args[1] > call.args[0] &&
              (double)call.args[1] < 2147483648.0))
            return af_fail(p, start, "setCullFog distance range");
        i = 2;
    }
    for (n = 0; n < 3; ++n)
        if (!(call.args[i + n] >= 0.0f && call.args[i + n] <= 1.0f))
            return af_fail(p, start, "fog color outside [0,1]");
    duration = (double)call.args[argc - 1] * (double)1000.0f;
    if (!(duration >= 0.0 && duration <= INT_MAX))
        return af_fail(p, start, "fog transition outside transport range");
    if (ctx->plan.count == COD1_AUTHOR_FOG_MAX_CALLS) return af_fail(p, start, "initial fog call limit");
    call.line = p->tokens[start].line; call.column = p->tokens[start].column;
    ctx->plan.calls[ctx->plan.count++] = call;
    return 1;
}

static int af_walk_sequence(AfParser *p, AfContext *ctx, size_t start,
                            size_t end, unsigned depth);

static int af_walk_statement(AfParser *p, AfContext *ctx,
                             const AfStatement *s, unsigned depth) {
    int has_fog = af_contains_fog(p, s->start, s->end);
    if (depth > AF_MAX_DEPTH) return af_fail(p, s->start, "profile nesting limit");
    if (ctx->flow == 1) return 1;
    if (s->kind == AF_BLOCK)
        return af_walk_sequence(p, ctx, s->first, s->end - 1, depth + 1);
    if (s->kind == AF_IF && has_fog) {
        AfContext *arms;
        AfStatement child;
        int selected = 0, ok = 0;
        if (ctx->flow == 2) return af_fail(p, s->start, "earlier control flow may delay or bypass initial fog");
        if (af_contains_fog(p, s->condition_start, s->condition_end))
            return af_fail(p, s->start, "fog call in condition is unsupported");
        /* Keep bounded branch copies on the heap: a default Win32 thread
         * stack must not carry two large write-tracking arrays per level. */
        arms = (AfContext *)malloc(2 * sizeof(*arms));
        if (!arms) return af_fail(p, s->start, "profile branch allocation failed");
        arms[0] = *ctx; arms[1] = *ctx;
        /* Validate both profile arms before accepting either one. */
        if (!af_statement(p, s->first, s->end, depth + 1, &child) ||
            !af_walk_statement(p, &arms[0], &child, depth + 1)) goto branch_done;
        if (s->second != AF_NONE &&
            (!af_statement(p, s->second, s->end, depth + 1, &child) ||
             !af_walk_statement(p, &arms[1], &child, depth + 1))) goto branch_done;
        if (!af_condition(p, ctx, s->condition_start, s->condition_end, &selected)) goto branch_done;
        *ctx = arms[selected ? 0 : 1]; ok = 1;
branch_done:
        free(arms);
        return ok;
    }
    if (s->kind == AF_IF || s->kind == AF_OPAQUE) {
        if (has_fog) return af_fail(p, s->start, "fog in a loop, switch or developer region is unsupported");
        if (!af_record_writes(p, ctx, s->start, s->end)) return 0;
        if (af_contains_boundary(p, s->start, s->end) ||
            af_contains_unbounded_control(p, s->start, s->end)) ctx->flow = 2;
        return 1;
    }
    if (has_fog) {
        if (ctx->flow == 2) return af_fail(p, s->start, "earlier control flow may delay or bypass initial fog");
        if (!af_fog_kind(p, s->start)) return af_fail(p, s->start, "receiver, thread or expression fog is unsupported");
        return af_fog_call(p, ctx, s);
    }
    if (!af_record_writes(p, ctx, s->start, s->end)) return 0;
    if (af_contains_boundary(p, s->start, s->end)) ctx->flow = 1;
    return 1;
}

static int af_walk_sequence(AfParser *p, AfContext *ctx, size_t start,
                            size_t end, unsigned depth) {
    AfStatement statement;
    while (start < end) {
        if (!af_statement(p, start, end, depth, &statement) ||
            !af_walk_statement(p, ctx, &statement, depth)) return 0;
        start = statement.end;
    }
    return start == end || af_fail(p, start, "profile statement range");
}

int Cod1AuthorFogParse(const char *source, size_t size,
                      Cod1AuthorFogReadCvar read_cvar, void *user,
                      Cod1AuthorFogPlan *out, char *error, size_t error_size) {
    AfParser parser;
    AfContext context;
    size_t i, main_start = AF_NONE, main_end = AF_NONE, close;
    int ok = 0;
    if (out) memset(out, 0, sizeof(*out));
    if (error && error_size) error[0] = 0;
    memset(&parser, 0, sizeof(parser)); memset(&context, 0, sizeof(context));
    parser.source = source; parser.size = size; parser.read_cvar = read_cvar;
    parser.user = user; parser.error = error; parser.error_size = error_size;
    if (!source || !out || !size || size > COD1_AUTHOR_FOG_MAX_SOURCE || memchr(source, 0, size)) {
        af_fail(&parser, AF_NONE, "invalid source pointer, size or embedded NUL"); goto done;
    }
    if (!af_lex(&parser)) goto done;
    for (i = 0; i < parser.count; ++i) {
        if (af_kind(&parser, i, AF_DEV_OPEN)) { i = parser.tokens[i].match; continue; }
        if ((af_fog_kind(&parser, i) || af_word_ci(&parser, i, "getcvar") ||
             af_word_ci(&parser, i, "setcvar")) && af_kind(&parser, i + 1, '(') &&
            af_kind(&parser, parser.tokens[i + 1].match + 1, '{')) {
            af_fail(&parser, i, "profile builtin has a conflicting local declaration"); goto done;
        }
        if (af_word(&parser, i, "main") && af_kind(&parser, i + 1, '(')) {
            close = parser.tokens[i + 1].match;
            if (af_kind(&parser, close + 1, '{')) {
                if (i && !af_kind(&parser, i - 1, ';') && !af_kind(&parser, i - 1, '}') &&
                    !af_kind(&parser, i - 1, AF_DEV_CLOSE)) {
                    af_fail(&parser, i, "main is not an unqualified top-level declaration"); goto done;
                }
                if (main_start != AF_NONE) { af_fail(&parser, i, "duplicate main declaration"); goto done; }
                if (close != i + 2) { af_fail(&parser, i, "main must have no parameters"); goto done; }
                main_start = close + 2; main_end = parser.tokens[close + 1].match;
            }
        }
        if (af_open(parser.tokens[i].kind)) i = parser.tokens[i].match;
    }
    if (main_start == AF_NONE) { af_fail(&parser, AF_NONE, "top-level main declaration missing"); goto done; }
    if (!af_walk_sequence(&parser, &context, main_start, main_end, 0)) goto done;
    *out = context.plan; ok = 1;
done:
    free(parser.tokens);
    return ok;
}
