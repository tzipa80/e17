/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include <e.h>
#include "e_mod_main.h"
#include "imap2.h"

#if 0
#define D(args...) printf(##args)
#else
#define D(args...)
#endif

/*
 * TODO:
 * * Let the user select between Unseen, Recent and New mail
 * * Don't fail on incomplete data from the server
 */

static ImapClient *_mail_imap_client_find (Ecore_Con_Server *server);
static ImapClient *_mail_imap_client_get (Config_Box *cb);
static int _mail_imap_server_add (void *data, int type, void *event);
static int _mail_imap_server_del (void *data, int type, void *event);
static int _mail_imap_server_data (void *data, int type, void *event);
static int _mail_imap_server_data_parse (ImapClient *ic, char *line);
static void _mail_imap_client_logout (ImapClient *ic);
static void _mail_imap_server_idle (ImapClient *ic);
static void _mail_imap_server_noop (ImapClient *ic);

static int   elements (char *p);
static char *find_rn (char *data, unsigned int size);

static Evas_List *iclients = NULL;

void
_mail_imap_check_mail (void *data)
{
   Ecore_Con_Type type;
   Evas_List *l;

   for (l = iclients; l; l = l->next)
     {
	ImapClient *ic;

	ic = l->data;
	ic->data = data;
	D ("Checking (%s:%s): %p\n", ic->config->host, ic->config->new_path, ic->server);
	if (!ic->server)
	  {
	     if (!ic->add_handler)
	       ic->add_handler =
		  ecore_event_handler_add (ECORE_CON_EVENT_SERVER_ADD,
					   _mail_imap_server_add, NULL);
	     if (!ic->del_handler)
	       ic->del_handler =
		  ecore_event_handler_add (ECORE_CON_EVENT_SERVER_DEL,
					   _mail_imap_server_del, NULL);
	     if (!ic->data_handler)
	       ic->data_handler =
		  ecore_event_handler_add (ECORE_CON_EVENT_SERVER_DATA,
					   _mail_imap_server_data, NULL);

	     if (ic->config->local)
	       type = ECORE_CON_LOCAL_SYSTEM;
	     else
	       type = ECORE_CON_REMOTE_SYSTEM;

	     if (ecore_con_ssl_available_get () && (ic->config->ssl))
	       type |= ECORE_CON_USE_SSL;
	     ic->state = IMAP_STATE_DISCONNECTED;
	     ic->server =
		ecore_con_server_connect (type, ic->config->host,
					  ic->config->port, NULL);
	     ic->cmd = 1;
	     ic->idle = -1;
	     ic->idling = 0;
	  }
	else
	  {
	     if (ic->idling) _mail_imap_server_idle (ic);
	     else _mail_imap_server_noop (ic);
	     /* Need to set this to revert the state of the icon */
	     _mail_set_text (ic->data);
	  }
     }
}

void
_mail_imap_add_mailbox (void *data)
{
   Config_Box *cb;

   cb = data;
   if (!cb)
     return;
   /* Client get will create the client if it does not exist */
   _mail_imap_client_get (cb);
}

void
_mail_imap_del_mailbox (void *data)
{
   ImapClient *ic;
   Config_Box *cb;

   cb = data;
   if (!cb)
     return;
   ic = _mail_imap_client_get (cb);
   if (!ic)
     return;
   if (ic->add_handler)
     ecore_event_handler_del (ic->add_handler);
   if (ic->del_handler)
     ecore_event_handler_del (ic->del_handler);
   if (ic->data_handler)
     ecore_event_handler_del (ic->data_handler);
   iclients = evas_list_remove (iclients, ic);
   _mail_imap_client_logout (ic);
   E_FREE (ic);
}

void
_mail_imap_shutdown ()
{
   while (iclients)
     {
	ImapClient *ic;

	ic = iclients->data;
	if (ic->add_handler)
	  ecore_event_handler_del (ic->add_handler);
	if (ic->del_handler)
	  ecore_event_handler_del (ic->del_handler);
	if (ic->data_handler)
	  ecore_event_handler_del (ic->data_handler);
	iclients = evas_list_remove_list (iclients, iclients);
	_mail_imap_client_logout (ic);
	E_FREE (ic->prev.data);
	E_FREE (ic);
     }
}

/* PRIVATES */
static ImapClient *
_mail_imap_client_find (Ecore_Con_Server *server)
{
   Evas_List *l;

   for (l = iclients; l; l = l->next)
     {
	ImapClient *ic;

	ic = l->data;
	if (ic->server == server)
	  return ic;
     }
   return NULL;
}

