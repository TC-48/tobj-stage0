// NOTE: this is a simplified version for testing
// TODO: support multiple inputs (once merge has been implemented)

#include <argparse/argparse.h>
#include <toolcommon/output.h>

#include <tobj/link.h>
#include <tc48/util.h>
#include <tc48/mem.h>

#include <stdlib.h>
#include <stdarg.h>

static const char *const usages[] = {
    "tobj-link [options...] input.t48b",
    NULL,
};

int tool_show_error_as_fn(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    tool_output_valist(TOOL_OUTPUT_ERROR, fmt, ap);
    va_end(ap);
    return 0; // this is ignored anyway
}

int main(int argc, const char* argv[]) {
    const char* output_path = "raw.t48b";
    struct argparse_option options[] = {
        OPT_HELP(),
        OPT_STRING('o', "output", &output_path, "path to the output executable", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usages, 0);
    argparse_describe(&argparse,
        "link a tobj object file into a raw TC-48 executable",
        "github: https://github.com/TC-48/tobj-stage0/tree/main"
    );
    argc = argparse_parse(&argparse, argc, argv);

    if (argc != 1) {
        argparse_usage(&argparse);
        return 2;
    }

    const char* input_path = argparse.out[0];
    tc48_memory* obj = tc48_load_t48b(input_path);
    if (!obj) {
        tool_show_error("failed to load object file: %s", input_path);
        return 1;
    }

    tc48_memory* exe = NULL;
    tobj_link_result res = tobj_to_raw_exe(obj, NULL, &exe);

    if (res.code != TOBJ_LINK_SUCCESS) {
        tobj_print_link_error(res, &tool_show_error_as_fn);
        tc48_mem_free(obj);
        return 1;
    }

    tc48_mem_save(exe, output_path);
    tool_show_info("successfully linked %s to %s", input_path, output_path);

    tc48_mem_free(exe);
    tc48_mem_free(obj);
    return 0;
}
