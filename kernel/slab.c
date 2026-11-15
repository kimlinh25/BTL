#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

#define SLAB_MAGIC     0x534C4142U // Chu ky "SLAB" nhan dien trang Slab
#define SLAB_ALIGN     8U          // Can le toi thieu 8 bytes
#define SLAB_NCLASSES  2           // S03a/S04a: 2 classes (64 B va 128 B)
#define SLAB_FREE_BYTE 1           // Mau byte 1 khi free (giong S02)

#ifndef SLAB_TEST_CASE
#define SLAB_TEST_CASE 0
#endif

#ifndef SLAB_STATS_TEST
#define SLAB_STATS_TEST 0
#endif

#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

extern char end[]; // Dia chi ket thuc vung nho nhan tu kernel.ld

// Bang kich thuoc chuan: class 0 = 64 B, class 1 = 128 B
static const uint16 slab_class_sizes[SLAB_NCLASSES] = {64U, 128U};

// Con tro nhung trong 8 B dau cua moi slot dang trong
struct slab_slot {
  struct slab_slot *next;
};

// Metadata Header dat tai offset 0x000 cua moi trang Slab 4KB (Dung 32 bytes)
struct slab_header {
  uint32 magic;               // 4 B: 0x534C4142U ("SLAB")
  uint16 obj_size;            // 2 B: Buoc nhay slot cua trang (64 hoac 128)
  uint16 capacity;            // 2 B: Tong so slots toi da (63 hoac 31)
  uint16 free_count;          // 2 B: So luong slots hien dang trong
  uint16 class_idx;           // 2 B: Chi so class so huu (0 = 64B, 1 = 128B)
  uint32 _pad;                // 4 B: Dem can le tron 16 B truoc hai con tro
  struct slab_slot *freelist; // 8 B: Con tro toi slot trong dau tien
  struct slab_header *next;   // 8 B: Con tro noi sang trang Slab tiep theo
};

// Cau truc quan ly Cache theo tung Size Class
struct slab_cache {
  struct spinlock lock;      // Khoa rieng ("slab_c64", "slab_c128")
  uint16 obj_size;           // 64 hoac 128
  uint16 capacity;           // 63 hoac 31
  uint16 class_idx;          // 0 hoac 1
  struct slab_header *slabs; // Danh sach cac trang Slab thuoc class nay
};

// S04a (#38): Snapshot thong ke theo tung class (doc duoi khoa tung class)
struct slab_class_stats {
  uint32 class_size;         // bytes: stride cua class (64 hoac 128)
  uint32 cap_per_slab;       // slots/slab: 63 (class64) hoac 31 (class128)
  uint32 slab_pages;         // pages: tong so trang slab cache dang giu
  uint32 empty_slabs;        // pages: so trang slab rong dang tai su dung
  uint32 total_capacity;     // slots: slab_pages * cap_per_slab
  uint32 live;               // objects: so object dang song
  uint32 free_slots;         // slots: so slot dang ranh
  uint32 page_bytes;         // bytes: slab_pages * PGSIZE (4096 B)
  uint32 obj_capacity_bytes; // bytes: total_capacity * class_size
  uint32 live_obj_bytes;     // bytes: live * class_size (tinh theo stride)
  uint32 metadata_bytes;     // bytes: slab_pages * sizeof(slab_header) (32B)
};

static struct slab_cache slab_caches[SLAB_NCLASSES];
static int slab_initialized = 0;
static int slab_oom_sim = 0;

// Chon class nho nhat du chua yeu cau:
// - size == 0 hoac size > 128 -> tra -1
// - 1 <= size <= 64           -> tra 0 (class64)
// - 65 <= size <= 128         -> tra 1 (class128)
static int
slab_FindClassIndex(uint64 size)
{
  if (size == 0)
    return -1;
  for (int i = 0; i < SLAB_NCLASSES; i++) {
    if (size <= slab_class_sizes[i])
      return i;
  }
  return -1;
}

