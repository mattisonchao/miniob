//
// Created by windwheel on 2021/10/24.
//

#ifndef MINIDB_CLOCK_REPLACER_H
#define MINIDB_CLOCK_REPLACER_H


#include <vector>
#include <mutex>
#include "common.h"

class ClockReplacer {

public:
    ClockReplacer(PageNum page_nums){
        using std::vector;
        ref_bits_ = vector<bool>(page_nums, false);
        in_replacer = vector<bool>(page_nums, false);
        buffer_pool_size = page_nums;
        size = 0;
        clockhand = 0;

    }

    ClockReplacer(ClockReplacer *pReplacer);

    ~ClockReplacer() = default;

public:
    std::vector<bool > ref_bits_; //待被替换的page进行标记状态
    std::vector<bool> in_replacer;//正在被替换的page
    size_t buffer_pool_size; //缓冲池管理器的大小
    std::mutex latch_;//维持操作系统空闲列表换页的互斥锁
    int size; //维护当前动态遍历的空闲列表大小
    int clockhand; //记录当前遍历到的Page 利用同余摸循环遍历空闲列表
    Frame *frame;//待分配的新页


public:
    bool Victim(Frame *frame); //双指针遍历待替换的freelist列表
    void Pin(Frame *frame); //Pin住的页 正在准备被替换
    void Unpin(Frame *frame); //已经被淘汰掉的Page
};




#endif //MINIDB_CLOCK_REPLACER_H
