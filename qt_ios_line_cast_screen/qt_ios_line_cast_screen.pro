QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/SoundPlay.cpp \
    src/ffmpegcommon.cpp \
    src/airplaysource.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/H264Decoder.cpp \
    src/utils/gcusb.cpp \
    src/utils/RW.cpp \
    src/videowindow.cpp

HEADERS += \
    src/SoundPlay.h \
    src/ffmpegcommon.h \
    src/airplaysource.h \
    src/mainwindow.h \
    src/H264Decoder.h \
    src/utils/gcusb.h \
    src/utils/RW.h \
    src/videowindow.h

FORMS += \
    src/mainwindow.ui \
    src/videowindow.ui

INCLUDEPATH += $$PWD/src
INCLUDEPATH += $$PWD/src\utils
INCLUDEPATH += $$PWD/../packages/ios_line_cast_screen/include
INCLUDEPATH += $$PWD/../packages/libusb-win32/include

INCLUDEPATH += $$PWD/../packages/ffmpeg/include
LIBS += $$PWD/../packages/ffmpeg/lib/avcodec.lib
LIBS += $$PWD/../packages/ffmpeg/lib/avdevice.lib
LIBS += $$PWD/../packages/ffmpeg/lib/avfilter.lib
LIBS += $$PWD/../packages/ffmpeg/lib/avformat.lib
LIBS += $$PWD/../packages/ffmpeg/lib/avutil.lib
LIBS += $$PWD/../packages/ffmpeg/lib/postproc.lib
LIBS += $$PWD/../packages/ffmpeg/lib/swresample.lib
LIBS += $$PWD/../packages/ffmpeg/lib/swscale.lib

INCLUDEPATH += $$PWD/../packages/SDL2/include
LIBS += $$PWD/../packages/SDL2/lib/x64/SDL2.lib
LIBS += $$PWD/../packages/SDL2/lib/x64/SDL2test.lib
LIBS += $$PWD/../packages/SDL2/lib/x64/SDL2main.lib

INCLUDEPATH += $$PWD/../packages/libwdi/include


win32:CONFIG(debug, debug|release): {
    LIBS += -L$$QMAKE_SKIA_DIR/out/Debug-x64
    INCLUDEPATH += $$QMAKE_SKIA_DIR/out/Debug-x64
    DEPENDPATH += $$QMAKE_SKIA_DIR/out/Debug-x64
    QMAKE_CFLAGS_DEBUG += -MTd
    QMAKE_CXXFLAGS_DEBUG += -MTd

    LIBS += $$PWD/../packages/ios_line_cast_screen/lib/debug_x64/ios_line_cast_screen.lib
    LIBS += $$PWD/../packages/libusb-win32/lib/debug_x64/libusb0.lib
    LIBS += $$PWD/../packages/libwdi/lib/libwdi-x64-mtd-vs2019.lib
}
else:win32:CONFIG(release, debug|release): {
    LIBS += -L$$QMAKE_SKIA_DIR/out/Release-x64
    INCLUDEPATH += $$QMAKE_SKIA_DIR/out/Release-x64
    DEPENDPATH += $$QMAKE_SKIA_DIR/out/Release-x64
    #win32:QMAKE_CXXFLAGS += /MD
    QMAKE_CFLAGS_RELEASE += -MT
    QMAKE_CXXFLAGS_RELEASE += -MT

    LIBS += $$PWD/../packages/ios_line_cast_screen/lib/release_x64/ios_line_cast_screen.lib
    LIBS += $$PWD/../packages/libusb-win32/lib/release_x64/libusb0.lib
    LIBS += $$PWD/../packages/libwdi/lib/libwdi-x64-mt-vs2019.lib
}


DEFINES  -= UNICODE

