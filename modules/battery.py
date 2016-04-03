#!/usr/bin/env python
#
# Copyright (C) Damien Elmes <resolve@repose.cx>, 2001.
# This file is licensed under the GPL. Please see COPYING for more details.
#


"""
Display battery usage information and monitor for low available battery.

The default warn and shutdown percentages are very conservative, due
to a badly designed Compaq laptop I own which 'loses' 10% of the total
capacity if allowed to fully discharge. Ideally the info file's
warning and low capacity information should be used instead.
"""

# percentage when a warning should appear
THRESHOLD_WARN = 11
# percentage when the computer should be automatically shut down
THRESHOLD_SHUTDOWN = 9
# total battery capacity (if acpi doesn't know this value)
TOTAL_CAPACITY = 3736.0

ACPI_PROC_DIR = "/proc/acpi/battery/"
STATE_FILE = "state"
INFO_FILE = "info"

import commands
import os
import re
import string
import pyosd, pyosd.daemon as pyd

class BatteryStatus:
    """Provides battery status information.

    Provides status information such as the charging state and
    remaining capacity. Usage:

       bs = BatteryStatus()
       bs.check()
       print bs.availablePercentage"""


    def __init__(self, battery="BAT0"):
        """The battery argument can be used to specify a particular battery to
        use, otherwise the first one will be used."""

        self.batteryPath = os.path.join(ACPI_PROC_DIR, battery)
        stateFilePath = os.path.join(self.batteryPath, STATE_FILE)

        try:
            self.stateFile = open(stateFilePath)
        except:
            raise "Couldn't open battery state file: %s" % \
                  stateFilePath

        self.findTotalCapacity()

    def findTotalCapacity(self):
        "Try and derive the total capacity if it's available"

        infoFilePath = os.path.join(self.batteryPath, INFO_FILE)
        try:
            infoFile = open(infoFilePath)
            data = self.readFile(infoFile)
            if data['last full capacity']:
                full = int(string.split(
                    data['last full capacity'])[0])
                if full:
                    self.totalCapacity = full
                    return

        except IOError:
            self.totalCapacity = TOTAL_CAPACITY

    def check(self):
        """Check battery status"""

        self.data = self.readFile(self.stateFile)
        self.remainingCapacity = int(string.split(self.data['remaining capacity'])[0])
        self.percentAvailable = (self.remainingCapacity / float(self.totalCapacity)) * 100

    def readFile(self, fileObj):
        """Read an ACPI info or state file into a hash"""

        fileObj.seek(0)
        lines = fileObj.readlines()

        data = {}
        for line in lines:
            (id, val) = string.split(line, ":", maxsplit=1)
            data[id] = string.strip(val)

        return data

class plugin:

    def __init__(self):
        self.plugin_name = "battery"
        self.plugin_desc = "Display ACPI battery status"
        self.plugin_keys = ["bat"]

        self.batstat = BatteryStatus()
        self.increment = 100

        self.check_battery()

    def check_battery(self):
        """Check battery status and warn when it reaches a certain level"""

        self.batstat.check()

        if self.batstat.data['charging state'] == 'discharging':
            # if the battery is discharging, check we're not running out of
            # power

            if self.batstat.percentAvailable < THRESHOLD_SHUTDOWN:
                pyd.top.set_timeout(-1)
                pyd.top.display(("Critical! Battery at less than %d%%" +
                                " - shutting down") % threshhold_shutdown)
                os.system("sync")
                os.system("sudo halt")
                return

            if self.batstat.percentAvailable < THRESHOLD_WARN:
                pyd.top.set_timeout(-1)
                pyd.top.display("Alert! Battery at %d%%" %
                                self.batstat.percentAvailable)

            if self.batstat.percentAvailable < self.increment:
                self.bat()
                self.increment = self.batstat.percentAvailable - 5
        else:
            pyd.top.set_timeout(3)
            self.increment = 100

        pyd.reactor.callLater(30, self.check_battery)

    def bat(self, *args):
        """Display battery info"""

        self.batstat.check()

        if self.batstat.data['charging state'] in ('unknown', 'charged'):
            state = "On A/C power."
        else:
            state = self.batstat.data['charging state']

        state = state + " (at %d%%)" % self.batstat.percentAvailable

        pyd.top.set_colour("#bbbbFF")
        pyd.top.set_timeout(3)
        pyd.top.display(state)
        pyd.top.display(self.batstat.percentAvailable,
                        type=pyosd.TYPE_PERCENT, line=1)

