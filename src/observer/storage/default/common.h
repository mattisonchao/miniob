//
// Created by windwheel on 2021/10/24.
//

#ifndef MINIDB_COMMON_H
#define MINIDB_COMMON_H
#define BP_INVALID_PAGE_NUM (-1)
#define BP_PAGE_SIZE (1 << 12)
#define BP_FILE_SUB_HDR_SIZE (sizeof(BPFileSubHeader))
#define BP_BUFFER_SIZE 50
#define MAX_OPEN_FILE 1024
#define BP_PAGE_DATA_SIZE (BP_PAGE_SIZE - sizeof(PageNum))


#endif //MINIDB_COMMON_H

typedef int PageNum;



typedef struct{
    PageNum page_num;
    char data[BP_PAGE_DATA_SIZE];
} Page;

typedef struct{
    bool dirty;
    unsigned int pin_count;
    unsigned long acc_time;
    int file_desc;
    Page page;
} Frame;


typedef struct {
    PageNum page_count;
    int allocated_pages;
} BPFileSubHeader;

class BPFileHandle{
public:
    BPFileHandle() {
        memset(this, 0, sizeof(*this));
    }

public:
    bool bopen;
    const char *file_name;
    int file_desc;
    Frame *hdr_frame;
    Page *hdr_page;
    char *bitmap;
    BPFileSubHeader *file_sub_header;
} ;

//因为在操作系统中页和页表项都会有落盘的页表ID长度，所以需要做数据类型的适配

using frame_id_t = int32_t;
using page_id = int32_t;
using old_t = uint16_t;



