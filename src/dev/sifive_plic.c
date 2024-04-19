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


#include <nautilus/irqdev.h>
#include <nautilus/interrupt.h>
#include <nautilus/of/fdt.h>
#include <nautilus/of/dt.h>
#include <nautilus/endian.h>

#ifndef NAUT_CONFIG_DEBUG_SIFIVE_PLIC
#undef DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif

#define PLIC_INFO(fmt, args...)     INFO_PRINT("[PLIC] " fmt, ##args)
#define PLIC_DEBUG(fmt, args...)    DEBUG_PRINT("[PLIC] " fmt, ##args)
#define PLIC_WARN(fmt, args...)    WARN_PRINT("[PLIC] " fmt, ##args)
#define PLIC_ERROR(fmt, args...)    ERROR_PRINT("[PLIC] " fmt, ##args)

#define PLIC_PRIORITY_OFFSET 0x000000U
#define PLIC_PENDING_OFFSET 0x1000U
#define PLIC_PENDING_STRIDE 0x80U
#define PLIC_ENABLED_OFFSET 0x002000U
#define PLIC_ENABLED_STRIDE 0x80U

#define PLIC_CONTEXT_OFFSET 0x200000U
#define PLIC_CONTEXT_STRIDE 0x1000U
#define PLIC_CONTEXT_THRESHOLD_REGISTER_OFFSET 0x0U
#define PLIC_CONTEXT_CLAIM_REGISTER_OFFSET 0x4U

#define PLIC_PRIORITY(plic_ptr, n) (PLIC_PRIORITY_OFFSET + (n) * sizeof(uint32_t))

#define PLIC_BASE_HWIRQ 1

#define MAX_NUM_PLICS 1

#define PLIC_SMODE_HLIC_HWIRQ 9

struct sifive_plic *global_plic_array[MAX_NUM_PLICS];

struct sifive_plic_context 
{
  struct sifive_plic *plic;
  int ctx_num;

  uint32_t hartid;
  nk_irq_t hwirq;
};

struct sifive_plic 
{
  void *mmio_base;
  size_t mmio_size;

  int num_contexts;
  struct sifive_plic_context *contexts;

  uint32_t num_irqs;
  nk_irq_t irq_base;
  struct nk_irq_desc *irq_descs;
};

static int plic_context_is_supervisor(struct sifive_plic_context *ctx) 
{
  if(ctx->hwirq == PLIC_SMODE_HLIC_HWIRQ) {
    return 1;
  } else {
    return 0;
  }
}

struct sifive_plic_context * plic_cpu_supervisor_context(struct sifive_plic *plic, cpu_id_t cpuid) {
    uint32_t hartid = nk_get_cpu(cpuid)->hartid;
    for(int i = 0; i < plic->num_contexts; i++) {
        struct sifive_plic_context *ctx = &plic->contexts[i];
        if(ctx->hartid != hartid) {
            continue;
        }
        if(!plic_context_is_supervisor(ctx)) {
            continue;
        }
        return ctx;
    }
    return NULL;
}

static int plic_context_irq_pending(struct sifive_plic_context *ctx, nk_hwirq_t hwirq) {
    volatile uint32_t *bitmap = (uint32_t*)(ctx->plic->mmio_base + PLIC_PENDING_OFFSET + (PLIC_PENDING_STRIDE * ctx->ctx_num));
    size_t bitmap_index = hwirq / 32;
    size_t bitmap_bit = hwirq % 32;

    return !!(bitmap[bitmap_index] & (1ULL << bitmap_bit));
}

static int plic_context_irq_enabled(struct sifive_plic_context *ctx, nk_hwirq_t hwirq) {
    volatile uint32_t *bitmap = (uint32_t*)(ctx->plic->mmio_base + PLIC_ENABLED_OFFSET + (PLIC_ENABLED_STRIDE * ctx->ctx_num));
    size_t bitmap_index = hwirq / 32;
    size_t bitmap_bit = hwirq % 32;

    return !!(bitmap[bitmap_index] & (1ULL << bitmap_bit));
}

