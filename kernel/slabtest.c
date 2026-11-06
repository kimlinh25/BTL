// T03: harness kiem thu tang object (kmalloc/kmfree), hop dong slab64.
// Chay tren CPU 0 ngay sau slab_test(), truoc khi cac CPU khac khoi dong.
// Khong goi lai kmalloc_init(). Cache co the da giu san trang slab trong
// do slab_test() tao ra, nen harness chi so voi so dem luc bat dau.
// Bat bang: make qemu SLABTEST=1
//   SLABTEST_INJECT=1: gia lap loi logic (trung, ghi de, ro trang)
//   SLABTEST_INJECT=2: gia lap kmalloc tra NULL/dia chi lech/object trung
//                      de chung minh harness chan ca khong an toan
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

#define ST_CONTRACT "slab64"
#define ST_MAGIC    0x534C4142U
#define ST_HDR      32  // kich thuoc header slab (S01)
#define ST_N        200 // so object song cung luc: hon 3 trang 63 slot
#define ST_CYCLES   5   // so chu ky do sau warm-up (O06)

extern char end[]; // dia chi dau tien sau kernel (kernel.ld)

// Bang bien theo hop dong hien tai: 1 = phai cap duoc, 0 = phai tra 0.
static struct {
  uint64 size;
  int ok;
} st_bounds[] = {{0, 0}, {1, 1}, {64, 1}, {65, 0}, {128, 0}, {129, 0}};

// Kich thuoc yeu cau cua workload, lap vong theo chi so object
static uint64 st_sizes[] = {1, 8, 13, 32, 63, 64};
#define ST_NSIZES (sizeof(st_sizes) / sizeof(st_sizes[0]))

static int st_pass, st_fail;
static int st_inject;
static int st_unsafe;      // 1 = co object sai/trung, khong duoc ghi
static char *st_obj[ST_N]; // dia chi kmalloc tra ve (co the sai khi inject=2)
static uint64 st_sz[ST_N]; // kich thuoc da yeu cau
static int st_own[ST_N];   // 1 = object hop le, duy nhat, test dang so huu
static char *st_stash[3];  // inject=2: object that bi thay the, tra khi don

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

// Object hop le: khac 0, can le 8B, nam trong RAM, trang co magic SLAB,
// dung bien slot (header 32B + k*obj_size, k < capacity) va size <= obj_size.
// Chi doc header sau khi biet dia chi nam trong RAM.
static int
st_valid(char *p, uint64 size)
{
  if (p == 0 || ((uint64)p % 8) != 0)
    return 0;
  if (p < (char *)PGROUNDUP((uint64)end) || (uint64)p + size > PHYSTOP)
    return 0;
  char *pg = (char *)PGROUNDDOWN((uint64)p);
  uint32 magic = *(uint32 *)pg;
  uint16 objsz = *(uint16 *)(pg + 4);
  uint16 cap = *(uint16 *)(pg + 6);
  if (magic != ST_MAGIC || objsz == 0 || size > objsz || p < pg + ST_HDR)
    return 0;
  uint64 off = (uint64)(p - pg - ST_HDR);
  return (off % objsz) == 0 && (off / objsz) < cap;
}

// 1 neu vung [a, a+sa) chong len [b, b+sb)
static int
st_overlap(char *a, uint64 sa, char *b, uint64 sb)
{
  return a < b + sb && b < a + sa;
}

// 1 neu object i chong len mot object dang so huu khac
static int
st_clash(int i)
{
  for (int j = 0; j < ST_N; j++)
    if (j != i && st_own[j] &&
        st_overlap(st_obj[i], st_sz[i], st_obj[j], st_sz[j]))
      return 1;
  return 0;
}

// Byte mau cua object i tai vi tri j
static uchar
st_byte(int i, int j)
{
  return (uchar)(0x40 + i * 7 + j);
}

// B01/B02: kich thuoc bien theo hop dong; object cap duoc thi tra ngay
static void
st_bounds_test(void)
{
  for (int i = 0; i < sizeof(st_bounds) / sizeof(st_bounds[0]); i++) {
    uint64 sz = st_bounds[i].size;
    char *p = kmalloc(sz);
    int got = p != 0;
    int ok = st_bounds[i].ok ? st_valid(p, sz) : !got;
    if (ok)
      st_pass++;
    else
      st_fail++;
    printk("KT|case=B01-size-%lu|expected=%s|want=%d|actual=%d|result=%s\n", sz,
           st_bounds[i].ok ? "valid" : "null", st_bounds[i].ok, got,
           ok ? "PASS" : "FAIL");
    if (st_valid(p, sz))
      kmfree(p);
  }
  kmfree(0); // hop dong: no-op; neu sai kernel panic, khong toi duoc dong duoi
  st_report("B02-kmfree-null", "no_panic", 1, 1, 1);
}

// inject=2: thay 3 dia chi bang NULL, object trung, dia chi lech.
// Object that duoc cat vao st_stash de tra lai khi don dep.
static void
st_corrupt(void)
{
  st_stash[0] = st_obj[2];
  st_obj[2] = 0; // gia lap kmalloc tra NULL
  st_stash[1] = st_obj[4];
  st_obj[4] = st_obj[3]; // gia lap cap trung object 3
  st_sz[4] = st_sz[3];
  st_stash[2] = st_obj[6];
  st_obj[6] = st_stash[2] + 8; // gia lap dia chi lech khoi bien slot
}

