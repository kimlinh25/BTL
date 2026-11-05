#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define SLAB_MAGIC 0x534C4142U
#define SLAB_ALIGN 8U
#define SLAB_NCLASSES 2

#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))

extern char end[];

static const uint16 slab_class_sizes[SLAB_NCLASSES] = {64U, 128U};

struct slab_slot {
  struct slab_slot *next;
};

struct slab_header {
  uint32 magic;
  uint16 obj_size;
  uint16 capacity;
  uint16 free_count;
  uint16 class_idx;
  uint32 _pad;
  struct slab_slot *freelist;
  struct slab_header *next;
};

struct slab_cache {
  struct spinlock lock;
  uint16 obj_size;
  uint16 capacity;
  uint16 class_idx;
  struct slab_header *slabs;
};

static struct slab_cache slab_caches[SLAB_NCLASSES];
static int slab_initialized = 0;
static int slab_oom_inject = 0;

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

static int
slab_ClassFreeSlots(int idx)
{
  int total = 0;
  struct slab_cache *c = &slab_caches[idx];
  acquire(&c->lock);
  for (struct slab_header *it = c->slabs; it != 0; it = it->next) {
    total += it->free_count;
  }
  release(&c->lock);
  return total;
}

// Khởi tạo một trang vật lý 4KB mới thành một trang Slab
static struct slab_header *
slab_page_create(void)
{
  if (slab_test_fail_page_alloc)
    return 0;

  void *page = kalloc();
  if (page == 0)
    return 0;

  struct slab_header *hdr = (struct slab_header *)page;
  hdr->magic = SLAB_MAGIC;
  hdr->obj_size = slab_cache.obj_size;
  hdr->capacity = slab_cache.capacity;
  hdr->free_count = slab_cache.capacity;
  hdr->_pad = 0;
  hdr->_reserved = 0;
  hdr->freelist = 0;
  hdr->next = 0;

  char *slot_base = (char *)page + sizeof(struct slab_header);

  for (int i = hdr->capacity - 1; i >= 0; i--) {
    struct slab_slot *s =
        (struct slab_slot *)(slot_base + (uint64)i * c->obj_size);
    s->next = hdr->freelist;
    hdr->freelist = s;
  }

  return hdr;
}

void
kmalloc_init(void)
{
  initlock(&slab_cache.lock, "slab_cache");

  for (int i = 0; i < SLAB_NCLASSES; i++) {
    uint32 req = slab_class_sizes[i];
    uint32 s_min =
        req < sizeof(struct slab_slot) ? sizeof(struct slab_slot) : req;
    uint32 stride = ALIGN_UP(s_min, SLAB_ALIGN);

  if (sizeof(struct slab_header) != 32 ||
      sizeof(struct slab_header) + s_aligned > PGSIZE)
    panic("kmalloc_init: invalid header or object size");

    initlock(&slab_caches[i].lock, i == 0 ? "slab_c64" : "slab_c128");
    slab_caches[i].obj_size = (uint16)stride;
    slab_caches[i].capacity =
        (uint16)((PGSIZE - sizeof(struct slab_header)) / stride);
    slab_caches[i].class_idx = (uint16)i;
    slab_caches[i].slabs = 0;
  }

  slab_oom_inject = 0;
  slab_initialized = 1;
}

void *
kmalloc(uint64 size)
{
  if (!slab_initialized)
    kmalloc_init();

  int idx = slab_FindClassIndex(size);
  if (idx < 0)
    return 0;

  acquire(&slab_cache.lock);

  struct slab_header *curr = slab_cache.slabs;
  while (curr != 0 && curr->free_count == 0) {
    curr = curr->next;
  }

  if (curr == 0) {
    curr = slab_page_create();
    if (curr == 0) {
      release(&slab_cache.lock);
      return 0;
    }
    curr->next = slab_cache.slabs;
    slab_cache.slabs = curr;
  }

  struct slab_slot *slot = curr->freelist;
  curr->freelist = slot->next;
  curr->free_count--;

  release(&slab_cache.lock);

  memset((void *)slot, 0, slab_cache.obj_size);
  return (void *)slot;
}

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

  acquire(&slab_cache.lock);

  // Kiểm tra chữ ký SLAB_MAGIC ở đầu trang
  if (hdr->magic != SLAB_MAGIC || hdr->obj_size != slab_cache.obj_size)
    panic("kmfree: invalid slab magic or corrupted header");

  if (addr < (uint64)hdr + sizeof(struct slab_header))
    panic("kmfree: pointer inside slab header");

  uint64 offset = (uint64)ptr - slot_base;
  if ((offset % hdr->obj_size) != 0 ||
      (offset / hdr->obj_size) >= hdr->capacity)
    panic("kmfree: unaligned slot pointer");

  // Kiểm tra giải phóng kép (Double Free) bằng cả free_count lẫn duyệt danh sách freelist
  if (hdr->free_count >= hdr->capacity)
    panic("kmfree: free_count overflow");

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

  struct slab_slot *slot = (struct slab_slot *)ptr;
  slot->next = hdr->freelist;
  hdr->freelist = slot;
  hdr->free_count++;

  release(&slab_cache.lock);
}

