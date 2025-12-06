#define NOB_STRIP_PREFIX
#define NOB_IMPLEMENTATION
#include "nob.h"

#define BUILD "build/"
#define GLFW "glfw/"

bool build_test_glfw(Cmd *cmd, bool run_after_building)
{
    if (!mkdir_if_not_exists(BUILD GLFW)) return false;
    cmd_append(cmd, "gcc", "-I./tests/glfw/raylib-glfw/glfw/include", "-o", BUILD GLFW "main");
    cmd_append(cmd, "tests/glfw/raylib-glfw/rglfw.c", "tests/glfw/main.c");
    cmd_append(cmd, "-lm", "-lvulkan");
    if (!cmd_run(cmd)) return false;
    
    if (run_after_building) {
        cmd_append(cmd, BUILD GLFW "main");
        if (!cmd_run(cmd)) return false;
    }

    return true;
}

void log_usage(const char *program)
{
    nob_log(INFO, "usage: %s [flags]", program);
    nob_log(INFO, "    -h, help");
    nob_log(INFO, "    -r, run after building (defaults to glfw for now)");
    // TODO: add a -t flag when I support more than one windowing manager
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);
    Nob_Cmd cmd = {0};

    bool run_after_building = false;
    const char *program = shift(argv, argc);
    if (argc) {
        const char *flag = shift(argv, argc);
        if (strcmp(flag, "-r") ==  0) run_after_building = true;
        if (strcmp(flag, "-h") ==  0) {
            log_usage(program);
            return 0;
        }
    }

    if (!mkdir_if_not_exists(BUILD)) return 1;
    if (!build_test_glfw(&cmd, run_after_building)) return 1;

    return 0;
}
