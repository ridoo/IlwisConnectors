#-------------------------------------------------
#
# Project created by QtCreator 2012-09-19T13:18:13
#
#-------------------------------------------------

CONFIG += plugin
TARGET = gdalconnector

include(global.pri)

QT       -= gui

TEMPLATE = lib

DEFINES += GDALCONNECTOR_LIBRARY

SOURCES += \
    gdalconnector/gdalconnector.cpp \
    gdalconnector/gdalmodule.cpp \
    gdalconnector/gdalproxy.cpp \
    gdalconnector/gdalitem.cpp \
    gdalconnector/coverageconnector.cpp \
    gdalconnector/coordinatesystemconnector.cpp \
    gdalconnector/domainconnector.cpp \
    gdalconnector/gridcoverageconnector.cpp \
    gdalconnector/gdalobjectfactory.cpp \
    gdalconnector/georefconnector.cpp \
    gdalconnector/gdalfeatureconnector.cpp \
    gdalconnector/gdalfeaturetableconnector.cpp \
    gdalconnector/gdaltableloader.cpp \
    gdalconnector/gdalcatalogexplorer.cpp \
    gdalconnector/gdalcatalogfileexplorer.cpp

HEADERS += \
    gdalconnector/gdalconnector.h\
    gdalconnector/gdalconnector_global.h \
    gdalconnector/gdalmodule.h \
    gdalconnector/gdalproxy.h \
    gdalconnector/gdalitem.h \
    gdalconnector/coverageconnector.h \
    gdalconnector/coordinatesystemconnector.h \
    gdalconnector/domainconnector.h \
    gdalconnector/gridcoverageconnector.h \
    gdalconnector/gdalobjectfactory.h \
    gdalconnector/georefconnector.h \
    gdalconnector/gdalfeatureconnector.h \
    gdalconnector/gdalfeaturetableconnector.h \
    gdalconnector/gdaltableloader.h \
    gdalconnector/gdalcatalogexplorer.h \
    gdalconnector/gdalcatalogfileexplorer.h

OTHER_FILES += \
    gdalconnector/gdalconnector.json \
    gdalconnector/resources/ogr_formats.config


LIBS += -L$$PWD/../libraries/$$PLATFORM$$CONF/core/ -lilwiscore \
        -L$$PWD/../libraries/$$PLATFORM$$CONF/ -llibgeos


INCLUDEPATH += $$PWD/../external/gdalheaders \
               $$PWD/../external/geos
win32{
    DLLDESTDIR = $$PWD/../output/$$PLATFORM$$CONF/bin/extensions/$$TARGET
}
unix {
    QMAKE_POST_LINK += $${QMAKE_COPY} $$PWD/../libraries/$$PLATFORM$$CONF/$$TARGET/lib$${TARGET}.so $$PWD/../output/$$PLATFORM$$CONF/bin/extensions/$$TARGET
}

resources.files = gdalconnector/resources/ogr_formats.config
resources.path = $$PWD/../output/$$PLATFORM$$CONF/bin/extensions/$$TARGET/resources

INSTALLS += resources
