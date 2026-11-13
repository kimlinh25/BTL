// T03: harness kiem thu tang object (kmalloc/kmfree), hop dong slab64/128.
// Chay tren CPU 0 ngay sau slab_test(), truoc khi cac CPU khac khoi dong.
// Khong goi lai kmalloc_init(). Cache co the da giu san trang slab trong
// do slab_test() tao ra, nen harness chi so voi so dem luc bat dau.
// Bat bang: make qemu SLABTEST=1
//   SLABTEST_INJECT=1: gia lap loi logic (trung, ghi de, ro trang)
//   SLABTEST_INJECT=2: gia lap kmalloc tra NULL/dia chi lech/object trung
//                      de chung minh harness chan ca khong an toan
//   SLABTEST_INJECT=3: gia lap kmalloc tra NULL/dia chi sai/object trung
//                      khi cap lai trong O04b
//   SLABTEST_INJECT=4: gia lap stash chua object dang so huu, de chung minh
//                      buoc don khong free trung
// Moi dong log bat dau bang "KT|" de loc bang grep.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#ifndef ST_REV
#define ST_REV "unknown"
#endif
#ifndef ST_CPUS
#define ST_CPUS 0
#endif

#define ST_CONTRACT "slab64/128"
#define ST_MAGIC    0x534C4142U
#define ST_HDR      32  // kich thuoc header slab (S01)
#define ST_N        200 // so object song cung luc, tron hai class
#define ST_CYCLES   5   // so chu ky do sau warm-up (O06)
#define ST_NSTASH   8   // so o cat object that bi thay the khi inject

extern char end[]; // dia chi dau tien sau kernel (kernel.ld)

// Bang bien theo hop dong hien tai: obj_size cua class phai nhan size do,
// 0 = kmalloc phai tra 0.
static struct {
  uint64 size;
  uint64 cls;
} st_bounds[] = {{0, 0},     {1, 64},    {64, 64}, {65, 128},
                 {127, 128}, {128, 128}, {129, 0}};

// Kich thuoc yeu cau cua workload, lap vong theo chi so object:
// 6 size class64 va 4 size class128, nen ST_N object co ca hai class song.
static uint64 st_sizes[] = {1, 8, 13, 32, 63, 64, 65, 100, 127, 128};
#define ST_NSIZES (sizeof(st_sizes) / sizeof(st_sizes[0]))

static int st_pass, st_fail;
static int st_inject;
static int st_unsafe;             // 1 = co object sai/trung, khong duoc ghi
static int st_dupfree;            // so lan buoc don chan free trung
static char *st_obj[ST_N];        // dia chi kmalloc tra ve (co the sai)
static uint64 st_sz[ST_N];        // kich thuoc da yeu cau
static int st_own[ST_N];          // 1 = object hop le, duy nhat, dang so huu
static int st_new[ST_N];          // 1 = object vua duoc cap lai trong O04
static char *st_stash[ST_NSTASH]; // object that bi thay the, tra khi don

// In ket qua 1 ca: so actual voi want, dem PASS/FAIL
static void
st_report(char *id, char *expected, uint64 actual, uint64 want, int ok)
{
  if (ok)
    st_pass++;
  else
    st_fail++;
  printk("KT|case=%s|expected=%s|want=%lu|actual=%lu|result=%s\n", id, expected,
         want, actual, ok ? "PASS" : "FAIL");
}

// Ca bi bo qua vi dau vao khong an toan: tinh la FAIL
static void
st_skip(char *id)
{
  st_fail++;
  printk("KT|case=%s|expected=safe_input|result=FAIL|note=skipped_unsafe\n",
         id);
}

// Object hop le: khac 0, can le 8B, [p, p+size) nam trong RAM, trang co
// magic SLAB, dung bien slot (header 32B + k*obj_size, k < capacity) va
// size <= obj_size. So sanh dang PHYSTOP - p de p + size khong bi tran;
// chi doc header sau khi biet dia chi nam trong RAM.
static int
st_valid(char *p, uint64 size)
{
  uint64 a = (uint64)p;
  if (p == 0 || (a % 8) != 0)
    return 0;
  if (a < PGROUNDUP((uint64)end) || a >= PHYSTOP || size > PHYSTOP - a)
    return 0;
  char *pg = (char *)PGROUNDDOWN(a);
  uint32 magic = *(uint32 *)pg;
  uint16 objsz = *(uint16 *)(pg + 4);
  uint16 cap = *(uint16 *)(pg + 6);
  if (magic != ST_MAGIC || objsz == 0 || size > objsz || p < pg + ST_HDR)
    return 0;
  uint64 off = (uint64)(p - pg - ST_HDR);
  return (off % objsz) == 0 && (off / objsz) < cap;
}

