/*

 pyosd - a wrapper of libxosd which allows the displaying of "on screen display"
         messages.

 Started 7/12/01.
 Copyright (C) 2001-2004, Damien Elmes <resolve@repose.cx>.

 This file is licensed under the GPL. Please see the file COPYING
 for more details.

 To compile this file, you will need libxosd. I'm waiting for a response from
 the author about some patches, so for now use the version included with this
 distribution.

*/

#include <Python.h>
#include <signal.h>
#include <xosd.h>

// raised if there's an error in the underlying library (such as X not running)
static PyObject *pyosd_error;

// lazy prototypes
static PyObject *pyosd_init(PyObject *self, PyObject *args);
static PyObject *pyosd_deinit(PyObject *self, PyObject *args);
static PyObject *pyosd_display_string(PyObject *self, PyObject *args);
static PyObject *pyosd_display_perc(PyObject *self, PyObject *args);
static PyObject *pyosd_display_slider(PyObject *self, PyObject *args);
static PyObject *pyosd_set_colour(PyObject *self, PyObject *args);
static PyObject *pyosd_set_font(PyObject *self, PyObject *args);
static PyObject *pyosd_set_timeout(PyObject *self, PyObject *args);
static PyObject *pyosd_set_pos(PyObject *self, PyObject *args);
static PyObject *pyosd_set_vertical_offset(PyObject *self, PyObject *args);
static PyObject *pyosd_set_shadow_offset(PyObject *self, PyObject *args);
static PyObject *pyosd_set_shadow_colour(PyObject *self, PyObject *args);
static PyObject *pyosd_set_outline_offset(PyObject *self, PyObject *args);
static PyObject *pyosd_set_outline_colour(PyObject *self, PyObject *args);
static PyObject *pyosd_set_align(PyObject *self, PyObject *args);
static PyObject *pyosd_set_bar_length(PyObject *self, PyObject *args);
static PyObject *pyosd_scroll(PyObject *self, PyObject *args);
static PyObject *pyosd_hide(PyObject *self, PyObject *args);
static PyObject *pyosd_show(PyObject *self, PyObject *args);
static PyObject *pyosd_wait_until_no_display(PyObject *self, PyObject *args);
static PyObject *pyosd_is_onscreen(PyObject *self, PyObject *args);
static PyObject *pyosd_get_number_lines(PyObject *self, PyObject *args);
static PyObject *pyosd_set_horizontal_offset(PyObject* self, PyObject *args);

static PyMethodDef pyosd_methods[] = {
    {"init",              pyosd_init,              METH_VARARGS},
    {"deinit",            pyosd_deinit,            METH_VARARGS},
    {"display_string",    pyosd_display_string,    METH_VARARGS},
    {"display_perc",      pyosd_display_perc,      METH_VARARGS},
    {"display_slider",    pyosd_display_slider,    METH_VARARGS},
    {"set_font",          pyosd_set_font,          METH_VARARGS},
    {"set_colour",        pyosd_set_colour,        METH_VARARGS},
    {"set_timeout",       pyosd_set_timeout,       METH_VARARGS},
    {"set_pos",           pyosd_set_pos,           METH_VARARGS},
    {"set_vertical_offset",pyosd_set_vertical_offset,        METH_VARARGS},
    {"set_horizontal_offset",pyosd_set_horizontal_offset,	METH_VARARGS},
    {"set_shadow_offset", pyosd_set_shadow_offset, METH_VARARGS},
    {"set_shadow_colour", pyosd_set_shadow_colour, METH_VARARGS},
    {"set_outline_offset",pyosd_set_outline_offset,METH_VARARGS},
    {"set_outline_colour",pyosd_set_outline_colour,METH_VARARGS},
    {"set_align",	        pyosd_set_align,	       METH_VARARGS},
    {"set_bar_length",    pyosd_set_bar_length,    METH_VARARGS},
    {"scroll",            pyosd_scroll,            METH_VARARGS},
    {"hide",            pyosd_hide, METH_VARARGS},
    {"show",    pyosd_show, METH_VARARGS},
    {"wait_until_no_display", pyosd_wait_until_no_display, METH_VARARGS},
    {"is_onscreen", pyosd_is_onscreen, METH_VARARGS},
    {"get_number_lines", pyosd_get_number_lines, METH_VARARGS},
    {NULL,  NULL}
};

