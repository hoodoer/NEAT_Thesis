QT -= gui
QT += core network

# If you want to compile in support for LZMA based
# NCD functions, you need to add -DLZMA to the QMAKE_CXXFLAGS line,
# and add -llzma to the LIBS line.

QMAKE_CXXFLAGS += -w -O2 -march=native -fomit-frame-pointer

INCLUDEPATH += ./ncd ./ncd/complearn
#LIBS += -llzma

# Apparently some machines need -lz to link against zlib correctly
# I haven't run into it too often.
LIBS += -lz

HEADERS += \
    utils.h \
    timer.h \
    SVector2D.h \
    robotAgent.h \
    phenotype.h \
    parameters.h \
    networkserver.h \
    mathVector.h \
    mathMatrix.h \
    lineIntersect2D.h \
    genotype.h \
    genes.h \
    gameworld.h \
    CSpecies.h \
    CInnovation.h \
    Cga.h \
    networkserverthread.h \
    competitiveCoevolutionNeat.h \
    ncdBehaviorFuncs.h \
    ncd/osutil.h \
    ncd/compression-bridge.h \
    ncd/cl-util.h \
    ncd/cl-internal.h \
    ncd/complearn/cl-options.h \
    ncd/complearn/cl-core.h \
    ncd/complearn/cl-command.h \
    zlibNcd.h \
    sharedMemory.h

SOURCES += \
    robotAgent.cpp \
    phenotype.cpp \
    networkserver.cpp \
    genotype.cpp \
    gameworld.cpp \
    CSpecies.cpp \
    competitiveCoevolutionNeatMain.cpp \
    CInnovation.cpp \
    Cga.cpp \
    networkserverthread.cpp \
    ncdBehaviorFuncs.cpp \
    ncd/osutil.cpp \
    ncd/compression-bridge.cpp \
    ncd/cl-util.cpp \
    ncd/cl-options.cpp \
    ncd/cl-core.cpp \
    ncd/cl-command.cpp \
    sharedMemory.cpp

OTHER_FILES += \
    README.txt
