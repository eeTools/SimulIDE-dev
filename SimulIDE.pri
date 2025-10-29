
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
    $$PWD/src/angel/src

QMAKE_CXXFLAGS += -Wno-unused-parameter
QMAKE_CXXFLAGS += -Wno-deprecated-declarations
QMAKE_CXXFLAGS += -Wno-implicit-fallthrough
QMAKE_CXXFLAGS += -fno-strict-aliasing      #AngelScript
QMAKE_CXXFLAGS += -Wno-cast-function-type   #AngelScript
QMAKE_CXXFLAGS += -Wno-deprecated-copy      #AngelScript
QMAKE_CXXFLAGS += -Wno-invalid-offsetof     #AngelScript
QMAKE_CXXFLAGS -= -fPIC
QMAKE_CXXFLAGS += -fno-pic
QMAKE_CXXFLAGS += -Ofast
QMAKE_CXXFLAGS_DEBUG += -D_GLIBCXX_ASSERTIONS
QMAKE_CXXFLAGS_DEBUG -= -O
QMAKE_CXXFLAGS_DEBUG -= -O1
QMAKE_CXXFLAGS_DEBUG -= -O2
QMAKE_CXXFLAGS_DEBUG -= -O3
QMAKE_CXXFLAGS_DEBUG += -O0

LIBS += -lz

win32 {
    OS = Windows
    QMAKE_LIBS += -lwsock32
    RC_ICONS += $$PWD/resources/icons/simulide.ico
}
linux {
    OS = Linux
#    Note 1: QMAKE_HOST.arch refers to the architecture of the computer which is running
#            the qmake executable, which is the same as the architecture of the computer
#            which will run the resulting build of SimulIDE when cross-compilation is not
#            being used, but when cross-compilation is being used, QMAKE_HOST.arch is not
#            the same as the architecture of the computer which will run the resulting
#            build of SimulIDE, so QMAKE_HOST.arch cannot be solely relied on if
#            reliable detection of the architecture of the computer which will run the
#            resulting build of SimulIDE is desired.
#    Note 2: Unfortunately, unlike, for example, the CMAKE_SYSTEM_PROCESSOR variable
#            found in CMake, QMake does not appear to consistently support any direct
#            equivalent to that CMAKE_SYSTEM_PROCESSOR variable which could be reliably
#            used to determine the architecture of the computer which will run the
#            resulting build of SimulIDE. Therefore, to reliably detect the architecture
#            of the computer which will run the resulting build of SimulIDE in QMake,
#            it is necessary to check multiple variables in a particular order and
#            using a particular conditional logic, in order to have the best chance
#            of accurately detecting whether the resulting build of SimulIDE will be
#            run on a computer that has a 64-bit ARM processor or not.
#    Note 3: On almost all 64-bit ARM computers which are running GNU/Linux, Android,
#            Alpine Linux, ChimeraOS or any other operating system that has a Linux
#            kernel, regardless of the exact system userspace libraries, within QMake
#            running on that 64-bit ARM computer which has an operating system that
#            has a Linux kernel, QMAKE_HOST.arch will be equal to "aarch64" and not "arm64".
#            This is in contrast to some other operating systems that do not have
#            Linux kernels, like MacOS, on which QMAKE_HOST.arch may be equal to "arm64"
#            instead.
#    Note 4: The file as_callfunc_arm_gcc.S does not exist in SimulIDE because it is
#            for 32-bit ARM, which is not supported, so it has been removed. The
#            equivalent file for 64-bit ARM is named as_callfunc_arm64_gcc.S, and that
#            file does still exist in SimulIDE, so that is the file which should be
#            built when the SimulIDE that is being built will be run on a 64-bit ARM
#            computer running any operating system that has a Linux kernel (including
#            when the toolchain Clang is being used to compile SimulIDE, as well as
#            the toolchain GCC, because the assembler of the toolchain Clang supports,
#            at least in this situation, the same format of .S assembly file that the
#            assembler of the toolchain GCC supports)

#    This is the original check, which did not work
#    contains( QMAKE_HOST.arch, arm64 ) {
#        SOURCES += $$PWD/src/angel/src/as_callfunc_arm_gcc.S
#    }

#    This is the new check, which attempts to take all four of the above notes into
#    consideration as consistently as it is possible to in QMake.
#    It is not perfect, but passes all of the common test cases I have tested it with
#    and anticipate it might need to be used for.
#    An unhandled edge case here is cross-compiling from a 64-bit ARM computer to any
#    other kind of computer, but that edge case is relatively uncommon.

#    First, check if the triplet of the C compiler being used contains "aarch64".
#    if it does, then add the as_callfunc_arm64_gcc.S to the sources to be built.
#    This enables support for cross-compilation to 64-bit ARM computers running operating
#    systems that have a Linux kernel, as long as the QMake crossfile being used defined
#    the QMAKE_CC variable.
     contains( QMAKE_CC, .*aarch64.* ) {
         SOURCES += $$PWD/src/angel/src/as_callfunc_arm64_gcc.S
     }

#    Next, if the triplet of the C compiler being used does not start with "aarch64",
#    then check whether the QMAKE_HOST.arch variable is equal to "aarch64",
#    and if it is, then add the as_callfunc_arm64_gcc.S to the sources to be built.
#    This enables support for native compilation on 64-bit ARM computers running
#    operating systems that have a Linux kernel.
     !contains ( QMAKE_CC, .*aarch64.* ) {
         contains ( QMAKE_HOST.arch, aarch64 ) {
             SOURCES += $$PWD/src/angel/src/as_callfunc_arm64_gcc.S
         }
     }
}
macx {
    OS = MacOs
    ICON = $$PWD/resources/icons/simulide.icns

    contains( QMAKE_HOST.arch, arm64 ) {
        SOURCES += $$PWD/src/angel/src/as_callfunc_arm64_xcode.S
    }
    QMAKE_CXXFLAGS -= -stdlib=libc++
    QMAKE_LFLAGS   -= -stdlib=libc++

# To use gcc in MacOs you must force it.
# Edit to match your system:
    QMAKE_CC   = /usr/local/Cellar/gcc@7/7.5.0_4/bin/gcc-7
    QMAKE_CXX  = /usr/local/Cellar/gcc@7/7.5.0_4/bin/g++-7
    QMAKE_LINK = /usr/local/Cellar/gcc@7/7.5.0_4/bin/g++-7
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
