
#include <daemon/db/db_util.hpp>
#include <rocksdb/table.h>
#include <rocksdb/filter_policy.h>
#include <daemon/db/merge.hpp>

using namespace std;

bool init_rocksdb() {
    rocksdb::DB* db;
    ADAFS_DATA->rdb_path(ADAFS_DATA->metadir() + "/rocksdb"s);
    rocksdb::Options options;
    // Optimize RocksDB. This is the easiest way to get RocksDB to perform well
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    // create the DB if it's not already present
    options.create_if_missing = true;
    options.merge_operator.reset(new MetadataMergeOperator);
    optimize_rocksdb(options);

    // Disable Write-Ahead Logging if configured
    rocksdb::WriteOptions write_options{};
#if !defined(KV_WOL)
    write_options.disableWAL = true;
#endif
    ADAFS_DATA->rdb_write_options(write_options);

    ADAFS_DATA->rdb_options(options);
//    rocksdb::OptimisticTransactionDB* txn_db;
//    rocksdb::OptimisticTransactionOptions txn_options{};
//    ADAFS_DATA->txn_rdb_options(txn_options);
    ADAFS_DATA->spdlogger()->debug("{}() RocksDB options set. About to connect...", __func__);
    // open DB
//    auto s = rocksdb::OptimisticTransactionDB::Open(ADAFS_DATA->rdb_options(), ADAFS_DATA->rdb_path(), &txn_db);
    auto s = rocksdb::DB::Open(ADAFS_DATA->rdb_options(), ADAFS_DATA->rdb_path(), &db);

    if (s.ok()) {
//        db = txn_db->GetBaseDB(); // db connection for db operations without transactions
        shared_ptr<rocksdb::DB> s_db(db);
        ADAFS_DATA->rdb(s_db);
//        shared_ptr<rocksdb::OptimisticTransactionDB> s_txn_db(txn_db);
//        ADAFS_DATA->txn_rdb(s_txn_db);
        ADAFS_DATA->spdlogger()->info("{}() RocksDB connection established.", __func__);
        return true;
    } else {
        ADAFS_DATA->spdlogger()->error("{}() Error opening RocksDB: {}", __func__, s.ToString());
        return false;
    }
}

void optimize_rocksdb(rocksdb::Options& options) {
#if defined(KV_OPTIMIZE_RAMDISK)
    // as described at https://github.com/facebook/rocksdb/wiki/RocksDB-Tuning-Guide
    // use mmap read
    options.allow_mmap_reads = true;
    // disable block cache, enable blook filters and reduce the delta encoding restart interval
    rocksdb::BlockBasedTableOptions table_options{};
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(10, true));
    table_options.no_block_cache = true;
    table_options.block_restart_interval = 4;
    options.table_factory.reset(NewBlockBasedTableFactory(table_options));
    // enable lightweight compression (snappy or lz4). We use lz4 for now
    options.compression = rocksdb::CompressionType::kLZ4Compression;
    // set up compression more aggressively and allocate more threads for flush and compaction
    options.level0_file_num_compaction_trigger = 1;
    options.max_background_flushes = 8;
    options.max_background_compactions = 8;
    options.max_subcompactions = 4;
    // keep all the files open
    options.max_open_files = -1;
#elif defined(KV_OPTIMIZE)
    //    rocksdb::BlockBasedTableOptions block_options{};
//    block_options.block_size = 16384 * 2;
//    options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(block_options));
    // experimental settings
//    options.write_buffer_size = 512;
//    options.max_write_buffer_number = 16;
//    options.min_write_buffer_number_to_merge = 4;
    // These 4 below have the most impact
    options.max_bytes_for_level_base = 2048;
    options.max_bytes_for_level_multiplier = 10;
    options.target_file_size_base = 256;
    options.target_file_size_multiplier = 1;
    //
    options.max_background_flushes = 1;
    options.max_background_compactions = 48;
    options.level0_file_num_compaction_trigger = 1;
    options.level0_slowdown_writes_trigger = 48;
    options.level0_stop_writes_trigger = 56;
//    options.arena_block_size = 1024 * 8;
//    options.compression = rocksdb::kNoCompression; // doesnt do anything
#endif
#if defined(KV_WRITE_BUFFER)
    // write_buffer_size is multiplied by the write_buffer_number to get the amount of data hold in memory.
    // at min_write_buffer_number_to_merge rocksdb starts to flush entries out to disk
    options.write_buffer_size = KV_WRITE_BUFFER << 20;
    // XXX experimental values. We only want one buffer, which is held in memory
    options.max_write_buffer_number = 1;
    options.min_write_buffer_number_to_merge = 1;
#endif
}
