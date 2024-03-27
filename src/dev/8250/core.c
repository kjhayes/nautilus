
#include<dev/8250/core.h>
#include<nautilus/nautilus.h>
#include<nautilus/interrupt.h>
#include<nautilus/irqdev.h>
#include<nautilus/chardev.h>

#ifdef NAUT_CONFIG_HAS_PORT_IO
#include<nautilus/arch.h>
#endif

#ifndef NAUT_CONFIG_DEBUG_GENERIC_8250_UART
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif

#define INFO(fmt, args...) INFO_PRINT("[8250] " fmt, ##args)
#define DEBUG(fmt, args...) DEBUG_PRINT("[8250] " fmt, ##args)
#define ERROR(fmt, args...) ERROR_PRINT("[8250] " fmt, ##args)
#define WARN(fmt, args...) WARN_PRINT("[8250] " fmt, ##args)

int uart_8250_struct_init(struct uart_8250_port *port) 
{
  spinlock_init(&port->input_lock);
  spinlock_init(&port->output_lock);

  port->input_buf_head = 0;
  port->input_buf_tail= 0;
  port->output_buf_head = 0;
  port->output_buf_tail = 0;

  // Assign default ops where no ops were assigned
  if(port->ops.read_reg == NULL) {
    port->ops.read_reg = uart_8250_default_uart_8250_ops.read_reg;
  }
  if(port->ops.write_reg == NULL) {
    port->ops.write_reg = uart_8250_default_uart_8250_ops.write_reg;
  }
  if(port->ops.xmit_full == NULL) {
    port->ops.xmit_full = uart_8250_default_uart_8250_ops.xmit_full;
  }
  if(port->ops.recv_empty == NULL) {
    port->ops.recv_empty = uart_8250_default_uart_8250_ops.recv_empty;
  }
  if(port->ops.xmit_push == NULL) {
    port->ops.xmit_push = uart_8250_default_uart_8250_ops.xmit_push;
  }
  if(port->ops.recv_pull == NULL) {
    port->ops.recv_pull = uart_8250_default_uart_8250_ops.recv_pull;
  }
  if(port->ops.handle_irq == NULL) {
    port->ops.handle_irq = uart_8250_default_uart_8250_ops.handle_irq;
  }

  if(port->port.ops.set_baud_rate == NULL) {
    port->port.ops.set_baud_rate = uart_8250_default_uart_ops.set_baud_rate;
  }
  if(port->port.ops.set_word_length == NULL) {
    port->port.ops.set_word_length = uart_8250_default_uart_ops.set_word_length;
  }
  if(port->port.ops.set_parity == NULL) {
    port->port.ops.set_parity = uart_8250_default_uart_ops.set_parity;
  }
  if(port->port.ops.set_stop_bits == NULL) {
    port->port.ops.set_stop_bits = uart_8250_default_uart_ops.set_stop_bits;
  }

  return 0;
}

int uart_8250_input_buffer_empty(struct uart_8250_port *s)
{
    return s->input_buf_head == s->input_buf_tail;
}
int uart_8250_output_buffer_empty(struct uart_8250_port *s)
{
    return s->output_buf_head == s->output_buf_tail;
}
int uart_8250_input_buffer_full(struct uart_8250_port *s)
{
    return ((s->input_buf_tail + 1) % UART_8250_BUFFER_SIZE) == s->input_buf_head;
}
int uart_8250_output_buffer_full(struct uart_8250_port *s) 
{
    return ((s->output_buf_tail + 1) % UART_8250_BUFFER_SIZE) == s->output_buf_head;
}
void uart_8250_input_buffer_push(struct uart_8250_port *s, uint8_t data) {
    s->input_buf[s->input_buf_tail] = data;
    s->input_buf_tail = (s->input_buf_tail + 1) % UART_8250_BUFFER_SIZE;
}
uint8_t uart_8250_input_buffer_pull(struct uart_8250_port *s) {
    uint8_t temp = s->input_buf[s->input_buf_head];
    s->input_buf_head = (s->input_buf_head + 1) % UART_8250_BUFFER_SIZE;
    return temp;
}
void uart_8250_output_buffer_push(struct uart_8250_port *s, uint8_t data) {
    s->output_buf[s->output_buf_tail] = data;
    s->output_buf_tail = (s->output_buf_tail + 1) % UART_8250_BUFFER_SIZE;
}
uint8_t uart_8250_output_buffer_pull(struct uart_8250_port *s) {
    uint8_t temp = s->output_buf[s->output_buf_head];
    s->output_buf_head = (s->output_buf_head + 1) % UART_8250_BUFFER_SIZE;
    return temp;
}

