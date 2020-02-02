#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define BLOCK_SIZE (2048)

static int read_block(int fd, void *buf, const off_t off_blocks, const int retry) {
    for (int i = 0; i < retry; ++i) {
        if (pread(fd, buf, BLOCK_SIZE, (off_blocks * BLOCK_SIZE)) == BLOCK_SIZE) {
            return 0;
        } else {
            perror("pread");
            printf("Try %d: Read error on block %ld\n", i, off_blocks);
            sleep(1);
        }
    }
    return -1;
}

static void read_and_compare_block(const int devfd, const void *iso_mem, const off_t off_blocks) {
    const int retry = 3;
    char devbuf[BLOCK_SIZE] = "";
    const void *target = (char*)iso_mem + (off_blocks * BLOCK_SIZE);

    for (int i = 0; i < retry; ++i) {
        if (read_block(devfd, &devbuf, off_blocks, retry) != 0) {
            printf("Read failed!\n");
            return;
        }
        if (memcmp(&devbuf, target, BLOCK_SIZE) == 0) {
            printf("Read and Compare OK on block %ld\r", off_blocks);
            return;
        } else {
            printf("\nTry %d: Read and Compare ERROR on blocks %ld\n", i, off_blocks);
        }
    }
}

void read_and_compare(const char *devname, const char *isoname) {
    const off_t size = 7851737088;
    const off_t total_blocks = size / BLOCK_SIZE;

    int devfd = open(devname, O_RDONLY);
    if (!devfd) {
        perror(devname);
        return;
    }

    int isofd = open(isoname, O_RDONLY);
    if (!isofd) {
        perror(isoname);
        close(devfd);
        return;
    }

    void *iso_mem = mmap(NULL, size, PROT_READ, MAP_SHARED, isofd, 0);
    if (iso_mem == MAP_FAILED) {
        perror("iso mmap");
        close(devfd);
        close(isofd);
        return;
    }

    /*
    int outfd = open(output, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (!outfd) {
        perror(output);
        close(devfd);
        close(isofd);
        return;
    }
    */

    for (off_t curr_blocks = 0; curr_blocks < total_blocks; ++curr_blocks) {
        read_and_compare_block(devfd, iso_mem, curr_blocks);
    }

    printf("\nOK\n");
    munmap(iso_mem, size);
    close(devfd);
    close(isofd);
    //    close(outfd);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input1 input2\n", argv[0]);
        return 2;
    }

    struct stat st;
    lstat(argv[1], &st);
    printf("dev st_size: %ld\n", st.st_size);

    read_and_compare(argv[1], argv[2]);
    return 0;
}
