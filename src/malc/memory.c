#ifndef __MALC_MEMORY__
#define __MALC_MEMORY__

#include <bl/base/thread.h>
#include <bl/base/assert.h>
#include <bl/base/utility.h>
#include <bl/base/integer_manipulation.h>
#include <bl/base/default_allocator.h>

#include <malc/cfg.h>
#include <malc/memory.h>
#include <malc/tls_buffer.h>

/*----------------------------------------------------------------------------*/
bl_err memory_init (memory* m)
{
  bl_err err = bl_tss_init (&m->tss_key, &tls_buffer_destroy);
  if (err) {
    return err;
  }
  m->default_allocator  = get_default_alloc();
  m->cfg.heap_allocator = &m->default_allocator;
  boundedb_init (&m->bb);
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
void memory_destroy (memory* m, alloc_tbl const* alloc)
{
  /* TODO: this should manually deallocate all TLS buffers. A list with them
     needs to be maintained */
  bl_tss_destroy (m->tss_key);
  boundedb_destroy (&m->bb, alloc);
}
/*----------------------------------------------------------------------------*/
bl_err memory_tls_init(
  memory*          m,
  u32              bytes,
  alloc_tbl const* alloc,
  tls_destructor   destructor_fn,
  void*            destructor_context
  )
{
  if (malc_tls) {
    return bl_locked;
  }
  tls_buffer* t;
  u32 slots  = round_to_next_multiple (bytes, (u32) alloc_slot_size);
  bl_err err = tls_buffer_init(
    &t, alloc_slot_size, slots, alloc, destructor_fn, destructor_context
    );
  if (err) {
    return err;
  }
  err = bl_tss_set (m->tss_key, t);
  if (err) {
    tls_buffer_destroy (t);
    return err;
  }
  malc_tls = t;
  return bl_ok;
}
/*----------------------------------------------------------------------------*/
bl_err memory_bounded_buffer_init (memory* m, alloc_tbl const* alloc)
{
  return boundedb_reset(
    &m->bb,
    alloc,
    m->cfg.fixed_allocator_bytes,
    alloc_slot_size,
    m->cfg.fixed_allocator_max_slots,
    m->cfg.fixed_allocator_per_cpu
    );
}
/*----------------------------------------------------------------------------*/
void memory_tls_destroy_explicit (memory* m)
{
  void* b = (void*) malc_tls;
  tls_buffer_destroy (b);
}
/*----------------------------------------------------------------------------*/
bl_err memory_alloc (memory* m, u8** mem, alloc_tag* tag, u32 slots)
{
  bl_assert (m && mem && tag && slots);
  if (likely (!malc_tls)) {
    tls_buffer* t = (tls_buffer*) malc_tls;
    if (likely (tls_buffer_alloc (t, mem, slots) == bl_ok)) {
      *tag = alloc_tag_tls;
      return bl_ok;
    }
  }
  bl_err err = bl_would_overflow;
  if (m->cfg.fixed_allocator_bytes > 0) {
    err = boundedb_alloc (&m->bb, mem, slots);
    if (likely (!err)) {
      return bl_ok;
    }
  }
  if (m->cfg.heap_allocator) {
    *mem = bl_alloc (m->cfg.heap_allocator, slots * alloc_slot_size);
    *tag = alloc_tag_heap;
    return *mem ? bl_ok : bl_alloc;
  }
  return err;
}
/*----------------------------------------------------------------------------*/
void memory_dealloc (memory* m, u8* mem, alloc_tag tag, u32 slots)
{
  switch (tag) {
  case alloc_tag_tls:
    tls_buffer_dealloc (mem, slots, alloc_slot_size);
    break;
  case alloc_tag_bounded:
    boundedb_dealloc (&m->bb, mem, slots);
    break;
  case alloc_tag_heap:
    bl_dealloc (m->cfg.heap_allocator, mem);
    break;
  default:
    bl_assert (false && "bug");
    break;
  }
}
/*----------------------------------------------------------------------------*/

#endif /* __MALC_MEMORY__ */