int uart_8250_interrupt_handler(struct nk_irq_action *action, struct nk_regs *regs, void *state) 
{
  struct uart_8250_port *port = (struct uart_8250_port *)state;

  unsigned int iir;
  int res;

  do {
      iir = uart_8250_read_reg(port,UART_8250_IIR);
      res = uart_8250_handle_irq(port,iir);
  } while(((iir & 0x1) != 1) && res == 0);

  if(res != 0) {
      // Shouldn't print from an interrupt handler,
      // but this shouldn't ever occur, so we want to see the error
      // (even if it means deadlock)
      ERROR("uart_8250_handle_irq returned a non-zero exit code! (res = %d)\n", res);
      return res;
  }
  
  return 0;
}


/*
 * Implementations of "uart_ops"
 */ 

int generic_8250_set_baud_rate(struct uart_port *port, unsigned int baud_rate) 
{
  struct uart_8250_port *p = (struct uart_8250_port*)port;
  
  // Enable access to dll and dlm
  unsigned long lcr;
  lcr = uart_8250_read_reg(p,UART_8250_LCR);
  uart_8250_write_reg(p,UART_8250_LCR,lcr|UART_8250_LCR_DLAB);

  // TODO: Support Baudrates other than 115200
//#ifdef NAUT_CONFIG_DW_8250_UART_EARLY_OUTPUT
  // KJH - HACK the rockpro clock system is complicated so getting the baudrate right is very difficult
  // luckily, U-Boot can use the UART for output too, and so we can just leave the baud rate alone
//#else
//  if(baud_rate != 115200) {
//    return -1;
//  }
//  uart_8250_write_reg(p, UART_8250_DLL, 1);
//  uart_8250_write_reg(p, UART_8250_DLH, 0);
//#endif

  // Restore LCR
  uart_8250_write_reg(p,UART_8250_LCR,lcr);

  return 0;
}
int generic_8250_set_word_length(struct uart_port *port, unsigned int bits) 
{
  struct uart_8250_port *p = (struct uart_8250_port*)port;

  if(bits >= 5 && bits <= 8) {
    unsigned int lcr;
    lcr = uart_8250_read_reg(p, UART_8250_LCR);
    lcr &= ~UART_8250_LCR_WORD_LENGTH_MASK;
    lcr |= (bits - 5) & UART_8250_LCR_WORD_LENGTH_MASK;
    uart_8250_write_reg(p,UART_8250_LCR,lcr);
    return 0;
  } else {
    return -1;
  } 
}
int generic_8250_set_parity(struct uart_port *port, unsigned int parity)
{
  struct uart_8250_port *p = (struct uart_8250_port*)port;

  unsigned int lcr;
  lcr = uart_8250_read_reg(p,UART_8250_LCR);
  lcr &= ~UART_8250_LCR_PARITY_MASK;
  switch(parity) {
    case UART_PARITY_NONE:
      lcr |= UART_8250_LCR_PARITY_NONE;
      break;
    case UART_PARITY_ODD:
      lcr |= UART_8250_LCR_PARITY_ODD;
      break;
    case UART_PARITY_EVEN:
      lcr |= UART_8250_LCR_PARITY_EVEN;
      break;
    case UART_PARITY_MARK:
      lcr |= UART_8250_LCR_PARITY_MARK;
      break;
    case UART_PARITY_SPACE:
      lcr |= UART_8250_LCR_PARITY_SPACE;
      break;
    default:
      return -1;
  }
  uart_8250_write_reg(p,UART_8250_LCR,lcr);
  return 0;
}
int generic_8250_set_stop_bits(struct uart_port *port, unsigned int bits)
{
  struct uart_8250_port *p = (struct uart_8250_port*)port;

  unsigned int lcr;
  lcr = uart_8250_read_reg(p,UART_8250_LCR);
  switch(bits) {
    case 1:
      lcr &= ~UART_8250_LCR_DUAL_STOP_BIT;
      break;
    case 2:
      lcr |= UART_8250_LCR_DUAL_STOP_BIT;
      break;
    default:
      return -1;
  }
  uart_8250_write_reg(p,UART_8250_LCR,lcr);
  return 0;
}

