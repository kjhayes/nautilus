/*
 * This file is part of the Nautilus AeroKernel developed
 * by the Constellation, Interweaving, Hobbes, and V3VEE
 * Projects with funding from the United States National
 * Science Foundation and the Department of Energy.
 *
 * The V3VEE Project is a joint project between Northwestern University
 * and the University of New Mexico.  The Hobbes Project is a collaboration
 * led by Sandia National Laboratories that includes several national
 * laboratories and universities. The Interweaving Project is a
 * joint project between Northwestern University and Illinois Institute
 * of Technology.   The Constellation Project is a joint project
 * between Northwestern University, Carnegie Mellon University,
 * and Illinois Institute of Technology.
 * You can find out more at:
 * http://www.v3vee.org
 * http://xstack.sandia.gov/hobbes
 * http://interweaving.org
 * http://constellation-project.net
 *
 * Copyright (c) 2023, Kevin Hayes <kjhayes@u.northwestern.edu>
 * Copyright (c) 2023, The V3VEE Project  <http://www.v3vee.org>
 *                     The Hobbes Project <http://xstack.sandia.gov/hobbes>
 * All rights reserved.
 *
 * Authors: Kevin Hayes <kjhayes@u.northwestern.edu>
 *
 * This is free software.  You are permitted to use,
 * redistribute, and modify it as specified in the file "LICENSE.txt".
 */

#include <nautilus/nautilus.h>
#include <nautilus/errno.h>
#include <nautilus/argp.h>

static inline int check_whitespace(char c) {
  switch(c) {
      case '\n':
      case '\t':
      case '\r':
      case ' ':
          return 1;
          break;
      default:
          return 0;
  }
}

struct nk_args *
nk_argp_create_args(char *buf) 
{
    int did_malloc = 0;
    int i;
    int is_whitespace;
    size_t size;
    struct nk_args *args;

    int argc = 0;
    int prev_whitespace = 1;
    char *iter = buf;

    char *start, *end;

    while(*iter != '\0') {
        is_whitespace = check_whitespace(*iter);
        if(prev_whitespace && !is_whitespace) {
            argc++;
        }
        prev_whitespace = is_whitespace;
        iter++;
    }

    size = sizeof(struct nk_args) + (argc * sizeof(char*));
    args = malloc(size);
    memset(args,0,size);
    args->argc = argc;
    did_malloc = 1;

    iter = buf;
    for(i = 0; i < argc; i++) {
        while(*iter != '\0' && check_whitespace(*iter)) {
            iter++;
        }
        if(*iter == '\0') {
            ERROR_PRINT("nk_argp_create_args: end of buffer encountered unexpectedly (parsed (%d) args but argc=(%d)!\n", i, argc);
            goto err_exit;
        }
        start = iter;
        while(*iter != '\0' && !check_whitespace(*iter)) {
            iter++;
        }
        end = iter;
        size = (void*)end - (void*)start;
        args->argv[i] = malloc(size+1);
        args->argv[i][size] = '\0';
        memcpy(args->argv[i],start,size);
    }

    return args;

err_exit:
    if(did_malloc) {
      for(i = 0; i < args->argc; i++) {
          if(args->argv[i] != NULL) {
              free(args->argv[i]);
          }
      }
      free(args);
    }
    return NULL;
}

void
nk_argp_destroy_args(struct nk_args *args) {
    for(int i = 0; i < args->argc; i++) {
        free((void*)(args->argv[i]));
    }
    free(args);
}

static int check_char_arg(const char *arg, char c) {
    if(arg[0] == '\0') {
        return 0;
    }
    if(arg[0] == '-') {
        if(arg[1] == '-') {
            // This is a str arg not a char arg
            return 0;
        }
        do {
            arg++;
            if(*arg == c) {
                return 1;
            }
        } while(*arg != '\0');
    }
    return 0;
}

int nk_argp_find_char_arg(struct nk_args *args, char c) {
    for(int i = 0; i < args->argc; i++) {
        if(check_char_arg(args->argv[i],c)) {
            return i;
        }
    }
    return -ENXIO;
}

static int check_str_arg(const char *arg, const char *str) {
    if(arg[0] != '-' || arg[1] != '-') {
        return 0;
    }
    if(strcmp(arg+2, str) == 0) {
        return 1;
    }
    return 0;
}

int nk_argp_find_str_arg(struct nk_args *args, const char *str) {
    for(int i = 0; i < args->argc; i++) {
        if(check_str_arg(args->argv[i],str)) {
            return i;
        }
    }
    return -ENXIO;
}

