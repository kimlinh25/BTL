#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "defs.h"

#define PAGESTRESS_ROUNDS 32
#define PAGES_PER_HART    8
#define WAIT_TICKS        50000000ULL

#if PAGESTRESS_CPUS < 1 || PAGESTRESS_CPUS > NCPU
#error "PAGESTRESS requires CPUS between 1 and NCPU"
#endif

#if PAGESTRESS_INJECT < 0 || PAGESTRESS_INJECT > 1
#error "PAGESTRESS_INJECT must be 0 or 1"
#endif

extern char end[];

static uchar *pages[NCPU][PAGES_PER_HART];
static uint64 held_from[NCPU];
static uint64 verified_at[NCPU];
static int arrivals;
static uint64 free_before;

// Each hart advances its own target. All harts must pass one phase before
// the cumulative counter can reach the target of the next phase.
static void
wait_for_harts(int *target, char *phase, int round, int hart)
{
  *target += PAGESTRESS_CPUS;
  __atomic_add_fetch(&arrivals, 1, __ATOMIC_ACQ_REL);

  uint64 start = r_time();
  int seen;
  while ((seen = __atomic_load_n(&arrivals, __ATOMIC_ACQUIRE)) < *target) {
    if (r_time() - start >= WAIT_TICKS) {
      printk("P05|phase=timeout|stage=%s|round=%d|hart=%d|seen=%d|target=%d\n",
             phase, round, hart, seen, *target);
      panic("P05: hart barrier timeout");
    }
  }
}

static uchar *
page_at(int index)
{
  return pages[index / PAGES_PER_HART][index % PAGES_PER_HART];
}

static uchar
page_pattern(int round, int hart, int slot)
{
  return 1 + (31 * round + hart * PAGES_PER_HART + slot) % 251;
}

// The injection changes only what the comparison sees. The list of pages
// owned by the test stays intact, so every allocation is still freed once.
static int
find_duplicate(int inject)
{
  int total = PAGESTRESS_CPUS * PAGES_PER_HART;
  for (int i = 0; i < total; i++) {
    for (int j = i + 1; j < total; j++) {
      uchar *other = page_at(inject && j == 1 ? 0 : j);
      if (page_at(i) == other)
        return 1;
    }
  }
  return 0;
}

static void
verify_pattern(uchar *page, uchar expected, int round, int hart, int slot)
{
  for (int offset = 0; offset < PGSIZE; offset++) {
    if (page[offset] != expected) {
      printk("P05|phase=pattern-fail|round=%d|hart=%d|slot=%d|offset=%d\n",
             round, hart, slot, offset);
      panic("P05: page pattern mismatch");
    }
  }
}

static void
log_held_interval(int round)
{
  uint64 last_held = held_from[0];
  uint64 first_verified = verified_at[0];
  for (int hart = 1; hart < PAGESTRESS_CPUS; hart++) {
    if (held_from[hart] > last_held)
      last_held = held_from[hart];
    if (verified_at[hart] < first_verified)
      first_verified = verified_at[hart];
  }
  if (first_verified < last_held)
    panic("P05: no common hold interval");

  printk("P05|phase=hold|round=%d|harts=%d|pages=%d|overlap_ticks=%lu\n", round,
         PAGESTRESS_CPUS, PAGESTRESS_CPUS * PAGES_PER_HART,
         first_verified - last_held);
}