void
init_pyosd(void)
{
  PyObject *self;
  PyObject *dict;

  // create the module and add the functions
  self = Py_InitModule("_pyosd", pyosd_methods);

  // init custom exception
  dict = PyModule_GetDict(self);

  pyosd_error = PyErr_NewException("pyosd.error", NULL, NULL);
  PyDict_SetItemString(dict, "error", pyosd_error);
}

////////////////////////////////////////////////////////////////////////


// check to see that osd is a valid pointer. if it's not, raise an error
// and return return 0
static int assert_osd(xosd *osd, char *error)
{
  if (!osd) {
    PyErr_SetString(pyosd_error, error);
    return 0;
  }

  return 1;
}

// pyosd_init(lines)
// initialise the osd interface
static PyObject *
pyosd_init(PyObject *self, PyObject *args)
{
  int lines;
  PyObject *pyc_osd;
  xosd *osd = NULL;

  sigset_t newset;

  if(!PyArg_ParseTuple(args, "i", &lines))
    return NULL;

  // ensure we're not currently initialised
  if (osd)
    pyosd_deinit(NULL, NULL);

  // due to an unfortunate interaction with readline, we have to ensure
  // that signals are disabled while calling xosd_init - this stops the
  // threads it spawns from accepting SIGINT, and tripping up python

  sigemptyset(&newset);
  sigaddset(&newset, SIGINT);

  sigprocmask(SIG_BLOCK, &newset, NULL);

  // bring it up
  osd = xosd_create(lines);

  // turn SIGINT back on for the main app
  sigprocmask(SIG_UNBLOCK, &newset, NULL);

  if(!osd) {
    // we don't use assert_osd here, because we want to pass back the error
    // from the underlying code
    PyErr_SetString(pyosd_error, xosd_error);
    return NULL;
  }

  // we've now got a osd reference, which we need to package up and return
  // to the surrounding python code
  pyc_osd = PyCObject_FromVoidPtr((void *)osd, NULL);

  return pyc_osd;
}

static PyObject *
pyosd_deinit(PyObject *self, PyObject *args)
{
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "O", &pyc_osd))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(osd==NULL) {
    PyErr_SetString(pyosd_error, "Already deinitialised");
    return NULL;
  }

  // deinit - the wrapping python should clear out the
  // cobject as well, as it's no longer valid.
  xosd_destroy(osd);
  osd = NULL;

  Py_INCREF(Py_None);
  return Py_None;
}


// pyosd_display_string(line, string)
static PyObject *
pyosd_display_string(PyObject *self, PyObject *args)
{
  int line;
  char *str;
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "Ois", &pyc_osd, &line, &str))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  xosd_display(osd, line, XOSD_string, str);

  Py_INCREF(Py_None);
  return Py_None;
}

// FIXME
// pyosd_display_perc(line, percentage)
static PyObject *
pyosd_display_perc(PyObject *self, PyObject *args)
{
  int line;
  int perc;
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "Oii", &pyc_osd, &line, &perc))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  xosd_display(osd, line, XOSD_percentage, perc);

  Py_INCREF(Py_None);
  return Py_None;
}

