# Copyright (c) 2008-2009 Simon Busch
#
# This file is part of python-elementary.
#
# python-elementary is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# python-elementary is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with python-elementary.  If not, see <http://www.gnu.org/licenses/>.
#

def Entry_markup_to_utf8(str):
    cdef const_char_ptr string
    string = elm_entry_markup_to_utf8(str)
    if string == NULL:
        return None
    return string

def Entry_utf8_to_markup(str):
    cdef const_char_ptr string
    string = elm_entry_utf8_to_markup(str)
    if string == NULL:
        return None
    return string

class EntryAnchorInfo:
    def __init__(self):
        self.name = None
        self.button = 0
        self.x = 0
        self.y = 0
        self.w = 0
        self.h = 0

class EntryAnchorHoverInfo:
    def __init__(self):
        self.anchor_info = None
        self.hover = None
        self.hover_parent = (0, 0, 0, 0)
        self.hover_left = False
        self.hover_right = False
        self.hover_top = False
        self.hover_bottom = False

def _entryanchor_conv(long addr):
    cdef Elm_Entry_Anchor_Info *ei = <Elm_Entry_Anchor_Info *>addr
    eai = EntryAnchorInfo()
    eai.name = ei.name
    eai.button = ei.button
    eai.x = ei.x
    eai.y = ei.y
    eai.w = ei.w
    eai.h = ei.h
    return eai
    
def _entryanchorhover_conv(long addr):
    cdef Elm_Entry_Anchor_Hover_Info *ehi = <Elm_Entry_Anchor_Hover_Info *>addr
    eahi = EntryAnchorHoverInfo()
    eahi.hover = Hover(None, <object>ehi.hover)
    eahi.hover_parent = (ehi.hover_parent.x, ehi.hover_parent.y,
                       ehi.hover_parent.w, ehi.hover_parent.h)
    eahi.hover_left = ehi.hover_left
    eahi.hover_right = ehi.hover_right
    eahi.hover_top = ehi.hover_top
    eahi.hover_bottom = ehi.hover_bottom
    return eahi

