
#include<dev/pl011.h>

#define UART_DATA        0x0
#define UART_RECV_STATUS 0x4
#define UART_ERR_CLR     0x4
#define UART_FLAG        0x18
#define UART_IRDA_LP_CTR 0x20
#define UART_INT_BAUD    0x24
#define UART_FRAC_BAUD   0x28
#define UART_LINE_CTRL_H 0x2C
#define UART_CTRL        0x30
#define UART_INT_FIFO    0x34
#define UART_INT_MASK    0x38
#define UART_RAW_INT     0x3C
#define UART_MASK_INT    0x40
#define UART_INT_CLR     0x44
#define UART_DMA_CTRL    0x48

#define UART_PERIP_ID_0  0xFE0
#define UART_PERIP_ID_1  0xFE4
#define UART_PERIP_ID_2  0xFE8
#define UART_PERIP_ID_3  0xFEC
#define UART_CELL_ID_0   0xFF0
#define UART_CELL_ID_1   0xFF4
#define UART_CELL_ID_2   0xFF8
#define UART_CELL_ID_3   0xFFC

#define UART_FLAG_CLR_TO_SEND (1<<0)
#define UART_FLAG_DATA_SET_RDY (1<<1)
#define UART_FLAG_DATA_CARR_DETECT (1<<2)
#define UART_FLAG_BSY (1<<3)
#define UART_FLAG_RECV_FIFO_EMPTY (1<<4)
#define UART_FLAG_TRANS_FIFO_FULL (1<<5)
#define UART_FLAG_RECV_FIFO_FULL (1<<6)
#define UART_FLAG_TRANS_FIFO_EMPTY (1<<7)
#define UART_FLAG_RING (1<<8)

#define UART_LINE_CTRL_BRK_BITMASK (1<<0)
#define UART_LINE_CTRL_PARITY_ENABLE_BITMASK (1<<1)
#define UART_LINE_CTRL_EVEN_PARITY_BITMASK (1<<2)
#define UART_LINE_CTRL_TWO_STP_BITMASK (1<<3)
#define UART_LINE_CTRL_FIFO_ENABLE_BITMASK (1<<4)
#define UART_LINE_CTRL_WORD_LENGTH_BITMASK (0b11<<5)
#define UART_LINE_CTRL_STICK_PARITY_BITMASK (1<<7)

#define UART_CTRL_ENABLE_BITMASK (1<<0)
#define UART_CTRL_SIR_ENABLE_BITMASK (1<<1)
#define UART_CTRL_SIR_LOW_POW_BITMASK (1<<2)
#define UART_CTRL_LOOPBACK_ENABLE_BITMASK (1<<7)
#define UART_CTRL_TRANS_ENABLE_BITMASK (1<<8)
#define UART_CTRL_RECV_ENABLE_BITMASK (1<<9)

static inline void pl011_write_reg(struct pl011_uart *p, uint32_t reg, uint32_t val) {
  *(volatile uint32_t*)(p->mmio_base + reg) = val;
}
static inline void pl011_and_reg(struct pl011_uart *p, uint32_t reg, uint32_t val) {
  *(volatile uint32_t*)(p->mmio_base + reg) &= val;
}
static inline void pl011_or_reg(struct pl011_uart *p, uint32_t reg, uint32_t val) {
  *(volatile uint32_t*)(p->mmio_base + reg) |= val;
}

static inline void pl011_enable(struct pl011_uart *p) {
  pl011_or_reg(p, UART_CTRL, UART_CTRL_ENABLE_BITMASK);
}
static inline void pl011_disable(struct pl011_uart *p) {
  pl011_and_reg(p, UART_CTRL, ~UART_CTRL_ENABLE_BITMASK);
}

static void pl011_wait(struct pl011_uart *p) {
  while(
      *((volatile uint32_t*)(p->mmio_base + UART_FLAG)) & UART_FLAG_BSY
    ||*((volatile uint32_t*)(p->mmio_base + UART_FLAG)) & UART_FLAG_TRANS_FIFO_FULL
    ){
    // Loop while the busy bit is set or the fifo is full
  }
}

static inline void pl011_enable_fifo(struct pl011_uart *p) {
  pl011_or_reg(p, UART_LINE_CTRL_H, UART_LINE_CTRL_FIFO_ENABLE_BITMASK);
}

static inline void pl011_disable_fifo(struct pl011_uart *p) {
  pl011_and_reg(p, UART_LINE_CTRL_H, ~UART_LINE_CTRL_FIFO_ENABLE_BITMASK);
}

static inline void pl011_configure(struct pl011_uart *p) {

  // Disable the PL011 for configuring
  pl011_disable(p);

  // Wait for any transmission to complete
  pl011_wait(p);

  // Flushes the fifo queues
  pl011_disable_fifo(p);

  // div = clock / (16 * baudrate)
  // 2^6 * div = (2^2 * clock) / baudrate
  uint32_t div_mult = ((1<<6)*(p->clock)) / p->baudrate;
  uint16_t div_int = (div_mult>>6) & 0xFFFF;
  uint16_t div_frac = div_mult & 0x3F;

  // Write the baudrate divisior registers
  pl011_write_reg(p, UART_INT_BAUD, div_int);
  pl011_write_reg(p, UART_FRAC_BAUD, div_frac);

  // Set the word size to 8 bits
  pl011_or_reg(p, UART_LINE_CTRL_H, UART_LINE_CTRL_WORD_LENGTH_BITMASK);

  // Completely mask all interrupts (not needed when all we want is to do is transmit)
  pl011_write_reg(p, UART_MASK_INT, 0x3FF);

  // Disable receiving
  pl011_and_reg(p, UART_CTRL, ~UART_CTRL_RECV_ENABLE_BITMASK);

  // Re-enable the PL011
  pl011_enable(p);
  
}

void pl011_uart_init(struct pl011_uart *p, void *base, uint64_t clock) {
  p->mmio_base = base;
  p->clock = clock; 
  p->baudrate = 100000;

  pl011_configure(p);
}

static inline void __pl011_uart_putchar(struct pl011_uart *p, unsigned char c) {
  pl011_wait(p);
  pl011_write_reg(p, UART_DATA, (uint32_t)c);
}

void pl011_uart_putchar(struct pl011_uart *p, unsigned char c) {
  __pl011_uart_putchar(p, c);
}

void pl011_uart_puts(struct pl011_uart *p, const char *data) {
  uint64_t i = 0;
  while(data[i] != '\0') {
    __pl011_uart_putchar(p, data[i]);
    i++;
  }
}

void pl011_uart_write(struct pl011_uart *p, const char *data, size_t n) {
  for(uint64_t i = 0; i < n; i++) {
    __pl011_uart_putchar(p, data[i]);
  }
}