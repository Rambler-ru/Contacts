include(../config.inc)

TARGET             = $$TARGET_LOADER
TEMPLATE           = app
QT                += xml network

macx: {
  QT              += webkit
}

LIBS              += -L../libs
LIBS              += -l$$TARGET_UTILS
win32:LIBS        += -lholdemutils -lAdvapi32 -lUser32 -lOle32
DEPENDPATH        += ..
INCLUDEPATH       += ..
DESTDIR            = ../..
include(loader.pri)

#Appication icon
win32:RC_FILE      = loader.rc
macx:ICON          = ../../virtus.icns

#SVN Info
SVN_REVISION=$$system(svnversion -n -c ./../../)
win32 {
  exists(svninfo.h):system(del svninfo.h)
  !isEmpty(SVN_REVISION):system(echo $${LITERAL_HASH}define SVN_REVISION \"$$SVN_REVISION\" >> svninfo.h) {
    DEFINES       += SVNINFO
  }
} else {
  exists(svninfo.h):system(rm -f svninfo.h)
  !isEmpty(SVN_REVISION):system(echo \\$${LITERAL_HASH}define SVN_REVISION \\\"$${SVN_REVISION}\\\" >> svninfo.h) {
    DEFINES       += SVNINFO
  }
}

#Translation
TRANS_SOURCE_ROOT  = ..
include(../translations.inc)

#Install
include(../install.inc)
target.path        = $$INSTALL_BINS
resources.path     = $$INSTALL_RESOURCES
resources.files    = ../../resources/*
documents.path     = $$INSTALL_DOCUMENTS
documents.files    = ../../AUTHORS ../../CHANGELOG ../../README ../../COPYING
translations.path  = $$INSTALL_TRANSLATIONS
translations.files = ../../translations/*
INSTALLS           = target resources documents translations

#Linux desktop install
unix:!macx {
  icons.path       = $$INSTALL_PREFIX/$$INSTALL_RES_DIR/pixmaps
  icons.files      = ../../resources/menuicons/shared/vacuum.png
  INSTALLS        += icons

  desktop.path     = $$INSTALL_PREFIX/$$INSTALL_RES_DIR/applications
  desktop.files    = ../../src/packages/linux/*.desktop
  INSTALLS        += desktop
}

#MaxOS Install
macx {
  UTILS_LIB_NAME   = lib$${TARGET_UTILS}.1.0.0.dylib
  UTILS_LIB_LINK   = lib$${TARGET_UTILS}.1.dylib

  lib_utils.path   = $$INSTALL_LIBS
  lib_utils.extra  = cp -f ../libs/$$UTILS_LIB_NAME $(INSTALL_ROOT)$$INSTALL_LIBS/$$UTILS_LIB_NAME && \
		     ln -sf $$UTILS_LIB_NAME $(INSTALL_ROOT)$$INSTALL_LIBS/$$UTILS_LIB_LINK
  INSTALLS        += lib_utils

  name_tool.path   = $$INSTALL_BINS
  name_tool.extra  = install_name_tool -change $$UTILS_LIB_LINK @executable_path/../Frameworks/$$UTILS_LIB_LINK $(INSTALL_ROOT)$$INSTALL_BINS/$$INSTALL_APP_DIR/Contents/MacOS/$$TARGET_LOADER
  INSTALLS        += name_tool
}
