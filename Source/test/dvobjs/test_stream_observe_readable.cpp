#include "purc.h"

#include <gtest/gtest.h>

TEST(observe, basic)
{
    // TODO: reopen
    if (1)
        return;

    const char *hvml =
    "<!DOCTYPE hvml>"
    "<hvml target=\"html\" lang=\"en\">"
    "    <head>"
    "        <update on=\"$TIMERS\" to=\"unite\">"
    "            ["
    "                { \"id\" : \"clock\", \"interval\" : 1000, \"active\" : \"yes\" }"
    "            ]"
    "        </update>"
    "    </head>"
    ""
    "    <body>"
    "        <div id=\"stream\">"
    ""
    "            <div id=\"c_title\">"
    "                <h2 id=\"c_title\">Stream observe<br/>"
    "                    <span id=\"content\">$DATETIME.fmtbdtime('%Y-%m-%dT%H:%M:%S', null)</span>"
    "                </h2>"
    "                <init as='stream_pipe' with=\"$STREAM.open('pipe:///var/tmp/stream_pipe', 'read create nonblock')\"/>"
    ""
    "                <observe on=\"$stream_pipe\" for=\"event:readable\">"
    "                    <update on=\"#content\" at=\"textContent\" with=\"$EJSON.stringify($stream_pipe.readlines(1))\" />"
    "                    <forget on=\"$stream_pipe\" for=\"event:readable\"/>"
    "                </observe>"
    ""
    ""
    "                <observe on=\"$TIMERS\" for=\"expired:clock\">"
    "                    <forget on=\"$TIMERS\" for=\"expired:clock\"/>"
    "                    <update on=\"#content\" at=\"textContent\" with=\"$EJSON.stringify($STREAM.open('pipe:///var/tmp/stream_pipe', 'write').writelines('message write to pipe'))\" />"
    "                </observe>"
    ""
    "                <p>this is after observe</p>"
    "            </div>"
    ""
    "        </div>"
    "    </body>"
    ""
    "</hvml>";

    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;

    // initial purc
    ret = purc_init_ex (PURC_MODULE_HVML, "cn.fmsoft.hybridos.test",
            "test_stream_observe", &info);

    ASSERT_EQ (ret, PURC_ERROR_OK);

    // get statitics information
    const struct purc_variant_stat * stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);

    purc_vdom_t vdom = purc_load_hvml_from_string(hvml);
    ASSERT_NE(vdom, nullptr);

    purc_run(PURC_VARIANT_INVALID, NULL);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);

    unlink("/var/tmp/stream_pipe");
}