static ImapClient *
_mail_imap_client_get (Config_Box *cb)
{
   ImapClient *ic;
   Evas_List *l;

   if (!cb)
     return NULL;

   for (l = iclients; l; l = l->next)
     {
	ic = l->data;
	if (ic->config == cb) return ic;
     }

   ic = E_NEW (ImapClient, 1);
   ic->config = cb;

   ic->cmd = 1;
   ic->state = IMAP_STATE_DISCONNECTED;

   ic->config->num_new = 0;
   ic->config->num_total = 0;

   iclients = evas_list_append (iclients, ic);
   return ic;
}

static int
_mail_imap_server_add (void *data, int type, void *event)
{
   Ecore_Con_Event_Server_Add *ev = event;
   ImapClient *ic;

   ic = _mail_imap_client_find (ev->server);
   if (!ic)
     return 1;

   ic->state = IMAP_STATE_CONNECTED;
   ic->cmd = 1;
   return 0;
}

static int
_mail_imap_server_del (void *data, int type, void *event)
{
   Ecore_Con_Event_Server_Del *ev = event;
   ImapClient *ic;

   ic = _mail_imap_client_find (ev->server);
   if (!ic)
     return 1;

   if (ic->state != IMAP_STATE_DISCONNECTED)
     {
	printf ("The connection was unexpectedly shut down, consider reducing the check time.\n");
	ic->state = IMAP_STATE_DISCONNECTED;
     }

   E_FREE (ic->prev.data);
   ecore_con_server_del (ic->server);
   ic->server = NULL;

   _mail_set_text (ic->data);

   if ((ic->config->num_new > 0) 
       && (ic->config->use_exec) && (ic->config->exec))
     _mail_start_exe (ic->config);

   return 0;
}

