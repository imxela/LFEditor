QT       += core gui

RC_ICONS = res/images/lfeditor.ico

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/workers/filereadworker.cpp \
    src/workers/filewriteworker.cpp \
    src/main.cpp \
    src/ui/editorwindow.cpp \
    src/ui/preferencemanager.cpp \
    src/ui/preferencesdialog.cpp \
    src/workers/workerbase.cpp

HEADERS += \
    src/ui/editorwindow.h \
    src/workers/filereadworker.h \
    src/workers/filewriteworker.h \
    src/ui/preferencemanager.h \
    src/ui/preferencesdialog.h \
    src/workers/workerbase.h

FORMS += \
    src/ui/editorwindow.ui \
    src/ui/preferencesdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


INCLUDEPATH += $$PWD/src

RESOURCES += \
	res/resources.qrc
