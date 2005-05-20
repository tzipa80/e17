#include "e.h"

/* local subsystem functions */
static int _e_ipc_cb_client_add(void *data, int type, void *event);
static int _e_ipc_cb_client_del(void *data, int type, void *event);
static int _e_ipc_cb_client_data(void *data, int type, void *event);
static void _e_ipc_reply_double_send(Ecore_Ipc_Client *client, double val, int opcode);
static void _e_ipc_reply_int_send(Ecore_Ipc_Client *client, int val, int opcode);
static void _e_ipc_reply_2int_send(Ecore_Ipc_Client *client, int val1, int val2, int opcode);
static char *_e_ipc_str_list_get(Evas_List *strs, int *bytes);
static char *_e_ipc_simple_str_dec(char *data, int bytes);
static char **_e_ipc_multi_str_dec(char *data, int bytes, int str_count);

static int _e_ipc_double_dec(char *data, int bytes, double *dest);
static int _e_ipc_int_dec(char *data, int bytes, int *dest);

/* encode functions, Should these be global? */
ECORE_IPC_ENC_EVAS_LIST_PROTO(_e_ipc_module_list_enc);
ECORE_IPC_ENC_EVAS_LIST_PROTO(_e_ipc_font_available_list_enc);
ECORE_IPC_ENC_EVAS_LIST_PROTO(_e_ipc_font_fallback_list_enc);
ECORE_IPC_ENC_EVAS_LIST_PROTO(_e_ipc_font_default_list_enc);
ECORE_IPC_ENC_STRUCT_PROTO(_e_ipc_font_default_enc);
ECORE_IPC_ENC_EVAS_LIST_PROTO(_e_ipc_mouse_binding_list_enc);
ECORE_IPC_ENC_STRUCT_PROTO(_e_ipc_mouse_binding_enc);
ECORE_IPC_DEC_STRUCT_PROTO(_e_ipc_mouse_binding_dec);
ECORE_IPC_ENC_EVAS_LIST_PROTO(_e_ipc_key_binding_list_enc);
ECORE_IPC_ENC_STRUCT_PROTO(_e_ipc_key_binding_enc);
ECORE_IPC_DEC_STRUCT_PROTO(_e_ipc_key_binding_dec);
ECORE_IPC_ENC_EVAS_LIST_PROTO(_e_ipc_path_list_enc);
ECORE_IPC_ENC_STRUCT_PROTO(_e_ipc_focus_policy_enc);
ECORE_IPC_DEC_STRUCT_PROTO(_e_ipc_focus_policy_dec);

/* local subsystem globals */
static Ecore_Ipc_Server *_e_ipc_server  = NULL;

/* externally accessible functions */
int
e_ipc_init(void)
{
   char buf[1024];
   char *disp;
   
   disp = getenv("DISPLAY");
   if (!disp) disp = ":0";
   snprintf(buf, sizeof(buf), "enlightenment-(%s)", disp);
   _e_ipc_server = ecore_ipc_server_add(ECORE_IPC_LOCAL_USER, buf, 0, NULL);
   if (!_e_ipc_server) return 0;
   ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_ADD, _e_ipc_cb_client_add, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DEL, _e_ipc_cb_client_del, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_CLIENT_DATA, _e_ipc_cb_client_data, NULL);

   e_ipc_codec_init();
   return 1;
}

void
e_ipc_shutdown(void)
{
   e_ipc_codec_shutdown();
   if (_e_ipc_server)
     {
	ecore_ipc_server_del(_e_ipc_server);
	_e_ipc_server = NULL;
     }
}

/* local subsystem globals */
static int
_e_ipc_cb_client_add(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Event_Client_Add *e;
   
   e = event;
   if (ecore_ipc_client_server_get(e->client) != _e_ipc_server) return 1;
   printf("E-IPC: client %p connected to server!\n", e->client);
   return 1;
}

static int
_e_ipc_cb_client_del(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Event_Client_Del *e;
   
   e = event;
   if (ecore_ipc_client_server_get(e->client) != _e_ipc_server) return 1;
   printf("E-IPC: client %p disconnected from server!\n", e->client);
   /* delete client sruct */
   ecore_ipc_client_del(e->client);
   return 1;
}