// obj_size cua class phai nhan size theo hop dong: 1..64 -> 64, 65..128 -> 128
static uint64
st_want(uint64 size)
{
  return size <= 64 ? 64 : 128;
}

// obj_size ghi trong header trang chua p; chi goi khi st_valid(p) dung
static uint64
st_objsz(char *p)
{
  return *(uint16 *)((char *)PGROUNDDOWN((uint64)p) + 4);
}

// Object i duoc so huu: hop le va nam dung class theo size da xin
static int
st_ok(int i)
{
  return st_valid(st_obj[i], st_sz[i]) &&
         st_objsz(st_obj[i]) == st_want(st_sz[i]);
}

// 1 neu vung [a, a+sa) chong len [b, b+sb)
static int
st_overlap(char *a, uint64 sa, char *b, uint64 sb)
{
  return a < b + sb && b < a + sa;
}

// 1 neu [p, p+sz) chong len mot object dang so huu (tru object skip)
static int
st_hit_owned(char *p, uint64 sz, int skip)
{
  for (int j = 0; j < ST_N; j++)
    if (j != skip && st_own[j] && st_overlap(p, sz, st_obj[j], st_sz[j]))
      return 1;
  return 0;
}

// 1 neu [p, p+sz) chong len mot object trong stash
static int
st_hit_stash(char *p, uint64 sz)
{
  for (int k = 0; k < ST_NSTASH; k++)
    if (st_stash[k] != 0 && st_overlap(p, sz, st_stash[k], 1))
      return 1;
  return 0;
}

// 1 neu object i chong len object dang so huu khac hoac object trong stash
static int
st_clash(int i)
{
  return st_hit_owned(st_obj[i], st_sz[i], i) ||
         st_hit_stash(st_obj[i], st_sz[i]);
}

// Cat object that vao stash de tra khi don dep
static void
st_stash_put(char *p)
{
  for (int k = 0; k < ST_NSTASH; k++)
    if (st_stash[k] == 0) {
      st_stash[k] = p;
      return;
    }
}

// Byte mau cua object i tai vi tri j
static uchar
st_byte(int i, int j)
{
  return (uchar)(0x40 + i * 7 + j);
}

// B01/B02: kich thuoc bien theo hop dong; object cap duoc phai nam dung class
// (actual = obj_size, 0 = NULL, 1 = con tro sai) va duoc tra ngay
static void
st_bounds_test(void)
{
  for (int i = 0; i < sizeof(st_bounds) / sizeof(st_bounds[0]); i++) {
    uint64 sz = st_bounds[i].size;
    char *p = kmalloc(sz);
    uint64 cls = st_bounds[i].cls;
    uint64 got = p == 0 ? 0 : st_valid(p, sz) ? st_objsz(p) : 1;
    if (got == cls)
      st_pass++;
    else
      st_fail++;
    printk("KT|case=B01-size-%lu|expected=%s|want=%lu|actual=%lu|result=%s\n",
           sz, cls ? "class_obj_size" : "null", cls, got,
           got == cls ? "PASS" : "FAIL");
    if (st_valid(p, sz))
      kmfree(p);
  }
  kmfree(0); // hop dong: no-op; neu sai kernel panic, khong toi duoc dong duoi
  st_report("B02-kmfree-null", "no_panic", 1, 1, 1);
}

