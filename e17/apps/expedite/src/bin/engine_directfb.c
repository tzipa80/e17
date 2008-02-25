#include "main.h"

#include <Evas_Engine_DirectFB.h>

static IDirectFB *_dfb;
static IDirectFBEventBuffer *_window_event;
static IDirectFBEventBuffer *_input_event;
static IDirectFBDisplayLayer *_layer;
static DFBResult _err;
static IDirectFBWindow *_dfb_window;
static IDirectFBSurface *_dfb_surface;

#define DFBCHECK(x...) \
     {                                                                 \
          _err = x;                                                    \
          if (_err != DFB_OK) {                                        \
               fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );  \
               DirectFBErrorFatal( #x, _err );                         \
          }                                                            \
     }

int
engine_directfb_args(int argc, char **argv)
{
   Evas_Engine_Info_DirectFB *einfo;
   DFBWindowDescription desc;
   int i;
   int ok = 0;

   for (i = 1; i < argc; i++)
     {
	if ((!strcmp(argv[i], "-e")) && (i < (argc - 1)))
	  {
	     i++;
	     if (!strcmp(argv[i], "directfb")) ok = 1;
	  }
     }
   if (!ok) return 0;

   evas_output_method_set(evas, evas_render_method_lookup("directfb"));
   einfo = (Evas_Engine_Info_DirectFB *)evas_engine_info_get(evas);
   if (!einfo)
     {
	printf("Evas does not support the DirectFB Engine\n");
	return 0;
     }

   DFBCHECK(DirectFBInit(NULL, NULL));
   DFBCHECK(DirectFBCreate(&_dfb));
   DFBCHECK(_dfb->GetDisplayLayer(_dfb, DLID_PRIMARY, &_layer));
   DFBCHECK(_dfb->CreateEventBuffer(_dfb, &_window_event));
   DFBCHECK(_dfb->CreateInputEventBuffer(_dfb, DICAPS_ALL, DFB_FALSE, &_input_event));

   memset(&desc, 0, sizeof(DFBWindowDescription));
   desc.flags = (DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS);
   desc.posx = 0;
   desc.posy = 0;
   desc.width = win_w;
   desc.height = win_h;
   desc.caps = DWCAPS_ALPHACHANNEL;

   DFBCHECK(_layer->CreateWindow(_layer, &desc, &_dfb_window));
   DFBCHECK(_dfb_window->AttachEventBuffer(_dfb_window, _window_event));
   DFBCHECK(_dfb_window->SetOptions(_dfb_window, DWOP_NONE));
   DFBCHECK(_dfb_window->SetOpacity(_dfb_window, 0xFF));
   DFBCHECK(_dfb_window->GetSurface(_dfb_window, &_dfb_surface));

   einfo->info.dfb = _dfb;
   einfo->info.surface = _dfb_surface;
   einfo->info.flags = DSDRAW_BLEND;
   evas_engine_info_set(evas, (Evas_Engine_Info *) einfo);

   return 1;
}

static void
engine_directfb_quit(void)
{
   DFBCHECK(_input_event->Release(_input_event));
   DFBCHECK(_window_event->Release(_window_event));
   DFBCHECK(_dfb_surface->Release(_dfb_surface));
   DFBCHECK(_dfb_window->Release(_dfb_window));
   DFBCHECK(_layer->Release(_layer));
   DFBCHECK(_dfb->Release(_dfb));

   evas_engine_info_set(evas, NULL);

   engine_abort();
}

static const char *
key_string(DFBInputDeviceKeySymbol sym)
{
   switch (sym)
     {
      case DIKS_ESCAPE:
	{
	   static const char k[] = "Escape";
	   return k;
	}
      case DIKS_RETURN:
	{
	   static const char k[] = "Return";
	   return k;
	}
      case DIKS_CURSOR_LEFT:
	{
	   static const char k[] = "Left";
	   return k;
	}
      case DIKS_CURSOR_RIGHT:
	{
	   static const char k[] = "Right";
	   return k;
	}
      default:
	{
	   static char k[2];
	   k[0] = (char)sym;
	   k[1] = 0;
	   return k;
	}
     }
}

