// T02 / N03: harness kiem thu tang trang (kalloc/kfree).
// Chay tren CPU 0 ngay sau kinit(), truoc khi cac CPU khac khoi dong,
// nen khong co ai khac cap phat: so trang dem duoc la chinh xac.
// Bat bang: make qemu KALLOCTEST=1
//   KALLOCTEST_INJECT=1: gia lap loi de chung minh cac ca bat duoc loi
//   KALLOCTEST_INJECT=2: gia lap allocator tra NULL/dia chi sai/trang trung
//                        de chung minh harness chan ca khong an toan
//   KALLOCTEST_INJECT=3: gia lap kalloc tra lai trang da cap trong P07
//   KALLOCTEST_INJECT=4: gia lap kalloc khong tra 0 khi het ma tra lai trang
//                        da co trong bitmap (P07c)
// Moi dong log bat dau bang "KT|" de loc bang grep.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#ifndef KT_REV
#define KT_REV "unknown"
#endif
#ifndef KT_CPUS
#define KT_CPUS 0
#endif

#define KT_N 64 // so trang song cung luc trong cac ca P01-P05

extern char end[]; // dia chi dau tien sau kernel (kernel.ld)

static int kt_pass, kt_fail;
static int kt_inject;
static int kt_unsafe;        // 1 = co trang sai/trung, khong duoc ghi vao trang
static char *kt_pages[KT_N]; // dia chi kalloc tra ve (co the sai khi inject=2)
static int kt_own[KT_N];     // 1 = trang hop le, duy nhat, test dang so huu
static char *kt_stash[3];    // inject=2: trang that bi thay the, tra khi don

// P07 ghi nho trang da cap bang bitmap ngoai trang (1 bit/trang), khong ghi
// gi vao chinh trang: kalloc co the ghi de trang, va trang trung se tao vong.
#define KT_NPAGES ((PHYSTOP - KERNBASE) / PGSIZE)
static uchar kt_bm[KT_NPAGES / 8];

// 1 neu trang hop le p da duoc danh dau trong bitmap
static int
kt_bm_has(char *p)
{
  uint64 idx = ((uint64)p - KERNBASE) / PGSIZE;
  return (kt_bm[idx / 8] & (1 << (idx % 8))) != 0;
}

// Dia chi hop le: khac 0, chia het PGSIZE, nam trong [PGROUNDUP(end), PHYSTOP)
static int
kt_valid(char *p)
{
  return p != 0 && ((uint64)p % PGSIZE) == 0 &&
         p >= (char *)PGROUNDUP((uint64)end) && (uint64)p < PHYSTOP;
}

// In ket qua 1 ca: so actual voi want, dem PASS/FAIL
static void
kt_report(char *id, char *expected, uint64 actual, uint64 want, int ok)
{
  if (ok)
    kt_pass++;
  else
    kt_fail++;
  printk("KT|case=%s|expected=%s|want=%lu|actual=%lu|result=%s\n", id, expected,
         want, actual, ok ? "PASS" : "FAIL");
}

// Ca bi bo qua vi dau vao khong an toan: tinh la FAIL
static void
kt_skip(char *id)
{
  kt_fail++;
  printk("KT|case=%s|expected=safe_input|result=FAIL|note=skipped_unsafe\n",
         id);
}

// inject=2: thay 3 dia chi bang NULL, dia chi lech, trang trung.
// Trang that duoc cat vao kt_stash de tra lai khi don dep.
static void
kt_corrupt(void)
{
  kt_stash[0] = kt_pages[2];
  kt_pages[2] = 0; // gia lap kalloc tra NULL
  kt_stash[1] = kt_pages[4];
  kt_pages[4] = kt_pages[3]; // gia lap cap trung trang 3
  kt_stash[2] = kt_pages[6];
  kt_pages[6] = kt_stash[2] + 8; // gia lap dia chi lech
}

