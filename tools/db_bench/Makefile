current_dir=$(shell pwd)
leveldb_dir=$(current_dir)/src/db/leveldb
kv_db_dir=$(current_dir)/src/db/kv_db
kv_objs_dir=$(current_dir)/../../src
benchmark_dir=$(current_dir)/src/benchmark


bin=db_bench

all:
	@echo "Please specify a target \`leveldb_test\` or \`kv_db_test\`"

leveldb_test:
	@echo "tests for leveldb"
	$(MAKE) -C $(benchmark_dir) KV_PATH=$(leveldb_dir)
	cp $(benchmark_dir)/$(bin) ./db_bench_leveldb

kv_db_test:
	@echo "tests for kv-db"
	$(MAKE) -C $(kv_objs_dir) clean
	$(MAKE) -C $(kv_objs_dir)
	$(MAKE) -C $(benchmark_dir) KV_PATH=$(kv_db_dir) KV_OBJS_DIR=$(kv_objs_dir)
	cp $(benchmark_dir)/$(bin) ./db_bench_kvdb

clean:
	$(MAKE) -C $(benchmark_dir) clean 
	rm -f ./db_bench_kvdb ./db_bench_leveldb
