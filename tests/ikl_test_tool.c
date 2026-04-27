/* iklang test runner
 *
 * compile:  gcc -o ikl_test_tool ikl_test_tool.c
 * run:      ./ikl_test_tool          (from tests/ directory)
 *
 * For every *.ikl file in the same directory as this binary, the tool:
 *   1. Compiles it with ../build/iklc
 *   2. Runs the resulting binary and captures its stdout
 *   3. Compares the output to the companion *.expected file
 *   4. Reports PASS / FAIL / SKIP (no .expected file) per test
 */

#define _GNU_SOURCE
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_PATH 4096
#define MAX_OUTPUT (64 * 1024)

static char g_compiler[MAX_PATH];
static char g_tests_dir[MAX_PATH];
static int g_pass, g_fail, g_skip;

#define CLR_RED "\033[0;31m"
#define CLR_GREEN "\033[0;32m"
#define CLR_YELLOW "\033[1;33m"
#define CLR_RESET "\033[0m"

/* Fork, optionally chdir to workdir, exec argv, capture stdout (and stderr
   when capture_stderr != 0) into buf[0..cap-1].  Returns exit code or -1. */
static int run_capture(const char *workdir, char *const argv[], char *buf,
                       size_t cap, int capture_stderr) {
    int fds[2];
    if (pipe(fds))
        return -1;

    pid_t pid = fork();
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        return -1;
    }

    if (pid == 0) {
        if (workdir && chdir(workdir) != 0)
            _exit(126);
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        if (capture_stderr) {
            dup2(fds[1], STDERR_FILENO);
        } else {
            int nul = open("/dev/null", O_WRONLY);
            if (nul >= 0) {
                dup2(nul, STDERR_FILENO);
                close(nul);
            }
        }
        close(fds[1]);
        execvp(argv[0], argv);
        _exit(127);
    }

    close(fds[1]);
    size_t n = 0;
    ssize_t r;
    while (n < cap - 1 && (r = read(fds[0], buf + n, cap - 1 - n)) > 0)
        n += r;
    buf[n] = '\0';
    close(fds[0]);

    int st;
    waitpid(pid, &st, 0);
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

static char *slurp(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    rewind(f);
    char *b = malloc((size_t)len + 1);
    if (!b) {
        fclose(f);
        return NULL;
    }
    fread(b, 1, (size_t)len, f);
    b[len] = '\0';
    fclose(f);
    return b;
}

static void rm_rf(const char *path) {
    char cmd[MAX_PATH + 8];
    snprintf(cmd, sizeof cmd, "rm -rf %s", path);
    (void)system(cmd);
}

