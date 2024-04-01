#include <nautilus/nautilus.h>
#include <nautilus/naut_types.h>
#include <arch/riscv/sbi.h>

#include <nautilus/intrinsics.h>

// bitmap of locally detected SBI extensions
enum sbi_extension {
  SBI_EXTENSION_TIMER = 1,
  SBI_EXTENSION_IPI,
  SBI_EXTENSION_RFENCE,
  SBI_EXTENSION_HSM,
  SBI_EXTENSION_SRST,
};

static unsigned int sbi_ext;




static inline bool_t sbi_ext_present(enum sbi_extension e) { return sbi_ext & (1 << e); }

struct sbiret sbi_generic_call_2(unsigned long extension, unsigned long function) {
  return sbi_call(extension, function);
}

bool_t sbi_probe_extension(unsigned long extension) { return sbi_call(SBI_PROBE_EXTENSION, extension).value != 0; }

long sbi_legacy_set_timer(uint64_t stime_value) { return sbi_call(SBI_SET_TIMER, stime_value).error; }
long sbi_legacy_send_ipis(const unsigned long *hart_mask) { return sbi_call(SBI_SEND_IPI, hart_mask).error; }
long sbi_legacy_clear_ipi(void) { panic("TODO: clear sip.SSIP here!\n"); return -1; }

/*
status_t sbi_boot_hart(uint hartid, paddr_t start_addr, ulong arg) {
    if (!sbi_ext_present(SBI_EXTENSION_HSM))
        return ERR_NOT_IMPLEMENTED;
    // try to use the HSM implementation to boot a cpu
    struct sbiret ret = sbi_call(SBI_EXT_HSM_SIG, 0, hartid, start_addr, arg);
    if (ret.error < 0) {
        return ERR_INVALID_ARGS;
    }
    return NO_ERROR;
}
*/

void sbi_early_init(void) {
  // read the presence of some features
  /* sbi_ext |= sbi_probe_extension(SBI_EXT_TIMER_SIG) ? (1 << SBI_EXTENSION_TIMER) : 0; */
  /* sbi_ext |= sbi_probe_extension(SBI_EXT_IPI_SIG) ? (1 << SBI_EXTENSION_IPI) : 0; */
  /* sbi_ext |= sbi_probe_extension(SBI_EXT_RFENCE_SIG) ? (1 << SBI_EXTENSION_RFENCE) : 0; */
  /* sbi_ext |= sbi_probe_extension(SBI_EXT_HSM_SIG) ? (1 << SBI_EXTENSION_HSM) : 0; */
  /* sbi_ext |= sbi_probe_extension(SBI_EXT_SRST_SIG) ? (1 << SBI_EXTENSION_SRST) : 0; */
}



#define LOG(...) PFXLOG(GRN "SBI", __VA_ARGS__)


void sbi_init(void) {
  printk("spec version %ld impl id %ld version %ld\n", sbi_generic_call_2(SBI_GET_SBI_SPEC_VERSION).value,
      sbi_generic_call_2(SBI_GET_SBI_IMPL_ID).value, sbi_generic_call_2(SBI_GET_SBI_IMPL_VERSION).value);

  // print the extensions detected
  printk("extension TIMER %d\n", sbi_ext_present(SBI_EXTENSION_TIMER));
  printk("extension IPI %d\n", sbi_ext_present(SBI_EXTENSION_IPI));
  printk("extension RFENCE %d\n", sbi_ext_present(SBI_EXTENSION_RFENCE));
  printk("extension HSM %d\n", sbi_ext_present(SBI_EXTENSION_HSM));
  printk("extension SRST %d\n", sbi_ext_present(SBI_EXTENSION_SRST));
}

