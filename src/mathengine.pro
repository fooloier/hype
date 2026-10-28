QT = core gui
TEMPLATE = lib
CONFIG += staticlib release c++17 exceptions
CONFIG -= exceptions_off ltcg
TARGET = hypemath
isEmpty(MICROTEX): MICROTEX = "$$PWD/../build/deps/MicroTeX"
INCLUDEPATH += "$$MICROTEX/src" "$$PWD"
DEFINES += BUILD_QT NDEBUG
ENGINE_DIRS = atom box core fonts utils res/builtin res/font res/parser res/reg res/sym
for(dir, ENGINE_DIRS) {
    SOURCES += $$files("$$MICROTEX/src/$$dir/*.cpp")
}
SOURCES += "$$MICROTEX/src/latex.cpp" "$$MICROTEX/src/render.cpp" \
    "$$MICROTEX/src/platform/qt/graphic_qt.cpp" "$$PWD/mathengine.cpp"
