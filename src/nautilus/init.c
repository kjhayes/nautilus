
#include <nautilus/init.h>
#include <nautilus/nautilus.h>
#include <nautilus/errno.h>

#ifndef NAUT_CONFIG_DEBUG_INIT
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...) 
#endif

#define INFO(fmt, args...)  INFO_PRINT("init: " fmt, ##args)
#define DEBUG(fmt, args...) DEBUG_PRINT("init: " fmt, ##args)
#define WARN(fmt, args...)  WARN_PRINT("init: " fmt, ##args)
#define ERROR(fmt, args...) ERROR_PRINT("init: " fmt, ##args)

static int
nk_handle_init_stage_section(
        int(*start[])(void),
        int(*end[])(void),
        const char *stage) 
{
    size_t sizeof_section = (void*)end - (void*)start;
    size_t num_functions = sizeof_section / sizeof(int(*)(void));

    DEBUG_PRINT("handling stage: \"%s\"...\n", stage);
    DEBUG_PRINT("section size in bytes: %llu bytes\n", sizeof_section);
    DEBUG_PRINT("section function count: %llu functions\n", num_functions);

    int total_num_failed = 0;
    int total_num_success = 0;
    int total_num_defer = 0;

    int num_failed, num_success, num_defer;

    do {
      num_failed = 0;
      num_success = 0;
      num_defer = 0;

      for(size_t i = 0; i < num_functions; i++) {
          int(*init_func)(void) = start[i];
          if(init_func == NULL) {
              // We already handled this one (either success or failure)
              continue;
          }

          DEBUG_PRINT("Calling function at address: %p\n", (void*)init_func);
          int res = init_func();

          switch(res) {
              case -EINIT_DEFER:
                  // We'll try again later
                  DEBUG_PRINT("init function deferred!\n");
                  num_defer++;
                  continue;
              case 0:
              case -ENXIO: // init function for subsystem which just doesn't exist
                  num_success++;
                  break;
              default:
                  DEBUG_PRINT("init function failed!\n");
                  num_failed++;
                  break;
          }

          start[i] = NULL;
      }

      total_num_failed += num_failed;
      total_num_success += num_success;
      total_num_defer += num_defer;

      // Keep looping while
      //    - At least 1 function deferred
      //    - At least 1 function succeeded or failed (not everyone deferred)
    } while(num_defer > 0 && (num_success + num_failed) > 0);

    if(num_defer > 0) {
        // We ended up with every function left trying to defer
        // (This is a failure scenario)
        total_num_failed += num_defer;
    }

    // Every function either failed or succeeded
    ASSERT(total_num_failed + total_num_success == num_functions);

    DEBUG_PRINT("finished handling stage: \"%s\" (total_failed = %lu, total_success = %lu, total_deferrals = %lu)\n", 
            stage, total_num_failed, total_num_success, total_num_defer);

    return total_num_failed;
}

#define X(STAGE)\
int nk_handle_init_stage_##STAGE(void) {\
    extern int(*__start_init_stage_##STAGE[])(void);\
    extern int(*__stop_init_stage_##STAGE[])(void);\
    return nk_handle_init_stage_section(\
            __start_init_stage_##STAGE,\
            __stop_init_stage_##STAGE,\
            #STAGE);\
}
NK_INIT_STAGES_LIST
#undef X

// Example Init Function / Nautilus Welcome Message

#define NAUT_WELCOME                                      \
  "Welcome to                                         \n" \
  "    _   __               __   _  __                \n" \
  "   / | / /____ _ __  __ / /_ (_)/ /__  __ _____    \n" \
  "  /  |/ // __ `// / / // __// // // / / // ___/    \n" \
  " / /|  // /_/ // /_/ // /_ / // // /_/ /(__  )     \n" \
  "/_/ |_/ \\__,_/ \\__,_/ \\__//_//_/ \\__,_//____/  \n" \
  "+===============================================+  \n" \
  " Kyle C. Hale (c) 2014 | Northwestern University   \n" \
  "+===============================================+  \n\n"

static int 
nautilus_welcome_init(void) {
  printk(NAUT_WELCOME);
  return 0;
}

// Must run after "silent" stage
nk_declare_static_init(nautilus_welcome_init);