// S04a (#38): Getter chi goi sau kmalloc_init(), tra 0 hoac -1 neu tham so sai
static int
slab_get_class_stats(int idx, struct slab_class_stats *out)
{
  if (!slab_initialized || idx < 0 || idx >= SLAB_NCLASSES || out == 0)
    return -1;

  struct slab_cache *c = &slab_caches[idx];
  uint32 pages = 0, empty = 0, total_cap = 0, free_s = 0;

  acquire(&c->lock);
  uint32 stride = c->obj_size;
  uint32 cap_per_slab = c->capacity;
  for (struct slab_header *it = c->slabs; it != 0; it = it->next) {
    pages++;
    total_cap += it->capacity;
    free_s += it->free_count;
    if (it->free_count == it->capacity)
      empty++;
  }
  release(&c->lock);

  out->class_size = stride;
  out->cap_per_slab = cap_per_slab;
  out->slab_pages = pages;
  out->empty_slabs = empty;
  out->total_capacity = total_cap;
  out->live = total_cap - free_s;
  out->free_slots = free_s;
  out->page_bytes = pages * PGSIZE;
  out->obj_capacity_bytes = total_cap * stride;
  out->live_obj_bytes = out->live * stride;
  out->metadata_bytes = pages * (uint32)sizeof(struct slab_header);
  return 0;
}

// Dem tong so slot dang trong trong toan bo cac trang Slab hien co cua 1 class
static int
slab_ClassFreeSlots(int idx)
{
  struct slab_class_stats st;
  if (slab_get_class_stats(idx, &st) != 0)
    panic("slab_ClassFreeSlots: stats getter failed");
  return (int)st.free_slots;
}

// Khoi tao mot trang 4096 B moi tu kalloc() thanh trang Slab cua class `c`
static struct slab_header *
slab_NewPage(struct slab_cache *c)
{
  void *page = slab_oom_sim ? 0 : kalloc();
  if (page == 0)
    return 0; // Nhanh OOM an toan: tra ve 0, khong lam hong trang thai

  struct slab_header *hdr = (struct slab_header *)page;
  hdr->magic = SLAB_MAGIC;
  hdr->obj_size = c->obj_size;
  hdr->capacity = c->capacity;
  hdr->free_count = c->capacity;
  hdr->class_idx = c->class_idx;
  hdr->_pad = 0;
  hdr->freelist = 0;
  hdr->next = 0;

  char *slot_base = (char *)page + sizeof(struct slab_header);
  memset(slot_base, SLAB_FREE_BYTE, PGSIZE - sizeof(struct slab_header));

  for (int i = (int)c->capacity - 1; i >= 0; i--) {
    struct slab_slot *s =
      (struct slab_slot *)(slot_base + (uint64)i * c->obj_size);
    s->next = hdr->freelist;
    hdr->freelist = s;
  }

  return hdr;
}

// 5.1. Luong Khoi Tao (kmalloc_init)
void
kmalloc_init(void)
{
  if (sizeof(struct slab_header) != 32)
    panic("kmalloc_init: slab_header must be 32 bytes");

  for (int i = 0; i < SLAB_NCLASSES; i++) {
    uint32 req_size = slab_class_sizes[i];
    uint32 s_min =
      req_size < sizeof(struct slab_slot) ? sizeof(struct slab_slot) : req_size;
    uint32 stride = ALIGN_UP(s_min, SLAB_ALIGN);

    if (sizeof(struct slab_header) + stride > PGSIZE)
      panic("kmalloc_init: invalid class stride");

    initlock(&slab_caches[i].lock, i == 0 ? "slab_c64" : "slab_c128");
    slab_caches[i].obj_size = (uint16)stride;
    slab_caches[i].capacity =
      (uint16)((PGSIZE - sizeof(struct slab_header)) / stride);
    slab_caches[i].class_idx = (uint16)i;
    slab_caches[i].slabs = 0;
  }

  slab_initialized = 1;
}

