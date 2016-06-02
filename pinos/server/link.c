/* Pinos
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

#include <string.h>

#include <gio/gio.h>

#include "pinos/client/pinos.h"
#include "pinos/client/enumtypes.h"

#include "pinos/server/link.h"
#include "pinos/server/port.h"

#include "pinos/dbus/org-pinos.h"

#define PINOS_LINK_GET_PRIVATE(obj)  \
   (G_TYPE_INSTANCE_GET_PRIVATE ((obj), PINOS_TYPE_LINK, PinosLinkPrivate))

struct _PinosLinkPrivate
{
  PinosDaemon *daemon;
  PinosLink1 *iface;

  gchar *object_path;

  gulong input_id, output_id;
  gboolean active;
  PinosPort *output;
  PinosPort *input;
  GBytes *possible_formats;
  GBytes *format;
};

G_DEFINE_TYPE (PinosLink, pinos_link, G_TYPE_OBJECT);

enum
{
  PROP_0,
  PROP_DAEMON,
  PROP_OUTPUT,
  PROP_INPUT,
  PROP_OBJECT_PATH,
  PROP_POSSIBLE_FORMATS,
  PROP_FORMAT,
};

enum
{
  SIGNAL_REMOVE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static gboolean
on_output_send (PinosPort *port, PinosBuffer *buffer, GError **error, gpointer user_data)
{
  PinosLink *link = user_data;
  PinosLinkPrivate *priv = link->priv;

  return pinos_port_receive_buffer (priv->input, buffer, error);
}

static gboolean
on_input_send (PinosPort *port, PinosBuffer *buffer, GError **error, gpointer user_data)
{
  PinosLink *link = user_data;
  PinosLinkPrivate *priv = link->priv;

  return pinos_port_receive_buffer (priv->output, buffer, error);
}

static void
pinos_link_get_property (GObject    *_object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  PinosLink *link = PINOS_LINK (_object);
  PinosLinkPrivate *priv = link->priv;

  switch (prop_id) {
    case PROP_DAEMON:
      g_value_set_object (value, priv->daemon);
      break;

    case PROP_OUTPUT:
      g_value_set_object (value, priv->output);
      break;

    case PROP_INPUT:
      g_value_set_object (value, priv->input);
      break;

    case PROP_OBJECT_PATH:
      g_value_set_string (value, priv->object_path);
      break;

    case PROP_POSSIBLE_FORMATS:
      g_value_set_boxed (value, priv->possible_formats);
      break;

    case PROP_FORMAT:
      g_value_set_boxed (value, priv->format);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (link, prop_id, pspec);
      break;
  }
}

static void
pinos_link_set_property (GObject      *_object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  PinosLink *link = PINOS_LINK (_object);
  PinosLinkPrivate *priv = link->priv;

  switch (prop_id) {
    case PROP_DAEMON:
      priv->daemon = g_value_dup_object (value);
      break;

    case PROP_OUTPUT:
      priv->output = g_value_dup_object (value);
      break;

    case PROP_INPUT:
      priv->input = g_value_dup_object (value);
      break;

    case PROP_OBJECT_PATH:
      priv->object_path = g_value_dup_string (value);
      break;

    case PROP_POSSIBLE_FORMATS:
      if (priv->possible_formats)
        g_bytes_unref (priv->possible_formats);
      priv->possible_formats = g_value_dup_boxed (value);
      break;

    case PROP_FORMAT:
      if (priv->format)
        g_bytes_unref (priv->format);
      priv->format = g_value_dup_boxed (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (link, prop_id, pspec);
      break;
  }
}

static void
link_register_object (PinosLink *link)
{
  PinosLinkPrivate *priv = link->priv;
  PinosObjectSkeleton *skel;
  gchar *name;

  name = g_strdup_printf (PINOS_DBUS_OBJECT_LINK);
  skel = pinos_object_skeleton_new (name);
  g_free (name);

  pinos_object_skeleton_set_link1 (skel, priv->iface);

  g_free (priv->object_path);
  priv->object_path = pinos_daemon_export_uniquely (priv->daemon, G_DBUS_OBJECT_SKELETON (skel));
  g_object_unref (skel);

  g_debug ("link %p: register object %s", link, priv->object_path);
}

static void
link_unregister_object (PinosLink *link)
{
  PinosLinkPrivate *priv = link->priv;

  g_debug ("link %p: unregister object", link);
  pinos_daemon_unexport (priv->daemon, priv->object_path);
}

static gboolean
on_activate (PinosPort *port, gpointer user_data)
{
  PinosLink *link = user_data;
  PinosLinkPrivate *priv = link->priv;

  if (priv->active)
    return TRUE;

  if (priv->input == port)
    pinos_port_activate (priv->output);
  else
    pinos_port_activate (priv->input);
  priv->active = TRUE;

  return TRUE;
}

static gboolean
on_deactivate (PinosPort *port, gpointer user_data)
{
  PinosLink *link = user_data;
  PinosLinkPrivate *priv = link->priv;

  if (!priv->active)
    return TRUE;

  if (priv->input == port)
    pinos_port_deactivate (priv->output);
  else
    pinos_port_deactivate (priv->input);
  priv->active = FALSE;

  return TRUE;
}

static void
pinos_link_constructed (GObject * object)
{
  PinosLink *link = PINOS_LINK (object);
  PinosLinkPrivate *priv = link->priv;
  GBytes *formats;

  priv->output_id = pinos_port_add_send_buffer_cb (priv->output,
                                 on_output_send,
                                 link,
                                 NULL);

  priv->input_id = pinos_port_add_send_buffer_cb (priv->input,
                                 on_input_send,
                                 link,
                                 NULL);

  g_object_get (priv->input, "possible-formats", &formats, NULL);
  g_object_set (priv->output, "possible-formats", formats, NULL);
  g_object_get (priv->output, "format", &formats, NULL);
  g_object_set (priv->input, "format", formats, NULL);

  g_signal_connect (priv->input, "activate", (GCallback) on_activate, link);
  g_signal_connect (priv->input, "deactivate", (GCallback) on_deactivate, link);
  g_signal_connect (priv->output, "activate", (GCallback) on_activate, link);
  g_signal_connect (priv->output, "deactivate", (GCallback) on_deactivate, link);

  g_debug ("link %p: constructed", link);
  link_register_object (link);

  G_OBJECT_CLASS (pinos_link_parent_class)->constructed (object);
}

static void
pinos_link_dispose (GObject * object)
{
  PinosLink *link = PINOS_LINK (object);
  PinosLinkPrivate *priv = link->priv;

  g_debug ("link %p: dispose", link);

  pinos_port_remove_send_buffer_cb (priv->input, priv->input_id);
  pinos_port_remove_send_buffer_cb (priv->output, priv->output_id);
  if (priv->active) {
    priv->active = FALSE;
    pinos_port_deactivate (priv->input);
    pinos_port_deactivate (priv->output);
  }
  g_clear_object (&priv->input);
  g_clear_object (&priv->output);
  link_unregister_object (link);

  G_OBJECT_CLASS (pinos_link_parent_class)->dispose (object);
}

static void
pinos_link_finalize (GObject * object)
{
  PinosLink *link = PINOS_LINK (object);
  PinosLinkPrivate *priv = link->priv;

  g_debug ("link %p: finalize", link);
  g_clear_pointer (&priv->possible_formats, g_bytes_unref);
  g_clear_pointer (&priv->format, g_bytes_unref);
  g_clear_object (&priv->daemon);
  g_clear_object (&priv->iface);
  g_free (priv->object_path);

  G_OBJECT_CLASS (pinos_link_parent_class)->finalize (object);
}

static void
pinos_link_class_init (PinosLinkClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  //PinosLinkClass *link_class = PINOS_LINK_CLASS (klass);

  g_type_class_add_private (klass, sizeof (PinosLinkPrivate));

  gobject_class->constructed = pinos_link_constructed;
  gobject_class->dispose = pinos_link_dispose;
  gobject_class->finalize = pinos_link_finalize;
  gobject_class->set_property = pinos_link_set_property;
  gobject_class->get_property = pinos_link_get_property;

  g_object_class_install_property (gobject_class,
                                   PROP_DAEMON,
                                   g_param_spec_object ("daemon",
                                                        "Daemon",
                                                        "The Daemon",
                                                        PINOS_TYPE_DAEMON,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_OUTPUT,
                                   g_param_spec_object ("output",
                                                        "Output",
                                                        "The output port",
                                                        PINOS_TYPE_PORT,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_INPUT,
                                   g_param_spec_object ("input",
                                                        "Input",
                                                        "The input port",
                                                        PINOS_TYPE_PORT,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_POSSIBLE_FORMATS,
                                   g_param_spec_boxed ("possible-formats",
                                                       "Possible Formats",
                                                       "The possbile formats of the link",
                                                       G_TYPE_BYTES,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT |
                                                       G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
                                   PROP_FORMAT,
                                   g_param_spec_boxed ("format",
                                                       "Format",
                                                       "The format of the link",
                                                       G_TYPE_BYTES,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT |
                                                       G_PARAM_STATIC_STRINGS));

  signals[SIGNAL_REMOVE] = g_signal_new ("remove",
                                         G_TYPE_FROM_CLASS (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0,
                                         NULL,
                                         NULL,
                                         g_cclosure_marshal_generic,
                                         G_TYPE_NONE,
                                         0,
                                         G_TYPE_NONE);
}

static void
pinos_link_init (PinosLink * link)
{
  PinosLinkPrivate *priv = link->priv = PINOS_LINK_GET_PRIVATE (link);

  priv->iface = pinos_link1_skeleton_new ();
  g_debug ("link %p: new", link);
}

PinosLink *
pinos_link_new (PinosDaemon *daemon,
                PinosPort   *output,
                PinosPort   *input,
                GBytes      *format_filter)
{
  PinosLink *link;
  PinosPort *tmp;

  if (pinos_port_get_direction (output) != PINOS_DIRECTION_OUTPUT) {
    tmp = output;
    output = input;
    input = tmp;
  }

  link = g_object_new (PINOS_TYPE_LINK,
                       "daemon", daemon,
                       "output", output,
                       "input", input,
                       "possible-formats", format_filter,
                       NULL);

  return link;
}

/**
 * pinos_link_remove:
 * @link: a #PinosLink
 *
 * Trigger removal of @link
 */
