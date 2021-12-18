#include "purc.h"

#include <gtest/gtest.h>

TEST(interpreter, basic)
{
    const char *hvmls[] = {
        "<hvml><head x=\"y\">hello<xinit a=\"b\">world<!--yes-->solid</xinit></head><body><timeout1/><timeout3/></body></hvml>",
        "<hvml><head x=\"y\">hello<xinit a=\"b\">w<timeout3/>orld<!--yes-->solid</xinit></head><body><timeout1/></body></hvml>",
        "<hvml><body><timeout1/><timeout9/><timeout2/></body></hvml>",
    };

    purc_instance_extra_info info = {};
    int ret = 0;
    bool cleanup = false;

    // initial purc
    ret = purc_init ("cn.fmsoft.hybridos.test", "test_init", &info);

    ASSERT_EQ (ret, PURC_ERROR_OK);

    // get statitics information
    struct purc_variant_stat * stat = purc_variant_usage_stat ();
    ASSERT_NE(stat, nullptr);

    for (size_t i=0; i<PCA_TABLESIZE(hvmls); ++i) {
        const char *hvml = hvmls[i];
        purc_vdom_t vdom = purc_load_hvml_from_string(hvml);
        ASSERT_NE(vdom, nullptr);
    }

    purc_run(PURC_VARIANT_INVALID, NULL);

    cleanup = purc_cleanup ();
    ASSERT_EQ (cleanup, true);
}

