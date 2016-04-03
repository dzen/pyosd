#!/usr/bin/env python
#
# $Id: daemon.py,v 1.2 2003/06/11 11:31:31 resolve Exp $
#
# Time-stamp: <2004-01-26 12:14:00 resolve>
#
# Copyright (C) Damien Elmes <resolve@repose.cx>, 2001.
# This file is licensed under the GPL. Please see COPYING for more details.
#

"""
A daemon to coordinate OSD messages.

This is a program that uses the twisted event framework to listen on a TCP
port for incoming messages. These messages consist of a command name, followed
by zero or more optional arguments. The command can either be 'display', to
display some text as an OSD, or the name of a command provided by a plugin
module.

The reason a daemon is useful is to allow separate programs to output to the
screen without their messages overlapping each other. Earlier messages on that
portion of the screen are hidden before the next one is displayed.

Modules are bits of python code which can take arbitrary actions when they
receive a string. A sample invocation of the 'volume' module, which takes an
argument to set the volume to, and then displays the current volume in a bar
graph:

   echo 'vol -5' | nc -q 0 localhost 8007

And to just print a string at the bottom of the screen, you might use:

   echo 'display -bot hello world' | nc -q 0 localhost 8007

nc is a program called 'netcat', available in most distributions, which makes
it easy to send a string to a TCP port.
"""

import os
import pyosd
import pyosd.daemon
import sys
import string

from twisted.protocols.basic import LineReceiver
from twisted.internet.protocol import Factory
from twisted.internet import reactor

PYOSD_DIR = os.path.expanduser("~/.pyosd")
PYOSD_SOCKET = os.path.join(PYOSD_DIR, "socket")
MODULES_DIR = os.path.join(PYOSD_DIR, "modules")

if __name__ == "__main__":

    if len(sys.argv)>1 and sys.argv[1] == "allinterfaces":
        allinterfaces=1
    else:
        allinterfaces=0

    args = []
    kwargs = {'shadow': 0}

    pyosd.daemon.top = apply(pyosd.osd, args, kwargs)
    pyosd.daemon.top.set_pos(pyosd.POS_TOP)
    pyosd.daemon.bot = apply(pyosd.osd, args, kwargs)
    pyosd.daemon.bot.set_pos(pyosd.POS_BOT)

    pyosd.daemon.top.set_outline_offset(1)
    pyosd.daemon.bot.set_outline_offset(1)

    class PyOSDServ:
        modules = {}
        error = 0
        files = os.listdir(MODULES_DIR)
        for f in files:
            try:
                namespace = {}
                execfile(os.path.join(MODULES_DIR, f), namespace)
                c = namespace['plugin']()
            except:
                print "Unable to load module: %s" % f
                error=1

            if not error:
                print "Adding plugin: %s" % f
                for k in c.plugin_keys:
                    modules[k] = c

    class PyOSDConn(LineReceiver):

        def __init__(self):
            self.delimiter = "\n"
            pass

        def lineReceived(self, line):

            s = string.split(line)

            if not s:
                print "Not s"
                return

            cmd = s[0]

            if PyOSDServ.modules.has_key(cmd):
                apply(getattr(PyOSDServ.modules[cmd], cmd), s[1:])
            else:
                print "Unknown command: %s" % line


    factory = Factory()
    factory.protocol = PyOSDConn

    pyosd.daemon.reactor = reactor

    if allinterfaces:
        print "Binding to all interfaces.."
        reactor.listenTCP(8007, factory) #, interface='127.0.0.1')
    else:
        reactor.listenTCP(8007, factory, interface='127.0.0.1')
    reactor.run()


