/* Simple Plugin API
 * Copyright (C) 2016 Wim Taymans <wim.taymans@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __SPA_POLL_H__
#define __SPA_POLL_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SpaPoll SpaPoll;

#define SPA_POLL_URI             "http://spaplug.in/ns/poll"
#define SPA_POLL_PREFIX          SPA_POLL_URI "#"
#define SPA_POLL__MainLoop       SPA_POLL_PREFIX "MainLoop"
#define SPA_POLL__DataLoop       SPA_POLL_PREFIX "DataLoop"

#include <spa/defs.h>
#include <spa/plugin.h>
#include <spa/dict.h>

/**
 * SpaPollFd:
 * @fd: a file descriptor
 * @events: events to watch
 * @revents: events after poll
 */
typedef struct {
  int   fd;
  short events;
  short revents;
} SpaPollFd;

/**
 * SpaPollNotifyData:
 * @user_data: user data
 * @fds: array of file descriptors
 * @n_fds: number of elements in @fds
 *
 * Data passed to #SpaPollNotify.
 */
typedef struct {
  void *user_data;
  SpaPollFd *fds;
  unsigned int n_fds;
} SpaPollNotifyData;

typedef int (*SpaPollNotify) (SpaPollNotifyData *data);

/**
 * SpaPollItem:
 * @id: id of the poll item. This will be set when
 *      adding the item to #SpaPoll.
 * @enabled: if the item is enabled
 * @fds: array of file descriptors to watch
 * @n_fds: number of elements in @fds
 * @idle_cb: callback called when there is no other work
 * @before_cb: callback called before starting the poll
 * @after_cb: callback called after the poll loop
 * @user_data: user data passed to callbacks
 */
typedef struct {
  uint32_t       id;
  bool           enabled;
  SpaPollFd     *fds;
  unsigned int   n_fds;
  SpaPollNotify  idle_cb;
  SpaPollNotify  before_cb;
  SpaPollNotify  after_cb;
  void          *user_data;
} SpaPollItem;

/**
 * SpaPoll:
 *
 * Register poll events
 */
struct _SpaPoll {
  /* pointer to the handle owning this interface */
  SpaHandle *handle;
  /* the total size of this structure. This can be used to expand this
   * structure in the future */
  size_t size;
  /**
   * SpaPoll::info
   *
   * Extra information
   */
  const SpaDict *info;
  /**
   * SpaPoll::add_item:
   * @poll: a #SpaPoll
   * @item: a #SpaPollItem
   *
   * Add @item to the list of polled items.
   *
   * The id in @item will be set and must be passed when updating or removing
   * the @item.
   *
   * Returns: #SPA_RESULT_OK on success
   *          #SPA_RESULT_INVALID_ARGUMENTS when @poll or @item is %NULL
   */
  SpaResult   (*add_item)            (SpaPoll         *poll,
                                      SpaPollItem     *item);

  SpaResult   (*update_item)         (SpaPoll         *poll,
                                      SpaPollItem     *item);

  SpaResult   (*remove_item)         (SpaPoll         *poll,
                                      SpaPollItem     *item);
};

#define spa_poll_add_item(n,...)          (n)->add_item((n),__VA_ARGS__)
#define spa_poll_update_item(n,...)       (n)->update_item((n),__VA_ARGS__)
#define spa_poll_remove_item(n,...)       (n)->remove_item((n),__VA_ARGS__)

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* __SPA_POLL_H__ */
