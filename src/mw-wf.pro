######################################################################
# wflash qmake project file.
#
# doragasu, 2018
######################################################################

TEMPLATE = app
TARGET = wflash
INCLUDEPATH += .

QT += widgets

DEFINES += QT_DEPRECATED_WARNINGS

# Uncomment following three lines for Windows static builds
#CONFIG+=static
#CONFIG+=no_smart_library_merge
#QTPLUGIN+=qwindows

# Extra headers
QMAKE_CXXFLAGS += $$GLIBINC
# Extra libraries
#LIBS += -lusb-1.0

DEFINES += QT

# Input files
HEADERS = cmds.h con_dlg.h flash_man.h flashdlg.h progbar.h rom_head.h util.h wflash.h
SOURCES += con_dlg.cpp flash_man.cpp flashdlg.cpp main.cpp progbar.c rom_head.c wflash.c