// V01: st_valid phai tu choi moi con tro xau ma khong doc header cua no.
// Con tro gan 2^64 la ca tai hien tran so: p + size quay vong ve so nho.
static void
st_valid_test(void)
{
  if (!st_own[0]) {
    st_skip("V01-valid-rejects-bad");
    return;
  }
  char *p = st_obj[0];
  char *pg = (char *)PGROUNDDOWN((uint64)p);
  struct {
    char *p;
    uint64 sz;
  } bad[] = {
    {0, 1},
    {p + 4, 1},
    {p + 8, 1},
    {pg + 8, 1},
    {(char *)KERNBASE, 1},
    {(char *)PHYSTOP, 1},
    {(char *)(PHYSTOP - 8), 64},
    {(char *)0xFFFFFFFFFFFFFFF8UL, 64},
  };
  int n = sizeof(bad) / sizeof(bad[0]);
  int rejected = 0;
  for (int k = 0; k < n; k++)
    rejected += !st_valid(bad[k].p, bad[k].sz);
  st_report("V01-valid-rejects-bad", "rejected", rejected, n,
            rejected == n && st_valid(p, st_sz[0]));
}

// inject=2: thay 3 dia chi bang NULL, object trung, dia chi lech.
// Object that duoc cat vao stash de tra lai khi don dep.
static void
st_corrupt(void)
{
  st_stash_put(st_obj[2]);
  st_obj[2] = 0; // gia lap kmalloc tra NULL
  st_stash_put(st_obj[4]);
  st_obj[4] = st_obj[3]; // gia lap cap trung object 3
  st_sz[4] = st_sz[3];
  st_stash_put(st_obj[6]);
  st_obj[6] = st_obj[6] + 8; // gia lap dia chi lech khoi bien slot
}

// O01: moi object cap ra phai hop le va dung class; object dat duoc so huu
static void
st_alloc_valid(void)
{
  int good = 0;
  for (int i = 0; i < ST_N; i++) {
    st_sz[i] = st_sizes[i % ST_NSIZES];
    st_obj[i] = kmalloc(st_sz[i]);
  }
  if (st_inject == 2)
    st_corrupt();
  for (int i = 0; i < ST_N; i++) {
    st_own[i] = st_ok(i);
    good += st_own[i];
  }
  if (good != ST_N)
    st_unsafe = 1;
  st_report("O01-valid-align8", "valid_slot", good, ST_N, good == ST_N);
}

// O02: object song khong trung, vung [p, p+size) khong chong lan, ke ca voi
// object trong stash. Object chong lan bi bo so huu (giu ban dau tien) de
// khong bi free 2 lan.
static void
st_unique(void)
{
  uint64 dup = 0;
  for (int i = 0; i < ST_N; i++) {
    if (st_own[i] && st_hit_stash(st_obj[i], st_sz[i])) {
      dup++;
      st_own[i] = 0;
    }
    for (int j = i + 1; j < ST_N; j++)
      if (st_own[i] && st_own[j] &&
          st_overlap(st_obj[i], st_sz[i], st_obj[j], st_sz[j])) {
        dup++;
        st_own[j] = 0;
      }
  }
  if (dup > 0)
    st_unsafe = 1;
  if (st_inject == 1)
    dup++; // gia lap phat hien chong lan, khong dung vao object that
  st_report("O02-unique-no-overlap", "overlap_pairs", dup, 0, dup == 0);
}

// O03: ghi mau rieng theo dung size yeu cau, doc lai sau khi ghi het,
// va header slab cua moi object van con nguyen.
// Bo qua neu co object sai/trung vi ghi vao do co the lam hong kernel.
static void
st_pattern(void)
{
  if (st_unsafe) {
    st_skip("O03-pattern-rw");
    return;
  }
  int good = 0;
  for (int i = 0; i < ST_N; i++)
    for (int j = 0; j < st_sz[i]; j++)
      st_obj[i][j] = st_byte(i, j);
  if (st_inject == 1)
    st_obj[3][0] ^= 0xFF; // gia lap ghi de, chi trong vung cua object 3
  for (int i = 0; i < ST_N; i++) {
    int ok = st_ok(i);
    for (int j = 0; ok && j < st_sz[i]; j++)
      if ((uchar)st_obj[i][j] != st_byte(i, j))
        ok = 0;
    good += ok;
  }
  if (st_inject == 1)
    st_obj[3][0] ^= 0xFF; // tra lai byte da dao o tren
  st_report("O03-pattern-rw", "objects_intact", good, ST_N, good == ST_N);
}

// 1 neu object i con dung mau st_byte
static int
st_intact(int i)
{
  for (int j = 0; j < st_sz[i]; j++)
    if ((uchar)st_obj[i][j] != st_byte(i, j))
      return 0;
  return 1;
}

