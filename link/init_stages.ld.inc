
#define __LINKER__
#include "../include/nautilus/init_stages.h"

.init_stages ALIGN(0x1000) : {

#define X(STAGE) \
  __start_init_stage_ ## STAGE = .;\
  KEEP(*(SORT_BY_NAME(.init.STAGE.*)));\
  __stop_init_stage_ ## STAGE = .;

NK_INIT_STAGES_LIST

#undef X

}
