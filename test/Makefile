CC = gcc
CXX = g++
C_FLAGS = -g -Wall -fPIC
CXX_FLAGS = -g -Wall --std=c++11 -fPIC
INCLUDES = -I ./
LIBS = -pthread

PROGNAME = SegmentTest IndexManagerTest SuperBlockManagerTest

all: $(PROGNAME)

%.o : %.cc
	${CXX} -c ${CXX_FLAGS} ${INCLUDES} $< -o $@

%.o : %.c
	${CC} -c ${C_FLAGS} ${INCLUDES} $< -o $@

OBJECTS =  ../Kvdb_Impl.o ../IndexManager.o ../SuperBlockManager.o ../DataHandle.o ../BlockDevice.o ../KernelDevice.o ../SegmentManager.o ../GcManager.o ../Utils.o ../Kvdb.o ../KeyDigestHandle.o ../rmd160.o 

SegmentTest:$(OBJECTS)
	$(CXX) $(CXX_FLAGS) $(OBJECTS) SegmentTest.cc -o SegmentTest $(INCLUDES) $(LIBS) -I /usr/local/include/gtest /usr/local/lib/libgtest.a
IndexManagerTest:$(OBJECTS)
	$(CXX) $(CXX_FLAGS) $(OBJECTS) IndexManagerTest.cc -o IndexManagerTest $(INCLUDES) $(LIBS) -I /usr/local/include/gtest /usr/local/lib/libgtest.a
SuperBlockManagerTest:$(OBJECTS)
	$(CXX) $(CXX_FLAGS) $(OBJECTS) SuperBlockManagerTest.cc -o SuperBlockManagerTest $(INCLUDES) $(LIBS) -I /usr/local/include/gtest /usr/local/lib/libgtest.a
.PHONY:clean
clean:
	rm -fr $(PROGNAME)

