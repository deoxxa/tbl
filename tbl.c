/*
 * Copyright (c) 2011 tbl developers
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

#include "tbl.h"

static void tbl_parse_integer(tbl_callbacks_t *callbacks, tbl_handle_t *handle)
{
  long long value;
  char *p, *q;

  q = memchr(handle->ptr, 'e', handle->end - handle->ptr);
  if (!q)
    longjmp(*handle->err, TBL_E_INVALID_DATA);

#ifdef HAVE_STRTOLL
  value = strtoll(handle->ptr, &p, 10);
#else
  p = (char *)handle->ptr;
#ifdef HAVE_ATOI64
  value = _atoi64(handle->ptr);
#else
  /* Fallback to 32bit atoi. Tests for large integers fail. */
  value = atoi(handle->ptr);
#endif
  /* dirty hack to look for the end of the number */
  while (*p == '-' || isdigit(*p))
    p++;
#endif

  if (p != q || errno == ERANGE)
    longjmp(*handle->err, TBL_E_INVALID_DATA);
  /* preceding 0 arent't allowed and i0e is still valid */
  if (value && *handle->ptr == '0')
    longjmp(*handle->err, TBL_E_INVALID_DATA);
  if (callbacks->integer && callbacks->integer(handle->ctx, value))
    longjmp(*handle->err, TBL_E_CANCELED_BY_USER);

  handle->ptr = q + 1; /* skip e */
}

void tbl_parse_string(int (*event_fn)(void *ctx, char *value, size_t length), tbl_handle_t *handle)
{
  size_t len;
  char *ptr, *endptr;

  ptr = memchr(handle->ptr, ':', handle->end - handle->ptr);
  if (!ptr)
    longjmp(*handle->err, TBL_E_INVALID_DATA);

  len = strtol(handle->ptr, &endptr, 10);
  if (errno == ERANGE || endptr != ptr || ++endptr + len > handle->end)
    longjmp(*handle->err, TBL_E_INVALID_DATA);
  if (event_fn && event_fn(handle->ctx, endptr, len))
    longjmp(*handle->err, TBL_E_CANCELED_BY_USER);

  handle->ptr = endptr + len; /* jump to next token */
}

void tbl_parse_list(tbl_callbacks_t *callbacks, tbl_handle_t *handle)
{
  /* list start */
  if (callbacks->list_start && callbacks->list_start(handle->ctx))
    longjmp(*handle->err, TBL_E_CANCELED_BY_USER);
  /* entries */
  while (*handle->ptr != 'e')
    tbl_parse_next(callbacks, handle);
  /* list end */
  if (callbacks->list_end && callbacks->list_end(handle->ctx))
    longjmp(*handle->err, TBL_E_CANCELED_BY_USER);

  handle->ptr++; /* skip 'e' */
}

void tbl_parse_dict(tbl_callbacks_t *callbacks, tbl_handle_t *handle)
{
  /* dict start */
  if (callbacks->dict_start && callbacks->dict_start(handle->ctx))
    longjmp(*handle->err, TBL_E_CANCELED_BY_USER);

  /* keys + entries */
  while (*handle->ptr != 'e') {
    tbl_parse_string(callbacks->dict_key, handle);
    tbl_parse_next(callbacks, handle);
  }
  /* dict end */
  if (callbacks->dict_end && callbacks->dict_end(handle->ctx))
    longjmp(*handle->err, TBL_E_CANCELED_BY_USER);

  handle->ptr++; /* skip 'e' */
}

void tbl_parse_next(tbl_callbacks_t *callbacks, tbl_handle_t *handle)
{
  char c = *handle->ptr++;

  if (handle->ptr >= handle->end)
    longjmp(*handle->err, TBL_E_INVALID_DATA);

  /* get type of next entry */
  if (c == 'i')
    tbl_parse_integer(callbacks, handle);
  else if (isdigit(c) != 0) {
    handle->ptr--; /* string has no prefix like i d or l to be skipped */
    tbl_parse_string(callbacks->string, handle);
  }
  else if (c == 'l')
    tbl_parse_list(callbacks, handle);
  else if (c == 'd')
    tbl_parse_dict(callbacks, handle);
  else
    longjmp(*handle->err, TBL_E_INVALID_DATA);
}

int tbl_parse(const char *buf, size_t length, tbl_callbacks_t *callbacks, void *ctx)
{
  jmp_buf env;
  int err;
  tbl_handle_t handle = { &env, buf, buf + length, ctx };

  if (!callbacks)
    return TBL_E_NO_CALLBACKS;

  err = setjmp(env);
  if (err == TBL_E_NONE && handle.ptr < handle.end)
    tbl_parse_next(callbacks, &handle);

  return err;
}

int tbl_gen_integer(tbl_handle_t *handle, long value)
{
  return 0;
}

int tbl_gen_string(tbl_handle_t *handle, const char *str, size_t len)
{
  return 0;
}

int tbl_gen_dict_open(tbl_handle_t *handle)
{
  return 0;
}

int tbl_gen_dict_close(tbl_handle_t *handle)
{
  return 0;
}

int tbl_gen_list_open(tbl_handle_t *handle)
{
  return 0;
}

int tbl_gen_list_close(tbl_handle_t *handle)
{
  return 0;
}