static void run_test(const char *ikl_path) {
    /* test name: basename without extension */
    const char *base = strrchr(ikl_path, '/');
    base = base ? base + 1 : ikl_path;
    char name[256];
    strncpy(name, base, sizeof name - 1);
    name[sizeof name - 1] = '\0';
    char *dot = strrchr(name, '.');
    if (dot)
        *dot = '\0';

    /* companion .expected file */
    char exp_path[MAX_PATH];
    strncpy(exp_path, ikl_path, MAX_PATH - 1);
    exp_path[MAX_PATH - 1] = '\0';
    char *edot = strrchr(exp_path, '.');
    if (edot)
        strcpy(edot, ".expected");

    if (access(exp_path, R_OK) != 0) {
        printf(CLR_YELLOW "SKIP" CLR_RESET "  %s\n", name);
        g_skip++;
        return;
    }

    /* isolated temp dir for compiler artifacts */
    char tmpdir[] = "/tmp/iklang_XXXXXX";
    if (!mkdtemp(tmpdir)) {
        g_fail++;
        return;
    }

    /* compile */
    char cbuf[MAX_OUTPUT];
    char *cargv[] = {g_compiler, (char *)ikl_path, NULL};
    int cret = run_capture(tmpdir, cargv, cbuf, sizeof cbuf, 1);

    char bin[MAX_PATH];
    snprintf(bin, sizeof bin, "%s/out", tmpdir);

    if (cret != 0 || access(bin, X_OK) != 0) {
        printf(CLR_RED "FAIL" CLR_RESET "  %-24s (compile error)\n", name);
        char *nl = strchr(cbuf, '\n');
        if (nl)
            *nl = '\0';
        if (cbuf[0])
            printf("         %s\n", cbuf);
        rm_rf(tmpdir);
        g_fail++;
        return;
    }

    /* run, capture stdout only */
    char actual[MAX_OUTPUT];
    char *rargv[] = {bin, NULL};
    run_capture(NULL, rargv, actual, sizeof actual, 0);

    rm_rf(tmpdir);

    /* compare against expected */
    char *expected = slurp(exp_path);
    if (!expected) {
        printf(CLR_RED "FAIL" CLR_RESET "  %s (cannot read .expected)\n", name);
        g_fail++;
        return;
    }

    if (strcmp(actual, expected) == 0) {
        printf(CLR_GREEN "PASS" CLR_RESET "  %s\n", name);
        g_pass++;
    } else {
        printf(CLR_RED "FAIL" CLR_RESET "  %s\n", name);
        /* report the first mismatching line */
        const char *ep = expected, *ap = actual;
        for (int line = 1; *ep || *ap; line++) {
            const char *enl = strchr(ep, '\n'), *anl = strchr(ap, '\n');
            int elen = enl ? (int)(enl - ep) : (int)strlen(ep);
            int alen = anl ? (int)(anl - ap) : (int)strlen(ap);
            if (elen != alen || memcmp(ep, ap, (size_t)elen) != 0) {
                printf("         line %d — expected \"%.*s\"  got \"%.*s\"\n",
                       line, elen, ep, alen, ap);
                break;
            }
            ep += elen + (enl ? 1 : 0);
            ap += alen + (anl ? 1 : 0);
        }
        g_fail++;
    }
    free(expected);
}

static int by_name(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

int main(void) {
    /* resolve binary location via /proc/self/exe to find tests/ and compiler */
    char self[MAX_PATH];
    ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
    if (n < 0) {
        perror("readlink");
        return 1;
    }
    self[n] = '\0';
    char *slash = strrchr(self, '/');
    if (slash)
        *slash = '\0';
    /* self is now the directory containing this binary (tests/) */
    strncpy(g_tests_dir, self, MAX_PATH - 1);
    snprintf(g_compiler, sizeof g_compiler, "%s/../build/iklc", self);

    if (access(g_compiler, X_OK) != 0) {
        fprintf(stderr, "error: compiler not found: %s\n", g_compiler);
        fprintf(stderr, "hint:  build the project first (e.g. make)\n");
        return 1;
    }

    /* collect all *.ikl files in tests/ */
    DIR *d = opendir(g_tests_dir);
    if (!d) {
        perror("opendir");
        return 1;
    }

    char *files[1024];
    int nf = 0;
    struct dirent *ent;
    while ((ent = readdir(d)) && nf < 1024) {
        size_t nl = strlen(ent->d_name);
        if (nl > 4 && strcmp(ent->d_name + nl - 4, ".ikl") == 0) {
            char path[MAX_PATH];
            snprintf(path, sizeof path, "%s/%s", g_tests_dir, ent->d_name);
            files[nf++] = strdup(path);
        }
    }
    closedir(d);
    qsort(files, (size_t)nf, sizeof *files, by_name);

    if (nf == 0) {
        fprintf(stderr, "no .ikl test files found in %s\n", g_tests_dir);
        return 1;
    }

    for (int i = 0; i < nf; i++) {
        run_test(files[i]);
        free(files[i]);
    }

    printf("\nResults: %d passed, %d failed", g_pass, g_fail);
    if (g_skip)
        printf(", %d skipped", g_skip);
    printf("\n");

    return g_fail > 0 ? 1 : 0;
}
