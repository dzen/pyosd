#!/usr/bin/env python
#
# $Id: volume.py,v 1.3 2003/06/11 11:31:26 resolve Exp $
#
# Time-stamp: <2002-04-01 00:25:37 resolve>
#
# Copyright (C) Damien Elmes <resolve@repose.cx>, 2001.
# This file is licensed under the GPL. Please see COPYING for more details.
#


"""
Plugin for PyOSDd which facilitates changing volume
"""

import commands
import os
import re

import pyosd, pyosd.daemon as pyd

class plugin:

    def __init__(self):
        self.plugin_name = "volume"
        self.plugin_desc = "Alter main system volume"
        self.plugin_keys = ["vol"]

    def vol(self, *args):
        """ Set or display the volume """

        if args:
            vol_str = args[0]

            # set the new volume
            os.system("aumix -v %s" % vol_str)

        self.display_volume()

    def display_volume(self):
        """ Determine current volume, and display it. """

        # twisted bug
        import signal
        signal.signal(signal.SIGCHLD, signal.SIG_DFL)

        # read volume in
        output = commands.getoutput("aumix -q")
        # locate main volume
        m = re.match("vol (\d+),", output)
        vol = int(m.group(1))

        # display!
        pyd.top.set_colour("#bbbbFF")
        pyd.top.set_timeout(3)
        pyd.top.display("Volume (%d%%)" % vol)
        pyd.top.display(vol, type=pyosd.TYPE_PERCENT, line=1)