// P01: moi trang cap ra phai hop le; trang hop le duoc danh dau so huu
static void
kt_alloc_align(void)
{
  int good = 0;
  for (int i = 0; i < KT_N; i++)
    kt_pages[i] = kalloc();
  if (kt_inject == 2)
    kt_corrupt();
  for (int i = 0; i < KT_N; i++) {
    kt_own[i] = kt_valid(kt_pages[i]);
    good += kt_own[i];
  }
  if (good != KT_N)
    kt_unsafe = 1;
  kt_report("P01-align-range", "aligned_in_range", good, KT_N, good == KT_N);
}

// P02: kalloc phai lap day trang bang byte 5; chi doc trang hop le
static void
kt_junk_fill(void)
{
  int good = 0;
  for (int i = 0; i < KT_N; i++) {
    if (!kt_own[i])
      continue;
    char *p = kt_pages[i];
    int ok = 1;
    for (int j = 0; ok && j < PGSIZE; j++)
      if (p[j] != 5)
        ok = 0;
    good += ok;
  }
  kt_report("P02-junk-fill-5", "all_bytes_5", good, KT_N, good == KT_N);
}

// P03: cac trang dang song phai doi mot khac nhau.
// Trang trung bi bo so huu (giu ban dau tien) de khong bi free 2 lan.
static void
kt_unique(void)
{
  uint64 dup = 0;
  for (int i = 0; i < KT_N; i++)
    for (int j = i + 1; j < KT_N; j++)
      if (kt_own[i] && kt_own[j] && kt_pages[i] == kt_pages[j]) {
        dup++;
        kt_own[j] = 0;
      }
  if (dup > 0)
    kt_unsafe = 1;
  if (kt_inject == 1)
    dup++; // gia lap phat hien trung, khong dung vao trang that
  kt_report("P03-unique-live", "dup_pairs", dup, 0, dup == 0);
}

// P04: ghi mau rieng cho tung trang, doc lai sau khi ghi het.
// Bo qua neu co trang sai/trung vi ghi vao do co the lam hong kernel.
static void
kt_pattern(void)
{
  if (kt_unsafe) {
    kt_skip("P04-pattern-rw");
    return;
  }
  int good = 0;
  for (int i = 0; i < KT_N; i++) {
    uint64 *w = (uint64 *)kt_pages[i];
    for (int j = 0; j < PGSIZE / 8; j++)
      w[j] = ((uint64)i << 32) | (uint64)j;
  }
  if (kt_inject == 1)
    ((uint64 *)kt_pages[3])[7] = 0; // gia lap ghi de
  for (int i = 0; i < KT_N; i++) {
    uint64 *w = (uint64 *)kt_pages[i];
    int ok = 1;
    for (int j = 0; ok && j < PGSIZE / 8; j++)
      if (w[j] != (((uint64)i << 32) | (uint64)j))
        ok = 0;
    good += ok;
  }
  kt_report("P04-pattern-rw", "pages_intact", good, KT_N, good == KT_N);
}

// 1 neu p da duoc tra o P05: trung trang dang so huu, hoac trung
// kt_stash[0..n-1] (chi xay ra khi allocator tra trung that su)
static int
kt_freed(char *p, int n)
{
  for (int i = 0; i < KT_N; i++)
    if (kt_own[i] && kt_pages[i] == p)
      return 1;
  for (int i = 0; i < n; i++)
    if (kt_stash[i] == p)
      return 1;
  return 0;
}

// P05: dem trang trong: cap N thi giam N; chi tra trang dang so huu
// (va trang that bi thay the khi inject=2), roi dem lai
static void
kt_conserve(uint64 before)
{
  uint64 mid = kfreepage_count();
  kt_report("P05a-count-after-alloc", "before-N", mid, before - KT_N,
            mid == before - KT_N);

  int skip = kt_inject == 1 ? 5 : -1; // gia lap leak: tam giu lai 1 trang
  for (int i = 0; i < KT_N; i++)
    if (kt_own[i] && i != skip)
      kfree(kt_pages[i]);
  // inject=2: tra trang that bi thay the, tru khi no trung trang da tra
  for (int i = 0; i < 3; i++)
    if (kt_valid(kt_stash[i]) && !kt_freed(kt_stash[i], i))
      kfree(kt_stash[i]);
  uint64 after = kfreepage_count();
  kt_report("P05b-count-after-free", "before", after, before, after == before);
  if (skip >= 0 && kt_own[skip])
    kfree(kt_pages[skip]); // tra lai de he thong van dung
}

