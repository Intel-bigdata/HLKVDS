#include "SegmentManager.h"

namespace kvdb{

    SegmentOnDisk::SegmentOnDisk():
        checksum(0), number_keys(0)
    {
        time_stamp = Timing::GetNow();
    }

    SegmentOnDisk::~SegmentOnDisk()
    {
        return;
    }

    SegmentStat::~SegmentStat()
    {
        return;
    }

    bool SegmentManager::InitSegmentForCreateDB(uint64_t device_capacity, uint64_t meta_size, uint64_t segment_size)
    {
        if ((segment_size != ((segment_size>>12)<<12)) || (device_capacity < meta_size))
        {
            return false;
        }

        m_begin_offset = meta_size;
        m_seg_size = segment_size;
        m_num_seg = (device_capacity - meta_size) / segment_size;
        m_current_seg = 0;

        
        //init segment table
        SegmentStat seg_stat;
        for (uint64_t seg_index = 0; seg_index < m_num_seg; seg_index++)
        {
           m_seg_table.push_back(seg_stat);
        }

        //write all segments to device
        SegmentOnDisk seg_ondisk;
        ssize_t offset = m_begin_offset;
        for (uint64_t seg_index = 0; seg_index < m_num_seg; seg_index++)
        {
            //write seg to device
            if ( m_bdev->pWrite(&seg_ondisk, sizeof(SegmentOnDisk), offset) != sizeof(SegmentOnDisk))
            {
                perror("can not write segment to device!");
                return false;
            }
            offset += m_seg_size;
        }
        
        //SegmentOnDisk seg_ondisk(now_time);
        return true;
    }

    bool SegmentManager::LoadSegmentTableFromDevice(uint64_t meta_size, uint64_t segment_size, uint64_t num_seg, uint64_t current_seg)
    {
        m_begin_offset = meta_size;
        m_seg_size = segment_size;
        m_num_seg = num_seg;
        m_current_seg = current_seg;

        ssize_t offset = m_begin_offset;
        for (uint64_t seg_index = 0; seg_index < m_num_seg; seg_index++)
        {
            SegmentOnDisk seg_ondisk;
            //write seg to device
            if ( m_bdev->pRead(&seg_ondisk, sizeof(SegmentOnDisk), offset) != sizeof(SegmentOnDisk))
            {
                perror("can not write segment to device!");
                return false;
            }
            offset += m_seg_size;
            SegmentStat seg_stat;
            if (seg_ondisk.number_keys != 0)
            {
                seg_stat.state = in_use;
            }
            else
            {
                seg_stat.state = un_use;
            }
            m_seg_table.push_back(seg_stat);
        }
        
        return true;
    }

    bool SegmentManager::GetNextInsertSegId(uint64_t& seg_id)
    {
        uint64_t seg_index = m_current_seg + 1; 
        
        while(seg_index != m_current_seg)
        {
            if (m_seg_table[seg_index].state == un_use)
            {
                seg_id = seg_index;
                return true;
            }
            seg_index++;
            if ( seg_index == m_num_seg)
            {
                seg_index = 0;
            }
        }
        return false;
    }

    bool SegmentManager::ComputeSegOffsetFromId(uint64_t seg_id, uint64_t& offset)
    {
        if(seg_id >= m_num_seg)
        {
            return false;
        }
        offset = m_begin_offset + (seg_id - 1) * m_seg_size;   
        return true;
    }

    bool SegmentManager::ComputeSegIdFromOffset(uint64_t offset, uint64_t& seg_id)
    {
        if(offset < m_begin_offset || 
           offset > m_begin_offset + m_seg_size * (m_num_seg - 1))
        {
            return false;
        }
        seg_id = (offset - m_begin_offset )/ m_seg_size;
        return true;
    }

    void SegmentManager::SetCurrentId(uint64_t seg_id)
    {
        m_current_seg = seg_id;
    }

    SegmentManager::SegmentManager(BlockDevice* bdev) : 
        m_begin_offset(0), m_seg_size(0), m_num_seg(0), m_current_seg(0), m_bdev(bdev)
    {
        return;
    }

    SegmentManager::~SegmentManager()
    {
        m_seg_table.clear();
    }
} //end namespace kvdb