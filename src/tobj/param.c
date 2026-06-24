#include <tobj/param.h>
#include <tobj/parse.h>

bool tobj_param_header(tobj_param param, tobj_header* header) {
    if (param.header != NULL) {
        *header = *param.header;
        return true;
    }

    return tobj_parse_header(param.data, param.off, header);
}
