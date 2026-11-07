// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct page_pool {
  struct spinlock lock; // Khóa bảo vệ freelist khi nhiều CPU truy cập cùng lúc
  struct run *freelist;
};

static struct page_pool
  kmem[NCPU]; // Mảng các page_pool, mỗi CPU có một page_pool riêng

void
kinit()
{
  for (int i = 0; i < NCPU; i++) {
    initlock(&kmem[i].lock, "kmem");
    kmem[i].freelist = 0;
  }
  freerange(end, (void *)PHYSTOP); // Khởi tạo các page free
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int id; // CPU id

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;
  push_off();   // Tắt ngắt để tránh bị preempted
  id = cpuid(); // Lấy CPU id hiện tại

  acquire(
    &kmem[id]
       .lock); // Yêu cầu quyền truy cập vào kmem.lock để đảm bảo rằng không có CPU nào khác đang truy cập vào freelist cùng lúc

  r->next = kmem[id].freelist;
  kmem[id].freelist = r;

  release(
    &kmem[id]
       .lock); // Giải phóng quyền truy cập vào kmem.lock để cho phép các CPU khác truy cập vào freelist
  pop_off();   // Bật lại ngắt
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  int id; // CPU id

  push_off();   // Tắt ngắt để tránh bị preempted
  id = cpuid(); // Lấy CPU id hiện tại

  // Giữ khóa khi nối trang trả về vào freelist
  acquire(&kmem[id].lock);

  r = kmem[id].freelist;
  if (r)
    kmem[id].freelist = r->next;

  release(&kmem[id].lock);
  pop_off(); // Bật lại ngắt

  if (r)
    memset((char *)r, 5, PGSIZE); // fill with junk

  return (void *)r;
}

uint64
kfreepage_count(void)
{
  struct run *r;
  uint64 count = 0;

  for (int i = 0; i < NCPU; i++) {
    acquire(&kmem[i].lock);

    r = kmem[i].freelist;
    while (r) {
      count++;
      r = r->next;
    }

    release(&kmem[i].lock);
  }

  return count;
}
