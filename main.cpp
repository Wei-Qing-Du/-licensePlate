#include "licenseplatedialog.h"
#include <QApplication>
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#define TRUN_ON_TEST 0
using namespace testing;

#if TRUN_ON_TEST
bool check()
{
    return true;
}
TEST(Function, check)
{
    EXPECT_EQ(1, 1);
    EXPECT_TRUE(check());
}
#endif
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    #if TRUN_ON_TEST
    ::testing::InitGoogleTest(&argc, argv);
    RUN_ALL_TESTS();
    #endif
    licensePlateDialog w;
    w.show();
    return a.exec();
}