// 5.2. Luong Cap Phat (kmalloc)
void *
kmalloc(uint64 size)
{
  if (!slab_initialized)
    kmalloc_init();

  int idx = slab_FindClassIndex(size);
  if (idx < 0)
    return 0;

  struct slab_cache *c = &slab_caches[idx];
  acquire(&c->lock);

  struct slab_header *curr = c->slabs;
  while (curr != 0 && curr->free_count == 0) {
    curr = curr->next;
  }

  if (curr == 0) {
    curr = slab_NewPage(c);
    if (curr == 0) {
      release(&c->lock);
      return 0;
    }
    curr->next = c->slabs;
    c->slabs = curr;
  }

  struct slab_slot *slot = curr->freelist;
  if (slot == 0)
    panic("kmalloc: corrupt freelist");

  curr->freelist = slot->next;
  curr->free_count--;
  uint16 obj_size = c->obj_size;

  release(&c->lock);

  // Zero-fill toan bo slot truoc khi tra ve
  memset((void *)slot, 0, obj_size);
  return (void *)slot;
}

// 5.3. Luong Thu Hoi (kmfree) & Tu Tra Cuu Chu So Huu
void
kmfree(void *ptr)
{
  if (ptr == 0)
    return;

  uint64 addr = (uint64)ptr;
  if (addr < (uint64)end || addr >= PHYSTOP)
    panic("kmfree: pointer out of physical memory range");

  struct slab_header *hdr = (struct slab_header *)PGROUNDDOWN(addr);
  if (hdr->magic != SLAB_MAGIC)
    panic("kmfree: invalid slab magic");

  uint16 idx = hdr->class_idx;
  if (idx >= SLAB_NCLASSES)
    panic("kmfree: invalid slab class index");

  struct slab_cache *c = &slab_caches[idx];
  if (hdr->obj_size != c->obj_size || hdr->capacity != c->capacity)
    panic("kmfree: corrupt slab header metadata");

  if (addr < (uint64)hdr + sizeof(struct slab_header))
    panic("kmfree: pointer inside slab header");

  uint64 offset = addr - ((uint64)hdr + sizeof(struct slab_header));
  if ((offset % c->obj_size) != 0)
    panic("kmfree: unaligned slot pointer");

  uint64 slot_idx = offset / c->obj_size;
  if (slot_idx >= c->capacity)
    panic("kmfree: slot index out of slab capacity");

  acquire(&c->lock);

  int owned = 0;
  for (struct slab_header *it = c->slabs; it != 0; it = it->next) {
    if (it == hdr) {
      owned = 1;
      break;
    }
  }
  if (!owned) {
    release(&c->lock);
    panic("kmfree: slab not owned by class cache");
  }

  if (hdr->free_count >= c->capacity) {
    release(&c->lock);
    panic("kmfree: free_count overflow / double free");
  }

  for (struct slab_slot *s = hdr->freelist; s != 0; s = s->next) {
    if ((void *)s == ptr) {
      release(&c->lock);
      panic("kmfree: double free detected");
    }
  }

  // Dien mau byte 1 khi free truoc khi gan slot->next
  memset(ptr, SLAB_FREE_BYTE, c->obj_size);

  struct slab_slot *slot = (struct slab_slot *)ptr;
  slot->next = hdr->freelist;
  hdr->freelist = slot;
  hdr->free_count++;

  release(&c->lock);
}