static int
_e_ipc_cb_client_data(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Ipc_Event_Client_Data *e;
   
   e = event;
   if (ecore_ipc_client_server_get(e->client) != _e_ipc_server) return 1;
   switch (e->minor)
     {
      case E_IPC_OP_MODULE_LOAD:
	if (e->data)
	  {
	     char *name;
	     
	     name = _e_ipc_simple_str_dec(e->data, e->size);
	     
	     if (!e_module_find(name))
	       {
		  e_module_new(name);
	       }
	     free(name);
             e_config_save_queue();
	  }
	break;
      case E_IPC_OP_MODULE_UNLOAD:
	if (e->data)
	  {
	     char *name;
	     E_Module *m;
	     
	     name = _e_ipc_simple_str_dec(e->data, e->size);
	     
	     if ((m = e_module_find(name)))
	       {
		  if (e_module_enabled_get(m))
		    e_module_disable(m);
		  e_object_del(E_OBJECT(m));
	       }
	     free(name);
             e_config_save_queue();
	  }
	break;
      case E_IPC_OP_MODULE_ENABLE:
	  {
	     char *name;
	     E_Module *m;
	     
	     name = _e_ipc_simple_str_dec(e->data, e->size);
	     
	     if ((m = e_module_find(name)))
	       {
		  if (!e_module_enabled_get(m))
		    e_module_enable(m);
	       }
	     free(name);
             e_config_save_queue();
	  }
	break;
      case E_IPC_OP_MODULE_DISABLE:
	  {      
	     char *name;
	     E_Module *m;
	     
	     name = _e_ipc_simple_str_dec(e->data, e->size);
	     
	     if ((m = e_module_find(name)))
	       {
		  if (e_module_enabled_get(m))
		    e_module_disable(m);
	       }
	     free(name);
             e_config_save_queue();
	  }
	break;
      case E_IPC_OP_MODULE_LIST:
	  {
	     /* encode module list (str,8-bit) */
	     Evas_List *modules;
	     char * data;
	     int bytes;

             modules = e_module_list();
	     data = _e_ipc_module_list_enc(modules, &bytes);
	     
	     /* send reply data */
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_MODULE_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
	  }
	break;
      case E_IPC_OP_BG_SET:
	  {
	     char *file;
	     Evas_List *managers, *l;

	     file = _e_ipc_simple_str_dec(e->data, e->size);
	     
	     E_FREE(e_config->desktop_default_background);
	     e_config->desktop_default_background = file;
	     
	     managers = e_manager_list();
	     for (l = managers; l; l = l->next)
	       {
		  Evas_List *ll;
		  E_Manager *man;
		  
		  man = l->data;
		  for (ll = man->containers; ll; ll = ll->next)
		    {
		       E_Container *con;
		       E_Zone *zone;
		       
		       con = ll->data;
		       zone = e_zone_current_get(con);
		       e_zone_bg_reconfigure(zone);
		    }
	       }
	     e_config_save_queue();
          }
	break;
      case E_IPC_OP_BG_GET:
	  {
	     char *bg;
	     
	     bg = e_config->desktop_default_background;
	     if (!bg) bg = "";
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_BG_GET_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   bg, strlen(bg) + 1);
 	  }
	break;
      case E_IPC_OP_FONT_AVAILABLE_LIST:
	  {
	     /* encode font available list (str) */
	     Evas_List *fonts_available;
	     int bytes;
	     char *data;
		  
	     fonts_available = e_font_available_list();	       
	     data = _e_ipc_font_available_list_enc(fonts_available, &bytes);	       
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_FONT_AVAILABLE_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     
	     e_font_available_list_free(fonts_available);
	     free(data);
	  }
	break;
      case E_IPC_OP_FONT_APPLY:
	  {
	     e_font_apply();
             e_config_save_queue();
	  }
 	break;
      case E_IPC_OP_FONT_FALLBACK_CLEAR:
	  {
	     e_font_fallback_clear();
             e_config_save_queue();
	  }
	break;
      case E_IPC_OP_FONT_FALLBACK_APPEND:
	  {
	     char * font_name;
	     
	     font_name = _e_ipc_simple_str_dec(e->data, e->size);
	     e_font_fallback_append(font_name);
	     free(font_name);

	     e_config_save_queue(); 
	  }
	break;
      case E_IPC_OP_FONT_FALLBACK_PREPEND:
	  {
	     char * font_name;

	     font_name = _e_ipc_simple_str_dec(e->data, e->size);	     
	     e_font_fallback_prepend(font_name);
	     free(font_name);	   
		
	     e_config_save_queue();  
	  }
	break;
      case E_IPC_OP_FONT_FALLBACK_LIST:
	  {
	  	
	     /* encode font fallback list (str) */
	     Evas_List *fallbacks;
	     int bytes;
	     char *data;
		 
	     fallbacks = e_font_fallback_list();

	     data = _e_ipc_font_fallback_list_enc(fallbacks, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_FONT_FALLBACK_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);

	  }
	break;
      case E_IPC_OP_FONT_FALLBACK_REMOVE:
	  {
	     char * font_name;

	     font_name = _e_ipc_simple_str_dec(e->data, e->size);
	     e_font_fallback_remove(font_name);
	     free(font_name);	     

	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_FONT_DEFAULT_SET:
	  {
	     char ** argv;
	     int argc;
	     
	     argc = 3;
	     
	     argv = _e_ipc_multi_str_dec(e->data, e->size, argc);
	     e_font_default_set(argv[0], argv[1], atoi(argv[2]));
	     free(argv);	     
	     
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_FONT_DEFAULT_GET:
	  {
	  
	     /* encode font default struct (str,str,32-bits)(E_Font_Default) */	     
	     E_Font_Default *efd;
	     char *data, *text_class;
	     int bytes;
	     
	     text_class = _e_ipc_simple_str_dec(e->data, e->size);	     
	     efd = e_font_default_get(text_class);	     
	     free(text_class);
	     data = _e_ipc_font_default_enc(efd, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_FONT_DEFAULT_GET_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
	  }
	break;
      case E_IPC_OP_FONT_DEFAULT_REMOVE:
	  {	  
	     char * text_class;
	     
	     text_class = _e_ipc_simple_str_dec(e->data, e->size);
	     e_font_default_remove(text_class);
	     free(text_class);	   

	     e_config_save_queue(); 
	  }
	break;
      case E_IPC_OP_FONT_DEFAULT_LIST:
	  {
	     /* encode font default struct list (str,str,32-bits)(E_Font_Default) */
	     Evas_List *defaults;
	     int bytes;
	     char *data;
		  
	     defaults = e_font_default_list();
	     data = _e_ipc_font_default_list_enc(defaults, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_FONT_DEFAULT_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);

	  }
	break;
      case E_IPC_OP_RESTART:
	  {
	     restart = 1;
	     ecore_main_loop_quit();
 	  }
	break;
      case E_IPC_OP_SHUTDOWN:
	  {
	     ecore_main_loop_quit();
 	  }
	break;
      case E_IPC_OP_LANG_LIST:
	  {
	     Evas_List *langs;
	     int bytes;
	     char *data;
	     
	     langs = (Evas_List *)e_intl_language_list();
	     data = _e_ipc_str_list_get(langs, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_LANG_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
	  }
	break;
      case E_IPC_OP_LANG_SET:
	  {
	     char *lang;
	     
	     lang = _e_ipc_simple_str_dec(e->data, e->size);
	     IF_FREE(e_config->language);
	     e_config->language = lang;
	     e_intl_language_set(e_config->language);
             e_config_save_queue();
	  }
	break;
      case E_IPC_OP_LANG_GET:
	  {
	     char *lang;
	     
	     lang = e_config->language;
	     if (!lang) lang = "";
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_LANG_GET_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   lang, strlen(lang) + 1);
	     free(data);
	  }
	break;
      case E_IPC_OP_BINDING_MOUSE_LIST:
	  {
	     Evas_List *bindings;
	     int bytes;
	     char *data;
	     
	     bindings = e_config->mouse_bindings;
	     data = _e_ipc_mouse_binding_list_enc(bindings, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_BINDING_MOUSE_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
	  }
	break;
      case E_IPC_OP_BINDING_MOUSE_ADD:
	  {
	     E_Config_Binding_Mouse bind, *eb;
	     
	     _e_ipc_mouse_binding_dec(e->data, e->size, &bind);
	       {
		  eb = e_config_binding_mouse_match(&bind);
		  if (!eb)
		    {
		       eb = E_NEW(E_Config_Binding_Key, 1);
		       e_config->mouse_bindings = evas_list_append(e_config->mouse_bindings, eb);
		       eb->context = bind.context;
		       eb->button = bind.button;
		       eb->modifiers = bind.modifiers;
		       eb->any_mod = bind.any_mod;
		       eb->action = strdup(bind.action);
		       eb->params = strdup(bind.params);
		       e_border_button_bindings_ungrab_all();
		       e_bindings_mouse_add(bind.context, bind.button, bind.modifiers,
					    bind.any_mod, bind.action, bind.params);
		       e_border_button_bindings_grab_all();
		       e_config_save_queue();
		    }
	       }
 	  }
	break;
      case E_IPC_OP_BINDING_MOUSE_DEL:
	  {
	     E_Config_Binding_Mouse bind, *eb;
	     
	     _e_ipc_mouse_binding_dec(e->data, e->size, &bind);
	       {
		  eb = e_config_binding_mouse_match(&bind);
		  if (eb)
		    {
		       e_config->mouse_bindings = evas_list_remove(e_config->mouse_bindings, eb);
		       IF_FREE(eb->action);
		       IF_FREE(eb->params);
		       IF_FREE(eb);
		       e_border_button_bindings_ungrab_all();
		       e_bindings_mouse_del(bind.context, bind.button, bind.modifiers,
					    bind.any_mod, bind.action, bind.params);
		       e_border_button_bindings_grab_all();
		       e_config_save_queue();
		    }
	       }
 	  }
	break;
      case E_IPC_OP_BINDING_KEY_LIST:
	  {
	     Evas_List *bindings;
	     int bytes;
	     char *data;
	     
	     bindings = e_config->key_bindings;
	     data = _e_ipc_key_binding_list_enc(bindings, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_BINDING_KEY_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);
	     free(data);
	  }
	break;
      case E_IPC_OP_BINDING_KEY_ADD:
	  {
	     E_Config_Binding_Key bind, *eb;
	     
	     _e_ipc_key_binding_dec(e->data, e->size, &bind);
	       {
		  eb = e_config_binding_key_match(&bind);
		  if (!eb)
		    {
		       eb = E_NEW(E_Config_Binding_Key, 1);
		       e_config->key_bindings = evas_list_append(e_config->key_bindings, eb);
		       eb->context = bind.context;
		       eb->modifiers = bind.modifiers;
		       eb->any_mod = bind.any_mod;
		       eb->key = strdup(bind.key);
		       eb->action = strdup(bind.action);
		       eb->params = strdup(bind.params);
		       e_managers_keys_ungrab();
		       e_bindings_key_add(bind.context, bind.key, bind.modifiers,
					  bind.any_mod, bind.action, bind.params);
		       e_managers_keys_grab();
		       e_config_save_queue();
		    }
	       }
 	  }
	break;
      case E_IPC_OP_BINDING_KEY_DEL:
	  {
	     E_Config_Binding_Key bind, *eb;
	     
	     _e_ipc_key_binding_dec(e->data, e->size, &bind);
	       {
		  eb = e_config_binding_key_match(&bind);
		  if (eb)
		    {
		       e_config->key_bindings = evas_list_remove(e_config->key_bindings, eb);
		       IF_FREE(eb->key);
		       IF_FREE(eb->action);
		       IF_FREE(eb->params);
		       IF_FREE(eb);
		       e_managers_keys_ungrab();
		       e_bindings_key_del(bind.context, bind.key, bind.modifiers,
					  bind.any_mod, bind.action, bind.params);
		       e_managers_keys_grab();
		       e_config_save_queue();
		    }
	       }
 	  }
	break;
      case E_IPC_OP_MENUS_SCROLL_SPEED_SET:
	if (e_ipc_codec_double_dec(e->data, e->size,
				   &(e_config->menus_scroll_speed)))
	  {
	     E_CONFIG_LIMIT(e_config->menus_scroll_speed, 1.0, 20000.0);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_MENUS_SCROLL_SPEED_GET:
	_e_ipc_reply_double_send(e->client,
				 e_config->menus_scroll_speed, 
				 E_IPC_OP_MENUS_SCROLL_SPEED_GET_REPLY);
	break;
      case E_IPC_OP_MENUS_FAST_MOVE_THRESHHOLD_SET:
	if (e_ipc_codec_double_dec(e->data, e->size,
				   &(e_config->menus_fast_mouse_move_threshhold)))
	  {
	     E_CONFIG_LIMIT(e_config->menus_fast_mouse_move_threshhold, 1.0, 2000.0);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_MENUS_FAST_MOVE_THRESHHOLD_GET:
	_e_ipc_reply_double_send(e->client,
				 e_config->menus_fast_mouse_move_threshhold, 
				 E_IPC_OP_MENUS_FAST_MOVE_THRESHHOLD_GET_REPLY);
	break;
      case E_IPC_OP_MENUS_CLICK_DRAG_TIMEOUT_SET:
	if (e_ipc_codec_double_dec(e->data, e->size,
				   &(e_config->menus_click_drag_timeout)))
	  {
	     E_CONFIG_LIMIT(e_config->menus_click_drag_timeout, 0.0, 10.0);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_MENUS_CLICK_DRAG_TIMEOUT_GET:
	_e_ipc_reply_double_send(e->client,
				 e_config->menus_click_drag_timeout,
				 E_IPC_OP_MENUS_CLICK_DRAG_TIMEOUT_GET_REPLY);
	break;
      case E_IPC_OP_BORDER_SHADE_ANIMATE_SET:
	if (e_ipc_codec_int_dec(e->data, e->size,
				&(e_config->border_shade_animate)))
	  {
	     E_CONFIG_LIMIT(e_config->border_shade_animate, 0, 1);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_BORDER_SHADE_ANIMATE_GET:
	_e_ipc_reply_int_send(e->client,
			      e_config->border_shade_animate,
			      E_IPC_OP_BORDER_SHADE_ANIMATE_GET_REPLY);
	break;
      case E_IPC_OP_BORDER_SHADE_TRANSITION_SET:
	if (e_ipc_codec_int_dec(e->data, e->size,
				&(e_config->border_shade_transition)))
	  {
	     E_CONFIG_LIMIT(e_config->border_shade_transition, 0, 3);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_BORDER_SHADE_TRANSITION_GET:
	_e_ipc_reply_int_send(e->client,
			      e_config->border_shade_transition,
			      E_IPC_OP_BORDER_SHADE_TRANSITION_GET_REPLY);
	break;
      case E_IPC_OP_BORDER_SHADE_SPEED_SET:
	if (e_ipc_codec_double_dec(e->data, e->size,
				   &(e_config->border_shade_speed)))
	  {
	     E_CONFIG_LIMIT(e_config->border_shade_speed, 1.0, 20000.0);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_BORDER_SHADE_SPEED_GET:
	_e_ipc_reply_double_send(e->client,
				 e_config->border_shade_speed,
				 E_IPC_OP_BORDER_SHADE_SPEED_GET_REPLY);
	break;
      case E_IPC_OP_FRAMERATE_SET:
	if (e_ipc_codec_double_dec(e->data, e->size,
				   &(e_config->framerate)))
	  {
	     E_CONFIG_LIMIT(e_config->image_cache, 1.0, 200.0);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_FRAMERATE_GET:
	_e_ipc_reply_double_send(e->client,
				 e_config->framerate,
				 E_IPC_OP_FRAMERATE_GET_REPLY);
	break;
      case E_IPC_OP_IMAGE_CACHE_SET:
	if (e_ipc_codec_int_dec(e->data, e->size,
				&(e_config->image_cache)))
	  {
	     E_CONFIG_LIMIT(e_config->image_cache, 0, 256 * 1024);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_IMAGE_CACHE_GET:
	_e_ipc_reply_int_send(e->client,
			      e_config->image_cache,
			      E_IPC_OP_IMAGE_CACHE_GET_REPLY);
	break;
      case E_IPC_OP_FONT_CACHE_SET:
	if (e_ipc_codec_int_dec(e->data, e->size,
				&(e_config->font_cache)))
	  {
	     E_CONFIG_LIMIT(e_config->font_cache, 0, 32 * 1024);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_FONT_CACHE_GET:
	_e_ipc_reply_int_send(e->client,
			      e_config->font_cache,
			      E_IPC_OP_FONT_CACHE_GET_REPLY);
	break;
      case E_IPC_OP_USE_EDGE_FLIP_SET:
	if (e_ipc_codec_int_dec(e->data, e->size,
				&(e_config->use_edge_flip)))
	  {
	     E_CONFIG_LIMIT(e_config->use_edge_flip, 0, 1);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_USE_EDGE_FLIP_GET:
	_e_ipc_reply_int_send(e->client,
			      e_config->use_edge_flip,
			      E_IPC_OP_USE_EDGE_FLIP_GET_REPLY);
	break;
      case E_IPC_OP_EDGE_FLIP_TIMEOUT_SET:
	if (e_ipc_codec_double_dec(e->data, e->size,
				   &(e_config->edge_flip_timeout)))
	  {
	     E_CONFIG_LIMIT(e_config->edge_flip_timeout, 0.0, 2.0);
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_EDGE_FLIP_TIMEOUT_GET:
	_e_ipc_reply_double_send(e->client,
				 e_config->edge_flip_timeout,
				 E_IPC_OP_EDGE_FLIP_TIMEOUT_GET_REPLY);
	break;
      case E_IPC_OP_DESKS_SET:
	if (e_ipc_codec_2int_dec(e->data, e->size,
				 &(e_config->zone_desks_x_count),
				 &(e_config->zone_desks_y_count)))
	  {
	     Evas_List *l;
	     
	     E_CONFIG_LIMIT(e_config->zone_desks_x_count, 1, 64);
	     E_CONFIG_LIMIT(e_config->zone_desks_y_count, 1, 64);
	     for (l = e_manager_list(); l; l = l->next)
	       {
		  E_Manager *man;
		  Evas_List *l2;
		  
		  man = l->data;
		  for (l2 = man->containers; l2; l2 = l2->next)
		    {
		       E_Container *con;
		       Evas_List *l3;
		       
		       con = l2->data;
		       for (l3 = con->zones; l3; l3 = l3->next)
			 {
			    E_Zone *zone;
			    
			    zone = l3->data;
			    e_zone_desk_count_set(zone,
						  e_config->zone_desks_x_count,
						  e_config->zone_desks_y_count);
			 }
		    }
	       }
	     e_config_save_queue();
	  }
	break;
      case E_IPC_OP_DESKS_GET:
	_e_ipc_reply_2int_send(e->client,
			       e_config->zone_desks_x_count,
			       e_config->zone_desks_y_count,
			       E_IPC_OP_DESKS_GET_REPLY);
	break;
      case E_IPC_OP_FOCUS_POLICY_SET:
	  {
	     E_Config_Focus_Policy policy;
	     
	     _e_ipc_focus_policy_dec(e->data, e->size, &policy);
	     e_config->focus_policy = policy.focus_policy;
	     e_config->raise_timer = policy.raise_timer;
	     
	  }
	break;
      case E_IPC_OP_FOCUS_POLICY_GET:
	  {
	     int bytes;
	     E_Config_Focus_Policy policy;
	     char *data;
	     
	     policy.focus_policy = e_config->focus_policy;
	     policy.raise_timer = e_config->raise_timer;
	     
	     data = _e_ipc_focus_policy_enc(&policy, &bytes);
	     
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_FOCUS_POLICY_GET_REPLY,
				   0, 0, 0,
				   data, bytes);
	     
	     free(data);
	     
	  }
	break;
      case E_IPC_OP_MODULE_DIRS_LIST:
	  {
	     Evas_List *dir_list;
	     char *data;
	     int bytes = 0;

	     dir_list = e_path_dir_list_get(path_modules);
	     data = _e_ipc_path_list_enc(dir_list, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_MODULE_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);

	     e_path_dir_list_free(dir_list);
	     free(data);
	     break;
	  }
      case E_IPC_OP_MODULE_DIRS_APPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_append(path_modules, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_MODULE_DIRS_PREPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_prepend(path_modules, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_MODULE_DIRS_REMOVE:
          {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_remove(path_modules, dir);

	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }

      /* Theme PATH IPC */
      case E_IPC_OP_THEME_DIRS_LIST:
	  {
	     Evas_List *dir_list;
             char *data;
	     int bytes = 0;

	     dir_list = e_path_dir_list_get(path_themes);
	     data = _e_ipc_path_list_enc(dir_list, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_THEME_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);

	     e_path_dir_list_free(dir_list);
	     free(data);
	     break;
	  }
      case E_IPC_OP_THEME_DIRS_APPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_append(path_themes, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_THEME_DIRS_PREPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_prepend(path_themes, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_THEME_DIRS_REMOVE:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_remove(path_themes, dir);

	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }

      /* Font Path IPC */
      case E_IPC_OP_FONT_DIRS_LIST:
          {
	     Evas_List *dir_list;
             char *data;
	     int bytes = 0;

	     dir_list = e_path_dir_list_get(path_fonts);
	     data = _e_ipc_path_list_enc(dir_list, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_FONT_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);

	     e_path_dir_list_free(dir_list);
	     free(data);
	     break;
	  }
      case E_IPC_OP_FONT_DIRS_APPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_append(path_fonts, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_FONT_DIRS_PREPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_prepend(path_fonts, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_FONT_DIRS_REMOVE:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_remove(path_fonts, dir);

	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
   
      /* data Path IPC */
      case E_IPC_OP_DATA_DIRS_LIST:
	  {
             Evas_List *dir_list;
	     char *data;
	     int bytes = 0;

	     dir_list = e_path_dir_list_get(path_data);
	     data = _e_ipc_path_list_enc(dir_list, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_DATA_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);

	     e_path_dir_list_free(dir_list);
	     free(data);
	     break;
	  }
      case E_IPC_OP_DATA_DIRS_APPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_append(path_data, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_DATA_DIRS_PREPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_prepend(path_data, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_DATA_DIRS_REMOVE:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_remove(path_data, dir);

	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }

      /* Images Path IPC */
      case E_IPC_OP_IMAGE_DIRS_LIST:
   	  {
	     Evas_List *dir_list;
             char *data;
	     int bytes = 0;

	     dir_list = e_path_dir_list_get(path_images);
	     data = _e_ipc_path_list_enc(dir_list, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_IMAGE_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);

	     e_path_dir_list_free(dir_list);
	     free(data);
	     break;
	  }
      case E_IPC_OP_IMAGE_DIRS_APPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_append(path_images, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
     case E_IPC_OP_IMAGE_DIRS_PREPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_prepend(path_images, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_IMAGE_DIRS_REMOVE:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_remove(path_images, dir);

	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }

      /* Init Path IPC */
      case E_IPC_OP_INIT_DIRS_LIST:
   	  {
	     Evas_List *dir_list;
             char *data;
	     int bytes = 0;

	     dir_list = e_path_dir_list_get(path_init);
	     data = _e_ipc_path_list_enc(dir_list, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_INIT_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);

	     e_path_dir_list_free(dir_list);
	     free(data);
	     break;
	  }
      case E_IPC_OP_INIT_DIRS_APPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_append(path_init, dir);
	     
	     free(dir);	   
	     e_config_save_queue();
	     break;
	  }
      case E_IPC_OP_INIT_DIRS_PREPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_prepend(path_init, dir);
	     
	     free(dir);	   
	     e_config_save_queue();
	     break;
	  }
      case E_IPC_OP_INIT_DIRS_REMOVE:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_remove(path_init, dir);

	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }

      /* Icon Path IPC */
      case E_IPC_OP_ICON_DIRS_LIST:
	  {
	     Evas_List *dir_list;
             char *data;
	     int bytes = 0;

	     dir_list = e_path_dir_list_get(path_icons);
	     data = _e_ipc_path_list_enc(dir_list, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_ICON_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);

	     e_path_dir_list_free(dir_list);
	     free(data);
	     break;
	  }
      case E_IPC_OP_ICON_DIRS_APPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_append(path_icons, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_ICON_DIRS_PREPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_prepend(path_icons, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_ICON_DIRS_REMOVE:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_remove(path_icons, dir);

	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }

      /* Icon Path IPC */
      case E_IPC_OP_BG_DIRS_LIST:
	  {
	     Evas_List *dir_list;
             char *data;
	     int bytes = 0;

	     dir_list = e_path_dir_list_get(path_backgrounds);
	     data = _e_ipc_path_list_enc(dir_list, &bytes);
	     ecore_ipc_client_send(e->client,
				   E_IPC_DOMAIN_REPLY,
				   E_IPC_OP_BG_DIRS_LIST_REPLY,
				   0/*ref*/, 0/*ref_to*/, 0/*response*/,
				   data, bytes);

	     e_path_dir_list_free(dir_list);
	     free(data);
	     break;
	  }
      case E_IPC_OP_BG_DIRS_APPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_append(path_backgrounds, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_BG_DIRS_PREPEND:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_prepend(path_backgrounds, dir);
	     
	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }
      case E_IPC_OP_BG_DIRS_REMOVE:
	  {	  
	     char * dir;
	     
	     dir = _e_ipc_simple_str_dec(e->data, e->size);
	     e_path_user_path_remove(path_backgrounds, dir);

	     free(dir);	   
	     e_config_save_queue(); 
	     break;
	  }

      default:
	break;
     }
   printf("E-IPC: client sent: [%i] [%i] (%i) \"%p\"\n", e->major, e->minor, e->size, e->data);
   /* ecore_ipc_client_send(e->client, 1, 2, 7, 77, 0, "ABC", 4); */
   /* we can disconnect a client like this: */
   /* ecore_ipc_client_del(e->client); */
   /* or we can end a server by: */
   /* ecore_ipc_server_del(ecore_ipc_client_server_get(e->client)); */
   return 1;
}  

static void
_e_ipc_reply_double_send(Ecore_Ipc_Client *client, double val, int opcode)
{
   void *data;
   int   bytes;
   
   if ((data = e_ipc_codec_double_enc(val, &bytes)))
     {
	ecore_ipc_client_send(client,
			      E_IPC_DOMAIN_REPLY,
			      opcode,
			      0/*ref*/, 0/*ref_to*/, 0/*response*/,
			      data, bytes);
	free(data);
     }
}

static void
_e_ipc_reply_int_send(Ecore_Ipc_Client *client, int val, int opcode)
{
   void *data;
   int   bytes;
   
   if ((data = e_ipc_codec_int_enc(val, &bytes)))
     {
	ecore_ipc_client_send(client,
			      E_IPC_DOMAIN_REPLY,
			      opcode,
			      0/*ref*/, 0/*ref_to*/, 0/*response*/,
			      data, bytes);
	free(data);
     }
}

static void
_e_ipc_reply_2int_send(Ecore_Ipc_Client *client, int val1, int val2, int opcode)
{
   void *data;
   int   bytes;
   
   if ((data = e_ipc_codec_2int_enc(val1, val2, &bytes)))
     {
	ecore_ipc_client_send(client,
			      E_IPC_DOMAIN_REPLY,
			      opcode,
			      0/*ref*/, 0/*ref_to*/, 0/*response*/,
			      data, bytes);
	free(data);
     }
}

/**
 * Encode a list of strings into a flattened data block that looks like
 * <str>0<str>0... (ie string chars - nul byte in between until the end)
 */
static char *
_e_ipc_str_list_get(Evas_List *strs, int *bytes)
{
   char *data = NULL;
   int pos = 0;
   Evas_List *l;
   
   *bytes = 0;
   for (l = strs; l; l = l->next)
     {
	char *p;

	p = l->data;

	*bytes += strlen(p) + 1;
	data = realloc(data, *bytes);

	memcpy(data + pos, p, strlen(p));
	pos = *bytes;
	data[pos - 1] = 0;
     }
   return data;
}

/**
 * Decode a simple string that was passed by an IPC client
 *
 * The returned string must be freed
 */
static char *
_e_ipc_simple_str_dec(char *data, int bytes)
{
   char *str;
   
   str = malloc(bytes + 1);
   str[bytes] = 0;
   memcpy(str, data, bytes);
   return str;
}

/**
 * Decode a list of strings and return an array, you need to pass
 * the string count that you are expecting back.
 *
 * Strings are encoded <str>0<str>0...
 *
 * free up the array that you get back
 */
static char **
_e_ipc_multi_str_dec(char *data, int bytes, int str_count)
{
   char **str_array;
   int i;
   
   /* Make sure our data is packed for us <str>0<str>0 */
   if (data[bytes - 1] != 0)
     return NULL;
   
   str_array = malloc(sizeof(char *) * str_count);
   
   for (i = 0; i < str_count; i++) 
     {
        str_array[i] = data;
	data += strlen(str_array[i]) + 1;
     }
   
   return str_array;
}

/* FIXME: just use eet for this - saves a lot of hassle */
   
/* list/struct encoding functions */
ECORE_IPC_ENC_EVAS_LIST_PROTO(_e_ipc_module_list_enc)
{
   ECORE_IPC_ENC_EVAS_LIST_HEAD_START(E_Module);
   ECORE_IPC_CNTS(name);
   ECORE_IPC_CNT8();
   ECORE_IPC_ENC_EVAS_LIST_HEAD_FINISH();
   int l1;
   ECORE_IPC_SLEN(l1, name);
   ECORE_IPC_PUTS(name, l1);
   ECORE_IPC_PUT8(enabled);
   ECORE_IPC_ENC_EVAS_LIST_FOOT();
}

ECORE_IPC_ENC_EVAS_LIST_PROTO(_e_ipc_font_available_list_enc)
{
   ECORE_IPC_ENC_EVAS_LIST_HEAD_START(E_Font_Available);
   ECORE_IPC_CNTS(name);
   ECORE_IPC_ENC_EVAS_LIST_HEAD_FINISH();
   int l1;
   ECORE_IPC_SLEN(l1, name);
   ECORE_IPC_PUTS(name, l1);
   ECORE_IPC_ENC_EVAS_LIST_FOOT();
}

ECORE_IPC_ENC_EVAS_LIST_PROTO(_e_ipc_font_fallback_list_enc)
{
   ECORE_IPC_ENC_EVAS_LIST_HEAD_START(E_Font_Fallback);
   ECORE_IPC_CNTS(name);
   ECORE_IPC_ENC_EVAS_LIST_HEAD_FINISH();
   int l1;
   ECORE_IPC_SLEN(l1, name);
   ECORE_IPC_PUTS(name, l1);
   ECORE_IPC_ENC_EVAS_LIST_FOOT();
}

ECORE_IPC_ENC_EVAS_LIST_PROTO(_e_ipc_font_default_list_enc)
{
   ECORE_IPC_ENC_EVAS_LIST_HEAD_START(E_Font_Default);
   ECORE_IPC_CNTS(text_class);
   ECORE_IPC_CNTS(font);
   ECORE_IPC_CNT32();
   ECORE_IPC_ENC_EVAS_LIST_HEAD_FINISH();
   int l1, l2;
   ECORE_IPC_SLEN(l1, text_class);
   ECORE_IPC_SLEN(l2, font);
   ECORE_IPC_PUTS(text_class, l1);
   ECORE_IPC_PUTS(font, l2);
   ECORE_IPC_PUT32(size);
   ECORE_IPC_ENC_EVAS_LIST_FOOT();
}

ECORE_IPC_ENC_STRUCT_PROTO(_e_ipc_font_default_enc)
{
   int l1, l2;
   ECORE_IPC_ENC_STRUCT_HEAD(E_Font_Default, 
			     ECORE_IPC_SLEN(l1, text_class) +
			     ECORE_IPC_SLEN(l2, font) +
			     4);
   ECORE_IPC_PUTS(text_class, l1);
   ECORE_IPC_PUTS(font, l2);
   ECORE_IPC_PUT32(size);
   ECORE_IPC_ENC_STRUCT_FOOT();
}

ECORE_IPC_ENC_EVAS_LIST_PROTO(_e_ipc_mouse_binding_list_enc)
{
   ECORE_IPC_ENC_EVAS_LIST_HEAD_START(E_Config_Binding_Mouse);
   ECORE_IPC_CNT32();
   ECORE_IPC_CNT32();
   ECORE_IPC_CNTS(action);
   ECORE_IPC_CNTS(params);
   ECORE_IPC_CNT8();
   ECORE_IPC_CNT8();
   ECORE_IPC_ENC_EVAS_LIST_HEAD_FINISH();
   int l1, l2;
   ECORE_IPC_PUT32(context);
   ECORE_IPC_PUT32(modifiers);
   ECORE_IPC_SLEN(l1, action);
   ECORE_IPC_SLEN(l2, params);
   ECORE_IPC_PUTS(action, l1);
   ECORE_IPC_PUTS(params, l2);
   ECORE_IPC_PUT8(button);
   ECORE_IPC_PUT8(any_mod);
   ECORE_IPC_ENC_EVAS_LIST_FOOT();
}

ECORE_IPC_ENC_STRUCT_PROTO(_e_ipc_mouse_binding_enc)
{
   int l1, l2;
   ECORE_IPC_ENC_STRUCT_HEAD(E_Config_Binding_Mouse,
			     4 + 4 +
			     ECORE_IPC_SLEN(l1, action) +
			     ECORE_IPC_SLEN(l2, params) +
			     1 + 1);
   ECORE_IPC_PUT32(context);
   ECORE_IPC_PUT32(modifiers);
   ECORE_IPC_PUTS(action, l1);
   ECORE_IPC_PUTS(params, l2);
   ECORE_IPC_PUT8(button);
   ECORE_IPC_PUT8(any_mod);
   ECORE_IPC_ENC_STRUCT_FOOT();
}

ECORE_IPC_DEC_STRUCT_PROTO(_e_ipc_mouse_binding_dec)
{
   ECORE_IPC_DEC_STRUCT_HEAD_MIN(E_Config_Binding_Mouse,
				 4 + 4 +
				 1 +
				 1 +
				 1 + 1);
   ECORE_IPC_CHEKS();
   ECORE_IPC_GET32(context);
   ECORE_IPC_GET32(modifiers);
   ECORE_IPC_GETS(action);
   ECORE_IPC_GETS(params);
   ECORE_IPC_GET8(button);
   ECORE_IPC_GET8(any_mod);
   ECORE_IPC_DEC_STRUCT_FOOT();
}

ECORE_IPC_ENC_EVAS_LIST_PROTO(_e_ipc_key_binding_list_enc)
{
   ECORE_IPC_ENC_EVAS_LIST_HEAD_START(E_Config_Binding_Key);
   ECORE_IPC_CNT32();
   ECORE_IPC_CNT32();
   ECORE_IPC_CNTS(key);
   ECORE_IPC_CNTS(action);
   ECORE_IPC_CNTS(params);
   ECORE_IPC_CNT8();
   ECORE_IPC_ENC_EVAS_LIST_HEAD_FINISH();
   int l1, l2, l3;
   ECORE_IPC_PUT32(context);
   ECORE_IPC_PUT32(modifiers);
   ECORE_IPC_SLEN(l1, key);
   ECORE_IPC_SLEN(l2, action);
   ECORE_IPC_SLEN(l3, params);
   ECORE_IPC_PUTS(key, l1);
   ECORE_IPC_PUTS(action, l2);
   ECORE_IPC_PUTS(params, l3);
   ECORE_IPC_PUT8(any_mod);
   ECORE_IPC_ENC_EVAS_LIST_FOOT();
}

ECORE_IPC_ENC_STRUCT_PROTO(_e_ipc_key_binding_enc)
{
   int l1, l2, l3;
   ECORE_IPC_ENC_STRUCT_HEAD(E_Config_Binding_Key,
			     4 + 4 +
			     ECORE_IPC_SLEN(l1, key) +
			     ECORE_IPC_SLEN(l2, action) +
			     ECORE_IPC_SLEN(l3, params) +
			     1);
   ECORE_IPC_PUT32(context);
   ECORE_IPC_PUT32(modifiers);
   ECORE_IPC_PUTS(key, l1);
   ECORE_IPC_PUTS(action, l2);
   ECORE_IPC_PUTS(params, l3);
   ECORE_IPC_PUT8(any_mod);
   ECORE_IPC_ENC_STRUCT_FOOT();
}

ECORE_IPC_DEC_STRUCT_PROTO(_e_ipc_key_binding_dec)
{
   ECORE_IPC_DEC_STRUCT_HEAD_MIN(E_Config_Binding_Key,
				 4 + 4 +
				 1 +
				 1 +
				 1 +
				 1);
   ECORE_IPC_CHEKS();
   ECORE_IPC_GET32(context);
   ECORE_IPC_GET32(modifiers);
   ECORE_IPC_GETS(key);
   ECORE_IPC_GETS(action);
   ECORE_IPC_GETS(params);
   ECORE_IPC_GET8(any_mod);
   ECORE_IPC_DEC_STRUCT_FOOT();
}

ECORE_IPC_ENC_EVAS_LIST_PROTO(_e_ipc_path_list_enc)
{
   ECORE_IPC_ENC_EVAS_LIST_HEAD_START(E_Path_Dir);
   ECORE_IPC_CNTS(dir);
   ECORE_IPC_ENC_EVAS_LIST_HEAD_FINISH();
   int l1;
   ECORE_IPC_SLEN(l1, dir);
   ECORE_IPC_PUTS(dir, l1);
   ECORE_IPC_ENC_EVAS_LIST_FOOT();
}

ECORE_IPC_ENC_STRUCT_PROTO(_e_ipc_focus_policy_enc)
{
   ECORE_IPC_ENC_STRUCT_HEAD(E_Config_Focus_Policy,
			     1 + 4);
   ECORE_IPC_PUT8(focus_policy);
   ECORE_IPC_PUT32(raise_timer);
   ECORE_IPC_ENC_STRUCT_FOOT();
}

ECORE_IPC_DEC_STRUCT_PROTO(_e_ipc_focus_policy_dec)
{
   ECORE_IPC_DEC_STRUCT_HEAD_MIN(E_Config_Focus_Policy,
				 1 + 4);
   ECORE_IPC_CHEKS();
   ECORE_IPC_GET8(focus_policy);
   ECORE_IPC_GET32(raise_timer);
   ECORE_IPC_DEC_STRUCT_FOOT();
}