cdef class Entry(Object):
    def __init__(self, c_evas.Object parent):
        Object.__init__(self, parent.evas)
        self._set_obj(elm_entry_add(parent.obj))

    def callback_changed_add(self, func, *args, **kwargs):
        self._callback_add("changed", func, *args, **kwargs)

    def callback_changed_del(self, func):
        self._callback_del("changed", func)

    def callback_changed_user_add(self, func, *args, **kwargs):
        self._callback_add("changed,user", func, *args, **kwargs)

    def callback_changed_user_del(self, func):
        self._callback_del("changed,user", func)

    def callback_selection_start_add(self, func, *args, **kwargs):
        self._callback_add("selection,start", func, *args, **kwargs)

    def callback_selection_start_del(self, func):
        self._callback_del("selection,start", func)

    def callback_selection_changed_add(self, func, *args, **kwargs):
        self._callback_add("selection,changed", func, *args, **kwargs)

    def callback_selection_changed_del(self, func):
        self._callback_del("selection,changed", func)

    def callback_selection_cleared_add(self, func, *args, **kwargs):
        self._callback_add("selection,cleared", func, *args, **kwargs)

    def callback_selection_cleared_del(self, func):
        self._callback_del("selection,cleared", func)

    def callback_selection_paste_add(self, func, *args, **kwargs):
        self._callback_add("selection,paste", func, *args, **kwargs)

    def callback_selection_paste_del(self, func):
        self._callback_del("selection,paste", func)

    def callback_selection_copy_add(self, func, *args, **kwargs):
        self._callback_add("selection,copy", func, *args, **kwargs)

    def callback_selection_copy_del(self, func):
        self._callback_del("selection,copy", func)

    def callback_selection_cut_add(self, func, *args, **kwargs):
        self._callback_add("selection,cut", func, *args, **kwargs)

    def callback_selection_cut_del(self, func):
        self._callback_del("selection,cut", func)

    def callback_cursor_changed_add(self, func, *args, **kwargs):
        self._callback_add("cursor,changed", func, *args, **kwargs)

    def callback_cursor_changed_del(self, func):
        self._callback_del("cursor,changed", func)

    def callback_anchor_clicked_add(self, func, *args, **kwargs):
        self._callback_add_full("anchor,clicked", _entryanchor_conv,
                                func, *args, **kwargs)

    def callback_anchor_clicked_del(self, func):
        self._callback_del_full("anchor,clicked", _entryanchor_conv,
                                func)

    def callback_anchor_hover_opened_add(self, func, *args, **kwargs):
        self._callback_add_full("anchor,hover,opened", _entryanchorhover_conv,
                                func, *args, **kwargs)

    def callback_anchor_hover_opened_del(self, func):
        self._callback_del_full("anchor,hover,opened", _entryanchorhover_conv,
                                func)

    def callback_activated_add(self, func, *args, **kwargs):
        self._callback_add("activated", func, *args, **kwargs)

    def callback_activated_del(self, func):
        self._callback_del("activated", func)

    def callback_clicked_add(self, func, *args, **kwargs):
        self._callback_add("clicked", func, *args, **kwargs)

    def callback_clicked_del(self, func):
        self._callback_del("clicked", func)

    def callback_double_clicked_add(self, func, *args, **kwargs):
        self._callback_add("clicked,double", func, *args, **kwargs)

    def callback_double_clicked_del(self, func):
        self._callback_del("clicked,double", func)

    def callback_focused_add(self, func, *args, **kwargs):
        self._callback_add("focused", func, *args, **kwargs)

    def callback_focused_del(self, func):
        self._callback_del("focused", func)

    def callback_unfocused_add(self, func, *args, **kwargs):
        self._callback_add("unfocused", func, *args, **kwargs)

    def callback_unfocused_del(self, func):
        self._callback_del("unfocused", func)

    def scrollable_set(self, scrollable):
        elm_entry_scrollable_set(self.obj, scrollable)

    def scrollable_get(self):
        return bool(elm_entry_scrollable_get(self.obj))

    property scrollable:
        def __get__(self):
            return self.scrollable_get()

        def __set__(self, value):
            self.scrollable_set(value)

    def single_line_set(self, single_line):
        elm_entry_single_line_set(self.obj, single_line)

    def single_line_get(self):
        return bool(elm_entry_single_line_get(self.obj))

    property single_line:
        def __get__(self):
            return self.single_line_get()

        def __set__(self, value):
            self.single_line_set(value)

    def password_set(self, password):
        elm_entry_password_set(self.obj, password)

    def password_get(self):
        return bool(elm_entry_password_get(self.obj))

    property password:
        def __get__(self):
            return self.password_get()

        def __set__(self, value):
            self.password_set(value)

    def entry_set(self, entry):
        elm_entry_entry_set(self.obj, entry)

    def entry_get(self):
        return elm_entry_entry_get(self.obj)

    property entry:
        def __get__(self):
            return self.entry_get()

        def __set__(self, value):
            self.entry_set(value)

    def selection_get(self):
        cdef const_char_ptr s
        s = elm_entry_selection_get(self.obj)
        if s == NULL:
            return ""
        return s

    def entry_insert(self, entry):
        elm_entry_entry_insert(self.obj, entry)

    def line_wrap_set(self, wrap):
        elm_entry_line_wrap_set(self.obj, wrap)

    def editable_set(self, editable):
        elm_entry_editable_set(self.obj, editable)

    def editable_get(self):
        return bool(elm_entry_editable_get(self.obj))

    property editable:
        def __get__(self):
            return self.editable_get()

        def __set__(self, value):
            self.editable_set(value)

    def select_none(self):
        elm_entry_select_none(self.obj)

    def select_all(self):
        elm_entry_select_all(self.obj)

    def cursor_next(self):
        return bool(elm_entry_cursor_next(self.obj))

    def cursor_prev(self):
        return bool(elm_entry_cursor_prev(self.obj))

    def cursor_up(self):
        return bool(elm_entry_cursor_up(self.obj))

    def cursor_down(self):
        return bool(elm_entry_cursor_down(self.obj))

    def cursor_begin_set(self):
        elm_entry_cursor_begin_set(self.obj)

    def cursor_end_set(self):
        elm_entry_cursor_end_set(self.obj)

    def cursor_line_begin_set(self):
        elm_entry_cursor_line_begin_set(self.obj)

    def cursor_line_end_set(self):
        elm_entry_cursor_line_end_set(self.obj)

    def cursor_selection_begin(self):
        elm_entry_cursor_selection_begin(self.obj)

    def cursor_selection_end(self):
        elm_entry_cursor_selection_end(self.obj)

    def cursor_is_format_get(self):
        return bool(elm_entry_cursor_is_format_get(self.obj))

    def cursor_is_visible_format_get(self):
        return bool(elm_entry_cursor_is_visible_format_get(self.obj))

    def cursor_content_get(self):
        return elm_entry_cursor_content_get(self.obj)

    def selection_cut(self):
        elm_entry_selection_cut(self.obj)

    def selection_copy(self):
        elm_entry_selection_copy(self.obj)

    def selection_paste(self):
        elm_entry_selection_paste(self.obj)

    def context_menu_disabled_set(self, disabled):
        elm_entry_context_menu_disabled_set(self.obj, disabled)

    def input_panel_enabled_set(self, enabled):
        elm_entry_input_panel_enabled_set(self.obj, enabled)

    def input_panel_enabled_get(self):
        return bool(elm_entry_input_panel_enabled_get(self.obj))

    def anchor_hover_parent_set(self, c_evas.Object anchor_hover_parent):
        elm_entry_anchor_hover_parent_set(self.obj, anchor_hover_parent.obj)

    def anchor_hover_parent_get(self):
        cdef c_evas.Evas_Object *anchor_hover_parent
        anchor_hover_parent = elm_entry_anchor_hover_parent_get(self.obj)
        return evas.c_evas._Object_from_instance(<long> anchor_hover_parent)

    property anchor_hover_parent:
        def __get__(self):
            return self.anchor_hover_parent_get()

        def __set__(self, value):
            self.anchor_hover_parent_set(value)

    def anchor_hover_style_set(self, style):
        elm_entry_anchor_hover_style_set(self.obj, style)

    def anchor_hover_style_get(self):
        return elm_entry_anchor_hover_style_get(self.obj)

    property anchor_hover_style:
        def __get__(self):
            return self.anchor_hover_style_get()

        def __set__(self, value):
            self.anchor_hover_style_set(value)

    def anchor_hover_end(self):
        elm_entry_anchor_hover_end(self.obj)

    markup_to_utf8 = staticmethod(Entry_markup_to_utf8)

    utf8_to_markup = staticmethod(Entry_utf8_to_markup)


_elm_widget_type_register("entry", Entry)
