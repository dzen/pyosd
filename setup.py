#!/usr/bin/env python

from distutils.core import setup, Extension

setup (name = "pyosd",
       version = "0.2.14",
       description = "Python wrapper for libosd",
       url = "http://repose.cx/pyosd/",
       author = "Damien Elmes",
       author_email = "pyosd@repose.cx",
       packages = ['pyosd'],
       ext_modules = \
       [Extension("_pyosd", ["_pyosd.c"],
                  libraries=["xosd"])]
      )

