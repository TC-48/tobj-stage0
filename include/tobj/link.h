#pragma once

#include <tobj/format.h>
#include <tobj/param.h>

#include <tc48/mem.h>

typedef enum tobj_link_code {
    TOBJ_LINK_SUCCESS,
    TOBJ_LINK_BAD_MAGIC,
    TOBJ_LINK_UNDEF_REF,
    TOBJ_LINK_BAD_HEADER,
    TOBJ_LINK_OUT_OF_MEM,
    TOBJ_LINK_BAD_SEC_IDX,
    TOBJ_LINK_BAD_SYM_IDX,
    TOBJ_LINK_BAD_OFF,
} tobj_link_code;

typedef struct tobj_link_result {
    tobj_link_code code;
    union {
        tc48_half magic;
        tc48_half sym;
    } details;
} tobj_link_result;

typedef int tobj_printf_like(const char* fmt, ...);
void tobj_print_link_error(tobj_link_result res, tobj_printf_like* fn);

tobj_link_result tobj_to_raw_exe(tobj_param obj, tc48_memory** out);
