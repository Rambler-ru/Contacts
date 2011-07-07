include(../config.inc)

TARGET             = $$TARGET_UTILS

TEMPLATE           = lib
CONFIG            += dll
QT                += xml webkit network
DEFINES           += UTILS_DLL
LIBS              += -L../libs
LIBS              += -lidn -lminizip -lzlib -lidle
macx: {
  QMAKE_LFLAGS    += -framework Carbon -framework Cocoa
} else:unix {
  LIBS            += -lXss
  CONFIG          += x11
} else:win32 {
  LIBS            += -luser32 -lComdlg32
  DLLDESTDIR       = ..\\..
  QMAKE_DISTCLEAN += $${DLLDESTDIR}\\$${TARGET}.dll
}
DEPENDPATH        += ..
INCLUDEPATH       += .. ../thirdparty/zlib
DESTDIR            = ../libs
include(utils.pri)

#Translation
TRANS_SOURCE_ROOT  = ..
include(../translations.inc)

#Install (for Mac OS X - in loader.pro)
!macx:{
  include(../install.inc)
  target.path      = $$INSTALL_LIBS
  INSTALLS         = target
}

win32:RC_FILE      = utils.rc

