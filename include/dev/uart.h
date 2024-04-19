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
#ifndef __UART_H__
#define __UART_H__

#define UART_PARITY_NONE  0
#define UART_PARITY_ODD   1
#define UART_PARITY_EVEN  2
#define UART_PARITY_MARK  3
#define UART_PARITY_SPACE 4

struct uart_port;

struct uart_ops 
{
  int(*set_baud_rate)(struct uart_port *, unsigned int baud_rate);
  int(*set_word_length)(struct uart_port *, unsigned int bits);
  int(*set_parity)(struct uart_port *, unsigned int parity);
  int(*set_stop_bits)(struct uart_port *, unsigned int bits);
};

struct uart_port 
{
  unsigned int baud_rate;
  unsigned int word_length;
  unsigned int parity;
  unsigned int stop_bits;

  struct uart_ops ops;
};

static inline
int uart_port_set_baud_rate(struct uart_port *port, unsigned int baud_rate)
{
  return port->ops.set_baud_rate(port, baud_rate);
}
static inline
int uart_port_set_word_length(struct uart_port *port, unsigned int bits) 
{
  return port->ops.set_word_length(port, bits);
}
static inline
int uart_port_set_parity(struct uart_port *port, unsigned int parity) 
{
  return port->ops.set_parity(port, parity);
}
static inline
int uart_port_set_stop_bits(struct uart_port *port, unsigned int bits) 
{
  return port->ops.set_stop_bits(port, bits);
}

#endif
