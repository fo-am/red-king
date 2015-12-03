######################################################################
# Automatically generated by qmake (2.01a) Sun Jul 5 17:49:45 2015
######################################################################

TEMPLATE = app
TARGET =
DEPENDPATH += . 2
INCLUDEPATH += . 2

QT += core gui

# Input
HEADERS += src/qt/MainWindow.h \
           src/qt/ui_redking.h \
           src/qt/canvas.h \
           src/qt/graph.h \
           src/qt/app.h \

SOURCES += src/qt/MainWindow.cpp \
           src/qt/canvas.cpp \
           src/qt/graph.cpp \
           src/qt/app.cpp \
           src/model/range_spedup_final.cpp \
		   src/model/model.cpp \
		   src/synth/additive_synth.cpp \
	 	   src/main.cpp

INCLUDEPATH += .
LIBS += -ljellyfish -lportaudio -lsndfile -llo -ldl -lpthread -lm
CONFIG+=debug

QMAKE_CXXFLAGS += -ggdb -Wall -Wno-unused -std=c++11 -DDONT_USE_FLUXA_GRAPH

# assets
RESOURCES     = redking.qrc
