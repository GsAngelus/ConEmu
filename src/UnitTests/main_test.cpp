//#include "tests.h"
#include <iostream>
#include <gtest/gtest.h>
#include "../common/wcwidth.h"
#include "../ConEmu/helper.h"

HWND ghWnd = NULL;
HINSTANCE g_hInstance = NULL;
bool gbMessagingStarted = false;
namespace FastConfig
{
HWND ghFastCfg = NULL;
}

int MsgBox(LPCTSTR lpText, UINT uType, LPCTSTR lpCaption /*= NULL*/, HWND ahParent /*= (HWND)-1*/, bool abModal /*= true*/)
{
	return IDCANCEL;
}

bool isCharAltFont(ucs32 inChar)
{
	return false;
}

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
