#include "test_utils.h"
#include "tube.h"
#include "tube_stream.h"

static uint8_t streamdata[] = {
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b,
    0x0c, 0x0d, 0x0e, 0x0f
};

CTEST_DATA(tube_stream)
{
    tube_stream_manager *sm;
    ls_err err;
};

CTEST_SETUP(tube_stream)
{
    data->sm = NULL;
    tube_stream_manager_create(0, &data->sm, &data->err);
}
CTEST_TEARDOWN(tube_stream)
{
    if (data->sm) {
        tube_stream_manager_destroy(data->sm);
    }
}

CTEST2(tube_stream, create_manager)
{
    tube_stream_manager *sm;
    bool                retval;
    
    retval = tube_stream_manager_create(0, &sm, &data->err);
    ASSERT_TRUE( retval );
    // ASSERT_TRUE( sm != NULL);
    tube_stream_manager_destroy(sm);
}

CTEST2(tube_stream, get_tube_manager)
{
    tube_manager *tm = tube_stream_manager_get_manager(data->sm);
    ASSERT_TRUE( (uintptr_t)tm == (uintptr_t)data->sm );
}

CTEST2(tube_stream, create)
{
    tube_stream *s;
    bool        retval;
    retval = tube_stream_create(&s, &data->err);
    ASSERT_TRUE( retval );
    tube_stream_destroy(s);
}

CTEST2(tube_stream, bind)
{
    tube_stream *s;
    tube        *t;
    bool        retval;
    
    tube_create(&t, &data->err);
    tube_stream_create(&s, &data->err);
    
    retval = tube_stream_bind(s, t, &data->err);
    ASSERT_FALSE( retval );
    ASSERT_EQUAL( data->err.code, LS_ERR_NO_IMPL );
    
    tube_stream_destroy(s);
    tube_destroy(t);
}

CTEST2(tube_stream, read)
{
    tube_stream *s;
    tube_stream_create(&s, &data->err);

    uint8_t buf[1024];
    size_t  len = sizeof(buf);
    ssize_t retval;
    retval = tube_stream_read(s, buf, len);
    ASSERT_EQUAL( retval, -1 );

    tube_stream_destroy(s);
}

CTEST2(tube_stream, write)
{
    tube_stream *s;
    bool        retval;
    tube_stream_create(&s, &data->err);

    uint8_t *buf = streamdata;
    size_t  len = sizeof(streamdata);
    retval = tube_stream_write(s, buf, len, &data->err);
    ASSERT_FALSE( retval );
    ASSERT_EQUAL( data->err.code, LS_ERR_NO_IMPL );
    
    tube_stream_destroy(s);
}

CTEST2(tube_stream, close)
{
    tube_stream *s;
    bool        retval;
    tube_stream_create(&s, &data->err);
    
    retval = tube_stream_close(s, &data->err);
    ASSERT_FALSE( retval );
    ASSERT_EQUAL( data->err.code, LS_ERR_NO_IMPL );
    
    tube_stream_destroy(s);
}