// All QEMU harts run this once after their boot setup, before any scheduler.
void
pagestress(void)
{
  int hart = cpuid();
  if (hart < 0 || hart >= PAGESTRESS_CPUS)
    panic("P05: unexpected hart");

  if (hart == 0) {
    free_before = kfreepage_count();
    printk("P05|phase=begin|rev=%s|cpus=%d|rounds=%d|pages_per_hart=%d"
           "|inject=%d|free_before=%lu\n",
           PAGESTRESS_REV, PAGESTRESS_CPUS, PAGESTRESS_ROUNDS, PAGES_PER_HART,
           PAGESTRESS_INJECT, free_before);
  }

  int target = 0;
  printk("P05|phase=ready|hart=%d\n", hart);
  wait_for_harts(&target, "start", 0, hart);

  for (int round = 0; round < PAGESTRESS_ROUNDS; round++) {
    int remote = round == 0 && PAGESTRESS_CPUS > 1;
    for (int slot = 0; slot < PAGES_PER_HART; slot++) {
      uchar *page = kalloc();
      uint64 addr = (uint64)page;
      if (page == 0 || (addr % PGSIZE) != 0 || addr < (uint64)end ||
          addr >= PHYSTOP) {
        printk("P05|phase=alloc-fail|round=%d|hart=%d|slot=%d|page=%p\n", round,
               hart, slot, (void *)page);
        panic("P05: invalid allocation");
      }
      pages[hart][slot] = page;
    }
    held_from[hart] = r_time();
    wait_for_harts(&target, "allocated", round, hart);

    if (hart == 0) {
      if (find_duplicate(0))
        panic("P05: duplicate live page");
      if (round == 0) {
        uint64 held = kfreepage_count();
        if (held + PAGESTRESS_CPUS * PAGES_PER_HART != free_before)
          panic("P05: unexpected free count while pages are held");
        printk("P05|phase=count-held|free=%lu\n", held);
        if (PAGESTRESS_INJECT) {
          if (!find_duplicate(1))
            panic("P05: duplicate injection not detected");
          printk("P05|phase=inject-check|case=duplicate|result=DETECTED\n");
        }
      }
    }
    // Complete the uniqueness check before anyone writes a test pattern.
    wait_for_harts(&target, "unique", round, hart);

    for (int slot = 0; slot < PAGES_PER_HART; slot++)
      memset(pages[hart][slot], page_pattern(round, hart, slot), PGSIZE);
    wait_for_harts(&target, "filled", round, hart);

    for (int slot = 0; slot < PAGES_PER_HART; slot++)
      verify_pattern(pages[hart][slot], page_pattern(round, hart, slot), round,
                     hart, slot);
    verified_at[hart] = r_time();
    if (remote && hart == 0)
      printk("P05|phase=handoff|from=0|to=1|page=%p\n", (void *)pages[0][0]);

    // For the remote round, ownership of hart 0's first page moves to hart 1
    // at this barrier. The original owner skips it in the free loop.
    wait_for_harts(&target, "verified", round, hart);
    if (hart == 0 && (round == 0 || round == PAGESTRESS_ROUNDS - 1))
      log_held_interval(round);

    for (int slot = remote && hart == 0 ? 1 : 0; slot < PAGES_PER_HART; slot++)
      kfree(pages[hart][slot]);

    if (remote && hart == 1) {
      uchar *page = pages[0][0];
      verify_pattern(page, page_pattern(round, 0, 0), round, hart, 0);
      kfree(page);
      printk(
        "P05|phase=remote-free|alloc_hart=0|free_hart=1|page=%p|result=PASS\n",
        (void *)page);
    }
    wait_for_harts(&target, "freed", round, hart);
  }

  // Peers wait here until hart 0 finishes counting. The scheduler can start
  // only after this measurement, so user processes do not affect it.
  if (hart == 0) {
    uint64 free_after = kfreepage_count();
    if (free_after != free_before) {
      printk("P05|phase=end|free_before=%lu|free_after=%lu|result=FAIL\n",
             free_before, free_after);
      panic("P05: free page count mismatch");
    }
    printk("P05|phase=end|cpus=%d|rounds=%d|allocations=%d|remote_frees=%d"
           "|free_before=%lu|free_after=%lu|result=PASS\n",
           PAGESTRESS_CPUS, PAGESTRESS_ROUNDS,
           PAGESTRESS_CPUS * PAGES_PER_HART * PAGESTRESS_ROUNDS,
           PAGESTRESS_CPUS > 1, free_before, free_after);
  }
  wait_for_harts(&target, "finish", PAGESTRESS_ROUNDS, hart);
}