struct uart_ops uart_8250_default_uart_ops = {
  .set_baud_rate = generic_8250_set_baud_rate,
  .set_word_length = generic_8250_set_word_length,
  .set_parity = generic_8250_set_parity,
  .set_stop_bits = generic_8250_set_stop_bits,
};

/*
 * Default uart_8250_ops implementations
 */

// read_reg and write_reg
#ifdef NAUT_CONFIG_HAS_PORT_IO
unsigned int generic_8250_read_reg_port8(struct uart_8250_port *port, int offset)
{
  return (unsigned int)inb(port->reg_base + (offset << port->reg_shift));
}
void generic_8250_write_reg_port8(struct uart_8250_port *port, int offset, unsigned int val) 
{
  outb((uint8_t)val, port->reg_base + (offset << port->reg_shift));
}
#endif

unsigned int generic_8250_read_reg_mem8(struct uart_8250_port *port, int offset) 
{
  return (unsigned int)*(volatile uint8_t*)(void*)(port->reg_base + (offset << port->reg_shift));
}
void generic_8250_write_reg_mem8(struct uart_8250_port *port, int offset, unsigned int val)
{
  *(volatile uint8_t*)(void*)(port->reg_base + (offset << port->reg_shift)) = (uint8_t)val;
}

unsigned int generic_8250_read_reg_mem32(struct uart_8250_port *port, int offset) 
{
  return (unsigned int)*(volatile uint32_t*)(void*)(port->reg_base + (offset << port->reg_shift));
}
void generic_8250_write_reg_mem32(struct uart_8250_port *port, int offset, unsigned int val) 
{
  *(volatile uint32_t*)(void*)(port->reg_base + (offset << port->reg_shift)) = (uint32_t)val;
}

// recv_empty
int generic_8250_recv_empty(struct uart_8250_port *port) 
{
  unsigned int lsr = uart_8250_read_reg(port, UART_8250_LSR);
  return !(lsr & UART_8250_LSR_DATA_READY);
}
// xmit_full
int generic_8250_xmit_full(struct uart_8250_port *port) 
{
  unsigned int lsr = uart_8250_read_reg(port, UART_8250_LSR);
  return !(lsr & UART_8250_LSR_XMIT_EMPTY);
}
// recv_pull
int generic_8250_recv_pull(struct uart_8250_port *port, uint8_t *dest)
{
  uint8_t c = uart_8250_read_reg(port, UART_8250_RBR);
  
  // Right now the enter key is returning '\r',
  // but chardev consoles ignore that, so we just 
  // force it to '\n'
  if((c&0xFF) == '\r') {
    c = '\n';
  }
  // Convert DEL into Backspace
  else if((c&0xFF) == 0x7F) {
    c = 0x08;
  }

  *dest = c;
  return 0;
}
// xmit_push
int generic_8250_xmit_push(struct uart_8250_port *port, uint8_t src) 
{
  uart_8250_write_reg(port, UART_8250_THR, (unsigned int)src);
  return 0;
}