#if SLAB_STATS_TEST
// S04a (#38): Doi chieu va in expected/actual cho day du cac truong thong ke
static void
slab_check_expected(int idx, const char *stage, uint32 exp_pages,
                    uint32 exp_empty, uint32 exp_cap, uint32 exp_live,
                    uint32 exp_free)
{
  struct slab_class_stats st;
  if (slab_get_class_stats(idx, &st) != 0)
    panic("slab_stats: getter failed");
  if (st.live + st.free_slots != st.total_capacity)
    panic("slab_stats: live + free_slots != total_capacity");

  uint32 exp_page_b = exp_pages * PGSIZE;
  uint32 exp_cap_b = exp_cap * st.class_size;
  uint32 exp_live_b = exp_live * st.class_size;
  uint32 exp_meta_b = exp_pages * (uint32)sizeof(struct slab_header);

  printk("slab_stats [%s] c%d: exp(slabs=%d empty=%d cap=%d live=%d free=%d "
         "page_B=%d cap_B=%d live_B=%d meta_B=%d) "
         "act(slabs=%d empty=%d cap=%d live=%d free=%d "
         "page_B=%d cap_B=%d live_B=%d meta_B=%d)\n",
         stage, st.class_size, exp_pages, exp_empty, exp_cap, exp_live,
         exp_free, exp_page_b, exp_cap_b, exp_live_b, exp_meta_b, st.slab_pages,
         st.empty_slabs, st.total_capacity, st.live, st.free_slots,
         st.page_bytes, st.obj_capacity_bytes, st.live_obj_bytes,
         st.metadata_bytes);

  if (st.slab_pages != exp_pages || st.empty_slabs != exp_empty ||
      st.total_capacity != exp_cap || st.live != exp_live ||
      st.free_slots != exp_free || st.page_bytes != exp_page_b ||
      st.obj_capacity_bytes != exp_cap_b || st.live_obj_bytes != exp_live_b ||
      st.metadata_bytes != exp_meta_b)
    panic("slab_stats: expected != actual");
}

// S04a (#38): Kiem tra moc vuot slot trong (+1 trang) va lap lai trong
// free_slots sau warm-up de xac nhan so trang on dinh
static void
slab_run_stats_workload(void)
{
  static void *held[256];

  for (int idx = 0; idx < SLAB_NCLASSES; idx++) {
    struct slab_class_stats base;
    if (slab_get_class_stats(idx, &base) != 0)
      panic("slab_stats: base getter failed");

    // Moc 1: Truoc workload (lay moc ban dau theo so lieu thuc cua cache)
    slab_check_expected(idx, "before", base.slab_pages, base.empty_slabs,
                        base.total_capacity, base.live, base.free_slots);

    uint32 fill_cnt = base.free_slots;
    if (fill_cnt + 1 > 256)
      panic("slab_stats: held buffer too small");

    // Cap vua het so slot trong hien co: so trang CHUA tang
    for (uint32 i = 0; i < fill_cnt; i++) {
      held[i] = kmalloc(base.class_size);
      if (held[i] == 0)
        panic("slab_stats: fill existing free_slots failed");
    }
    struct slab_class_stats mid;
    if (slab_get_class_stats(idx, &mid) != 0)
      panic("slab_stats: mid getter failed");
    if (mid.slab_pages != base.slab_pages || mid.free_slots != 0 ||
        mid.empty_slabs != 0)
      panic("slab_stats: page count changed before exhausting free_slots");

    // Cap them 1 object vuot so slot trong: so trang TANG DUNG LUC (+1 trang)
    held[fill_cnt] = kmalloc(base.class_size);
    if (held[fill_cnt] == 0)
      panic("slab_stats: overflow alloc failed");

    uint32 exp_live_pages = base.slab_pages + 1;
    uint32 exp_live_cap = base.total_capacity + base.cap_per_slab;
    uint32 exp_live_objs = base.live + fill_cnt + 1;
    uint32 exp_live_free = base.cap_per_slab - 1;

    // Moc 2: Luc con object song (vuot slot trong cua class -> empty_slabs=0)
    slab_check_expected(idx, "live", exp_live_pages, 0, exp_live_cap,
                        exp_live_objs, exp_live_free);

    // Tra het object cua test
    for (uint32 i = 0; i <= fill_cnt; i++)
      kmfree(held[i]);

    uint32 exp_after_empty =
      (base.live == 0) ? exp_live_pages : (base.empty_slabs + 1);
    uint32 exp_after_free = exp_live_cap - base.live;

    // Moc 3: Sau khi free (live ve moc ban dau, trang rong van duoc giu)
    slab_check_expected(idx, "after_free", exp_live_pages, exp_after_empty,
                        exp_live_cap, base.live, exp_after_free);

    // Moc 4: Lap mot luot cap/tra vua trong so slot trong sau warm-up
    // de xac nhan so trang on dinh (khong xin them trang moi)
    if (exp_after_free > 256)
      panic("slab_stats: held buffer too small for reuse pass");

    for (uint32 i = 0; i < exp_after_free; i++) {
      held[i] = kmalloc(base.class_size);
      if (held[i] == 0)
        panic("slab_stats: warmup reuse alloc failed");
    }
    slab_check_expected(idx, "reuse_live", exp_live_pages, 0, exp_live_cap,
                        exp_live_cap, 0);

    for (uint32 i = 0; i < exp_after_free; i++)
      kmfree(held[i]);
    slab_check_expected(idx, "reuse_free", exp_live_pages, exp_after_empty,
                        exp_live_cap, base.live, exp_after_free);
  }
}
#endif

