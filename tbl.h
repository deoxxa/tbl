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

#ifndef TBL_H
#define TBL_H

#include <stddef.h> /* size_t */
#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

enum tbl_error {
  TBL_E_NONE,
  TBL_E_INVALID_DATA,
  TBL_E_CANCELED_BY_USER,
  TBL_E_NO_CALLBACKS,
  TBL_E_UNKNOWN
};

struct tbl_handle {
  jmp_buf    *err;
  const char *ptr;
  const char *end;
  void       *ctx;
};

/* parsing is stopped if a callback returns something else than 0 */
struct tbl_callbacks {
  int (*integer)   (void *ctx, long long value);
  int (*string)    (void *ctx, char *value, size_t length);

  int (*list_start)(void *ctx);
  int (*list_end)  (void *ctx);

  int (*dict_start)(void *ctx);
  int (*dict_key)  (void *ctx, char *key, size_t length);
  int (*dict_end)  (void *ctx);
};

typedef struct tbl_handle tbl_handle_t;
typedef struct tbl_callbacks tbl_callbacks_t;

int tbl_parse(const char *buf, size_t length,
              const struct tbl_callbacks *callbacks, void *ctx);

/* the callback is passed so tbl_parse_string can call either the string callback or
 * the dict key callback */
static void tbl_parse_string(int (*event_fn)(void *ctx, char *value, size_t length),
                         struct tbl_handle *handle);
/* functions to parse container types */
static void tbl_parse_integer(const struct tbl_callbacks *callbacks, struct tbl_handle *handle);
static void tbl_parse_list(const struct tbl_callbacks *callbacks, struct tbl_handle *handle);
static void tbl_parse_dict(const struct tbl_callbacks *callbacks, struct tbl_handle *handle);
/* gets the first char of the buffer to decide which type has to be parsed */
static void tbl_parse_next(const struct tbl_callbacks *callbacks, struct tbl_handle *handle);

/* only prototypes; nothing of this is implemented yet */
int tbl_gen_integer(tbl_handle_t *handle, long value);
int tbl_gen_string(tbl_handle_t *handle, const char *str, size_t len);
int tbl_gen_dict_open(tbl_handle_t *handle);
int tbl_gen_dict_close(tbl_handle_t *handle);
int tbl_gen_list_open(tbl_handle_t *handle);
int tbl_gen_list_close(tbl_handle_t *handle);

#ifdef __cplusplus
}
#endif

#endif /* TBL_H */
