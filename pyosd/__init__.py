#!/usr/bin/env python
#
# Started 5/8/01 by Damien Elmes <resolve@repose.cx>
# This file is licensed under the GPL.
# Copyright 2001-2004 Damien Elmes
#

'''
pyosd - a wrapper of libxosd which allows the displaying of "on screen display"
         messages.

         example usage:

         | import pyosd
         | p = pyosd.osd()
         | # try fixed if the default_font gives you an error
         |
         | p.display("eschew obfuscation")
         | p.set_pos(pyosd.POS_BOT)
         | p.display("this will be down the bottom.")


         .. etc.

         Multiple instances can be used to display information in different
         fonts or colours across the screen.
         '''

import _pyosd
import re
import string

POS_TOP=0
POS_BOT=1
POS_MID=2

ALIGN_LEFT=0
ALIGN_CENTER=1
ALIGN_RIGHT=2

TYPE_STRING=0
TYPE_PERCENT=1
TYPE_SLIDER=2

error = _pyosd.error

default_font="-*-helvetica-medium-r-normal-*-*-360-*-*-p-*-*-*"

class osd:
    """ osd is a class used to create an object which can display messages on
    the screen. """

    def __init__(self, font=default_font, colour="#FFFFFF",timeout=3, \
                 pos=POS_TOP, offset=0, hoffset=0, shadow=0,
                 align=ALIGN_LEFT, lines=2, noLocale=False):
        """ Initialise the OSD library.

        This must be done before display() will work. It will automatically
        deinit if necessary.

        font(pyosd.default_font): standard string-style X font description
        colour('#FFFFFF'): standard string-style X colour description
        timeout(3): number of seconds to remain on screen (-1 for infinite)
        pos(POS_TOP): position, one of POS_TOP or POS_BOT
        offset(0): vertical offset from pos
        shadow(0): black shadow size
        lines(2): the max number of lines available to display at once.
        noLocale(False): disable setlocale()

        In order to display foreign characters properly, pyosd calls
        setlocale() when a new object is created. If you are using
        threads in your application, or if you wish to set the locale
        yourself, pass noLocale=True, and use code like the following
        at the top of your application:

            import locale
            locale.setlocale(locale.LC_ALL, "")
        """

        self._osd = _pyosd.init(lines)
        # save this as we won't have access to it on del
        self._deinit = _pyosd.deinit

        self.set_font(font)
        self.set_colour(colour)
        self.set_pos(pos)
        self.set_vertical_offset(offset)
        self.set_horizontal_offset(hoffset)
        self.set_shadow_offset(shadow)

        self.set_align(align)
        self.set_timeout(timeout)

        # this should be safe to run on each object initialisation
        if not noLocale:
            import locale
            locale.setlocale(locale.LC_ALL, "")

    def __del__(self):
        """ Shut down and clean up.

        Note that init() will automatically do this step if necessary. """

        # prevent python from dying if a user's silly enough to call this
        # directly
        if hasattr(self, '_osd'):
            self._deinit(self._osd)
            del self._osd

    def display(self, arg, type=TYPE_STRING, line=0):
        """ Display a string/bargraph/percentage using information from init()

        arg: a string or integer from 1-100, depending on the type
        -- defaults --
        type(TYPE_STRING): one of TYPE_STRING, TYPE_PERCENT, or TYPE_SLIDER
        line(0): the line to display text on

        The underlying library currently doesn't zero out previous lines that
        aren't being used, so if you wish to display something on, say, line 1,
        make sure you simultaneously display "" on line 0.
        """

        if line >= self.get_number_lines() or line < 0:
            raise ValueError, "specified line is out of range"

        if type==TYPE_STRING:
            _pyosd.display_string(self._osd, line, arg)

        elif type==TYPE_PERCENT:
            _pyosd.display_perc(self._osd, line, int(arg))

        elif type==TYPE_SLIDER:
            _pyosd.display_slider(self._osd, line, int(arg))

        else:
            raise ValueError, "type not in list of valid values!"

    def set_font(self, font):
        """Change the font.

        `font' should be a normal X font specification."""

        _pyosd.set_font(self._osd, font)

    def set_colour(self, c):
        """Change the colour."""

        _pyosd.set_colour(self._osd, c)

    def set_timeout(self, t):
        """Change the timeout.

        This takes effect immediately; anything that is currently displayed
        will wait the new timeout time before clearing."""

        _pyosd.set_timeout(self._osd, t)

    def set_pos(self, p):
        """Change the position to the top or bottom."""

        _pyosd.set_pos(self._osd, p)

    def set_align(self, a):
        """Change the alignment to left, center or right."""

        _pyosd.set_align(self._osd,a)

    def set_vertical_offset(self, o):
        """Change the vertical offset from the top or bottom."""

        _pyosd.set_vertical_offset(self._osd, o)

    def set_offset(self, o):
        """This method is here for compability issues. Usage of this is deprecated.
        set_vertical_offset should be used instead."""
        self.set_vertical_offset(o)

    def set_horizontal_offset(self,o):
        """Change the horizontal offset from the left or right."""

        _pyosd.set_horizontal_offset(self._osd,o)

    def set_shadow_colour(self, col):
        """Change the colour of the shadow."""

        _pyosd.set_shadow_colour(self._osd, col)

    def set_shadow_offset(self, o):
        """Change the offset of the shadow."""

        _pyosd.set_shadow_offset(self._osd, o)

    def set_outline_offset(self, o):
        """Change the offset of the text outline."""

        _pyosd.set_outline_offset(self._osd, o)

    def set_outline_colour(self, c):
        """Change the colour of the outline."""

        _pyosd.set_outline_colour(self._osd, c)

    def set_bar_length(self, o):
        """Change the bar length."""

        _pyosd.set_bar_length(self._osd, o)

    def scroll(self, lines=1):
        """Scroll the display."""
        if lines >= self.get_number_lines() or lines < 0:
            raise ValueError, "specified line is out of range"
        _pyosd.scroll(self._osd, lines)

    def hide(self):
       """Hide the display."""
       _pyosd.hide(self._osd)

    def show(self):
        """Show the display."""
        _pyosd.show(self._osd)

    def wait_until_no_display(self):
        """Block until nothing is displayed."""
        _pyosd.wait_until_no_display(self._osd)

    def is_onscreen(self):
        """True if PyOSD is currently displaying something."""
        return _pyosd.is_onscreen(self._osd)

    def get_number_lines(self):
        "Returns the maximum number of lines which can be displayed."
        return _pyosd.get_number_lines(self._osd)

