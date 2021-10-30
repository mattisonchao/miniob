//
// Created by windwheel on 2021/10/24.
//

//这里因为参考了cmu的讲义 所以实现的不是传统的lru，而是nru
//根据时钟进行淘汰的算法
#include <cstdlib>
#include "clock_replacer.h"

bool ClockReplacer::Victim(Frame *frame) {
    latch_.lock();
    if (size<=0){
        latch_.unlock();
        return false;
    }

    while (true){
        if (!in_replacer[clockhand]){
            clockhand = (clockhand+1) % buffer_pool_size;
            continue;
        }

        if (!ref_bits_[clockhand]){
            clockhand = (clockhand+1) % buffer_pool_size;
            continue;
        }

        frame->file_desc = clockhand;
        in_replacer[clockhand] = false;
        ref_bits_[clockhand] = false;
        size-=1;
        break;
    }
    latch_.unlock();
    return true;
}
//对空闲列表中 正在准备替换的page进行状态标记
void ClockReplacer::Pin(Frame *frame) {
    latch_.lock();
    if (in_replacer[frame->file_desc]){
        in_replacer[frame->file_desc] = false;
        ref_bits_[frame->file_desc] = false;
        size -=1;
    }
    latch_.unlock();
}

void ClockReplacer::Unpin(Frame *frame) {
    latch_.lock();
    if (!in_replacer[frame->file_desc]){
        in_replacer[frame->file_desc] = true;
        ref_bits_[frame->file_desc] = true;
    }
}

ClockReplacer::ClockReplacer(ClockReplacer *pReplacer) {

}

