QT       += core gui \
            multimedia \
            multimediawidgets \
            sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += debug_and_release

QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
include(./licenseplatedialog.pri)
include(./testQT/gtest_dependency.pri)

SOURCES += \
    main.cpp \

HEADERS += \
    ui_licenseplatedialog.h \
    yolo_v2_class.hpp

FORMS += \
    licenseplatedialog.ui

win32: {
    CONFIG(debug, debug|release) {
        OPENCV_LIB = lib/opencv_world430d.lib
        YOLOV3_LIB = yolov3Lib/Debug/yolo_cpp_dll.lib
    }
    CONFIG(release, debug|release) {
        OPENCV_LIB = lib/opencv_world430.lib
        YOLOV3_LIB = yolov3Lib/yolo_cpp_dll.lib
 }

    LIBS += $$PWD/OpenCV/$$OPENCV_LIB \
                   $$PWD/$$YOLOV3_LIB \
                   -L$$quote(C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v11.7/lib/x64) -lcublas -lcuda -lcudadevrt -lcudart -lcudart_static -lOpenCL


    INCLUDEPATH += $$PWD/OpenCV/include

   # debug { DESTDIR = opencv_world430d.dll }
    #release { DESTDIR = opencv_world430.dll }

   # opencvlibs.path = $$PWD/OpenCV/bin
    #opencvlibs.files = $$QMAKE_LIBDIR_QT/$$DESTDIR
    #opencvlibs.CONFIG = no_check_exist

   INSTALLS += opencvlibs
}

macx {
    LIBS += /Users/huangyaode/code/opencv/build_arm64/lib

    INCLUDEPATH += /opt/homebrew/Cellar/opencv/4.5.5_1/include/opencv4
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
