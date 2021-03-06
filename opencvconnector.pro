##-------------------------------------------------
#
# Project created by QtCreator 2012-09-19T13:18:13
#
#-------------------------------------------------

CONFIG += plugin
TARGET = opencvconnector

include(global.pri)

win32{
    DLLDESTDIR = $$PWD/../output/$$PLATFORM$$CONF/bin/extensions/$$TARGET
}

DESTDIR = $$PWD/../libraries/$$PLATFORM$$CONF/extensions/$$TARGET

TEMPLATE = lib

DEFINES += POSTGRESSQLCONNECTOR_LIBRARY

resources.files += opencvconnector/resources/libraries.config
resources.path = $$PWD/../output/$$PLATFORM$$CONF/bin/extensions/$$TARGET/resources

INSTALLS += resources

OTHER_FILES += \
    opencvconnector/opencvconnector.json 

LIBS += -L$$PWD/../libraries/$$PLATFORM$$CONF/ -lilwiscore \
        -L$$PWD/../libraries/$$PLATFORM$$CONF/ -llibgeos \
        -L$$PWD/../libraries/$$PLATFORM$$CONF/extensions/opencvconnector -lopencv_imgproc249.dll \
        -L$$PWD/../libraries/$$PLATFORM$$CONF/extensions/opencvconnector -lopencv_core249.dll
		
win32:CONFIG(release, debug|release): {
    QMAKE_CXXFLAGS_RELEASE += -O2
}

INCLUDEPATH +=  $$PWD/core \
                $$PWD/../external/geos \
                $$PWD/../external/opencv2 \

DEPENDPATH +=   $$PWD/core \
                $$PWD/../external/geos \
                $$PWD/../external/opencv2

HEADERS += \
    opencvconnector/opencvmodule.h \
    opencvconnector/opencvhelper.h \
    opencvconnector/sobelfilter.h \
    opencvconnector/laplacefilter.h \
    opencvconnector/scharrfilter.h \
    opencvconnector/bilateralfilter.h \
    opencvconnector/adaptivebilateralfilter.h \
    opencvconnector/boxfilter.h \
    opencvconnector/opencvoperation.h \
    opencvconnector/dilatefilter.h \
    opencvconnector/erodefilter.h \
    opencvconnector/gaussianblurfilter.h \
    opencvconnector/medianblurfilter.h \
    opencvconnector/comparehistograms.h \
    opencvconnector/histogramequalization.h

SOURCES += \
    opencvconnector/opencvmodule.cpp \
    opencvconnector/opencvhelper.cpp \
    opencvconnector/sobelfilter.cpp \
    opencvconnector/laplacefilter.cpp \
    opencvconnector/scharrfilter.cpp \
    opencvconnector/bilateralfilter.cpp \
    opencvconnector/adaptivebilateralfilter.cpp \
    opencvconnector/boxfilter.cpp \
    opencvconnector/opencvoperation.cpp \
    opencvconnector/dilatefilter.cpp \
    opencvconnector/erodefilter.cpp \
    opencvconnector/gaussianblurfilter.cpp \
    opencvconnector/medianblurfilter.cpp \
    opencvconnector/comparehistograms.cpp \
    opencvconnector/histogramequalization.cpp


