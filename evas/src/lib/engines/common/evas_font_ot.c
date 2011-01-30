#include "evas_font_ot.h"

#ifdef OT_SUPPORT
# include <hb.h>
# include <hb-ft.h>
#endif

#include "evas_common.h"

#include <Eina.h>
#include "evas_font_private.h"

EAPI Eina_Bool
evas_common_font_ot_is_enabled(void)
{
#ifdef OT_SUPPORT
   static int ret = -1;
   const char *env;
   if (ret != -1)
     {
        return ret;
     }

   env = getenv("EVAS_USE_OT");
   if (env && atoi(env))
     {
        ret = EINA_TRUE;
        return ret;
     }
#endif
   return EINA_FALSE;
}

#ifdef OT_SUPPORT
/* FIXME: doc. returns #items */
EAPI int
evas_common_font_ot_cluster_size_get(Evas_Text_Props *props, size_t char_index, int orig_len)
{
   int i;
   int items;
   int left_bound, right_bound;
   size_t base_cluster = EVAS_FONT_OT_POS_GET(props->ot_data->items[char_index]);
   for (i = (int) char_index ;
         (i >= 0) &&
         (EVAS_FONT_OT_POS_GET(props->ot_data->items[i]) == base_cluster) ;
         i--)
     ;
   left_bound = i;
   for (i = (int) char_index + 1;
         (i < (int) props->ot_data->len) &&
         (EVAS_FONT_OT_POS_GET(props->ot_data->items[i]) == base_cluster) ;
         i++)
     ;
   right_bound = i;
   if (right_bound == left_bound)
     {
        items = 1;
     }
   else if (props->bidi.dir == EVAS_BIDI_DIRECTION_RTL)
     {
        if (left_bound < 0)
          {
             items = orig_len -
                props->ot_data->items[left_bound + 1].source_pos;
          }
        else
          {
             items = props->ot_data->items[left_bound].source_pos -
                props->ot_data->items[left_bound + 1].source_pos;
          }
     }
   else
     {
        if (right_bound == (int) props->ot_data->len)
          {
             items = orig_len - props->ot_data->items[left_bound].source_pos;
          }
        else
          {
             items = props->ot_data->items[right_bound - 1].source_pos -
                props->ot_data->items[right_bound].source_pos;
          }
     }
   return (items > 0) ? items : 1;
}

static void
_evas_common_font_ot_shape(hb_buffer_t *buffer, FT_Face face)
{
   hb_face_t   *hb_face;
   hb_font_t   *hb_font;

   hb_face = hb_ft_face_create(face, NULL);
   hb_font = hb_ft_font_create(face, NULL);

   hb_shape(hb_font, hb_face, buffer, NULL, 0);
   hb_font_destroy(hb_font);
   hb_face_destroy(hb_face);
}

EAPI Eina_Bool
evas_common_font_ot_populate_text_props(void *_fn, const Eina_Unicode *text,
      Evas_Text_Props *props, int len)
{
   RGBA_Font *fn = (RGBA_Font *) _fn;
   RGBA_Font_Int *fi;
   hb_buffer_t *buffer;
   hb_glyph_position_t *positions;
   hb_glyph_info_t *infos;
   int slen;
   unsigned int i;
   if (!evas_common_font_ot_is_enabled()) return EINA_TRUE;
   if (props->ot_data)
     {
        evas_common_font_ot_props_unref(props->ot_data);
     }
   props->ot_data = calloc(1, sizeof(Evas_Font_OT_Data));
   props->ot_data->refcount = 1;

   fi = fn->fonts->data;
   if (fi->src->current_size != fi->size)
     {
        FTLOCK();
        FT_Activate_Size(fi->ft.size);
        FTUNLOCK();
        fi->src->current_size = fi->size;
     }
   /* Load the font needed for this script */
   evas_common_font_glyph_search(fn, &fi, *text);

   if (len < 0)
     {
        slen = eina_unicode_strlen(text);
     }
   else
     {
        slen = len;
     }

   buffer = hb_buffer_create(slen);
   hb_buffer_set_unicode_funcs(buffer, evas_common_language_unicode_funcs_get());
   hb_buffer_set_language(buffer, hb_language_from_string(
            evas_common_language_from_locale_get()));
   hb_buffer_set_script(buffer, props->script);
   hb_buffer_set_direction(buffer,
         (props->bidi.dir == EVAS_BIDI_DIRECTION_RTL) ?
         HB_DIRECTION_RTL : HB_DIRECTION_LTR);
   /* FIXME: add run-time conversions if needed, which is very unlikely */
   hb_buffer_add_utf32(buffer, (const uint32_t *) text, slen, 0, slen);

   _evas_common_font_ot_shape(buffer, fi->src->ft.face);

   props->ot_data->len = hb_buffer_get_length(buffer);
   props->ot_data->items = calloc(props->ot_data->len,
         sizeof(Evas_Font_OT_Data_Item));
   positions = hb_buffer_get_glyph_positions(buffer);
   infos = hb_buffer_get_glyph_infos(buffer);
   for (i = 0 ; i < props->ot_data->len ; i++)
     {
        props->ot_data->items[i].index = infos[i].codepoint;
        props->ot_data->items[i].source_pos = infos[i].cluster;
        props->ot_data->items[i].x_advance = positions[i].x_advance;
        props->ot_data->items[i].x_offset = positions[i].x_offset;
        props->ot_data->items[i].y_offset = positions[i].y_offset;
     }

   hb_buffer_destroy(buffer);

   return EINA_FALSE;
}

EAPI void
evas_common_font_ot_props_ref(Evas_Font_OT_Data *data)
{
   data->refcount++;
}

EAPI void
evas_common_font_ot_props_unref(Evas_Font_OT_Data *data)
{
   OTLOCK();
   if (--data->refcount == 0)
     {
        if (data->items)
          free(data->items);
        free(data);
     }
   OTUNLOCK();
}
#endif

