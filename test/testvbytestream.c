#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "unittest.h"
#include "diag.h"
#include "mrkcommon/dumpm.h"
#include "mrkcommon/util.h"
#include "mrkcommon/vbytestream.h"

#define MNUNIT_PARAMETRIZE(name, body) \
    for (size_t _ ## name = 0;         \
         _ ## name < countof(name);    \
         ++_ ## name) {                \
        body                           \
    }                                  \


#define MNUNIT_ARG(name) name[_ ## name]


UNUSED static void
test0(void)
{
    int i;
    mnvbytestream_t bs;

    vbytestream_init(&bs, 256, 0);

    for (i = 0; i < 40; ++i) {
        if (vbytestream_nprintf(&bs, 64, "%d ", i) < 0) {
            FAIL("vbytestream_nprintf");
        }
    }

    vbytestream_dump(&bs, /* 0 */ VBYTESTREAM_DUMP_FULL);

    vbytestream_write(&bs, 1);
    vbytestream_fini(&bs);


}


UNUSED static void
test1(void)
{
    int fd;
    mnvbytestream_t bs;
    ssize_t nwritten;
    const char *fname = "/tmp/mysmartctl-ada0";

    srandom(time(NULL));

    vbytestream_init(&bs, 4096, 0);
    if ((fd = open(fname, O_RDONLY)) < 0) {
        FAIL("open");
    }

    while (true) {
        ssize_t sz, nread;

        sz = random() % 1024;
        if (sz == 0) {
            continue;
        }
        if ((nread = vbytestream_read(&bs, fd, sz)) <= 0) {
            break;
        }
        //TRACE("sz=%ld nread=%ld", sz, nread);
    }

    lseek(fd, 0, SEEK_SET);
    (void)close(fd);

    //vbytestream_dump(&bs, 0);

    if ((fd = open("qwe", O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0) {
        FAIL("open");
    }
    nwritten = vbytestream_write(&bs, fd);
    (void)close(fd);
    TRACE("pos %d/%ld/%ld eod %d/%ld/%ld",
          bs.pos.idx,
          bs.pos.offt,
          bs.pos.total,
          bs.eod.idx,
          bs.eod.offt,
          bs.eod.total);
    vbytestream_fini(&bs);
}


UNUSED static void
_test2(size_t growsz, int n)
{
    int fd;
    mnvbytestream_t bs;
    ssize_t nwritten;
    const char *fname = "/tmp/mysmartctl-ada0";
    char fnamebuf[1024];

    srandom(time(NULL));

    vbytestream_init(&bs, growsz, 0);
    if ((fd = open(fname, O_RDONLY)) < 0) {
        FAIL("open");
    }

    while (true) {
        ssize_t sz, nread;
        mnbytes_t *buf;

        sz = random() % n;
        if (sz == 0) {
            continue;
        }
        buf = bytes_new(sz);
        sz = read(fd, BDATA(buf), sz);
        if (sz <= 0) {
            break;
        }
        BSZ(buf) = sz;
        nread = vbytestream_adopt(&bs, buf);
        assert(nread > 0);
        //TRACE("sz=%ld nread=%ld", sz, nread);
    }

    //lseek(fd, 0, SEEK_SET);
    (void)close(fd);

    //vbytestream_dump(&bs, 0);

    (void)snprintf(fnamebuf, sizeof(fnamebuf), "qwe-%08lx-%08x", growsz, n);
    if ((fd = open(fnamebuf, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0) {
        FAIL("open");
    }

    nwritten = vbytestream_write(&bs, fd);
    (void)close(fd);

    //TRACE("%s: nwritten %ld pos %d/%ld/%ld eod %d/%ld/%ld",
    //      fnamebuf,
    //      nwritten,
    //      bs.pos.idx,
    //      bs.pos.offt,
    //      bs.pos.total,
    //      bs.eod.idx,
    //      bs.eod.offt,
    //      bs.eod.total);

    vbytestream_fini(&bs);
}


static void
test2(void)
{
    size_t growsz[] = { 32, 64, 128, 256, 512, 1024, 2048, 4096 };
    int n[] = { 32, 64, 128, 256, 512, 1024, 2048, 4096 };

    MNUNIT_PARAMETRIZE(growsz,
    MNUNIT_PARAMETRIZE(n,
        _test2(MNUNIT_ARG(growsz), MNUNIT_ARG(n));
    ));
}


int
main(void)
{
    //test0();
    //test1();
    test2();
    return 0;
}

// vim:list