// inject=3: gia lap lan cap lai thu i trong O04b tra NULL, dia chi sai
// hoac object dang song. Object that duoc cat vao stash.
static char *
st_realloc_fault(int i, char *p)
{
  if (st_inject != 3 || i > 12)
    return p;
  st_stash_put(p);
  if (i == 10)
    return 0;
  if (i == 11)
    return p + 8;
  return st_obj[30];
}

// O04: tra roi cap lai cung size.
// a) moi class: object cap sau cung cua class do (nam o trang slab dau tien
//    con slot trong cua class) tra ve phai duoc cap lai dung slot do (LIFO)
// b) tra 10 object cua ca hai class roi cap lai: moi object cap lai phai hop
//    le, dung class, khong chong lan object khac, va so trang trong khong doi
// c) sau b: ghi mau vao object cap lai, roi kiem mau cua moi object so huu
// Object cap lai chi duoc so huu neu hop le va khong chong lan object khac.
static void
st_reuse(void)
{
  int same = 0;
  for (uint64 cls = 64; cls <= 128; cls += 64) {
    int L = ST_N - 1;
    while (L >= 0 && st_want(st_sz[L]) != cls)
      L--;
    if (L < 0 || !st_own[L])
      continue;
    char *a = st_obj[L];
    kmfree(a);
    st_obj[L] = kmalloc(st_sz[L]);
    st_own[L] = st_ok(L) && !st_clash(L);
    st_new[L] = st_own[L];
    same += st_obj[L] == a;
  }
  st_report("O04a-reuse-same-slot", "classes_same_addr", same, 2, same == 2);

  int tried = 0, good = 0;
  uint64 before = kfreepage_count();
  for (int i = 10; i < 20; i++)
    if (st_own[i]) {
      kmfree(st_obj[i]);
      st_own[i] = 0;
      st_obj[i] = st_realloc_fault(i, kmalloc(st_sz[i]));
      st_own[i] = st_ok(i) && !st_clash(i);
      st_new[i] = st_own[i];
      tried++;
      good += st_own[i];
    }
  uint64 after = kfreepage_count();
  printk("KT|info|o04b_free_before=%lu|o04b_free_after=%lu\n", before, after);
  st_report("O04b-reuse-valid-no-new-page", "realloc_owned", good, tried,
            tried > 0 && good == tried && after == before);

  if (st_unsafe) {
    st_skip("O04c-pattern-after-realloc");
    return;
  }
  int intact = 0, own = 0;
  for (int i = 0; i < ST_N; i++) {
    if (!st_own[i])
      continue;
    if (st_new[i])
      for (int j = 0; j < st_sz[i]; j++)
        st_obj[i][j] = st_byte(i, j);
    own++;
    intact += st_intact(i);
  }
  st_report("O04c-pattern-after-realloc", "owned_intact", intact, own,
            intact == own);
}

// O05: workload vuot 1 slab o ca hai class. Capacity lay tu header trang
// (hien tai 63 cho class64, 31 cho class128), khong viet cung.
// a) moi class: so object song > capacity, nam tren it nhat ceil(n/cap)
//    trang, va khong trang nao giu qua capacity object
// b) so trang lay them tu kalloc khong vuot tong ceil(n/cap) cua hai class
//    (cache co the da co trang trong cua slab_test, nen co the lay it hon)
static void
st_span(uint64 before, uint64 mid)
{
  static char *pages[ST_N];
  static int cnt[ST_N];
  uint64 need_all = 0;
  for (uint64 cls = 64; cls <= 128; cls += 64) {
    int npg = 0, n = 0, over = 0, cap = 0;
    for (int i = 0; i < ST_N; i++) {
      if (!st_own[i] || st_want(st_sz[i]) != cls)
        continue;
      n++;
      char *pg = (char *)PGROUNDDOWN((uint64)st_obj[i]);
      cap = *(uint16 *)(pg + 6);
      int k = 0;
      while (k < npg && pages[k] != pg)
        k++;
      if (k == npg) {
        pages[npg] = pg;
        cnt[npg++] = 0;
      }
      cnt[k]++;
    }
    for (int k = 0; k < npg; k++)
      if (cnt[k] > *(uint16 *)(pages[k] + 6))
        over++;
    int need = cap ? (n + cap - 1) / cap : 0;
    need_all += need;
    printk("KT|info|o05_class=%lu|objects=%d|capacity=%d|pages=%d\n", cls, n,
           cap, npg);
    st_report(cls == 64 ? "O05a-span-pages-c64" : "O05a-span-pages-c128",
              "pages>=ceil(n/cap)", npg, need,
              n > cap && npg >= need && over == 0);
  }
  uint64 taken = before - mid;
  st_report("O05b-new-pages", "taken<=sum_ceil(n/cap)", taken, need_all,
            mid <= before && taken <= need_all);
}

