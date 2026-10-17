#ifndef LUNA_RESOURCE_MANAGER_FS_OPS_H
#define LUNA_RESOURCE_MANAGER_FS_OPS_H

int fs_copy(const char *src, const char *dst);
int fs_move(const char *src, const char *dst);
int fs_remove_recursive(const char *path);
int fs_make_directory(const char *path, unsigned int mode);

#endif