int uart_8250_kick_output(struct uart_8250_port *port) 
{
    uint64_t count=0;

    while (!uart_8250_output_buffer_empty(port)) { 
	  uint8_t ls =  uart_8250_read_reg(port,UART_8250_LSR);
	  if (ls & 0x20) { 
	      // transmit holding register is empty
	      // drive a byte to the device
	      uint8_t data = uart_8250_output_buffer_pull(port);
	      uart_8250_xmit_push(port,data);
	      count++;
	  } else {
	      // chip is full, stop sending to it
	      // but since we have more data, have it
	      // interrupt us when it has room
	      uint8_t ier = uart_8250_read_reg(port,UART_8250_IER);
	      ier |= 0x2;
	      uart_8250_write_reg(port,UART_8250_IER,ier);
	      goto out;
	  }
    }
    
    // the chip has room, but we have no data for it, so
    // disable the transmit interrupt for now
    uint8_t ier = uart_8250_read_reg(port,UART_8250_IER);
    ier &= ~0x2;
    uart_8250_write_reg(port,UART_8250_IER,ier);

 out:
    if (count>0) { 
	    nk_dev_signal((struct nk_dev*)(port->dev));
    }

    return 0;
}
int uart_8250_kick_input(struct uart_8250_port *port) 
{
    uint64_t count=0;
    
    while (!uart_8250_input_buffer_full(port)) { 
	  uint8_t ls =  uart_8250_read_reg(port,UART_8250_LSR);
	  if (ls & 0x04) {
	      // parity error, skip this byte
	      continue;
	  }
	  if (ls & 0x08) { 
	      // framing error, skip this byte
	      continue;
	  }
	  if (ls & 0x10) { 
	      // break interrupt, but we do want this byte
	  }
	  if (ls & 0x02) { 
	      // overrun error - we have lost a byte
	      // but we do want this next one
	  }
	  if (ls & 0x01) { 
	      // data ready
	      // grab a byte from the device if there is room
	      uint8_t data;
          uart_8250_recv_pull(port,&data);
	      uart_8250_input_buffer_push(port,data);
	      count++;
	  } else {
	      // chip is empty, stop receiving from it
	      break;
	  }
    }
    if (count>0) {
	    nk_dev_signal((struct nk_dev *)port->dev);
    }
    return 0;
}

int generic_8250_handle_irq(struct uart_8250_port *port, unsigned int iir) 
{
  switch(iir & 0xF) {
    case UART_8250_IIR_NONE:
//      generic_8250_direct_putchar(port,'N');
      break;
    case UART_8250_IIR_MSR_RESET:
//      generic_8250_direct_putchar(port,'M');
      (void)uart_8250_read_reg(port,UART_8250_MSR);
      break;
    case UART_8250_IIR_XMIT_REG_EMPTY:
//      generic_8250_direct_putchar(port,'X');
      uart_8250_kick_output(port);
      break;
    case UART_8250_IIR_RECV_TIMEOUT:
//      generic_8250_direct_putchar(port,'T');
    case UART_8250_IIR_RECV_DATA_AVAIL:
//      generic_8250_direct_putchar(port,'R');
      uart_8250_kick_input(port);
      break;
    case UART_8250_IIR_LSR_RESET:
//      generic_8250_direct_putchar(port,'L');
      (void)uart_8250_read_reg(port,UART_8250_LSR);
      break;
    default:
//      generic_8250_direct_putchar(port,'U');
      break;
  }

  return 0;
}

struct uart_8250_ops uart_8250_default_uart_8250_ops = {
  .read_reg = generic_8250_read_reg_mem8,
  .write_reg = generic_8250_write_reg_mem8,
  .xmit_full = generic_8250_xmit_full,
  .recv_empty = generic_8250_recv_empty,
  .xmit_push = generic_8250_xmit_push,
  .recv_pull = generic_8250_recv_pull,
  .handle_irq = generic_8250_handle_irq
};

/*
 * Common helper functions
 */

int generic_8250_probe_scratch(struct uart_8250_port *uart) 
{
  uart_8250_write_reg(uart,UART_8250_SCR,0xde);
  if(uart_8250_read_reg(uart,UART_8250_SCR) != 0xde) {
    return 0;
  } else {
    return 1;
  }
}

void generic_8250_direct_putchar(struct uart_8250_port *port, uint8_t c) 
{   
  uint8_t ls;

  uart_8250_write_reg(port,UART_8250_THR,c);
  do {
    ls = (uint8_t)uart_8250_read_reg(port,UART_8250_LSR);
  }
  while (!(ls & UART_8250_LSR_XMIT_EMPTY)); 
}

