#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define BLOCK_SIZE (2048)

static char *dev_filename = NULL;
static int g_devfd = -1;

static int dev_close_and_reopen() {
    if (g_devfd > 0) {
        close(g_devfd);
    }
    g_devfd = open(dev_filename, O_RDONLY);
    if (!g_devfd) {
        perror(dev_filename);
        return -1;
    }
    return 0;
}

static int read_block(int fd, void *buf, const off_t off_blocks, const int retry) {
    for (int i = 0; i < retry; ++i) {
        if (pread(fd, buf, BLOCK_SIZE, (off_blocks * BLOCK_SIZE)) == BLOCK_SIZE) {
            return 0;
        } else {
            perror("pread");
            printf("Try %d: Read error on block %ld\n", i, off_blocks);
            close(fd);
            fd = -1;
            fd = open(dev_filename, O_RDONLY);
            if (!fd) {
                perror("reopen");
                return -1;
            }
        }
    }
    return -1;
}

static int read_and_compare_block(const void *iso_mem, const off_t off_blocks, const int isofd) {
    const int retry = 3;
    char devbuf[BLOCK_SIZE] = "";
    const void *target = (char*)iso_mem + (off_blocks * BLOCK_SIZE);

    for (int i = 0; i < retry; ++i) {
        if (read_block(g_devfd, &devbuf, off_blocks, retry) != 0) {
            printf("Read failed, fd: %d\n", g_devfd);
            return -1;
        }
        if (memcmp(&devbuf, target, BLOCK_SIZE) == 0) {
            printf("Read and Compare OK on block %ld\r", off_blocks);
            write(isofd, &devbuf, BLOCK_SIZE);
            return 0;
        } else {
            printf("\nTry %d: Read and Compare ERROR on blocks %ld\n", i, off_blocks);
            if (!posix_fadvise(g_devfd, (off_blocks * BLOCK_SIZE), BLOCK_SIZE, POSIX_FADV_DONTNEED)) {
                perror("fadvise");
                printf("Change to dev close and reopen\n");
                printf("Old fd: %d, ", g_devfd);
                dev_close_and_reopen();
                printf("reopen fd: %d\n", g_devfd);
            }
        }
    }
    return -1;
}

void read_and_compare(const char *devname, const char *isoname, const char *output) {
    const off_t size = 7851737088;
    const off_t total_blocks = size / BLOCK_SIZE;

    int devfd = open(devname, O_RDONLY);
    if (!devfd) {
        perror(devname);
        return;
    }
    g_devfd = devfd;

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

    int outfd = open(output, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (!outfd) {
        perror(output);
        close(devfd);
        close(isofd);
        return;
    }

    for (off_t curr_blocks = 0; curr_blocks < total_blocks; ++curr_blocks) {
        if (read_and_compare_block(iso_mem, curr_blocks, outfd) == -1) {
            break;
        }
    }

    printf("\nOK\n");
    munmap(iso_mem, size);
    close(devfd);
    close(isofd);
    close(outfd);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s input1 input2 output\n", argv[0]);
        return 2;
    }

    printf("input1: %s\n", argv[1]);
    printf("input2: %s\n", argv[2]);
    printf("output: %s\n", argv[3]);
    dev_filename = argv[1];
    read_and_compare(argv[1], argv[2], argv[3]);
    return 0;
}
