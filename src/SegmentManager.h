#ifndef _KV_DB_SEGMENT_MANAGER_H_
#define _KV_DB_SEGMENT_MANAGER_H_

#include <stdio.h>
#include <vector>

#include "Db_Structure.h"
#include "BlockDevice.h"
#include "Utils.h"

using namespace std;

namespace kvdb{
    enum SegUseStat{un_use, in_use, full_use};

    class SegmentOnDisk{
    public:
        uint64_t time_stamp;
        uint32_t checksum;
        uint32_t number_keys;
    public:
        SegmentOnDisk(uint64_t tstamp): time_stamp(tstamp), checksum(0), number_keys(0){}
        SegmentOnDisk();
        ~SegmentOnDisk();
    }__attribute__((__packed__));


    class SegmentStat{
    public:
        SegUseStat state;
        int inuse_size;

    public:
        SegmentStat() : state(un_use), inuse_size(0){}
        ~SegmentStat();
    };

    class SegmentManager{
    public:
        bool InitSegmentForCreateDB(uint64_t device_capacity, uint64_t meta_size, uint64_t segment_size);
        bool LoadSegmentTableFromDevice(uint64_t meta_size, uint64_t segment_size, uint64_t num_seg, uint64_t current_seg);

        uint64_t GetNowSegId(){return m_current_seg;}
        uint64_t GetNumberOfSeg(){return m_num_seg;}
        uint64_t GetDataRegionSize(){return m_seg_size * m_num_seg;}
        

        bool GetNextInsertSegId(uint64_t& seg_id);
        bool ComputeSegOffsetFromId(uint64_t seg_id, uint64_t& offset);
        bool ComputeSegIdFromOffset(uint64_t offset, uint64_t& seg_id);

        void SetCurrentId(uint64_t seg_id);

        SegmentManager(BlockDevice* bdev);
        ~SegmentManager();

    private:
        vector<SegmentStat> m_seg_table;
        uint64_t m_begin_offset;
        uint64_t m_seg_size;
        uint64_t m_num_seg;
        uint64_t m_current_seg;
        

        BlockDevice* m_bdev;


    };
} //end namespace kvdb
#endif // #ifndef _KV_DB_SEGMENT_MANAGER_H_