//#include "tests.h"
#include <iostream>
#include <gtest/gtest.h>
#include "../ConEmu/helper.h"

namespace {
TEST(ConEmuTest, Main) {
	const int val = 1;
	EXPECT_EQ(val, 1);
}
}

int main(int argc, char** argv) {
	initMainThread();
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
