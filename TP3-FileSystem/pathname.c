#include <string.h>
#include "pathname.h"
#include "unixfilesystem.h"
#include "inode.h"
#include "directory.h"
#include "direntv6.h"

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    if (pathname[0] != '/') return -1;  // debe ser un path absoluto

    int currentInumber = 1;  // raíz
    struct inode currentInode;

    if (inode_iget(fs, currentInumber, &currentInode) < 0) return -1;

    const char *p = pathname + 1;  // saltar el primer '/'

    char component[15];
    while (*p) {
        // extraer siguiente componente del path
        int i = 0;
        while (*p != '/' && *p != '\0' && i < 14) {
            component[i++] = *p++;
        }
        component[i] = '\0';
        while (*p == '/') p++;  // saltear barras duplicadas

        struct direntv6 dirEnt;
        if (directory_findname(fs, component, currentInumber, &dirEnt) < 0) {
            return -1;  // no encontrado
        }

        currentInumber = dirEnt.d_inumber;

        if (inode_iget(fs, currentInumber, &currentInode) < 0) {
            return -1;  // fallo leyendo inodo
        }
    }

    return currentInumber;  // devolver el número de inodo final
}
