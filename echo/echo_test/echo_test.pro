TEMPLATE = app

CONFIG += c++17 console
CONFIG += link_prl


TARGET = echo_mlp_test
SOURCES = echo_mlp_test.cpp
INCLUDEPATH += ../../libmlp

LIBS += -llibmlp -lstdc++fs

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../libmlp/release/
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../libmlp/debug/
else:unix: LIBS += -L$$OUT_PWD/../../libmlp/

message($$LIBS)
