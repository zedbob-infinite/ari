#ifndef ari_io_h
#define ari_io_h


typedef struct AriFile_t
{
    const char *path;
    const char *filename;
    const char *rootname;
    const char *extname;
    const char *source;
} AriFile;

AriFile *get_file(const char *filepath);

#endif
