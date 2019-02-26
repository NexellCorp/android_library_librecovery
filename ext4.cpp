#include    <stdio.h>
#include    <errno.h>
#include    <stdarg.h>
#include    <stdlib.h>
#include    <string.h>
#include    <sys/types.h>
#include    <sys/stat.h>
#include    <fcntl.h>
#include    <unistd.h>

typedef struct ext4_file_header {
    unsigned int magic;
    unsigned short major;
    unsigned short minor;
    unsigned short file_header_size;
    unsigned short chunk_header_size;
    unsigned int block_size;
    unsigned int total_blocks;
    unsigned int total_chunks;
    unsigned int crc32;
} ext4_file_header;

typedef struct ext4_chunk_header {
    unsigned short type;
    unsigned short reserved;
    unsigned int chunk_size;
    unsigned int total_size;
} ext4_chunk_header;

#define EXT4_FILE_HEADER_MAGIC  0xED26FF3A
#define EXT4_FILE_HEADER_MAJOR  0x0001
#define EXT4_FILE_HEADER_MINOR  0x0000
#define EXT4_FILE_BLOCK_SIZE    0x1000

#define EXT4_FILE_HEADER_SIZE   (sizeof(struct ext4_file_header))
#define EXT4_CHUNK_HEADER_SIZE  (sizeof(struct ext4_chunk_header))

#define EXT4_CHUNK_TYPE_RAW     0xCAC1
#define EXT4_CHUNK_TYPE_FILL    0xCAC2
#define EXT4_CHUNK_TYPE_NONE    0xCAC3
#define SECTOR_BITS             9   /* 512B */

int check_ext4_compress(char *source, unsigned long long part_size)
{
    ext4_file_header *file_header;

    file_header = (ext4_file_header*)source;

    if (file_header->magic != EXT4_FILE_HEADER_MAGIC) {
        return -1;
    }

    if (file_header->major != EXT4_FILE_HEADER_MAJOR) {
        fprintf(stderr, "Invalid Version Info! 0x%2x\n", file_header->major);
        return -1;
    }

    if (file_header->file_header_size != EXT4_FILE_HEADER_SIZE) {
        fprintf(stderr, "Invalid File Header Size! 0x%8x\n",
                                file_header->file_header_size);
        return -1;
    }

    if (file_header->chunk_header_size != EXT4_CHUNK_HEADER_SIZE) {
        fprintf(stderr, "Invalid Chunk Header Size! 0x%8x\n",
                                file_header->chunk_header_size);
        return -1;
    }

    if (file_header->block_size != EXT4_FILE_BLOCK_SIZE) {
        fprintf(stderr, "Invalid Block Size! 0x%8x\n", file_header->block_size);
        return -1;
    }

    if ((part_size/file_header->block_size)  < file_header->total_blocks) {
        fprintf(stderr, "Invalid Volume Size! Image is bigger than partition size!\n");
        fprintf(stderr, "partion size %lld , image size %lld \n",
            part_size, (unsigned long long )file_header->total_blocks * file_header->block_size);
        fprintf(stderr, "Hang...\n");
        while(1);
    }
    /* image is compressed ext4 */
    return 0;

}

int writeCompressedExt4Image(char *imgBase, size_t imgSize, int fd) {
    off_t sectorSize;
    int totalChunks;
    ext4_chunk_header *chunkHeader;
    ext4_file_header *fileHeader;
    char *base = imgBase;
    size_t writeCount;

    fileHeader = (ext4_file_header *)base;
    totalChunks = fileHeader->total_chunks;

    fprintf(stderr, "%s: total chunk = %d\n", __func__, totalChunks);

    base += EXT4_FILE_HEADER_SIZE;

    while (totalChunks) {
         chunkHeader = (ext4_chunk_header *)base;
         sectorSize = (chunkHeader->chunk_size * fileHeader->block_size);
         fprintf(stderr, "sectorSize: %d\n", sectorSize);

		 usleep(100);

         switch (chunkHeader->type) {
         case EXT4_CHUNK_TYPE_RAW:
             fprintf(stderr, "raw chunk\n");
             writeCount = write(fd, base + EXT4_CHUNK_HEADER_SIZE, sectorSize);
             if (writeCount != sectorSize) {
                  fprintf(stderr, "%s: write error, mismatching count %d/%d\n", __func__, sectorSize, writeCount);
                  return -EIO;
             }
             break;

         case EXT4_CHUNK_TYPE_FILL:
             fprintf(stderr, "fill chunk\n");
             lseek(fd, sectorSize, SEEK_CUR);
             break;

         case EXT4_CHUNK_TYPE_NONE:
             fprintf(stderr, "none chunk\n");
             lseek(fd, sectorSize, SEEK_CUR);
             break;

         default:
             fprintf(stderr, "unknown chunk\n");
             lseek(fd, sectorSize, SEEK_CUR);
             break;
         }
         totalChunks--;
         base += chunkHeader->total_size;
    }

    fprintf(stderr, "%s: write done\n", __func__);
    return 0;
}
