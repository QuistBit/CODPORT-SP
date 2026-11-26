"""Exercise the real renderer/engine print path with long driver diagnostics.

Compiles function bodies extracted from the current sources, with in-memory
console/log sinks. This does not initialize OpenGL or emulate DroidDeck.
Run with --sanitize for AddressSanitizer and UndefinedBehaviorSanitizer.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


def function(source, signature):
    definition = re.search(r"(?m)^" + re.escape(signature) + r"[^;{\n]*\{", source)
    if not definition:
        raise ValueError("Missing function definition: " + signature)
    start = definition.start()
    body = definition.end() - 1
    tokens = re.finditer(
        r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[{}]',
        source[body:], re.DOTALL,
    )
    depth = 0
    for token in tokens:
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if depth == 0:
                return source[start:body + token.end()]
    raise ValueError("Incomplete function: " + signature)


FIXTURE = r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define QDECL
#define MAXPRINTMSG @PRINT_SIZE@
#define MAX_STRING_CHARS @STRING_SIZE@
#define qfalse 0
#define qtrue 1
#define S_COLOR_YELLOW "^3"
#define S_COLOR_RED "^1"
typedef int qboolean;
enum { PRINT_ALL, PRINT_WARNING, PRINT_DEVELOPER };
typedef struct { int integer; } cvar_t;
static cvar_t dedicated = {0}, developer = {1}, logging = {2};
static cvar_t *com_dedicated = &dedicated;
static cvar_t *com_developer = &developer;
static cvar_t *com_logfile = &logging;
static char *rd_buffer;
static int rd_buffersize;
static void (*rd_flush)(const char *);
static int logfile = 1;
static char console_output[65536], system_output[65536], log_output[65536];
static size_t largest_message;
static int checks;
static struct {
    char vendor_string[MAX_STRING_CHARS];
    char renderer_string[MAX_STRING_CHARS];
    char version_string[MAX_STRING_CHARS];
    char extensions_string[4 * MAX_STRING_CHARS];
    int maxTextureSize, maxActiveTextures;
} glConfig;
static struct { void (*Printf)(int, const char *, ...); } ri;

static void append(char *sink, const char *message) {
    size_t length = strlen(message);
    assert(length < MAXPRINTMSG);
    assert(strlen(sink) + length < 65536);
    if (length > largest_message) largest_message = length;
    strcat(sink, message);
}
static void CL_ConsolePrint(const char *text) { append(console_output, text); }
static void Sys_Print(const char *text) { append(system_output, text); }
static void Com_WriteLog(const char *text) { append(log_output, text); }
static int FS_Initialized(void) { return 1; }
static int FS_FOpenFileWrite(const char *path) { (void)path; return 1; }
static void Q_strcat(char *dest, int capacity, const char *text) {
    assert(strlen(dest) + strlen(text) < (size_t)capacity);
    strcat(dest, text);
}
static void reset_sinks(void) {
    console_output[0] = system_output[0] = log_output[0] = 0;
    largest_message = 0;
}
static void expect_output(const char *expected) {
    assert(!strcmp(console_output, expected));
    assert(!strcmp(system_output, expected));
    assert(!strcmp(log_output, expected));
    assert(largest_message < MAXPRINTMSG);
    ++checks;
}
'''


TESTS = r'''
int main(void) {
    const size_t lengths[] = {0, 1, 1023, 1024, 4079, 4080, 4095};
    const char pattern[] = "GL_ARB_example GL_EXT_another ";
    char expected[16384], original[sizeof(glConfig.extensions_string)];
    char long_message[3 * MAXPRINTMSG], truncated[MAXPRINTMSG];
    size_t test, index;
    ri.Printf = CL_RefPrintf;
    strcpy(glConfig.vendor_string, "Mesa");
    strcpy(glConfig.renderer_string, "zink (test device)");
    strcpy(glConfig.version_string, "4.6 Compatibility Profile");
    glConfig.maxTextureSize = 16384;
    glConfig.maxActiveTextures = 8;
    for (test = 0; test < sizeof(lengths) / sizeof(lengths[0]); ++test) {
        size_t length = lengths[test];
        assert(length < sizeof(glConfig.extensions_string));
        for (index = 0; index < length; ++index)
            glConfig.extensions_string[index] = pattern[index % (sizeof(pattern) - 1)];
        glConfig.extensions_string[length] = 0;
        strcpy(original, glConfig.extensions_string);
        snprintf(expected, sizeof(expected),
                 "\nGL_VENDOR: %s\nGL_RENDERER: %s\nGL_VERSION: %s\n"
                 "GL_EXTENSIONS: %s\nGL_MAX_TEXTURE_SIZE: %d\n"
                 "GL_MAX_ACTIVE_TEXTURES_ARB: %d\n",
                 glConfig.vendor_string, glConfig.renderer_string,
                 glConfig.version_string, glConfig.extensions_string,
                 glConfig.maxTextureSize, glConfig.maxActiveTextures);
        reset_sinks();
        print_driver_diagnostics();
        expect_output(expected);
        assert(!strcmp(original, glConfig.extensions_string));
    }

    memset(long_message, 'x', sizeof(long_message) - 1);
    long_message[sizeof(long_message) - 1] = 0;
    memset(truncated, 'x', sizeof(truncated) - 1);
    truncated[sizeof(truncated) - 1] = 0;
    reset_sinks();
    Com_Printf("%s", long_message);
    expect_output(truncated);
    reset_sinks();
    Com_DPrintf("%s", long_message);
    expect_output(truncated);
    reset_sinks();
    CL_RefPrintf(PRINT_ALL, "%s", long_message);
    expect_output(truncated);
    reset_sinks();
    truncated[0] = '^'; truncated[1] = '3';
    CL_RefPrintf(PRINT_WARNING, "%s", long_message);
    expect_output(truncated);
    reset_sinks();
    truncated[1] = '1';
    CL_RefPrintf(PRINT_DEVELOPER, "%s", long_message);
    expect_output(truncated);
    reset_sinks();
    developer.integer = 0;
    CL_RefPrintf(PRINT_DEVELOPER, "%s", long_message);
    expect_output("");
    developer.integer = 1;
    reset_sinks();
    CL_RefPrintf(PRINT_ALL, "value=%d literal=%s\n", 7, "%s%p");
    expect_output("value=7 literal=%s%p\n");
    reset_sinks();
    R_PrintLongString("literal %s%p\n");
    expect_output("literal %s%p\n");
    printf("PASS: %d startup logging checks; complete driver text, bounded sinks, "
           "unchanged capability string\n", checks);
    return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    common = (root / "src/qcommon/common.c").read_text()
    client = (root / "src/client/cl_main.c").read_text()
    renderer = (root / "src/renderer/tr_init.c").read_text()
    shared = (root / "src/game/q_shared.h").read_text()
    types = (root / "src/cgame/tr_types.h").read_text()
    print_size = re.search(r"#define MAXPRINTMSG\s+(\d+)", common).group(1)
    assert re.search(r"#define MAXPRINTMSG\s+(\d+)", client).group(1) == print_size
    string_size = re.search(r"#define MAX_STRING_CHARS\s+(\d+)", shared).group(1)
    assert re.search(r"extensions_string\[4\s*\*\s*MAX_STRING_CHARS\]", types)

    # Compile the actual startup statements, not an independently rewritten
    # approximation. Their following texture-limit line must also be reached.
    gfx = function(renderer, "void GfxInfo_f( void )")
    start = gfx.index('\tri.Printf( PRINT_ALL, "\\nGL_VENDOR:')
    last = gfx.index('\tri.Printf( PRINT_ALL, "GL_MAX_ACTIVE_TEXTURES_ARB:')
    end = gfx.index("\n", last)
    diagnostics = "void print_driver_diagnostics(void) {\n" + gfx[start:end] + "\n}\n"
    source = FIXTURE.replace("@PRINT_SIZE@", print_size).replace("@STRING_SIZE@", string_size)
    for text, signature in (
        (common, "void QDECL Com_Printf("),
        (common, "void QDECL Com_DPrintf("),
        (client, "void QDECL CL_RefPrintf("),
        (renderer, "static void R_PrintLongString("),
    ):
        source += "\n" + function(text, signature) + "\n"
    source += diagnostics + TESTS
    flags = ["-std=c99", "-O1" if args.sanitize else "-O2", "-g", "-Wall", "-Wextra"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    with tempfile.TemporaryDirectory(prefix="codport-startup-log-") as temporary:
        src = Path(temporary) / "startup_logging.c"
        exe = Path(temporary) / "startup_logging"
        src.write_text(source)
        subprocess.run([os.environ.get("CC", "cc"), *flags, str(src), "-o", str(exe)], check=True)
        env = os.environ.copy()
        if args.sanitize:
            env.setdefault("ASAN_OPTIONS", "detect_leaks=1:halt_on_error=1")
            env.setdefault("UBSAN_OPTIONS", "halt_on_error=1:print_stacktrace=1")
        subprocess.run([str(exe)], check=True, env=env)


if __name__ == "__main__":
    main()
