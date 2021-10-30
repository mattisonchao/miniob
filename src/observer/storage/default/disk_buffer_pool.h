/* Copyright (c) 2021 Xie Meiyi(xiemeiyi@hust.edu.cn) and OceanBase and/or its affiliates. All rights reserved.
miniob is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

//
// Created by Longda on 2021/4/13.
//
#ifndef __OBSERVER_STORAGE_COMMON_PAGE_MANAGER_H_
#define __OBSERVER_STORAGE_COMMON_PAGE_MANAGER_H_


#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>

#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <vector>
#include <list>
#include <mutex>
#include <unordered_map>


#include "rc.h"
//#include "clock_replacer.h"
#include "common.h"
#include "clock_replacer.h"

typedef int PageNum;


// sizeof(Page) should be equal to BP_PAGE_SIZE

typedef struct {
  bool open;
  Frame *frame;
} BPPageHandle;


class BPManager {
public:
    BPManager(int size = BP_BUFFER_SIZE) {
        this->size = size;
        frame = new Frame[size];
        allocated = new bool[size];
        for (int i = 0; i < size; i++) {
            allocated[i] = false;
            frame[i].pin_count = 0;
        }

        for(size_t i=0;i<size;i++){
            free_list->push_back(reinterpret_cast<Frame *>(static_cast<int>(i)));
        }
        bpPageHandle = new BPPageHandle ;
        clockReplacer = new ClockReplacer(size);
        diskBufferPool = new DiskBufferPool();
    }

    ~BPManager() {
        delete[] frame;
        delete[] allocated;
        delete clockReplacer;
        size = 0;
        frame = nullptr;
        allocated = nullptr;
        diskBufferPool = nullptr;
    }

    Frame *alloc() {
        std::lock_guard<std::mutex> lock(latch_);
        Frame* res = nullptr;
        res->file_desc = -1;
        latch_.lock();
        if (!free_list->empty()){
            res = free_list->front();
            free_list->pop_front();
        }else{
            //TODO 判断是否是需要被替换的page
            if (!clockReplacer->Victim(res)){
                latch_.unlock();
            }
            Frame* replacedPage = &frame[res->file_desc];
            if (replacedPage->dirty){
                //TODO 进行刷盘操作
                replacedPage->dirty = false;
            }

            bpPageHandle->frame = res;
            bpPageHandle->open = true;

            page_table.erase(replacedPage->file_desc);
            //TODO 从磁盘管理器分配一片内存
            diskBufferPool->allocate_page(res->file_desc,bpPageHandle);
            frame[res->file_desc] =

            bpPageHandle->open = false;

        }
        return nullptr; // TODO for test
    }

    Frame *get(int file_desc, PageNum page_num) {
        return nullptr; // TODO for test
    }

    Frame *getFrame() { return frame; }

    bool *getAllocated() { return allocated; }

public:
    int size;
    Frame * frame = nullptr;
    bool *allocated = nullptr;
    std::list<Frame *>*free_list; //最近需要替换的Page
    std::mutex latch_;//维持操作系统空闲列表换页的互斥锁
    ClockReplacer *clockReplacer; //访问操作系统page的工具类
    std::unordered_map<page_id ,frame_id_t> page_table;//记录正在被替换的页表项
    BPPageHandle *bpPageHandle;
};

class DiskBufferPool {
public:
    /**
    * 创建一个名称为指定文件名的分页文件
    */
    RC create_file(const char *file_name);

    /**
     * 根据文件名打开一个分页文件，返回文件ID
     * @return
     */
    RC open_file(const char *file_name, int *file_id);

    /**
     * 关闭fileID对应的分页文件
     */
    RC close_file(int file_id);

    /**
     * 根据文件ID和页号获取指定页面到缓冲区，返回页面句柄指针。
     * @return
     */
    RC get_this_page(int file_id, PageNum page_num, BPPageHandle *page_handle);

    /**
     * 在指定文件中分配一个新的页面，并将其放入缓冲区，返回页面句柄指针。
     * 分配页面时，如果文件中有空闲页，就直接分配一个空闲页；
     * 如果文件中没有空闲页，则扩展文件规模来增加新的空闲页。
     */
    RC allocate_page(int file_id, BPPageHandle *page_handle);

    /**
     * 根据页面句柄指针返回对应的页面号
     */
    RC get_page_num(BPPageHandle *page_handle, PageNum *page_num);

    /**
     * 根据页面句柄指针返回对应的数据区指针
     */
    RC get_data(BPPageHandle *page_handle, char **data);

    /**
     * 丢弃文件中编号为pageNum的页面，将其变为空闲页
     */
    RC dispose_page(int file_id, PageNum page_num);

    /**
     * 释放指定文件关联的页的内存， 如果已经脏， 则刷到磁盘，除了pinned page
     * @param file_handle
     * @param page_num 如果不指定page_num 将刷新所有页
     */
    RC force_page(int file_id, PageNum page_num);

    /**
     * 标记指定页面为“脏”页。如果修改了页面的内容，则应调用此函数，
     * 以便该页面被淘汰出缓冲区时系统将新的页面数据写入磁盘文件
     */
    RC mark_dirty(BPPageHandle *page_handle);

    /**
     * 此函数用于解除pageHandle对应页面的驻留缓冲区限制。
     * 在调用GetThisPage或AllocatePage函数将一个页面读入缓冲区后，
     * 该页面被设置为驻留缓冲区状态，以防止其在处理过程中被置换出去，
     * 因此在该页面使用完之后应调用此函数解除该限制，使得该页面此后可以正常地被淘汰出缓冲区
     */
    RC unpin_page(BPPageHandle *page_handle);

    /**
     * 获取文件的总页数
     */
    RC get_page_count(int file_id, int *page_count);

    RC flush_all_pages(int file_id);

protected:
    RC allocate_block(Frame **buf);
    RC dispose_block(Frame *buf);

    /**
     * 刷新指定文件关联的所有脏页到磁盘，除了pinned page
     * @param file_handle
     * @param page_num 如果不指定page_num 将刷新所有页
     */
    RC force_page(BPFileHandle *file_handle, PageNum page_num);
    RC force_all_pages(BPFileHandle *file_handle);
    RC check_file_id(int file_id);
    RC check_page_num(PageNum page_num, BPFileHandle *file_handle);
    RC load_page(PageNum page_num, BPFileHandle *file_handle, Frame *frame);
    RC flush_block(Frame *frame);

private:
    BPManager bp_manager_;
    BPFileHandle *open_list_[MAX_OPEN_FILE] = {nullptr};
};

DiskBufferPool *theGlobalDiskBufferPool();


#endif //__OBSERVER_STORAGE_COMMON_PAGE_MANAGER_H_
