#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
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

struct KLogParser {
    int fd;
    void *base;
    unsigned size;
    unsigned start;
    unsigned curr;
};

/*****************************************************************************/

static struct log *toLog(struct KLogParser *parser)
{
    return (struct log *)&((u8 *)parser->base)[parser->curr];
}

static char *log_text(const struct log *msg)
{
    return (char *)msg + sizeof(struct log);
}

static int parse(int fd, unsigned int offset)
{
    int ret;
    int count = 0;
    struct stat st;
    struct log *log;
    struct KLogParser parser;
    static char buf[512];

    ret = fstat(fd, &st);
    ASSERT(ret == 0);
    ASSERT(st.st_size > offset);

    parser.fd = fd;
    parser.base = (u8 *)mmap(NULL,
            st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    ASSERT(MAP_FAILED != parser.base);
    parser.size = st.st_size;
    parser.start = offset;
    parser.curr = offset;

    for ( ; ; ) {
        log = toLog(&parser);
        if (parser.curr + log->len > parser.size) {
            printf("curr:%u, len=%u, size:%u\n", parser.curr, (unsigned)log->len, parser.size);
            break;
        }
        snprintf(buf, log->text_len + 1, "%s", log_text(log));
        printf("[%7u] [%5u.%06u %02x %d] %s\n", parser.curr,
                (unsigned)(log->ts_nsec / 1000000000),
                (unsigned)(log->ts_nsec % 1000000000) / 1000,
                log->flags, log->level,
                buf);
        parser.curr += log->len;
        count++;
    }

    return count;
}

/*****************************************************************************/

int main(int argc, char **argv)
{
    int fd;
    unsigned long offset;

    offset = strtoul(argv[2], NULL, 0);
    printf("offset: %lu\n", offset);

    fd = open(argv[1], O_RDONLY);
    ASSERT(fd > 0);

    parse(fd, offset);

    return 0;
}
