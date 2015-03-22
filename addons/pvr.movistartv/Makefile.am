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

LIBS            = @abs_top_srcdir@/lib/tinyxml/libtinyxml.la @abs_top_srcdir@/lib/jsoncpp/libjsoncpp.la

include ../Makefile.include.am

INCLUDES+=-Isrc -I@abs_top_srcdir@/lib/jsoncpp/include

libpvrmovistartv_addon_la_SOURCES = src/client.cpp \
									src/MovistarTVDVBSTP.cpp \
									src/MovistarTVRPC.cpp \
                                    src/PVRMovistarData.cpp

libpvrmovistartv_addon_la_LDFLAGS = @TARGET_LDFLAGS@