// O01: moi object cap ra phai hop le; object hop le duoc danh dau so huu
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
    st_own[i] = st_valid(st_obj[i], st_sz[i]);
    good += st_own[i];
  }
  if (good != ST_N)
    st_unsafe = 1;
  st_report("O01-valid-align8", "valid_slot", good, ST_N, good == ST_N);
}

// O02: object song khong trung, vung [p, p+size) khong chong lan.
// Object chong lan bi bo so huu (giu ban dau tien) de khong bi free 2 lan.
static void
st_unique(void)
{
  uint64 dup = 0;
  for (int i = 0; i < ST_N; i++)
    for (int j = i + 1; j < ST_N; j++)
      if (st_own[i] && st_own[j] &&
          st_overlap(st_obj[i], st_sz[i], st_obj[j], st_sz[j])) {
        dup++;
        st_own[j] = 0;
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
    int ok = st_valid(st_obj[i], st_sz[i]);
    for (int j = 0; ok && j < st_sz[i]; j++)
      if ((uchar)st_obj[i][j] != st_byte(i, j))
        ok = 0;
    good += ok;
  }
  st_report("O03-pattern-rw", "objects_intact", good, ST_N, good == ST_N);
}

// O04: tra roi cap lai cung size.
// a) object cap sau cung (nam o trang slab dau tien con slot trong) tra ve
//    phai duoc cap lai dung slot do (LIFO)
// b) tra 10 object roi cap lai 10: so trang trong khong doi
// Object cap lai chi duoc so huu neu hop le va khong chong lan object khac.
static void
st_reuse(void)
{
  int L = ST_N - 1;
  if (!st_own[L]) {
    st_skip("O04a-reuse-same-slot");
  } else {
    char *a = st_obj[L];
    kmfree(a);
    st_obj[L] = kmalloc(st_sz[L]);
    st_own[L] = st_valid(st_obj[L], st_sz[L]) && !st_clash(L);
    st_report("O04a-reuse-same-slot", "same_addr", (uint64)st_obj[L], (uint64)a,
              st_obj[L] == a);
  }

  uint64 before = kfreepage_count();
  for (int i = 10; i < 20; i++)
    if (st_own[i]) {
      kmfree(st_obj[i]);
      st_own[i] = 0;
      st_obj[i] = kmalloc(st_sz[i]);
      st_own[i] = st_valid(st_obj[i], st_sz[i]) && !st_clash(i);
    }
  uint64 after = kfreepage_count();
  st_report("O04b-reuse-no-new-page", "free_pages_same", after, before,
            after == before);
}

// O05: workload vuot 1 slab.
// a) object song nam tren it nhat ceil(n/63) trang, khong trang nao qua 63
// b) so trang lay them tu kalloc khong vuot ceil(N/63) (cache co the da co
//    trang trong cua slab_test, nen co the lay it hon, ke ca 0)
static void
st_span(uint64 before, uint64 mid)
{
  static char *pages[ST_N];
  static int cnt[ST_N];
  int npg = 0, n = 0, over = 0;
  for (int i = 0; i < ST_N; i++) {
    if (!st_own[i])
      continue;
    n++;
    char *pg = (char *)PGROUNDDOWN((uint64)st_obj[i]);
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
  int need = (n + 62) / 63;
  st_report("O05a-span-pages", "pages>=ceil(n/63)", npg, need,
            npg >= need && over == 0);
  uint64 taken = before - mid;
  st_report("O05b-new-pages", "taken<=ceil(N/63)", taken, (ST_N + 62) / 63,
            mid <= before && taken <= (ST_N + 62) / 63);
}

// Tra moi object dang so huu, va object that bi thay the khi inject=2
static void
st_release(void)
{
  for (int i = 0; i < ST_N; i++)
    if (st_own[i]) {
      kmfree(st_obj[i]);
      st_own[i] = 0;
    }
  for (int i = 0; i < 3; i++) {
    if (st_valid(st_stash[i], 1))
      kmfree(st_stash[i]);
    st_stash[i] = 0;
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
    st_own[i] = st_valid(st_obj[i], st_sz[i]) && !st_clash(i);
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
  uint64 before = kfreepage_count();
  printk("KT|begin|harness=object|rev=%s|cpus=%d|inject=%d|contract=%s|n=%d|"
         "free_before=%lu\n",
         ST_REV, ST_CPUS, inject, ST_CONTRACT, ST_N, before);
  st_bounds_test();
  st_alloc_valid();
  uint64 mid = kfreepage_count();
  st_unique();
  st_pattern();
  st_reuse();
  st_span(before, mid);
  st_release();
  st_stable();
  printk("KT|end|harness=object|pass=%d|fail=%d|free_after=%lu|summary=%s\n",
         st_pass, st_fail, kfreepage_count(),
         st_fail == 0 ? "ALL_PASS" : "HAS_FAIL");
}