// pyosd_display_slider(line, slider)
static PyObject *
pyosd_display_slider(PyObject *self, PyObject *args)
{
  int line;
  int slider;
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "Oii", &pyc_osd, &line, &slider))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  xosd_display(osd, line, XOSD_slider, slider);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_set_font(PyObject *self, PyObject *args)
{
  char *font;
  int res;
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "Os", &pyc_osd, &font))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  res = xosd_set_font(osd, font);

  if(res==-1) {
    PyErr_SetString(pyosd_error, xosd_error);
    return NULL;
  }

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_set_colour(PyObject *self, PyObject *args)
{
  char *colour;
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "Os", &pyc_osd, &colour))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  xosd_set_colour(osd, colour);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_set_timeout(PyObject *self, PyObject *args)
{
  int timeout;
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "Oi", &pyc_osd, &timeout))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  xosd_set_timeout(osd, timeout);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_set_pos(PyObject *self, PyObject *args)
{
  int pos;
  PyObject *pyc_osd;
  xosd *osd;

  xosd_pos osd_pos;

  if(!PyArg_ParseTuple(args, "Oi", &pyc_osd, &pos))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  if (pos==0)
    osd_pos = XOSD_top;
  else if (pos==1)
    osd_pos = XOSD_bottom;
	else if (pos == 2)
		osd_pos = XOSD_middle;
	else {
    PyErr_SetString(PyExc_ValueError, "OSD position not in range");
    return NULL;
  }

  xosd_set_pos(osd, pos);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_set_align(PyObject *self, PyObject *args)
{
  int align;
  PyObject *pyc_osd;
  xosd *osd;

  xosd_align osd_align;

  if(!PyArg_ParseTuple(args, "Oi", &pyc_osd, &align))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  if (align==0)
    osd_align = XOSD_left;
  else if (align==1)
    osd_align = XOSD_center;
  else if (align==2)
    osd_align = XOSD_right;
  else{
    PyErr_SetString(PyExc_ValueError, "OSD align not in range");
    return NULL;
  }

  xosd_set_align(osd, align);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_set_bar_length(PyObject *self, PyObject *args)
{
  int bar_length;
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "Oi", &pyc_osd, &bar_length))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  xosd_set_bar_length(osd, bar_length);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_set_vertical_offset(PyObject *self, PyObject *args)
{
  int offset;
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "Oi", &pyc_osd, &offset))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  xosd_set_vertical_offset(osd, offset);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_set_horizontal_offset(PyObject *self, PyObject *args)
{
  int offset;
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "Oi", &pyc_osd, &offset))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  xosd_set_horizontal_offset(osd, offset);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_set_shadow_colour(PyObject *self, PyObject *args)
{
  char *colour;
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "Os", &pyc_osd, &colour))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  xosd_set_shadow_colour(osd, colour);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_set_shadow_offset(PyObject *self, PyObject *args)
{
  int offset;
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "Oi", &pyc_osd, &offset))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  xosd_set_shadow_offset(osd, offset);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_set_outline_offset(PyObject *self, PyObject *args)
{
  int offset;
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "Oi", &pyc_osd, &offset))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  xosd_set_outline_offset(osd, offset);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_set_outline_colour(PyObject *self, PyObject *args)
{
  char *colour;
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "Os", &pyc_osd, &colour))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  xosd_set_outline_colour(osd, colour);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_scroll(PyObject *self, PyObject *args)
{
  int amount;
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "Oi", &pyc_osd, &amount))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  xosd_scroll(osd, amount);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_hide(PyObject *self, PyObject *args)
{
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "O", &pyc_osd))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  xosd_hide(osd);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_show(PyObject *self, PyObject *args)
{
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "O", &pyc_osd))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  xosd_show(osd);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_wait_until_no_display(PyObject *self, PyObject *args)
{
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "O", &pyc_osd))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  xosd_wait_until_no_display(osd);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject *
pyosd_is_onscreen(PyObject *self, PyObject *args)
{
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "O", &pyc_osd))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  return Py_BuildValue("i", xosd_is_onscreen(osd));

}

static PyObject *
pyosd_get_number_lines(PyObject *self, PyObject *args)
{
  PyObject *pyc_osd;
  xosd *osd;

  if(!PyArg_ParseTuple(args, "O", &pyc_osd))
    return NULL;

  osd = (xosd *)PyCObject_AsVoidPtr(pyc_osd);

  if(!assert_osd(osd, "Run init() first!"))
    return NULL;

  return Py_BuildValue("i", xosd_get_number_lines(osd));

}

// vim: set sw=2 ts=2:
