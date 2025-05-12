#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "inode.h"
#include "diskimg.h"


/**
 * TODO
 */


#include "inode.h"
#include "diskimg.h"
#include "unixfilesystem.h"

#define INODES_PER_BLOCK (DISKIMG_SECTOR_SIZE / sizeof(struct inode))
#define NUM_INDIRECT_PTRS 256

int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    if (inumber < 1) return -1;

    int inode_block = INODE_START_SECTOR + (inumber - 1) / INODES_PER_BLOCK;

    struct inode inodes[INODES_PER_BLOCK];
    int rc = diskimg_readsector(fs->dfd, inode_block, inodes);
    if (rc == -1) return -1;

    *inp = inodes[(inumber - 1) % INODES_PER_BLOCK];
    return 0;
}



/**
 * TODO
 */




int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    if ((inp->i_mode & IALLOC) == 0) return -1; // inodo no asignado

    if ((inp->i_mode & ILARG) == 0) {
        // archivo chico: acceso directo
        if (blockNum < 0 || blockNum >= 8) return -1;
        return inp->i_addr[blockNum];
    }

    // archivo largo: acceso indirecto o doblemente indirecto
    if (blockNum < 7 * NUM_INDIRECT_PTRS) {
        int indirectBlockNum = blockNum / NUM_INDIRECT_PTRS;
        int offset = blockNum % NUM_INDIRECT_PTRS;

        uint16_t indirectBlock[NUM_INDIRECT_PTRS];

        int rc = diskimg_readsector(fs->dfd, inp->i_addr[indirectBlockNum], indirectBlock);
        if (rc == -1) return -1;

        return indirectBlock[offset];
    } else {
        // doble indirección
        blockNum -= 7 * NUM_INDIRECT_PTRS;

        int indirect1Index = blockNum / NUM_INDIRECT_PTRS;
        int indirect2Index = blockNum % NUM_INDIRECT_PTRS;

        uint16_t indirect1Block[NUM_INDIRECT_PTRS];
        int rc1 = diskimg_readsector(fs->dfd, inp->i_addr[7], indirect1Block);
        if (rc1 == -1) return -1;

        uint16_t indirect2Block[NUM_INDIRECT_PTRS];
        int rc2 = diskimg_readsector(fs->dfd, indirect1Block[indirect1Index], indirect2Block);
        if (rc2 == -1) return -1;

        return indirect2Block[indirect2Index];
    }
}


int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
