QT += core
QT -= gui

CONFIG += c++17 
CONFIG += console
CONFIG += static
CONFIG += release
CONFIG -= app_bundle

SOURCES += \        
        main.cpp \
        resinfo.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \    
    resinfo.h
