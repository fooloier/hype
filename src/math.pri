SOURCES += "$$PWD/math.cpp"
HEADERS += "$$PWD/math.h"
LIBS += "$$PWD/../build/deps/math/libhypemath.a" -ltinyxml2
PRE_TARGETDEPS += "$$PWD/../build/deps/math/libhypemath.a"
