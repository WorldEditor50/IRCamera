QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    mlx90640.cpp \
    serialport.cpp \
    yserialport.cpp

HEADERS += \
    mainwindow.h \
    mlx90640.h \
    serialport.h \
    yserialport.h

FORMS += \
    mainwindow.ui


win32 {
    QMAKE_CXXFLAGS += /utf-8

    LIBPATH = D:/home/3rdparty
    #opencv
    OPENCV_PATH = $$LIBPATH/opencv48
    INCLUDEPATH += $$OPENCV_PATH/include
    LIBS += -L$$OPENCV_PATH/x64/vc16/lib -lopencv_highgui480 \
                                -lopencv_imgcodecs480 \
                                -lopencv_imgproc480 \
                                -lopencv_core480

}
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
