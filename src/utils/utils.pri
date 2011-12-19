HEADERS = utilsexport.h \
	jid.h \
	versionparser.h \
	errorhandler.h \
	stanza.h \
	action.h \
	menu.h \
	unzipfile.h \
	message.h \
	iconsetdelegate.h \
	toolbarchanger.h \
	datetime.h \
	filestorage.h \
	iconstorage.h \
	stylestorage.h \
	menubarchanger.h \
	statusbarchanger.h \
	ringbuffer.h \
	widgetmanager.h \
	options.h \
	autosizetextedit.h \
	balloontip.h \
	closebutton.h \
	actionbutton.h \
	htmltoolbutton.h \
	borderstructs.h \
	custombordercontainer.h \
	custombordercontainer_p.h \
	customborderstorage.h \
	systemmanager.h \
	graphicseffectsstorage.h \
	log.h \
	imagemanager.h \
	custominputdialog.h \
	customlistview.h \
	customlabel.h \
	nonmodalopenfiledialog.h \
	networking.h \
	networking_p.h

SOURCES = jid.cpp \
	versionparser.cpp \
	errorhandler.cpp \
	stanza.cpp \
	action.cpp \
	menu.cpp \
	unzipfile.cpp \
	message.cpp \
	iconsetdelegate.cpp \
	toolbarchanger.cpp \
	datetime.cpp \
	filestorage.cpp \
	iconstorage.cpp \
	stylestorage.cpp \
	menubarchanger.cpp \
	statusbarchanger.cpp \
	ringbuffer.cpp \
	widgetmanager.cpp \
	options.cpp \
	autosizetextedit.cpp \
	balloontip.cpp \
	closebutton.cpp \
	actionbutton.cpp \
	htmltoolbutton.cpp \
	custombordercontainer.cpp \
	customborderstorage.cpp \
	systemmanager.cpp \
	graphicseffectsstorage.cpp \
	log.cpp \
	imagemanager.cpp \
	custominputdialog.cpp \
	customlistview.cpp \
	customlabel.cpp \
	nonmodalopenfiledialog.cpp \
	networking.cpp

macx: {

HEADERS += \
	macwidgets.h

OBJECTIVE_SOURCES += \
	macwidgets.mm

}
