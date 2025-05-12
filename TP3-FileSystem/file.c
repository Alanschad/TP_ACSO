#include "file.h"
#include "diskimg.h"
#include "inode.h"
#include "unixfilesystem.h"
#include <string.h>

int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    struct inode inode;
    if (inode_iget(fs, inumber, &inode) < 0) {
        return -1;  // error al obtener el inodo
    }

    int sector = inode_indexlookup(fs, &inode, blockNum);
    if (sector < 0) return -1;

    int filesize = (inode.i_size0 << 16) | inode.i_size1;
    int startByte = blockNum * DISKIMG_SECTOR_SIZE;

    if (startByte >= filesize) {
        return 0; // el bloque está más allá del fin del archivo
    }

    int bytesLeft = filesize - startByte;
    int bytesToRead = (bytesLeft < DISKIMG_SECTOR_SIZE) ? bytesLeft : DISKIMG_SECTOR_SIZE;

    int rc = diskimg_readsector(fs->dfd, sector, buf);
    if (rc == -1) return -1;

    return bytesToRead;
}