static void
engine_directfb_event_window(const DFBWindowEvent *ev)
{
   fprintf(stderr, "===> Window Event (type=%#10x, window=%d) <===\n",
	   ev->type, ev->window_id);

   if (ev->type & DWET_POSITION)
     {
	fprintf(stderr, "\tDWET_POSITION %d, %d\n", ev->x, ev->y);
     }

   if (ev->type & DWET_SIZE)
     {
	fprintf(stderr, "\tDWET_SIZE %dx%d\n", ev->w, ev->h);
	evas_output_size_set(evas, ev->w, ev->h);
     }

   if (ev->type & DWET_CLOSE)
     {
	fprintf(stderr, "\tDWET_CLOSE\n");
	engine_directfb_quit();
     }

   if (ev->type & DWET_DESTROYED)
     {
	fprintf(stderr, "\tDWET_DESTROYED\n");
	engine_directfb_quit();
     }

   if (ev->type & DWET_GOTFOCUS)
     {
	fprintf(stderr, "\tDWET_GOTFOCUS\n");
     }

   if (ev->type & DWET_LOSTFOCUS)
     {
	fprintf(stderr, "\tDWET_LOSTFOCUS\n");
     }

   if (ev->type & DWET_KEYDOWN)
     {
	const char *k;

	k = key_string(ev->key_symbol);
	fprintf(stderr, "\tDWET_KEYDOWN key_symbol=%s\n", k);

	if (k)
	  evas_event_feed_key_down(evas, k, k, NULL, NULL, 0, NULL);
     }

   if (ev->type & DWET_KEYUP)
     {
	const char *k;

	k = key_string(ev->key_symbol);
	fprintf(stderr, "\tDWET_KEYUP key_symbol=%s\n", k);

	if (k)
	  evas_event_feed_key_up(evas, k, k, NULL, NULL, 0, NULL);
     }

   if (ev->type & DWET_BUTTONDOWN)
     {
	fprintf(stderr, "\tDWET_BUTTONDOWN pos=(%d, %d) cur_pos=(%d, %d) "
		"button=%#x buttons=%#x\n",
		ev->x, ev->y, ev->cx, ev->cy, ev->button, ev->buttons);
	evas_event_feed_mouse_move(evas, ev->cx, ev->cy, 0, NULL);
	evas_event_feed_mouse_down(evas, ev->button, EVAS_BUTTON_NONE, 0, NULL);
     }

   if (ev->type & DWET_BUTTONUP)
     {
	fprintf(stderr, "\tDWET_BUTTONUP pos=(%d, %d) cur_pos=(%d, %d) "
		"button=%#x buttons=%#x\n",
		ev->x, ev->y, ev->cx, ev->cy, ev->button, ev->buttons);
	evas_event_feed_mouse_move(evas, ev->cx, ev->cy, 0, NULL);
	evas_event_feed_mouse_up(evas, ev->button, EVAS_BUTTON_NONE, 0, NULL);
     }

   if (ev->type & DWET_MOTION)
     {
	fprintf(stderr, "\tDWET_MOTION pos=(%d, %d) cur_pos=(%d, %d) "
		"buttons=%#x\n",
		ev->x, ev->y, ev->cx, ev->cy, ev->buttons);
	evas_event_feed_mouse_move(evas, ev->x, ev->y, 0, NULL);
     }

   if (ev->type & DWET_LEAVE)
     {
	fprintf(stderr, "\tDWET_LEAVE pos=(%d, %d) cur_pos=(%d, %d)\n",
		ev->x, ev->y, ev->cx, ev->cy);
	evas_event_feed_mouse_out(evas, 0, NULL);
     }

   if (ev->type & DWET_ENTER)
     {
	fprintf(stderr, "\tDWET_ENTER pos=(%d, %d) cur_pos=(%d, %d)\n",
		ev->x, ev->y, ev->cx, ev->cy);
	evas_event_feed_mouse_in(evas, 0, NULL);
     }

   if (ev->type & DWET_WHEEL)
     {
	fprintf(stderr, "\tDWET_WHEEL step=%d\n", ev->step);
     }


   fprintf(stderr, "\n");
}

void
engine_directfb_loop(void)
{
   DFBEvent ev;

   while ((_input_event->GetEvent(_input_event, &ev) == DFB_OK) ||
	  (_window_event->GetEvent(_window_event, &ev) == DFB_OK))
     {
	fprintf(stderr, "got dfb input event %d\n", ev.clazz);
	switch (ev.clazz)
	  {
	   case DFEC_NONE:
	      fprintf(stderr, "No event?\n");
	      break;
	   case DFEC_INPUT:
	      fprintf(stderr, "Input\n");
	      break;
	   case DFEC_WINDOW:
	      engine_directfb_event_window((const DFBWindowEvent *)&ev);
	      break;
	   case DFEC_USER:
	      fprintf(stderr, "User\n");
	      break;
	   case DFEC_UNIVERSAL:
	      fprintf(stderr, "Universal\n");
	      break;
	  }
     }
}
