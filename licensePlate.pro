QT       += core gui \
            multimedia \
            multimediawidgets \
            sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += debug_and_release

QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO

# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000
include(./licenseplatedialog.pri)
include(./testQT/gtest_dependency.pri)

SOURCES += \
    main.cpp \

HEADERS += \
    ui_licenseplatedialog.h

FORMS += \
    licenseplatedialog.ui

# Uses OpenCV 4.x (with the DNN module). Darknet / CUDA dependencies have been removed.
win32: {
    CONFIG(debug, debug|release) {
        OPENCV_LIB = lib/opencv_world430d.lib
    }
    CONFIG(release, debug|release) {
        OPENCV_LIB = lib/opencv_world430.lib
    }

    LIBS += $$PWD/OpenCV/$$OPENCV_LIB
    INCLUDEPATH += $$PWD/OpenCV/include
}

macx {
    LIBS += -L/opt/homebrew/lib -lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_dnn -lopencv_highgui
    INCLUDEPATH += /opt/homebrew/include/opencv4
}

unix:!macx {
    CONFIG += link_pkgconfig
    PKGCONFIG += opencv4
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