void
pinos_link_remove (PinosLink *link)
{
  g_return_if_fail (PINOS_IS_LINK (link));

  g_debug ("link %p: remove", link);
  g_signal_emit (link, signals[SIGNAL_REMOVE], 0, NULL);
}

/**
 * pinos_link_get_object_path:
 * @link: a #PinosLink
 *
 * Get the object patch of @link
 *
 * Returns: the object path of @source.
 */
const gchar *
pinos_link_get_object_path (PinosLink *link)
{
  PinosLinkPrivate *priv;

  g_return_val_if_fail (PINOS_IS_LINK (link), NULL);
  priv = link->priv;

  return priv->object_path;
}

/**
 * pinos_link_get_possible_formats:
 * @link: a #PinosLink
 *
 * Get the possible formats of @link
 *
 * Returns: the possible formats or %NULL
 */
GBytes *
pinos_link_get_possible_formats (PinosLink *link)
{
  PinosLinkPrivate *priv;

  g_return_val_if_fail (PINOS_IS_LINK (link), NULL);
  priv = link->priv;

  return priv->possible_formats;
}

/**
 * pinos_link_get_format:
 * @link: a #PinosLink
 *
 * Get the format of @link
 *
 * Returns: the format or %NULL
 */
GBytes *
pinos_link_get_format (PinosLink *link)
{
  PinosLinkPrivate *priv;

  g_return_val_if_fail (PINOS_IS_LINK (link), NULL);
  priv = link->priv;

  return priv->format;
}