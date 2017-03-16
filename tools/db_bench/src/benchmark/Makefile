UTIL_SRC=env.cc \
	env_posix.cc \
	histogram.cc \
	logging.cc \
	port_posix.cc \
	status.cc \
	testutil.cc
UTIL_OBJ=$(patsubst %.cc, %.o, $(UTIL_SRC))
BENCH_SRC=db_bench.hpp
BENCH_LIBS=-lpthread

KV_COMMON_INC=-I../db

ifdef KV_PATH
	include $(KV_PATH)/Makefile
endif

CXXFLAGS=-O2 -DNDEBUG
CXX=g++

all:db_bench

kv_example:
	@echo $(KV_SRC)
	cp $(KV_SRC) .

$(UTIL_OBJ):%.o:%.cc
	@echo $(UTIL_OBJ)
	$(CXX) $(CXXFLAGS) -c $< -o $@

db_bench:$(UTIL_OBJ) kv_example $(BENCH_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(UTIL_OBJ) $(KV_OBJS) $(notdir $(KV_SRC)) $(BENCH_SRC) $(KV_DB_INC) $(KV_DB_LIB_PATH) $(KV_DB_LIB) $(BENCH_LIBS) $(KV_COMMON_INC)
	
	 

.PHONY:clean
clean:
	rm -rf $(UTIL_OBJ)
	rm -f $(notdir $(KV_SRC))
	rm -f db_bench
