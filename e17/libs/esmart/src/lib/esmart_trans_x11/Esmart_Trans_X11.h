#ifndef _ESMART_TRANS_X11_H
#define _ESMART_TRANS_X11_H
#include <Evas.h>
#include <Ecore_X.h>

#ifdef __cplusplus 
extern "C" {
#endif

typedef struct _Esmart_Trans_X11 Esmart_Trans_X11;

struct _Esmart_Trans_X11
{
    Evas_Object *obj, *clip;
    int x, y, w, h;
};

Evas_Object * esmart_trans_x11_new(Evas *e);
void esmart_trans_x11_window_set(Evas_Object *o, Ecore_X_Window win);
void esmart_trans_x11_freshen(Evas_Object *o, int x, int y, int w,
int h);

#ifdef __cplusplus
}
#endif
#endif

