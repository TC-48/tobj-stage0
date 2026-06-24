#include <argparse/argparse.h>
#include <toolcommon/output.h>

#include <tobj/merge.h>
#include <tc48/util.h>
#include <tc48/mem.h>

#include <stdlib.h>
#include <stdarg.h>

static const char *const usages[] = {
    "tobj-merge [options...] input.obj.t48b [input2.obj.t48b ...]",
    NULL,
};

int main(int argc, const char* argv[]) {
    const char* output_path = "merged.t48b";
    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('o', "output", &output_path, "path to the output merged object", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usages, 0);
    argparse_describe(&argparse,
        "merge one or more tobj object files into a single ternary object",
        "github: https://github.com/TC-48/tobj-stage0/tree/main"
    );
    argc = argparse_parse(&argparse, argc, argv);

    if (argc < 1) {
        argparse_usage(&argparse);
        return 2;
    }

    tc48_memory** objects = malloc(sizeof(tc48_memory*) * argc);
    tobj_param* params = malloc(sizeof(tobj_param) * argc);
    if (!objects || !params) {
        tool_show_error("out of memory");
        free(objects);
        free(params);
        return 1;
    }

    for (int i = 0; i < argc; ++i) {
        const char* input_path = argparse.out[i];
        objects[i] = tc48_load_t48b(input_path);
        if (!objects[i]) {
            tool_show_error("failed to load object file: %s", input_path);
            for (int j = 0; j < i; ++j) {
                tc48_mem_free(objects[j]);
            }
            free(objects);
            free(params);
            return 1;
        }
        params[i] = (tobj_param){ .data = objects[i] };
    }

    tc48_memory* merged = tobj_merge(params, (tc48_half)argc);
    if (!merged) {
        tool_show_error("failed to merge object files");
        for (int i = 0; i < argc; ++i) {
            tc48_mem_free(objects[i]);
        }
        free(objects);
        free(params);
        return 1;
    }

    tc48_mem_save(merged, output_path);
    tool_show_info("successfully merged to %s", output_path);

    tc48_mem_free(merged);
    for (int i = 0; i < argc; ++i) {
        tc48_mem_free(objects[i]);
    }
    free(objects);
    free(params);
    return 0;
}
