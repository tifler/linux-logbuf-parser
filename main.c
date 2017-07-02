#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "parser.h"
#include "XDebug.h"

typedef unsigned long long u64;
typedef unsigned short u16;
typedef unsigned char u8;

enum log_flags {
    LOG_NOCONS      = 1,    /* already flushed, do not print to console */
    LOG_NEWLINE     = 2,    /* text ended with a newline */
    LOG_PREFIX      = 4,    /* text started with a prefix */
    LOG_CONT        = 8,    /* text is a fragment of a continuation line */
};

struct log {
    u64 ts_nsec;            /* timestamp in nanoseconds */
    u16 len;                /* length of entire record */
    u16 text_len;           /* length of text buffer */
    u16 dict_len;           /* length of dictionary buffer */
    u8 facility;            /* syslog facility */
    u8 flags:5;             /* internal record flags */
    u8 level:3;             /* syslog level */
};

/*****************************************************************************/

static int parse(int fd, unsigned long offset)
{
    int count = 0;
    struct stat st;
    struct log *log;

    ret = fstat(fd, &st);
    ASSERT(ret == 0);
    ASSERT(st.st_size > offset);

    base = (u8 *)mmap(NULL,
            st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    ASSERT(!MAP_FAILED(map));

    do {
        log = next_log(parser);
    };

    return count;
}

/*****************************************************************************/

int main(int argc, char **argv)
{
    int fd;
    unsigned long offset;

    offset = strtoul(argv[2], NULL, 0);

    fd = open(argv[1], O_RDONLY);
    ASSERT(fd > 0);

    parse(fd, offset);

    return 0;
}
