#pragma once

#include <tobj/format.h>

#include <tc48/word.h>
#include <tc48/mem.h>

#include <stdbool.h>

typedef struct tobj_param {
    const tc48_memory* data;
    const tc48_word    off;
    const tobj_header* header; // nullable
} tobj_param;

bool tobj_param_header(tobj_param param, tobj_header* header);