// Don dep: tra object trong stash truoc, roi object dang so huu.
// Object trong stash chi duoc tra khi hop le, khong trung object dang so huu
// va khong lap lai trong stash; neu khong thi bo qua va dem vao st_dupfree.
static void
st_release(void)
{
  for (int k = 0; k < ST_NSTASH; k++) {
    char *s = st_stash[k];
    st_stash[k] = 0;
    if (!st_valid(s, 1))
      continue;
    if (st_hit_owned(s, 1, -1) || st_hit_stash(s, 1)) {
      st_dupfree++;
      continue;
    }
    kmfree(s);
  }
  for (int i = 0; i < ST_N; i++) {
    if (st_own[i])
      kmfree(st_obj[i]);
    st_own[i] = 0;
    st_new[i] = 0;
  }
}

// 1 chu ky workload: cap ST_N object, kiem hop le + khong chong lan, tra het.
// Tra ve so object khong so huu duoc (0 la dung).
static int
st_cycle(void)
{
  int bad = 0;
  for (int i = 0; i < ST_N; i++) {
    st_sz[i] = st_sizes[i % ST_NSIZES];
    st_obj[i] = kmalloc(st_sz[i]);
    st_own[i] = st_ok(i) && !st_clash(i);
    bad += !st_own[i];
  }
  st_release();
  return bad;
}

// O06: sau 1 chu ky warm-up, cac chu ky cung workload khong lam so trang
// trong giam them. Khong doi so trang ve muc truoc lan cap dau: trang slab
// trong duoc giu lai trong cache, do khong phai leak.
static void
st_stable(void)
{
  char *held = 0;
  int bad = st_cycle(); // warm-up
  uint64 base = kfreepage_count();
  uint64 grew = 0, low = base;
  for (int c = 0; c < ST_CYCLES && bad == 0; c++) {
    bad += st_cycle();
    if (st_inject == 1 && c == 2)
      held = kalloc(); // gia lap ro 1 trang trong chu ky 3
    uint64 now = kfreepage_count();
    if (now != base)
      grew++;
    if (now < low)
      low = now;
  }
  printk("KT|info|o06_base=%lu|o06_low=%lu|cycles=%d|bad=%d\n", base, low,
         ST_CYCLES, bad);
  if (bad)
    st_skip("O06-stable-after-warmup");
  else
    st_report("O06-stable-after-warmup", "cycles_with_page_change", grew, 0,
              grew == 0);
  if (held)
    kfree(held); // tra lai de he thong van dung
}

void
slabtest(int inject)
{
  st_pass = st_fail = 0;
  st_inject = inject;
  st_unsafe = 0;
  st_dupfree = 0;
  uint64 before = kfreepage_count();
  printk("KT|begin|harness=object|rev=%s|cpus=%d|inject=%d|contract=%s|n=%d|"
         "free_before=%lu\n",
         ST_REV, ST_CPUS, inject, ST_CONTRACT, ST_N, before);
  st_bounds_test();
  st_alloc_valid();
  uint64 mid = kfreepage_count();
  st_valid_test();
  st_unique();
  st_pattern();
  st_reuse();
  st_span(before, mid);
  if (st_inject == 4 && st_own[5]) {
    st_stash_put(st_obj[5]); // gia lap stash giu object dang so huu
    st_stash_put(st_obj[5]); // va giu lap 2 lan
  }
  st_release();
  st_report("C01-cleanup-no-double-free", "dup_skipped", st_dupfree, 0,
            st_dupfree == 0);
  st_stable();
  printk("KT|end|harness=object|pass=%d|fail=%d|free_after=%lu|summary=%s\n",
         st_pass, st_fail, kfreepage_count(),
         st_fail == 0 ? "ALL_PASS" : "HAS_FAIL");
}
