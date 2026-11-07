#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

#define SLAB_MAGIC    0x534C4142U
#define SLAB_ALIGN    8U
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

static struct slab_header *
slab_NewPage(struct slab_cache *c)
{
  if (slab_oom_inject)
    return 0;

  void *page = kalloc();
  if (page == 0)
    return 0;

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
  for (int i = (int)c->capacity - 1; i >= 0; i--) {
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
  if (sizeof(struct slab_header) != 32)
    panic("kmalloc_init: slab_header must be 32 bytes");

  for (int i = 0; i < SLAB_NCLASSES; i++) {
    uint32 req = slab_class_sizes[i];
    uint32 s_min =
      req < sizeof(struct slab_slot) ? sizeof(struct slab_slot) : req;
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

  release(&c->lock);
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

  struct slab_slot *slot = (struct slab_slot *)ptr;
  slot->next = hdr->freelist;
  hdr->freelist = slot;
  hdr->free_count++;

  release(&c->lock);
}

void
slab_test(void)
{
  printk("slab_test: starting S03a verification (class64 & class128)...\n");
  if (!slab_initialized)
    kmalloc_init();

  if (slab_caches[0].obj_size != 64 || slab_caches[0].capacity != 63)
    panic("slab_test: wrong class64 config");
  if (slab_caches[1].obj_size != 128 || slab_caches[1].capacity != 31)
    panic("slab_test: wrong class128 config");

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