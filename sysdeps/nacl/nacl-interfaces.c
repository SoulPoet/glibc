/* Using NaCl interface tables.
   Copyright (C) 2014 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <errno.h>
#include <nacl-interfaces.h>
#include <ldsodefs.h>


/* These magic symbols are provided implicitly by the linker to
   give us the bounds of the specially-named sections.  */

extern const struct nacl_interface __start_nacl_mandatory_interface_names[]
  attribute_hidden;
extern const struct nacl_interface __stop_nacl_mandatory_interface_names[]
  attribute_hidden;

extern uintptr_t __start_nacl_mandatory_interface_tables[]
  attribute_hidden;
extern uintptr_t __stop_nacl_mandatory_interface_tables[]
  attribute_hidden;

/* We use weak references for the optional ones, since they
   might not be included at all in any given statically-linked program.  */

extern const struct nacl_interface __start_nacl_optional_interface_names[]
  attribute_hidden __attribute__ ((weak));
extern const struct nacl_interface __stop_nacl_optional_interface_names[]
  attribute_hidden __attribute__ ((weak));

extern uintptr_t __start_nacl_optional_interface_tables[]
  attribute_hidden __attribute__ ((weak));
extern uintptr_t __stop_nacl_optional_interface_tables[]
  attribute_hidden __attribute__ ((weak));

static uintptr_t *
next_nacl_table (uintptr_t *t,
                 const struct nacl_interface *i)
{
  return (void *) t + i->table_size;
}

static void __attribute__ ((noreturn))
missing_mandatory_interface (const struct nacl_interface *i)
{
  static const char before[] =
    "FATAL: NaCl IRT interface query failed for essential interface \"";
  static const char after[] =
    "\"\n";

  if (__nacl_irt_fdio.write != NULL)
    {
      size_t wrote;
      (*__nacl_irt_fdio.write) (2, before, sizeof before - 1, &wrote);
      (*__nacl_irt_fdio.write) (2, i->name, i->namelen - 1, &wrote);
      (*__nacl_irt_fdio.write) (2, after, sizeof after - 1, &wrote);
    }

  if (__nacl_irt_basic.exit != NULL)
    (*__nacl_irt_basic.exit) (-1);

  __builtin_trap ();
}

static void
initialize_mandatory_interfaces (void)
{
  const struct nacl_interface *i = __start_nacl_mandatory_interface_names;
  uintptr_t *t = __start_nacl_mandatory_interface_tables;
  while (i < __stop_nacl_mandatory_interface_names)
    {
      if (__nacl_irt_query (i->name, t, i->table_size) != i->table_size)
        missing_mandatory_interface (i);

      t = next_nacl_table (t, i);
      i = next_nacl_interface (i);
    }
}


static int
nacl_missing_optional_interface (void)
{
  return ENOSYS;
}

static void
initialize_optional_interfaces (void)
{
  const struct nacl_interface *i = __start_nacl_optional_interface_names;
  uintptr_t *t = __start_nacl_optional_interface_tables;
  while (i < __stop_nacl_optional_interface_names)
    {
      size_t filled = __nacl_irt_query (i->name, t, i->table_size);
      if (filled == 0)
        for (size_t slot = 0; slot < i->table_size / sizeof *t; ++slot)
          t[slot] = (uintptr_t) &nacl_missing_optional_interface;

      t = next_nacl_table (t, i);
      i = next_nacl_interface (i);
    }
}


void attribute_hidden
__nacl_initialize_interfaces (void)
{
  initialize_mandatory_interfaces ();
  initialize_optional_interfaces ();
}