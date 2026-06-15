#define SOKOL_IMPL
#define SOKOL_DUMMY_BACKEND

#include <sokol_gfx.h>

int main(void)
{
    sg_desc desc = {0};
    sg_setup(&desc);

    if (!sg_isvalid()) {
        return 1;
    }

    if (sg_query_backend() != SG_BACKEND_DUMMY) {
        sg_shutdown();
        return 2;
    }

    sg_shutdown();

    if (sg_isvalid()) {
        return 3;
    }

    return 0;
}