// P06: kfree lap day trang bang byte 1 (tru 8 byte dau la con tro next)
static void
kt_free_fill(void)
{
  char *p = kalloc();
  int ok = 0;
  if (kt_valid(p)) {
    kfree(p);
    ok = 1;
    for (int j = 8; j < PGSIZE; j++)
      if (p[j] != 1)
        ok = 0;
  }
  kt_report("P06-free-fill-1", "bytes8_end_are_1", ok, 1, ok);
}

// P07: cap den khi het (OOM). Moi trang phai hop le va chua tung duoc cap
// trong lan nay (bitmap); gap trang sai hoac trung thi dung vong lap.
// Don dep: tra moi trang co bit trong bitmap dung mot lan.
static void
kt_exhaust(uint64 before)
{
  uint64 got = 0, bad = 0, dup = 0;
  char *p, *prev = 0;
  while ((p = kalloc()) != 0) {
    char *q = p;
    if (kt_inject == 2 && got == 10)
      q = p + 8; // gia lap dia chi lech
    if (kt_inject == 3 && got == 10)
      q = prev; // gia lap kalloc tra lai trang vua cap
    if (!kt_valid(q)) {
      bad++;
      if (kt_valid(p) && !kt_bm_has(p))
        kfree(p); // trang that hop le, chua co trong bitmap: tra lai
      break;
    }
    if (kt_bm_has(q)) {
      dup++;
      if (q != p && kt_valid(p) && !kt_bm_has(p))
        kfree(p); // trang that chua co trong bitmap: tra lai
      break;
    }
    uint64 idx = ((uint64)q - KERNBASE) / PGSIZE;
    kt_bm[idx / 8] |= 1 << (idx % 8);
    prev = q;
    got++;
  }
  kt_report("P07a-oom-count", "before", got, before, got == before);
  kt_report("P07b-oom-bad-addr", "0", bad, 0, bad == 0);
  kt_report("P07e-oom-dup", "0", dup, 0, dup == 0);
  if (bad || dup) {
    kt_skip("P07c-oom-returns-0");
  } else {
    p = kalloc();
    if (kt_inject == 4)
      p = prev; // gia lap kalloc tra trang da co trong bitmap thay vi 0
    kt_report("P07c-oom-returns-0", "0", (uint64)p, 0, p == 0);
    // Chi tra trang chua co trong bitmap; trang da co se duoc tra 1 lan
    // o buoc don dep ben duoi, khong tra 2 lan.
    if (kt_valid(p) && !kt_bm_has(p))
      kfree(p);
  }
  for (uint64 idx = 0; idx < KT_NPAGES; idx++)
    if (kt_bm[idx / 8] & (1 << (idx % 8))) {
      kt_bm[idx / 8] &= ~(1 << (idx % 8));
      kfree((char *)(KERNBASE + idx * PGSIZE));
    }
  uint64 after = kfreepage_count();
  kt_report("P07d-count-after-oom", "before", after, before, after == before);
}

void
kalloctest(int inject)
{
  kt_pass = kt_fail = 0;
  kt_inject = inject;
  kt_unsafe = 0;
  uint64 before = kfreepage_count();
  printk("KT|begin|rev=%s|cpus=%d|inject=%d|n=%d|free_before=%lu\n", KT_REV,
         KT_CPUS, inject, KT_N, before);
  kt_alloc_align();
  kt_junk_fill();
  kt_unique();
  kt_pattern();
  kt_conserve(before);
  kt_free_fill();
  kt_exhaust(before);
  printk("KT|end|pass=%d|fail=%d|free_after=%lu|summary=%s\n", kt_pass, kt_fail,
         kfreepage_count(), kt_fail == 0 ? "ALL_PASS" : "HAS_FAIL");
}
