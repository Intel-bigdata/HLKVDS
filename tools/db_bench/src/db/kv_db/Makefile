
#KV_PATH varible is defined before                                                 
#KV_OBJS_DIR 

OBJS=HashFun.o Kvdb_Impl.o IndexManager.o SuperBlockManager.o DataHandle.o BlockDevice.o KernelDevice.o Utils.o Kvdb.o
KV_OBJS=$(addprefix $(KV_OBJS_DIR)/, $(OBJS))
KV_SRC=$(wildcard $(KV_PATH)/*.cc)                                                 
KV_DB_LIB_PATH=                    #-L$(KV_PATH)/lib                                                    
KV_DB_LIB=                         #-lleveldb -lpthread -lsnappy                                             
KV_DB_INC=-I$(KV_OBJS_DIR)         #-I$(KV_PATH)/include
