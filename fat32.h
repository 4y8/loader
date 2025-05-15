#ifndef _FAT32_H_
#define _FAT32_H_

#include "bootloader.h"
#include "../libc/u.h"

typedef struct {
  uint32 cluster;
  uint32 length;
  uint32 opened;
  uint32 position;
} fat32_file;

struct vm;

void fat32_newfs(uint8 part,uint32 offset);


int fat32_open_dir(char *fname, struct vm *vm);
void fat32_close_dir(int fd, struct vm *vm);
int fat32_next_entry_dir(int fd, i32 *name, struct vm *vm);

#endif
