#
# Makefile for the PVR demo add-on for XBMC PVR
#
# See the README for copyright information and
# how to reach the author.
#

ADDONBINNAME    = Kodi_MovistarTV
ADDONNAME       = pvr.movistartv
LIBNAME         = libpvrmovistartv-addon
lib_LTLIBRARIES = libpvrmovistartv-addon.la

LIBS            = @abs_top_srcdir@/lib/tinyxml/libtinyxml.la

include ../Makefile.include.am

libpvrdemo_addon_la_SOURCES = src/client.cpp \
                              src/PVRMovistarData.cpp
libpvrdemo_addon_la_LDFLAGS = @TARGET_LDFLAGS@

