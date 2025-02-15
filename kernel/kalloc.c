// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

extern uint64 parse_dtb_memory_size(uint64);

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

// Memory size after parsing DTB
uint64 mem_size = 0;

// Read the value from 0x1020 That gives the address where DTB is loaded by QEMU.
// It's right after the reset vector that's installed by QEMU in the emulated ROM
// https://github.com/qemu/qemu/blob/master/hw/riscv/virt.c#L431-L458
// https://github.com/qemu/qemu/blob/master/hw/riscv/virt.c#L83
#define DTB_ADDRESS 0x8FE00000

void
kinit()
{  
  mem_size = parse_dtb_memory_size(DTB_ADDRESS);
  
  if (mem_size == 0) {
    mem_size = 128 * 1024 * 1024; // Fallback to default 128MB if parsing fails
  }

  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP(mem_size));
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
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

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP(mem_size))
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
