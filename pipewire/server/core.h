/* PipeWire
 * Copyright (C) 2015 Wim Taymans <wim.taymans@gmail.com>
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

#ifndef __PIPEWIRE_CORE_H__
#define __PIPEWIRE_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <spa/log.h>

struct pw_global;

#include <pipewire/client/type.h>
#include <pipewire/server/access.h>
#include <pipewire/server/main-loop.h>
#include <pipewire/server/data-loop.h>
#include <pipewire/server/node.h>
#include <pipewire/server/link.h>
#include <pipewire/server/node-factory.h>

typedef int (*pw_bind_func_t) (struct pw_global *global,
			       struct pw_client *client, uint32_t version, uint32_t id);

/** \class pw_global
 *
 * \brief A global object visible to all clients
 *
 * A global object is visible to all clients and represents a resource
 * that can be used or inspected.
 */
struct pw_global {
	struct pw_core *core;		/**< the core */
	struct pw_client *owner;	/**< the owner of this object, NULL when the
					  *  PipeWire server is the owner */

	struct spa_list link;		/**< link in core list of globals */
	uint32_t id;			/**< server id of the object */
	uint32_t type;			/**< type of the object */
	uint32_t version;		/**< version of the object */
	void *object;			/**< object associated with the global */

	/** Emited when the global is destroyed */
	PW_SIGNAL(destroy_signal, (struct pw_listener *listener, struct pw_global *global));
};

/** \class pw_core
 *
 * \brief the core PipeWire object
 *
 * The server core object manages all resources available on the
 * server.
 */
struct pw_core {
	struct pw_global *global;	/**< the global of the core */

	struct pw_core_info info;	/**< info about the core */

	struct pw_properties *properties;	/**< properties of the core */

	struct pw_type type;		/**< type map and common types */
	struct pw_access *access;	/**< access control checks */

	struct pw_map objects;		/**< map of known objects */

	struct spa_list resource_list;		/**< list of resources */
	struct spa_list registry_resource_list;	/**< list of registry resources */
	struct spa_list global_list;		/**< list of globals */
	struct spa_list client_list;		/**< list of clients */
	struct spa_list node_list;		/**< list of nodes */
	struct spa_list node_factory_list;	/**< list of node factories */
	struct spa_list link_list;		/**< list of links */

	struct pw_main_loop *main_loop;	/**< main loop for control */
	struct pw_data_loop *data_loop;	/**< data loop for data passing */

	struct spa_support *support;	/**< support for spa plugins */
	uint32_t n_support;		/**< number of support items */

	/** Emited when the core is destroyed */
	PW_SIGNAL(destroy_signal, (struct pw_listener *listener, struct pw_core *core));

	/** Emited when a global is added */
	PW_SIGNAL(global_added, (struct pw_listener *listener,
				 struct pw_core *core, struct pw_global *global));
	/** Emited when a global is removed */
	PW_SIGNAL(global_removed, (struct pw_listener *listener,
				   struct pw_core *core, struct pw_global *global));
};

struct pw_core *
pw_core_new(struct pw_main_loop *main_loop, struct pw_properties *props);

void
pw_core_destroy(struct pw_core *core);

void
pw_core_update_properties(struct pw_core *core, const struct spa_dict *dict);

bool
pw_core_add_global(struct pw_core *core,
		   struct pw_client *owner,
		   uint32_t type,
		   uint32_t version,
		   void *object, pw_bind_func_t bind,
		   struct pw_global **global);

int
pw_global_bind(struct pw_global *global,
	       struct pw_client *client, uint32_t version, uint32_t id);

void
pw_global_destroy(struct pw_global *global);

struct spa_format *
pw_core_find_format(struct pw_core *core,
		    struct pw_port *output,
		    struct pw_port *input,
		    struct pw_properties *props,
		    uint32_t n_format_filters,
		    struct spa_format **format_filters,
		    char **error);

struct pw_port *
pw_core_find_port(struct pw_core *core,
		  struct pw_port *other_port,
		  uint32_t id,
		  struct pw_properties *props,
		  uint32_t n_format_filters,
		  struct spa_format **format_filters,
		  char **error);

struct pw_node_factory *
pw_core_find_node_factory(struct pw_core *core, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* __PIPEWIRE_CORE_H__ */