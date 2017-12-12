QT -= gui
QT += core

QMAKE_CXXFLAGS += -w -O2 -march=native -fomit-frame-pointer

LIBS += -lglut -lGLU

SOURCES += \
    genomeTesterMain.cpp \
    robotAgent.cpp \
    gameworld.cpp \
    phenotype.cpp \
    genotype.cpp \
    CInnovation.cpp

HEADERS += \
    robotAgent.h \
    gameworld.h \
    mathMatrix.h \
    mathVector.h \
    timer.h \
    SVector2D.h \
    phenotype.h \
    genotype.h \
    genes.h \
    parameters.h \
    lineIntersect2D.h \
    utils.h \
    CInnovation.h