static void * plic_context_register_block(struct sifive_plic_context *ctx) {
    return ctx->plic->mmio_base + PLIC_CONTEXT_OFFSET + (ctx->ctx_num * PLIC_CONTEXT_STRIDE);
}

static uint32_t plic_context_threshold(struct sifive_plic_context *ctx) {
    volatile uint32_t *threshold_reg = plic_context_register_block(ctx) + PLIC_CONTEXT_THRESHOLD_REGISTER_OFFSET;
    return *threshold_reg;
}

static void plic_context_set_threshold(struct sifive_plic_context *ctx, uint32_t threshold) {
    volatile uint32_t *threshold_reg = plic_context_register_block(ctx) + PLIC_CONTEXT_THRESHOLD_REGISTER_OFFSET;
    *threshold_reg = threshold;
}

static uint32_t plic_priority(struct sifive_plic *plic, nk_hwirq_t hwirq) {
    volatile uint32_t *priority_array = (volatile uint32_t*)plic->mmio_base;
    return priority_array[hwirq];
}

static void plic_set_priority(struct sifive_plic *plic, nk_hwirq_t hwirq, uint32_t priority) {
    volatile uint32_t *priority_array = (volatile uint32_t*)plic->mmio_base;
    priority_array[hwirq] = priority;
}

static uint32_t plic_context_claim(struct sifive_plic_context *ctx) {
    volatile uint32_t *claim_reg = plic_context_register_block(ctx) + PLIC_CONTEXT_CLAIM_REGISTER_OFFSET;
    return *claim_reg;
}

static void plic_context_complete(struct sifive_plic_context *ctx, uint32_t claimed) {
    volatile uint32_t *claim_reg = plic_context_register_block(ctx) + PLIC_CONTEXT_CLAIM_REGISTER_OFFSET;
    *claim_reg = claimed;
}

static int plic_dev_get_characteristics(void *state, struct nk_irq_dev_characteristics *c)
{
  memset(c, 0, sizeof(struct nk_irq_dev_characteristics));
  return 0;
}

int plic_percpu_init(void) 
{
  for(int i = 0; i < MAX_NUM_PLICS; i++) {
    struct sifive_plic *plic = (struct sifive_plic*)global_plic_array[i];
    if(plic != NULL) {
        struct sifive_plic_context *ctx = plic_cpu_supervisor_context(plic, my_cpu_id());
        if(ctx != NULL) {
          plic_context_set_threshold(ctx, 0);
        } else {
            PLIC_WARN("Could not find supervisor context for CPU (%u)!\n", my_cpu_id());
        }
    }
  }
  return 0;
}

static int plic_dev_revmap(void *state, nk_hwirq_t hwirq, nk_irq_t *irq) 
{
  struct sifive_plic *plic = (struct sifive_plic*)state;
  if(hwirq < PLIC_BASE_HWIRQ || hwirq >= PLIC_BASE_HWIRQ + plic->num_irqs) {
    return -1;
  }
  *irq = plic->irq_base + (hwirq-1);
  return 0;
}

static int plic_dev_translate(void *state, nk_dev_info_type_t type, const void *raw, nk_hwirq_t *out) 
{
  struct sifive_plic *plic = (struct sifive_plic*)state;
  uint32_t *raw_cells = (uint32_t*)raw;
  switch(type) {
    case NK_DEV_INFO_OF:
      *out = be32toh(raw_cells[0]);
      return 0;
    default:
      PLIC_ERROR("Cannot translate nk_dev_info IRQ's of type other than OF!\n");
      return -1;
  }
}

static int plic_dev_ack_irq(void *state, nk_hwirq_t *hwirq) 
{
  struct sifive_plic *plic = (struct sifive_plic*)state;
  struct sifive_plic_context *ctx = plic_cpu_supervisor_context(plic, my_cpu_id());
  *hwirq = plic_context_claim(ctx);
  return 0;
}