static int
_mail_imap_server_data (void *data, int type, void *event)
{
   Ecore_Con_Event_Server_Data *ev = event;
   ImapClient *ic;
   char *reply, *p, *pp;
   char out[1024];
   unsigned int len, size;

   ic = _mail_imap_client_find (ev->server);
   if (!ic)
     return 1;
   if (ic->state == IMAP_STATE_DISCONNECTED)
     return 1;

   D ("Data from %s:%s\n", ic->config->host, ic->config->new_path);

   /* Hijack server data.
    * We require minimum 2 characters, as the minimum server data is '\r\n' */
   if ((ic->prev.data) || (ev->size < 2))
     {
	/* TODO: Check that we don't grow to big! */
	ic->prev.data = realloc (ic->prev.data, ic->prev.size + ev->size);
	memcpy (ic->prev.data + ic->prev.size, ev->data, ev->size);
	ic->prev.size += ev->size;
	E_FREE (ev->data);
	if (ic->prev.size < 2) return 0;

	reply = ic->prev.data;
	size = ic->prev.size;
	ic->prev.data = NULL;
	ic->prev.size = 0;
     }
   else
     {
	reply = ev->data;
	size = ev->size;
     }
   ev->data = NULL;
   /* Check for correct EOD */
   if ((*(reply + size - 2) != '\r') && (*(reply + size - 1) != '\n'))
     {
	D ("Wrong eod %s:%s\n", ic->config->host, ic->config->new_path);
	/* We got incomplete data, search for last EOD */
	unsigned int pos = 0;
	char *data;

	data = reply;
	while (pos < (size - 1))
	  {
	     if ((*(reply + pos) == '\r') && (*(reply + pos + 1) == '\n'))
	       data = reply + pos + 2;
	     pos++;
	  }
	ic->prev.size = size - (data - reply);
	ic->prev.data = malloc (ic->prev.size);
	memcpy (ic->prev.data, data, ic->prev.size);

	/* Remove captured data from reply */
	if (data == reply)
	  E_FREE (reply);
	else
	  size -= ic->prev.size;
     }

   if (reply)
     {
	p = reply;
	pp = p;
	while ((p) && ((p - reply) < size))
	  {
	     /* find EOL */
	     pp = find_rn (p, size - (pp - p));
	     if (!pp)
	       {
		  printf ("Imap Failure: Couldn't find EOL\n");
		  _mail_imap_client_logout (ic);
		  return 0;
	       }
	     *pp = '\0';
	     /* parse data */
	     if (!_mail_imap_server_data_parse (ic, p))
	       {
		  _mail_imap_client_logout (ic);
		  return 0;
	       }
	     /* next */
	     p = pp + 2;
	  }
	free (reply);
     }

   switch (ic->state)
     {
      case IMAP_STATE_DISCONNECTED:
	 /* ignore */
	 break;
      case IMAP_STATE_CONNECTED:
	 if (ic->idle == -1)
	   {
	      /* Check whether this server supports idle */
	      len = snprintf (out, sizeof (out), "A%04i CAPABILITY\r\n", ic->cmd++);
	      ecore_con_server_send (ic->server, out, len);
	   }
	 else
	   {
	      /* Log in */
	      len = snprintf (out, sizeof (out), "A%04i LOGIN %s %s\r\n", ic->cmd++,
			      ic->config->user, ic->config->pass);
	      ecore_con_server_send (ic->server, out, len);
	      ic->state = IMAP_STATE_AUTHENTICATED;
	   }
	 break;
      case IMAP_STATE_AUTHENTICATED:
	 len = snprintf (out, sizeof (out), "A%04i EXAMINE %s\r\n", ic->cmd++,
			 ic->config->new_path);
	 ecore_con_server_send (ic->server, out, len);
	 ic->state = IMAP_STATE_SEARCH_UNSEEN;
	 break;
      case IMAP_STATE_IDLING:
	 if ((ic->idle == 1) && (!ic->idling))
	   {
	      D ("Begin idle\n");
	      len = snprintf (out, sizeof (out), "A%04i IDLE\r\n", ic->cmd++);
	      ecore_con_server_send (ic->server, out, len);
	      ic->idling = 1;
	   }
	 break;
      case IMAP_STATE_SEARCH_UNSEEN:
	 _mail_imap_server_idle (ic);
	 len = snprintf (out, sizeof (out), "A%04i SEARCH UNSEEN UNDELETED\r\n", ic->cmd++);
	 ecore_con_server_send (ic->server, out, len);
	 ic->state = IMAP_STATE_IDLING;
	 break;
      case IMAP_STATE_SEARCH_RECENT:
	 _mail_imap_server_idle (ic);
	 len = snprintf (out, sizeof (out), "A%04i SEARCH RECENT UNDELETED\r\n", ic->cmd++);
	 ecore_con_server_send (ic->server, out, len);
	 ic->state = IMAP_STATE_IDLING;
	 break;
      case IMAP_STATE_SEARCH_NEW:
	 _mail_imap_server_idle (ic);
	 len = snprintf (out, sizeof (out), "A%04i SEARCH NEW UNDELETED\r\n", ic->cmd++);
	 ecore_con_server_send (ic->server, out, len);
	 ic->state = IMAP_STATE_IDLING;
	 break;
     }
   if (ic->cmd > 9999)
     ic->cmd = 1;
   _mail_set_text (ic->data);
   D ("\n");
   return 0;
}