int generic_8250_enable_fifos(struct uart_8250_port *port) 
{
  unsigned int fcr = uart_8250_read_reg(port,UART_8250_FCR);

  uart_8250_write_reg(port,UART_8250_FCR,
      UART_8250_FCR_ENABLE_FIFO|
      UART_8250_FCR_ENABLE_64BYTE_FIFO|
      UART_8250_FCR_64BYTE_TRIGGER_LEVEL_56BYTE);

  unsigned int iir = uart_8250_read_reg(port,UART_8250_IIR);
  unsigned int fifo_mask = iir & UART_8250_IIR_FIFO_MASK;

  switch(fifo_mask) {
    case UART_8250_IIR_FIFO_ENABLED:
      if(iir & UART_8250_IIR_64BYTE_FIFO_ENABLED) {
        return 0;
      } else {
        return 0;
      }
    default:
    case UART_8250_IIR_FIFO_NON_FUNC:
    case UART_8250_IIR_FIFO_NONE:
      return -1;
  }
}
int generic_8250_disable_fifos(struct uart_8250_port *port) 
{
  uart_8250_write_reg(port,UART_8250_FCR,0);
  return 0;
}
int generic_8250_clear_fifos(struct uart_8250_port *port) 
{
  unsigned int fcr = uart_8250_read_reg(port, UART_8250_FCR);
  uart_8250_write_reg(port, UART_8250_FCR,
      fcr|
      UART_8250_FCR_CLEAR_RECV_FIFO|
      UART_8250_FCR_CLEAR_XMIT_FIFO);
  return 0;
}

int generic_8250_enable_recv_interrupts(struct uart_8250_port *port) 
{
  unsigned int ier = uart_8250_read_reg(port, UART_8250_IER);
  ier |= UART_8250_IER_RECV_DATA_AVAIL | UART_8250_IER_RECV_LINE_STATUS;
  uart_8250_write_reg(port, UART_8250_IER, ier);
  return 0;
}

/*
 * Implementations of the 8250 nk_char_dev_int ops
 */

int generic_8250_char_dev_get_characteristics(void *state, struct nk_char_dev_characteristics *c) 
{
    memset(c,0,sizeof(*c));
    return 0;
}

int generic_8250_char_dev_read(void *state, uint8_t *dest) 
{
  struct uart_8250_port *uart = (struct uart_8250_port*)state;

  //generic_8250_direct_putchar(uart,'{');

  int rc = -1;
  int flags;

  flags = spin_lock_irq_save(&uart->input_lock);

  if(uart_8250_input_buffer_empty(uart)) {
    //generic_8250_direct_putchar(uart,'E');
    rc = 0;
  } else {
    *dest = uart_8250_input_buffer_pull(uart);
    rc = 1;
  }

  spin_unlock_irq_restore(&uart->input_lock, flags);

  //generic_8250_direct_putchar(uart,'}');
  return rc;
}

int generic_8250_char_dev_write(void *state, uint8_t *src)
{ 
  struct uart_8250_port *uart = (struct uart_8250_port*)state;

  int wc = -1;
  int flags;

  flags = spin_lock_irq_save(&uart->output_lock);

  if(uart_8250_output_buffer_full(uart)) {
    wc = 0;
  } else {
    uart_8250_output_buffer_push(uart, *src);
    uart_8250_kick_output(uart);
    wc = 1;
  }

  spin_unlock_irq_restore(&uart->output_lock, flags);
  return wc;
}

int generic_8250_char_dev_status(void *state) 
{
  struct uart_8250_port *uart = (struct uart_8250_port*)state;

  int status = 0;
  int flags;

  flags = spin_lock_irq_save(&uart->input_lock);
  if(!uart_8250_input_buffer_empty(uart)) {
    status |= NK_CHARDEV_READABLE;
  } 
  spin_unlock_irq_restore(&uart->input_lock, flags);

  flags = spin_lock_irq_save(&uart->output_lock);
  if(!uart_8250_output_buffer_full(uart)) {
    status |= NK_CHARDEV_WRITEABLE;
  } 
  spin_unlock_irq_restore(&uart->output_lock, flags);

  return status;
}

struct nk_char_dev_int generic_8250_char_dev_int = {
    .get_characteristics = generic_8250_char_dev_get_characteristics,
    .read = generic_8250_char_dev_read,
    .write = generic_8250_char_dev_write,
    .status = generic_8250_char_dev_status
};