static int plic_dev_eoi_irq(void *state, nk_hwirq_t hwirq) 
{
  struct sifive_plic *plic = (struct sifive_plic*)state;
  struct sifive_plic_context *ctx = plic_cpu_supervisor_context(plic, my_cpu_id());
  plic_context_complete(ctx, (uint32_t)hwirq);
  return 0;
}

// KJH - Why is priority unused?
static void plic_set_enabled(struct sifive_plic *plic, int hwirq, bool_t enable) 
{
    if (hwirq == 0) return;

    // TODO: this masks the interrupt so we can use it, but it is a bit ugly
    // alternatively what Nick does is iterate through all the interrupts for each
    // PLIC during init and mask them
    // https://github.com/ChariotOS/chariot/blob/7cf70757091b79cbb102a943a963dce516a8c667/arch/riscv/plic.cpp#L85-L88
    plic_set_priority(plic, hwirq, 7);

    size_t index = hwirq / 32;
    uint32_t mask = (1ULL << (hwirq % 32));

    for(int i = 0; i < plic->num_contexts; i++) 
    {
      struct sifive_plic_context *ctx = &plic->contexts[i];

      if(!plic_context_is_supervisor(ctx)) {
        continue;
      }

      volatile uint32_t *bitmap = (uint32_t*)(plic->mmio_base + PLIC_ENABLED_OFFSET + (i * PLIC_ENABLED_STRIDE));

      uint32_t val = bitmap[index];

      // printk("Hart: %d, hwirq: %d, v1: %d\n", hart, hwirq, val);
      if (enable)
          val |= mask;
      else
          val &= ~mask;

      //printk("Hart: %d, hwirq: %d, irq: %x, *irq: %x\n", hart, hwirq, &MREG(PLIC_ENABLE(plic, hwirq, hart)), val);
      bitmap[index] = val;
    }
}

static int plic_dev_enable_irq(void *state, nk_hwirq_t hwirq) 
{
  struct sifive_plic *plic = (struct sifive_plic*)state;
  plic_set_enabled(plic, hwirq, 1);
  return 0;
}

static int plic_dev_disable_irq(void *state, nk_hwirq_t hwirq) 
{
  struct sifive_plic *plic = (struct sifive_plic*)state; 
  plic_set_enabled(plic, hwirq, 0);
  return 0;
}

static int plic_dev_irq_status(void *state, nk_hwirq_t hwirq) 
{
  struct sifive_plic *plic = (struct sifive_plic*)state;

  int status = 0;

  struct sifive_plic_context *ctx = NULL;
  for(int i = 0; i < plic->num_contexts; i++) {
    if(plic_context_is_supervisor(&plic->contexts[i])) {
      ctx = &plic->contexts[i];
      break;
    }
  }

  if(ctx == NULL) {
    // BAD
    return IRQ_STATUS_ERROR;
  }

  status |= plic_context_irq_enabled(ctx, hwirq) ?
    IRQ_STATUS_ENABLED : 0;

  status |= plic_context_irq_pending(ctx, hwirq) ?
    IRQ_STATUS_PENDING : 0;

  // KJH - No idea how to get this right now (Possible we can't)
  status |= 0/*TODO*/ ?
    IRQ_STATUS_ACTIVE : 0;

  return status;
}

static int plic_interrupt_handler(struct nk_irq_action *action, struct nk_regs *regs, void *state) 
{
  struct sifive_plic *plic = (struct sifive_plic *)state;

  nk_hwirq_t hwirq;
  plic_dev_ack_irq(state, &hwirq);

  nk_irq_t irq = NK_NULL_IRQ;
  if(plic_dev_revmap(state, hwirq, &irq)) {
    return -1;
  }

  struct nk_irq_desc *desc = nk_irq_to_desc(irq);
  if(desc == NULL) {
    return -1;
  }

  nk_handle_irq_actions(desc, regs);

  plic_dev_eoi_irq(state, hwirq);

  return 0;
}