static int
_mail_imap_server_data_parse (ImapClient *ic, char *line)
{
   if (line[0] == 'A')
     {
	char *result, *value;

	result = strchr (line, ' ');
	if (!result)
	  {
	     printf ("Imap Failure: Missing result for tagged reply\n");
	     return 0;
	  }
	result++;
	value = strchr (result, ' ');
	if (value)
	  {
	     *value = '\0';
	     value++;
	  }
	else
	  value = "";
	/* This is a reply to one of our commands */
	if (!strcmp (result, "NO"))
	  {
	     printf ("Imap Failure: %s\n", value);
	     return 0;
	  }
	else if (!strcmp (result, "BAD"))
	  {
	     printf ("Imap Bad Command: %s\n", value);
	     return 0;
	  }
	else if (!strcmp (result, "OK"))
	  {
	     D ("Reply ok: %s\n", value);
	  }
	else
	  {
	     printf ("Unknown tagged reply: %s %s\n", line, value);
	  }
     }
   else if (line[0] == '*')
     {
	char *result, *value;

	result = strchr (line, ' ');
	if (!result)
	  {
	     printf ("Imap Failure: Missing result for tagged reply\n");
	     return 0;
	  }
	result++;
	value = strchr (result, ' ');
	if (value)
	  {
	     *value = '\0';
	     value++;
	  }
	else
	  value = "";

	if (!strcmp (result, "CAPABILITY"))
	  {
	     char *p, *pp;

	     p = value;
	     while (p)
	       {
		  pp = strchr (p, ' ');
		  if (pp) *pp = '\0';
		  if (!strcmp (p, "IDLE"))
		    {
		       D ("Server supports idle\n");
		       ic->idle = 1;
		    }
		  if (pp)
		    {
		       *pp = ' ';
		       p = pp + 1;
		    }
		  else
		    p = NULL;
	       }
	     if (ic->idle == -1) ic->idle = 0;
	  }
	else if (!strcmp (result, "OK"))
	  {
	     D ("Result OK: %s\n", value);
	  }
	else if (!strcmp (result, "FLAGS"))
	  {
	     D ("Flags: %s\n", value);
	  }
	else if (!strcmp (result, "SEARCH"))
	  {
	     ic->config->num_new = elements (value);
	     D ("New mail (%s:%s): %d\n", ic->config->host, ic->config->new_path, ic->config->num_new);
	  }
	else
	  {
	     char *p;

	     /* The result can be <n> <result> */
	     p = strchr (value, ' ');
	     if (p) *p = '\0';
	     if (!strcmp (value, "RECENT"))
	       {
		  D ("Recent mails: %d\n", atoi (result));
		  //ic->state = IMAP_STATE_SEARCH_UNSEEN;
		  //ic->state = IMAP_STATE_SEARCH_RECENT;
		  ic->state = IMAP_STATE_SEARCH_NEW;
	       }
	     else if (!strcmp (value, "EXISTS"))
	       {
		  D ("Existing mails: %d\n", atoi (result));
		  ic->config->num_total = atoi (result);
	       }
	     else if (!strcmp (value, "FETCH"))
	       {
		  D ("Reading mail: %d\n", atoi (result));
		  //ic->state = IMAP_STATE_SEARCH_UNSEEN;
		  //ic->state = IMAP_STATE_SEARCH_RECENT;
		  ic->state = IMAP_STATE_SEARCH_NEW;
	       }
	     else if (!strcmp (value, "EXPUNGE"))
	       {
		  D ("Deleting mail: %d\n", atoi (result));
		  //ic->state = IMAP_STATE_SEARCH_UNSEEN;
		  //ic->state = IMAP_STATE_SEARCH_RECENT;
		  ic->state = IMAP_STATE_SEARCH_NEW;
	       }
	     else
	       {
		  printf ("Unknown untagged reply: %s %s\n", line, value);
		  //ic->state = IMAP_STATE_SEARCH_UNSEEN;
		  //ic->state = IMAP_STATE_SEARCH_RECENT;
		  //ic->state = IMAP_STATE_SEARCH_NEW;
	       }
	  }
     }
   else if (line[0] == '+')
     {
	/* Continuation mode */
     }
   else
     {
	printf ("Unknown reply: %s\n", line);
     }
  return 1;
}

static void
_mail_imap_client_logout (ImapClient *ic)
{
   if (!ic)
     return;

   E_FREE (ic->prev.data);
   if (ic->server)
     {
	int len;
	char out[1024];

	len = snprintf (out, sizeof (out), "A%04i LOGOUT", ic->cmd++);
	ecore_con_server_send (ic->server, out, len);
	ecore_con_server_del (ic->server);
     }
   ic->server = NULL;
   ic->state = IMAP_STATE_DISCONNECTED;
}

static void
_mail_imap_server_idle (ImapClient *ic)
{
   char out[1024];
   int len;

   if (!ic->idling) return;
   len = snprintf (out, sizeof (out), "DONE\r\n");
   ecore_con_server_send (ic->server, out, len);
   ic->idling = 0;
 
   //ic->state = IMAP_STATE_SEARCH_UNSEEN;
   //ic->state = IMAP_STATE_SEARCH_RECENT;
   ic->state = IMAP_STATE_SEARCH_NEW;
}

static void
_mail_imap_server_noop (ImapClient *ic)
{
   char out[1024];
   int len;

   len = snprintf (out, sizeof (out), "A%04i NOOP\r\n", ic->cmd++);
   ecore_con_server_send (ic->server, out, len);
 
   //ic->state = IMAP_STATE_SEARCH_UNSEEN;
   //ic->state = IMAP_STATE_SEARCH_RECENT;
   ic->state = IMAP_STATE_SEARCH_NEW;
}

static int
elements (char *p)
{
	int count = 0;
	if (!p) return 0;
	do
	  {
	     if (*p) count++;
	     p = strchr (p, ' ');
	     if (p) p++;
	  }
	while (p);
	return count;
}

static char *
find_rn (char *data, unsigned int size)
{
   while (size >= 2)
     {
	if ((*data == '\r') && (*(data + 1) == '\n')) return data;
	data++;
	size--;
     }
   return NULL;
}

