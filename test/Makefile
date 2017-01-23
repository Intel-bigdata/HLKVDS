CC = gcc
CXX = g++
C_FLAGS = -g -Wall -fPIC
CXX_FLAGS = -g -Wall --std=c++11 -fPIC
INCLUDES = -I ./
LIBS = -pthread

PROGNAME = test_operations test_index_manager test_block_manager test_segment_manager

all: $(PROGNAME)

%.o : %.cc
	${CXX} -c ${CXX_FLAGS} ${INCLUDES} $< -o $@

%.o : %.c
	${CC} -c ${C_FLAGS} ${INCLUDES} $< -o $@

OBJECTS =  ../src/Kvdb_Impl.o ../src/IndexManager.o ../src/SuperBlockManager.o ../src/DataHandle.o ../src/BlockDevice.o ../src/KernelDevice.o ../src/SegmentManager.o ../src/GcManager.o ../src/Utils.o ../src/Kvdb.o ../src/KeyDigestHandle.o ../src/rmd160.o ../src/Options.o


test_operations:$(OBJECTS)
	$(CXX) $(CXX_FLAGS) $(OBJECTS) test_operations.cc -o test_operations $(INCLUDES) $(LIBS) -I /usr/local/include/gtest /usr/local/lib/libgtest.a
test_index_manager:$(OBJECTS)
	$(CXX) $(CXX_FLAGS) $(OBJECTS) test_index_manager.cc -o test_index_manager $(INCLUDES) $(LIBS) -I /usr/local/include/gtest /usr/local/lib/libgtest.a
test_block_manager:$(OBJECTS)
	$(CXX) $(CXX_FLAGS) $(OBJECTS) test_block_manager.cc -o test_block_manager $(INCLUDES) $(LIBS) -I /usr/local/include/gtest /usr/local/lib/libgtest.a
test_segment_manager:$(OBJECTS)
	$(CXX) $(CXX_FLAGS) $(OBJECTS) test_segment_manager.cc -o test_segment_manager $(INCLUDES) $(LIBS) -I /usr/local/include/gtest /usr/local/lib/libgtest.a
.PHONY:clean
clean:
	rm -fr $(PROGNAME)

