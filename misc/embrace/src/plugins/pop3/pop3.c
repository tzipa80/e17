/*
 * $Id$
 *
 * Copyright (C) 2004 Embrace project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore_Con.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include <embrace_plugin.h>

typedef enum {
	STATE_DISCONNECTED,
	STATE_CONNECTED,
	STATE_SERVER_READY,
	STATE_USER_OK,
	STATE_PASS_OK,
	STATE_STAT_OK
} State;

#define IDENT "EMBRACE_POP3_PLUGIN"
#define MAX_INTERVAL 300

static int on_server_add (void *udata, int type, void *event)
{
	Ecore_Con_Event_Server_Add *ev = event;
	MailBox *mb;

	if (strcmp ((char *) udata, IDENT))
		return 1;

	mb = ecore_con_server_data_get (ev->server);
	mailbox_property_set (mb, "state", (void *) STATE_CONNECTED);

	return 0;
}

static int on_server_data (void *udata, int type, void *event)
{
	Ecore_Con_Event_Server_Data *ev = event;
	MailBox *mb;
	State state;
	char inbuf[2048], outbuf[256];
	int num = 0, size = 0, len;

	if (strcmp ((char *) udata, IDENT))
		return 1;

	mb = ecore_con_server_data_get (ev->server);
	state = (State) mailbox_property_get (mb, "state");

	/* take the data and make a NUL-terminated string out of it */
	len = sizeof (inbuf) - 1;
	len = MIN(len, ev->size);

	memcpy (inbuf, ev->data, len);
	inbuf[len] = 0;
	embrace_strstrip (inbuf);

	if (!strncmp (inbuf, "-ERR", 4)) {
		fprintf (stderr, "[pop3] error: %s\n", &inbuf[5]);
		return 0;
	} else if (strncmp (inbuf, "+OK", 3)) {
		assert (false);
		return 0;
	}

	mailbox_property_set (mb, "state", (void *) ++state);

	switch (state) {
		case STATE_DISCONNECTED:
		case STATE_CONNECTED:
			assert (false);
			break;
		case STATE_SERVER_READY:
			len = snprintf (outbuf, sizeof (outbuf), "USER %s\r\n",
		                (char *) mailbox_property_get (mb, "user"));
			ecore_con_server_send (ev->server, outbuf, len);
			break;
		case STATE_USER_OK:
			len = snprintf (outbuf, sizeof (outbuf), "PASS %s\r\n",
		                (char *) mailbox_property_get (mb, "pass"));
			ecore_con_server_send (ev->server, outbuf, len);
			break;
		case STATE_PASS_OK:
			len = snprintf (outbuf, sizeof (outbuf), "STAT\r\n");
			ecore_con_server_send (ev->server, outbuf, len);
			break;
		case STATE_STAT_OK:
			if (sscanf (inbuf, "+OK %i %i", &num, &size) == 2) {
				mailbox_unseen_set (mb, num);
				mailbox_total_set (mb, num);

				ecore_con_server_send (ev->server, "QUIT", 4);
				ecore_con_server_del (ev->server);
			}

			break;
	}

	return 0;
}

static int on_server_del (void *udata, int type, void *event)
{
	Ecore_Con_Event_Server_Del *ev = event;
	MailBox *mb;
	char *host;

	if (strcmp ((char *) udata, IDENT))
		return 1;

	mb = ecore_con_server_data_get (ev->server);
	host = (char *) mailbox_property_get (mb, "host");

	if (mailbox_property_get (mb, "state") == STATE_DISCONNECTED)
		fprintf (stderr, "[pop3] cannot connect to '%s'\n", host);
	else {
		mailbox_property_set (mb, "state", STATE_DISCONNECTED);
		fprintf (stderr, "[pop3] lost connection to '%s'\n", host);
	}

	ecore_con_server_del (ev->server);

	return 0;
}

static bool pop3_check (MailBox *mb)
{
	Ecore_Con_Type type = ECORE_CON_REMOTE_SYSTEM;
	char *host;
	int port;

	host = mailbox_property_get (mb, "host"),
	port = (int) mailbox_property_get (mb, "port");

	assert (host);
	assert (port);

#ifdef HAVE_OPENSSL
	if (mailbox_property_get (mb, "ssl"))
		type |= ECORE_CON_USE_SSL;
#endif

	mailbox_property_set (mb, "state", STATE_DISCONNECTED);
	ecore_con_server_connect (type, host, port, mb);

	return true;
}

static int on_timer (void *udata)
{
	pop3_check (udata);

	return 1;
}

static bool pop3_add_mailbox (MailBox *mb)
{
	Ecore_Timer *timer;
	int interval;

	assert (mb);

	interval = MAX(mailbox_poll_interval_get (mb), MAX_INTERVAL);

	if (!(timer = ecore_timer_add (interval, on_timer, mb)))
		return false;

	mailbox_property_set (mb, "timer", timer);

	return true;
}

static bool pop3_remove_mailbox (MailBox *mb)
{
	Ecore_Timer *timer;

	assert (mb);

	if (!(timer = mailbox_property_get (mb, "timer")))
		ecore_timer_del (timer);

	free (mailbox_property_get (mb, "host"));
	free (mailbox_property_get (mb, "user"));
	free (mailbox_property_get (mb, "pass"));

	return true;
}

static bool pop3_load_config (MailBox *mb, E_DB_File *edb,
                              const char *root)
{
	char key[32], *str;
	int val, use_ssl = 0;

	assert (mb);
	assert (edb);
	assert (root);

#ifdef HAVE_OPENSSL
	/* check whether OpenSSL should be used */
	snprintf (key, sizeof (key), "%s/ssl", root);

	if (!e_db_int_get (edb, key, &use_ssl))
		use_ssl = 0;

	mailbox_property_set (mb, "ssl", (int *) use_ssl);
#endif

	/* read server */
	snprintf (key, sizeof (key), "%s/host", root);

	if (!(str = e_db_str_get (edb, key)))
		return false;

	mailbox_property_set (mb, "host", str);

	/* read port */
	snprintf (key, sizeof (key), "%s/port", root);

	if (!e_db_int_get (edb, key, &val))
		val = use_ssl ? 995 : 110;

	mailbox_property_set (mb, "port", (int *) val);

	/* read username */
	snprintf (key, sizeof (key), "%s/user", root);

	if ((str = e_db_str_get (edb, key)))
		mailbox_property_set (mb, "user", str);

	/* read password */
	snprintf (key, sizeof (key), "%s/pass", root);

	if ((str = e_db_str_get (edb, key)))
		mailbox_property_set (mb, "pass", str);

	return true;
}

static void pop3_shutdown ()
{
	ecore_con_shutdown ();
}

bool embrace_plugin_init (EmbracePlugin *ep)
{
	snprintf (ep->name, sizeof (ep->name), "%s", "pop3");

	ep->check = pop3_check;
	ep->load_config = pop3_load_config;
	ep->shutdown = pop3_shutdown;

	ep->add_mailbox = pop3_add_mailbox;
	ep->remove_mailbox = pop3_remove_mailbox;

	ecore_con_init ();

	ecore_event_handler_add (ECORE_CON_EVENT_SERVER_ADD,
	                         on_server_add, IDENT);
	ecore_event_handler_add (ECORE_CON_EVENT_SERVER_DATA,
	                         on_server_data, IDENT);
	ecore_event_handler_add (ECORE_CON_EVENT_SERVER_DEL,
	                         on_server_del, IDENT);

	return true;
}
