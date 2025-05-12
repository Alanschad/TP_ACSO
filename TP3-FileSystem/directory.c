#include "directory.h"
#include "file.h"
#include "inode.h"
#include "unixfilesystem.h"
#include "direntv6.h"
#include <string.h>
#include "diskimg.h"


#define DIRS_PER_BLOCK (DISKIMG_SECTOR_SIZE / sizeof(struct direntv6))

int directory_findname(struct unixfilesystem *fs, const char *name, int dirinumber, struct direntv6 *dirEnt) {
    struct inode dip;
    if (inode_iget(fs, dirinumber, &dip) < 0) return -1;

    if ((dip.i_mode & IFMT) != IFDIR) return -1;

    int numBlocks = ((dip.i_size0 << 16) | dip.i_size1) / DISKIMG_SECTOR_SIZE + 1;
    char blockBuf[DISKIMG_SECTOR_SIZE];

    for (int b = 0; b < numBlocks; b++) {
        int nbytes = file_getblock(fs, dirinumber, b, blockBuf);
        if (nbytes == -1) return -1;

        struct direntv6 *entries = (struct direntv6 *)blockBuf;
        int nentries = nbytes / sizeof(struct direntv6);

        for (int i = 0; i < nentries; i++) {
            if (strncmp(entries[i].d_name, name, 14) == 0) {
                *dirEnt = entries[i];
                return 0;
            }
        }
    }

    return -1; // no se encontró
}