// 6.1. Cac bai kiem thu tu dong trong slab_test()
void
slab_test(void)
{
  static void *c64_objs[64];
  static void *c128_objs[32];
  static void *fill128[64];

  printk("slab_test: starting verification (class64 & class128)...\n");
  if (!slab_initialized)
    kmalloc_init();

  // 1. Kiem tra cau hinh stride & capacity cua 2 class
  if (slab_caches[0].obj_size != 64 || slab_caches[0].capacity != 63)
    panic("slab_test: wrong class64 config");
  if (slab_caches[1].obj_size != 128 || slab_caches[1].capacity != 31)
    panic("slab_test: wrong class128 config");

  // Cac ca kiem thu am tinh SLAB_TEST_CASE=1/2/3
#if SLAB_TEST_CASE == 1
  printk("slab_test: running SLAB_TEST_CASE=1 (double free)...\n");
  void *p_df = kmalloc(64);
  if (p_df == 0)
    panic("slab_test: alloc failed for SLAB_TEST_CASE=1");
  kmfree(p_df);
  kmfree(p_df);
  panic("slab_test: SLAB_TEST_CASE=1 did not panic");
#elif SLAB_TEST_CASE == 2
  printk("slab_test: running SLAB_TEST_CASE=2 (unaligned pointer)...\n");
  void *p_un = kmalloc(64);
  if (p_un == 0)
    panic("slab_test: alloc failed for SLAB_TEST_CASE=2");
  kmfree((char *)p_un + 8);
  panic("slab_test: SLAB_TEST_CASE=2 did not panic");
#elif SLAB_TEST_CASE == 3
  printk("slab_test: running SLAB_TEST_CASE=3 (invalid slab page)...\n");
  void *raw_page = kalloc();
  if (raw_page == 0)
    panic("slab_test: kalloc failed for SLAB_TEST_CASE=3");
  kmfree((char *)raw_page + sizeof(struct slab_header));
  panic("slab_test: SLAB_TEST_CASE=3 did not panic");
#endif

  // Kiem tra tham so loi cua getter S04a
  struct slab_class_stats dummy;
  if (slab_get_class_stats(-1, &dummy) != -1 ||
      slab_get_class_stats(SLAB_NCLASSES, &dummy) != -1 ||
      slab_get_class_stats(0, 0) != -1)
    panic("slab_test: invalid getter args must return -1");

#if SLAB_STATS_TEST
  slab_check_expected(0, "init", 0, 0, 0, 0, 0);
  slab_check_expected(1, "init", 0, 0, 0, 0, 0);
#endif

  // 2. Kiem tra 6 moc bien (0, 1, 64, 65, 128, 129), kmfree(0) & zero-fill
  kmfree(0);
  if (kmalloc(0) != 0)
    panic("slab_test: kmalloc(0) must return 0");
  if (kmalloc(129) != 0)
    panic("slab_test: kmalloc(129) must return 0");

  void *b1 = kmalloc(1);
  void *b64 = kmalloc(64);
  void *b65 = kmalloc(65);
  void *b128 = kmalloc(128);
  if (!b1 || !b64 || !b65 || !b128)
    panic("slab_test: boundary valid alloc failed");

  if (((uint64)b1 % 8) != 0 || ((uint64)b64 % 8) != 0 ||
      ((uint64)b65 % 8) != 0 || ((uint64)b128 % 8) != 0)
    panic("slab_test: alignment < 8B");

  unsigned char *zb64 = (unsigned char *)b64;
  for (int i = 0; i < 64; i++) {
    if (zb64[i] != 0)
      panic("slab_test: class64 alloc not zero-filled");
  }
  unsigned char *zb128 = (unsigned char *)b128;
  for (int i = 0; i < 128; i++) {
    if (zb128[i] != 0)
      panic("slab_test: class128 alloc not zero-filled");
  }

  struct slab_header *h1 = (struct slab_header *)PGROUNDDOWN((uint64)b1);
  struct slab_header *h64 = (struct slab_header *)PGROUNDDOWN((uint64)b64);
  struct slab_header *h65 = (struct slab_header *)PGROUNDDOWN((uint64)b65);
  struct slab_header *h128 = (struct slab_header *)PGROUNDDOWN((uint64)b128);

  if (h1->obj_size != 64 || h64->obj_size != 64 || h1->class_idx != 0)
    panic("slab_test: 1..64 not routed to class64");
  if (h65->obj_size != 128 || h128->obj_size != 128 || h65->class_idx != 1)
    panic("slab_test: 65..128 not routed to class128");
  printk("slab_test: [1/4] boundary (0,1,64,65,128,129) & routing OK\n");

  // 3. Kiem tra mau byte 1 khi free & LIFO Reuse tren ca 2 class
  memset(b64, 0x55, 64);
  kmfree(b64);
  for (int i = (int)sizeof(struct slab_slot); i < 64; i++) {
    if (zb64[i] != SLAB_FREE_BYTE)
      panic("slab_test: class64 free did not fill byte 1");
  }
  void *b64_re = kmalloc(64);
  if (b64_re != b64)
    panic("slab_test: class64 LIFO reuse failed");
  for (int i = 0; i < 64; i++) {
    if (zb64[i] != 0)
      panic("slab_test: class64 reuse not zero-filled");
  }

  memset(b128, 0xAA, 128);
  kmfree(b128);
  for (int i = (int)sizeof(struct slab_slot); i < 128; i++) {
    if (zb128[i] != SLAB_FREE_BYTE)
      panic("slab_test: class128 free did not fill byte 1");
  }
  void *b128_re = kmalloc(128);
  if (b128_re != b128)
    panic("slab_test: class128 LIFO reuse failed");
  for (int i = 0; i < 128; i++) {
    if (zb128[i] != 0)
      panic("slab_test: class128 reuse not zero-filled");
  }

  kmfree(b1);
  kmfree(b64_re);
  kmfree(b65);
  kmfree(b128_re);
  printk("slab_test: [2/4] LIFO reuse for class64 & class128 OK\n");

  // 4. Kiem tra Multi-Slab & Pattern Isolation (Khong de du lieu giua 2 class)
  for (int i = 0; i < 64; i++) {
    c64_objs[i] = kmalloc((i % 2 == 0) ? 1 : 64);
    if (c64_objs[i] == 0)
      panic("slab_test: class64 multi-slab alloc failed");
    memset(c64_objs[i], (char)(0x30 + i), 64);
  }

  for (int i = 0; i < 32; i++) {
    c128_objs[i] = kmalloc((i % 2 == 0) ? 65 : 128);
    if (c128_objs[i] == 0)
      panic("slab_test: class128 multi-slab alloc failed");
    memset(c128_objs[i], (char)(0xA0 + i), 128);
  }

  if (PGROUNDDOWN((uint64)c64_objs[0]) == PGROUNDDOWN((uint64)c64_objs[63]))
    panic("slab_test: class64 did not allocate second slab page");
  if (PGROUNDDOWN((uint64)c128_objs[0]) == PGROUNDDOWN((uint64)c128_objs[31]))
    panic("slab_test: class128 did not allocate second slab page");

  for (int i = 0; i < 64; i++) {
    unsigned char *p = (unsigned char *)c64_objs[i];
    unsigned char exp = (unsigned char)(0x30 + i);
    for (int b = 0; b < 64; b++) {
      if (p[b] != exp)
        panic("slab_test: class64 pattern corrupted");
    }
  }

  for (int i = 0; i < 32; i++) {
    unsigned char *p = (unsigned char *)c128_objs[i];
    unsigned char exp = (unsigned char)(0xA0 + i);
    for (int b = 0; b < 128; b++) {
      if (p[b] != exp)
        panic("slab_test: class128 pattern corrupted");
    }
  }

  for (int i = 0; i < 64; i++)
    kmfree(c64_objs[i]);
  for (int i = 0; i < 32; i++)
    kmfree(c128_objs[i]);
  printk("slab_test: [3/4] multi-slab expansion & pattern isolation OK\n");

  // 5. Kiem tra OOM & Retry (giu nguyen live64, live128, fill128 va retry128)
  void *live64 = kmalloc(64);
  void *live128 = kmalloc(128);
  if (!live64 || !live128)
    panic("slab_test: pre-OOM alloc failed");
  memset(live64, 0x5A, 64);
  memset(live128, 0xA5, 128);

  int fill128_cnt = 0;
  while (slab_ClassFreeSlots(1) > 0) {
    if (fill128_cnt >= 64)
      panic("slab_test: fill128 overflow");
    fill128[fill128_cnt] = kmalloc(128);
    if (fill128[fill128_cnt] == 0)
      panic("slab_test: failed to fill existing class128 slots");
    memset(fill128[fill128_cnt], (char)(0xC0 + fill128_cnt), 128);
    fill128_cnt++;
  }

  if (slab_ClassFreeSlots(1) != 0)
    panic("slab_test: class128 still has free slots before OOM");

  slab_oom_sim = 1;
  if (kmalloc(128) != 0)
    panic("slab_test: expected 0 on OOM when class128 requests new page");
  if (kmalloc(65) != 0)
    panic("slab_test: expected 0 on OOM for kmalloc(65)");
  slab_oom_sim = 0;

  void *retry128 = kmalloc(128);
  if (retry128 == 0)
    panic("slab_test: retry alloc new page after OOM failed");
  memset(retry128, 0x7E, 128);

  unsigned char *chk64 = (unsigned char *)live64;
  unsigned char *chk128 = (unsigned char *)live128;
  for (int b = 0; b < 64; b++) {
    if (chk64[b] != 0x5A)
      panic("slab_test: live64 corrupted across OOM");
  }
  for (int b = 0; b < 128; b++) {
    if (chk128[b] != 0xA5)
      panic("slab_test: live128 corrupted across OOM");
  }

  for (int i = 0; i < fill128_cnt; i++) {
    unsigned char *p = (unsigned char *)fill128[i];
    unsigned char exp = (unsigned char)(0xC0 + i);
    for (int b = 0; b < 128; b++) {
      if (p[b] != exp)
        panic("slab_test: fill128 slot corrupted across OOM");
    }
  }

  kmfree(retry128);
  for (int i = 0; i < fill128_cnt; i++)
    kmfree(fill128[i]);
  kmfree(live128);
  kmfree(live64);
  printk("slab_test: [4/4] OOM (filled %d slots of class128) & retry OK\n",
         fill128_cnt + 1);

#if SLAB_STATS_TEST
  // S04a (#38): Kiem tra thong ke khi bat co SLAB_STATS_TEST=1
  slab_run_stats_workload();
#endif

  printk("slab_test: ALL PASSED (class64 cap=63, class128 cap=31)\n");
}