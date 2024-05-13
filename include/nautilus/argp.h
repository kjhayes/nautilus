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

#ifndef __NAUTILUS_ARGP_H__
#define __NAUTILUS_ARGP_H__

/*
 * Simple argument parsing tools for shell commands
 */

struct nk_args {
    int argc;
    char * argv[0];
};

// Takes a shell command "buf" and parses it into "argc" and "argv"
// Does not depend on "buf" anymore on return.
// returns NULL if an error occurred
struct nk_args * nk_argp_create_args(char *buf);
void nk_argp_destroy_args(struct nk_args *args);

// Returns:
//      - The argv index of the first argument that matches "c"
//      - -ENXIO if no argument matches 
// Ex. if c = 'r', then '-r', '-gr', '-rx' all match
int nk_argp_find_char_arg(struct nk_args *args, char c);

// Returns:
//      - The argv index of the first argument that matches "str"
//      - -ENXIO if no argument matches
// Ex. if str = "verbose", "--verbose" will match
int nk_argp_find_str_arg(struct nk_args *args, const char *str);

#endif
