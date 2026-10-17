NH_QT_MAJOR ?= 6

include ./NickelHook/NickelHook.mk

override LIBRARY  := libstorygraph.so
override SOURCES  += $(wildcard src/*.cc src/**/*.cc)
override CFLAGS   += -Wall -Wextra -Werror -Wno-missing-field-initializers -fvisibility=hidden
override CXXFLAGS += -Wall -Wextra -Werror -Wno-missing-field-initializers -isystemminiz -fvisibility=hidden -fvisibility-inlines-hidden
override PKGCONF  += Qt$(NH_QT_MAJOR)Widgets Qt$(NH_QT_MAJOR)Network
override MOCS     += $(wildcard src/*.h src/**/*.h)
override QRCS     += res/nickelstoregraph.qrc

include ./NickelHook/NickelHook.mk