void
slab_test(void)
{
  printk("slab_test: starting S02 verification...\n");

  if (slab_caches[0].obj_size != 64 || slab_caches[0].capacity != 63)
    panic("slab_test: wrong class64 config");
  if (slab_caches[1].obj_size != 128 || slab_caches[1].capacity != 31)
    panic("slab_test: wrong class128 config");

  kmfree(0);
  if (kmalloc(0) != 0)
    panic("slab_test: kmalloc(0) must return 0");
  if (kmalloc(129) != 0)
    panic("slab_test: kmalloc(129) must return 0");

  kmfree(p1);
  void *p3 = kmalloc(32);
  if (p3 != p1)
    panic("slab_test: LIFO reuse failed");
  if (((uint64)p3 & 7) != 0)
    panic("slab_test: reuse alignment failed");

  kmfree(p2);
  kmfree(p3);

  struct slab_header *h1 = (struct slab_header *)PGROUNDDOWN((uint64)b1);
  struct slab_header *h64 = (struct slab_header *)PGROUNDDOWN((uint64)b64);
  struct slab_header *h65 = (struct slab_header *)PGROUNDDOWN((uint64)b65);
  struct slab_header *h128 = (struct slab_header *)PGROUNDDOWN((uint64)b128);

  if (h1->obj_size != 64 || h64->obj_size != 64 || h1->class_idx != 0)
    panic("slab_test: 1..64 not routed to class64");
  if (h65->obj_size != 128 || h128->obj_size != 128 || h65->class_idx != 1)
    panic("slab_test: 65..128 not routed to class128");
  printk("slab_test: [1/4] boundary (0,1,64,65,128,129) & routing OK\n");

  kmfree(b64);
  void *b64_re = kmalloc(64);
  if (b64_re != b64)
    panic("slab_test: class64 LIFO reuse failed");

  kmfree(b128);
  void *b128_re = kmalloc(128);
  if (b128_re != b128)
    panic("slab_test: class128 LIFO reuse failed");

  kmfree(b1);
  kmfree(b64_re);
  kmfree(b65);
  kmfree(b128_re);
  printk("slab_test: [2/4] LIFO reuse for class64 & class128 OK\n");

  void *c64_objs[64];
  void *c128_objs[32];

  for (int i = 0; i < 64; i++) {
    c64_objs[i] = kmalloc((i % 2 == 0) ? 1 : 64);
    if (c64_objs[i] == 0)
      panic("slab_test: class64 multi-slab alloc failed");
    memset(c64_objs[i], (char)(0x30 + i), 64);
  }

  void *second_slab[63];
  second_slab[0] = kmalloc(64);
  if (second_slab[0] == 0)
    panic("slab_test: second slab alloc failed");
  if (((uint64)second_slab[0] & 7) != 0)
    panic("slab_test: second slab alignment failed");
  if (PGROUNDDOWN((uint64)slots[0]) == PGROUNDDOWN((uint64)second_slab[0]))
    panic("slab_test: second slab page not created");

  for (int i = 1; i < slab_cache.capacity; i++) {
    second_slab[i] = kmalloc(64);
    if (second_slab[i] == 0)
      panic("slab_test: second slab alloc failed");
    if (((uint64)second_slab[i] & 7) != 0)
      panic("slab_test: second slab alignment failed");
  }

  for (int i = 0; i < slab_cache.capacity; i++) {
    uchar *bytes = (uchar *)slots[i];
    uchar expected = (uchar)((i + 0x20) & 0xFF);
    for (int j = 0; j < 64; j++) {
      if (bytes[j] != expected)
        panic("slab_test: pattern corrupted");
    }
  }

  slab_test_fail_page_alloc = 1;
  if (kmalloc(64) != 0)
    panic("slab_test: simulated OOM was not reported");
  slab_test_fail_page_alloc = 0;

  void *retry = kmalloc(64);
  if (retry == 0 || ((uint64)retry & 7) != 0)
    panic("slab_test: allocation retry failed");

  for (int i = 0; i < slab_cache.capacity; i++) {
    kmfree(slots[i]);
    kmfree(second_slab[i]);
  }
  kmfree(retry);

  for (int i = 0; i < 64; i++)
    kmfree(c64_objs[i]);
  for (int i = 0; i < 32; i++)
    kmfree(c128_objs[i]);
  printk("slab_test: [3/4] multi-slab expansion & pattern isolation OK\n");

  void *live64 = kmalloc(64);
  if (live64 == 0)
    panic("slab_test: live64 alloc failed");
  memset(live64, 0x5A, 64);

  void *fill128[64];
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

  slab_oom_inject = 1;
  if (kmalloc(128) != 0)
    panic("slab_test: expected 0 on OOM when class128 requests new page");
  if (kmalloc(65) != 0)
    panic("slab_test: expected 0 on OOM for kmalloc(65)");
  slab_oom_inject = 0;

  void *retry128 = kmalloc(128);
  if (retry128 == 0)
    panic("slab_test: retry alloc new page after OOM failed");
  memset(retry128, 0x7E, 128);

  unsigned char *chk64 = (unsigned char *)live64;
  for (int b = 0; b < 64; b++) {
    if (chk64[b] != 0x5A)
      panic("slab_test: live64 corrupted across OOM");
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
  kmfree(live64);
  printk("slab_test: [4/4] OOM (filled %d slots of class128) & retry OK\n",
         fill128_cnt);

  printk("slab_test: ALL PASSED (class64 cap=63, class128 cap=31)\n");
}