static struct nk_irq_dev_int plic_dev_int = {
  .get_characteristics = plic_dev_get_characteristics,
  .ack_irq = plic_dev_ack_irq,
  .eoi_irq = plic_dev_eoi_irq,
  .enable_irq = plic_dev_enable_irq,
  .disable_irq = plic_dev_disable_irq,
  .irq_status = plic_dev_irq_status,
  .revmap = plic_dev_revmap,
  .translate = plic_dev_translate
};

static int plic_init_dev_info(struct nk_dev_info *info) 
{
  int did_alloc = 0;
  int did_register = 0;
  int did_context_alloc = 0;

  uint8_t flags = irq_disable_save();

  if(info->type != NK_DEV_INFO_OF) {
    PLIC_ERROR("Currently only support device tree initialization!\n");
    goto err_exit;
  }

  struct sifive_plic *plic = malloc(sizeof(struct sifive_plic));
  int found_global_slot = 0;
  for(int i = 0; i < MAX_NUM_PLICS; i++) {
      if(global_plic_array[i] == NULL) {
          global_plic_array[i] = plic;
          found_global_slot = 1;
      }
  }

  if(!found_global_slot) {
      PLIC_WARN("Found more PLIC's than MAX_NUM_PLICS (%u)!\n", MAX_NUM_PLICS);
      goto err_exit;
  }

  if(plic == NULL) 
  {
    PLIC_ERROR("Failed to allocate PLIC struct!\n");
    goto err_exit;
  }
  did_alloc = 1;

  memset(plic, 0, sizeof(struct sifive_plic));

  if(nk_dev_info_read_register_block(info, &plic->mmio_base, &plic->mmio_size)) {
    PLIC_ERROR("Failed to read register block!\n");
    goto err_exit;
  }

  if(nk_dev_info_read_u32(info, "riscv,ndev", &plic->num_irqs)) {
    PLIC_ERROR("Failed to read number of IRQ's!\n");
    goto err_exit;
  }

  PLIC_DEBUG("mmio_base = %p, mmio_size = %p, num_irqs = %u\n",
      plic->mmio_base, plic->mmio_size, plic->num_irqs);

  struct nk_irq_dev *dev = nk_irq_dev_register("plic", 0, &plic_dev_int, (void*)plic);

  if(dev == NULL) {
    PLIC_ERROR("Failed to register IRQ device!\n");
    goto err_exit;
  } else {
    did_register = 1;
  }

  nk_dev_info_set_device(info, (struct nk_dev*)dev);

  if(nk_request_irq_range(plic->num_irqs, &plic->irq_base)) {
    PLIC_ERROR("Failed to get IRQ numbers!\n");
    goto err_exit;
  }

  PLIC_DEBUG("plic->irq_base = %u\n", plic->irq_base);

  plic->irq_descs = nk_alloc_irq_descs(plic->num_irqs, PLIC_BASE_HWIRQ, 0, dev);
  if(plic->irq_descs == NULL) {
    PLIC_ERROR("Failed to allocate IRQ descriptors!\n");
    goto err_exit;
  }

  if(nk_assign_irq_descs(plic->num_irqs, plic->irq_base, plic->irq_descs)) {
    PLIC_ERROR("Failed to assign IRQ descriptors!\n");
    goto err_exit;
  }

  PLIC_DEBUG("Created and Assigned IRQ Descriptors\n");

  plic->num_contexts = nk_dev_info_num_irq(info);
  PLIC_DEBUG("num_contexts = %u\n", plic->num_contexts);

  plic->contexts = malloc(sizeof(struct sifive_plic_context) * plic->num_contexts);
  if(plic->contexts == NULL) {
    PLIC_ERROR("Failed to allocate PLIC contexts!\n");
    goto err_exit;
  }
  did_context_alloc = 1;

  int smode_handler_set = 0;
  
  for(int i = 0; i < plic->num_contexts; i++) {

    plic->contexts[i].plic = plic;
    plic->contexts[i].ctx_num = i;

    struct nk_dev_info *hlic_node = nk_dev_info_read_irq_parent(info, i);
    struct nk_dev_info *cpu_node = nk_dev_info_get_parent(hlic_node);
    uint32_t hartid = -1;
    if(nk_dev_info_read_u32(cpu_node, "reg", &hartid)) {
        PLIC_ERROR("Could not get Hart ID from parent node of Context's interrupt parent!\n");
        continue;
    } else {
        PLIC_DEBUG("IRQ Parent = %s\n", nk_dev_info_get_name(hlic_node));
        PLIC_DEBUG("IRQ Parent Parent = %s\n", nk_dev_info_get_name(cpu_node));
    }

    nk_irq_t ctx_irq = nk_dev_info_read_irq(info, i);

    if(ctx_irq == NK_NULL_IRQ) {
      PLIC_ERROR("Context (%u) does not exist\n", i);
      continue;
    }

    struct nk_irq_desc *desc = nk_irq_to_desc(ctx_irq);
    if(desc == NULL) {
        PLIC_ERROR("Could not get irq descriptor for Context (%u) HLIC IRQ!\n", i);
        continue;
    }
    plic->contexts[i].hwirq = desc->hwirq;
    plic->contexts[i].hartid = hartid;

    PLIC_DEBUG("Context (%u): hwirq = %u, hartid = %u\n", i, desc->hwirq, hartid);

    if(!smode_handler_set) {
      if(desc != NULL) {
        if(desc->hwirq == PLIC_SMODE_HLIC_HWIRQ) {
  
          // Register the supervisor interrupt handler
          if(nk_irq_add_handler_dev(ctx_irq, plic_interrupt_handler, (void*)plic, (struct nk_dev*)dev)) {
            PLIC_ERROR("Failed to assign IRQ handler to context (%u)!\n");
            goto err_exit;
          }
          nk_unmask_irq(ctx_irq);

          smode_handler_set = 1; 
        }
      } else {
        // Weird
        PLIC_WARN("PLIC read IRQ without interrupt descriptor (irq=%u)!\n", ctx_irq);
      }
    }
  }

  if(!smode_handler_set) {
    PLIC_ERROR("Could not find HLIC hwirq %u!\n", PLIC_SMODE_HLIC_HWIRQ);
    goto err_exit;
  }

  for(int i = 0; i < plic->num_irqs; i++) {
      nk_irq_t irq = plic->irq_base + i;
      nk_mask_irq(irq);
  }

  PLIC_DEBUG("initialized globally!\n");
  irq_enable_restore(flags);
  return 0;

err_exit:
  if(did_context_alloc) {
    free(plic->contexts);
  }
  if(did_alloc) {
    free(plic);
  }
  if(did_register) {
    nk_irq_dev_unregister(dev);
  }
  irq_enable_restore(flags);
  return -1;
}

static const char * plic_properties_names[] = {
  "interrupt-controller"
};

static const struct of_dev_properties plic_of_properties = {
  .names = plic_properties_names,
  .count = 1
};

static const struct of_dev_id plic_of_dev_ids[] = {
  { .properties = &plic_of_properties, .compatible = "sifive,plic-1.0.0" },
  { .properties = &plic_of_properties, .compatible = "riscv,plic0" }
};

static const struct of_dev_match plic_of_dev_match = {
  .ids = plic_of_dev_ids,
  .num_ids = sizeof(plic_of_dev_ids)/sizeof(struct of_dev_id),
  .max_num_matches = 1
};

int plic_init(void) 
{ 
  PLIC_DEBUG("init\n");
  for(int i = 0; i < MAX_NUM_PLICS; i++) {
      global_plic_array[i] = NULL;
  }
  return of_for_each_match(&plic_of_dev_match, plic_init_dev_info);
}

