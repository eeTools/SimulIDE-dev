
VERSION = "2.0.0"
RELEASE = ""

TEMPLATE = app
TARGET = simulide

QT += svg
QT += xml
QT += widgets
QT += concurrent
QT += serialport
QT += multimedia widgets

SOURCES      = $$files( $$PWD/src/*.cpp, true )
HEADERS      = $$files( $$PWD/src/*.h, true )
TRANSLATIONS = $$files( $$PWD/resources/translations/*.ts )
FORMS       += $$files( $$PWD/src/*.ui, true )
RESOURCES    = $$PWD/src/application.qrc

INCLUDEPATH += $$PWD/src \
    $$PWD/src/components \
    $$PWD/src/components/active \
    $$PWD/src/components/connectors \
    $$PWD/src/components/graphical \
    $$PWD/src/components/logic \
    $$PWD/src/components/meters \
    $$PWD/src/components/micro \
    $$PWD/src/components/other \
    $$PWD/src/components/other/truthtable \
    $$PWD/src/components/outputs \
    $$PWD/src/components/outputs/displays \
    $$PWD/src/components/outputs/leds \
    $$PWD/src/components/outputs/motors \
    $$PWD/src/components/passive \
    $$PWD/src/components/passive/reactive \
    $$PWD/src/components/passive/resistors \
    $$PWD/src/components/passive/resist_sensors \
    $$PWD/src/components/sources \
    $$PWD/src/components/subcircuits \
    $$PWD/src/components/switches \
    $$PWD/src/gui \
    $$PWD/src/gui/appdialogs \
    $$PWD/src/gui/circuitwidget \
    $$PWD/src/gui/componentlist \
    $$PWD/src/gui/dataplotwidget \
    $$PWD/src/gui/editorwidget \
    $$PWD/src/gui/editorwidget/debuggers \
    $$PWD/src/gui/editorwidget/dialogs \
    $$PWD/src/gui/filebrowser \
    $$PWD/src/gui/memory \
    $$PWD/src/gui/properties \
    $$PWD/src/gui/serial \
    $$PWD/src/gui/testing \
    $$PWD/src/simulator \
    $$PWD/src/simulator/elements \
    $$PWD/src/simulator/elements/active \
    $$PWD/src/simulator/elements/outputs \
    $$PWD/src/simulator/elements/passive \
    $$PWD/src/microsim \
    $$PWD/src/microsim/cores \
    $$PWD/src/microsim/cores/avr \
    $$PWD/src/microsim/cores/i51 \
    $$PWD/src/microsim/cores/pic \
    $$PWD/src/microsim/cores/mcs65 \
    $$PWD/src/microsim/cores/z80 \
    $$PWD/src/microsim/cores/scripted \
    $$PWD/src/microsim/cores/qemu \
    $$PWD/src/microsim/cores/qemu/esp32 \
    $$PWD/src/microsim/cores/qemu/stm32 \
    $$PWD/src/microsim/modules \
    $$PWD/src/microsim/modules/memory \
    $$PWD/src/microsim/modules/usart \
    $$PWD/src/microsim/modules/onewire\
    $$PWD/src/microsim/modules/twi \
    $$PWD/src/microsim/modules/tcp\
    $$PWD/src/microsim/modules/spi\
    $$PWD/src/microsim/modules/script\
    $$PWD/src/angel/include \
    $$PWD/src/angel/JIT \
    $$PWD/src/angel/src \
    /usr/include/python3.12 \

QMAKE_CXXFLAGS += -Wno-unused-parameter
QMAKE_CXXFLAGS += -Wno-deprecated-declarations
QMAKE_CXXFLAGS += -Wno-implicit-fallthrough
QMAKE_CXXFLAGS += -fno-strict-aliasing      #AngelScript
QMAKE_CXXFLAGS += -Wno-cast-function-type   #AngelScript
QMAKE_CXXFLAGS += -Wno-deprecated-copy      #AngelScript
QMAKE_CXXFLAGS += -Wno-invalid-offsetof     #AngelScript
QMAKE_CXXFLAGS += -Ofast
QMAKE_CXXFLAGS_DEBUG += -D_GLIBCXX_ASSERTIONS
QMAKE_CXXFLAGS_DEBUG -= -O
QMAKE_CXXFLAGS_DEBUG -= -O1
QMAKE_CXXFLAGS_DEBUG -= -O2
QMAKE_CXXFLAGS_DEBUG -= -O3
QMAKE_CXXFLAGS_DEBUG += -O0
QMAKE_LFLAGS += -lpython3.12

LIBS += -lz
LIBS += -lpython3.12

win32 {
    OS = Windows
    QMAKE_LIBS += -lwsock32
    RC_ICONS += $$PWD/resources/icons/simulide.ico
}
linux {
    OS = Linux
}
macx {
    OS = MacOs
    ICON = $$PWD/resources/icons/simulide.icns

    QMAKE_CXXFLAGS -= -stdlib=libc++
    QMAKE_LFLAGS   -= -stdlib=libc++

# To use gcc in MacOs you must force it.
# Edit to match your system:
    QMAKE_CC   = /usr/local/Cellar/gcc@7/7.5.0_4/bin/gcc-7
    QMAKE_CXX  = /usr/local/Cellar/gcc@7/7.5.0_4/bin/g++-7
    QMAKE_LINK = /usr/local/Cellar/gcc@7/7.5.0_4/bin/g++-7
}

contains( QMAKE_HOST.arch, arm64|aarch64 ) | contains( QMAKE_CC, .*aarch64.* ){
    SOURCES += $$PWD/src/angel/src/as_callfunc_arm64_gcc.S
}

contains( QMAKE_HOST.os, Windows ) {
    REV_NO = $$system("powershell -Command get-date -format yy-MM-dd")     # year-month-day
    BUILD_DATE = $$system("powershell -Command get-date -format dd-MM-yy") # day-month-year
}
else {
    REV_NO = $$system($(which date) +\"\\\"%y%m%d\\\"\")
    BUILD_DATE = $$system($(which date) +\"\\\"%d-%m-%y\\\"\")
}

CONFIG += qt 
CONFIG += warn_on
CONFIG += no_qml_debug
CONFIG *= c++11

DEFINES += REVNO=\\\"$$REV_NO\\\"
DEFINES += APP_VERSION=\\\"$$VERSION-$$RELEASE\\\"
DEFINES += BUILDDATE=\\\"$$BUILD_DATE\\\"

TARGET_NAME   = SimulIDE_$$VERSION-$$RELEASE
TARGET_PREFIX = $$BUILD_DIR/executables/$$TARGET_NAME

OBJECTS_DIR *= $$OUT_PWD/build/objects
MOC_DIR     *= $$OUT_PWD/build/moc
INCLUDEPATH += $$MOC_DIR

DESTDIR = $$TARGET_PREFIX

runLrelease.commands = \
    lrelease $$PWD/resources/translations/*.ts; \
    lrelease $$PWD/resources/translations/qt/*.ts; \
    $(MOVE) $$PWD/resources/translations/*.qm $$PWD/resources/qm; \
    $(MOVE) $$PWD/resources/translations/qt/*.qm $$PWD/resources/qm;

QMAKE_EXTRA_TARGETS += runLrelease
PRE_TARGETDEPS      += runLrelease

message( "-----------------------------------")
message( "    "                               )
message( "    "$$TARGET_NAME for $$OS         )
message( "    "                               )
message( "    Host:      "$$QMAKE_HOST.os     )
message( "    Date:      "$$BUILD_DATE        )
message( "    Qt version: "$$QT_VERSION       )
message( "    "                               )
message( "    Destination Folder:"            )
message( $$TARGET_PREFIX                      )
message( "-----------------------------------")
