#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "XDebug.h"

typedef unsigned long long u64;
typedef unsigned int u32;
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

static char *log_text(const struct log *msg)
{
    return (char *)msg + sizeof(struct log);
}

static struct log *log_from_idx(char *log_buf, u32 idx)
{
    struct log *msg = (struct log *)(log_buf + idx);

    /*
     * A length == 0 record is the end of buffer marker. Wrap around and
     * read the message at the start of the buffer.
     */
    if (!msg->len)
        return (struct log *)log_buf;

    return msg;
}

/* get next record; idx must point to valid msg */
static u32 log_next(char *log_buf, u32 idx)
{
    struct log *msg = (struct log *)(log_buf + idx);

    /* length == 0 indicates the end of the buffer; wrap */
    /*
     * A length == 0 record is the end of buffer marker. Wrap around and
     * read the message at the start of the buffer as *this* one, and
     * return the one after that.
     */
    DBG("msg->len = %d\n", msg->len);
    if (!msg->len) {
        msg = (struct log *)log_buf;
        return msg->len;
    }
    return idx + msg->len;
}

static int parse(int fd, u32 start)
{
    int ret;
    int count = 0;
    struct stat st;
    struct log *log;
    struct KLogParser parser;
    static char buf[512];
    u32 idx;
    u32 rotated = 0;

    ret = fstat(fd, &st);
    ASSERT(ret == 0);
    ASSERT(st.st_size > start);

    parser.fd = fd;
    parser.base = (u8 *)mmap(NULL,
            st.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    ASSERT(MAP_FAILED != parser.base);
    parser.size = st.st_size;
    parser.start = start;
    parser.curr = start;

    idx = start;
    for ( ; ; ) {
        log = log_from_idx(parser.base, idx);
        
        snprintf(buf, log->text_len + 1, "%s", log_text(log));
        printf("%7u: [%5u.%06u %02x %d] %s\n", idx,
                (unsigned)(log->ts_nsec / 1000000000),
                (unsigned)(log->ts_nsec % 1000000000) / 1000,
                log->flags, log->level,
                buf);
        count++;

        idx = log_next(parser.base, idx);
        DBG("log_next_idx = %u\n", idx);
        if (idx == 0 || idx >= parser.size) {
            if (rotated) {
                DBG("[EOF]\n");
                break;
            }
            rotated = 1;
            idx = 0;
        }
    }

    return count;
}

/*****************************************************************************/

static void usage(const char *program)
{
    fprintf(stderr, "Usage: %s <logbuf-dumpfile> [start-offset]\n", program);
    exit(EXIT_FAILURE);
}

/*****************************************************************************/

int main(int argc, char **argv)
{
    int fd;
    u32 offset = 0;

    if (argc < 2 || argc > 3)
        usage(argv[0]);

    fd = open(argv[1], O_RDONLY);
    if (fd < 0)
        usage(argv[0]);

    if (argc > 2)
        offset = strtoul(argv[2], NULL, 0);

    parse(fd, offset);

    return 0;
}
