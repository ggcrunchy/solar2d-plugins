/*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
*/

#include "CoronaLua.h"
#include <windows.h>
#include <cstdlib>
#include <vector>

static bool GetBytesFromBitmap (lua_State * L, HDC hdc, HBITMAP hBitmap)
{
    BITMAPINFO info = {};
    bool ok = false;

    info.bmiHeader.biSize = sizeof(info.bmiHeader);

    if (GetDIBits(hdc, hBitmap, 0, 0, nullptr, &info, DIB_RGB_COLORS))
    {
        // https://stackoverflow.com/a/3688682
        std::vector<BYTE> bytes(info.bmiHeader.biSizeImage);

        LONG abs_height = abs(info.bmiHeader.biHeight);

        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        info.bmiHeader.biHeight = -abs_height;

        ok = GetDIBits(hdc, hBitmap, 0, abs_height, bytes.data(), &info, DIB_RGB_COLORS);

        if (ok)
        {
            const BYTE * input = bytes.data();
            LONG count = info.bmiHeader.biWidth * abs_height;

            std::vector<BYTE> out(count * 3);
            BYTE * output = out.data();

            for (LONG i = 0; i < count; ++i)
            {
                output[0] = input[2];
                output[1] = input[1];
                output[2] = input[0];

                output += 3;
                input += 4;
            }

            lua_pushlstring(L, reinterpret_cast<char *>(out.data()), out.size()); // image
            lua_pushinteger(L, info.bmiHeader.biWidth); // image, width
            lua_pushinteger(L, abs_height); // image, width, height
        }
    }

    ReleaseDC(NULL, hdc);

    return ok;
}

CORONA_EXPORT int luaopen_plugin_winmisc (lua_State* L)
{
	lua_newtable(L);// winmisc

	luaL_Reg funcs[] = {
		{
			"CopyScreenToClipboard", [](lua_State * L)
			{
				// https://stackoverflow.com/a/28248531

                int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
                int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
                int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

                // copy screen to bitmap
                HDC     hScreen = GetDC(NULL);
                HDC     hDC = CreateCompatibleDC(hScreen);
                HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
                HGDIOBJ old_obj = SelectObject(hDC, hBitmap);
                BOOL    bRet = BitBlt(hDC, 0, 0, w, h, hScreen, x, y, SRCCOPY);

                // save bitmap to clipboard
                OpenClipboard(NULL);
                EmptyClipboard();
                SetClipboardData(CF_BITMAP, hBitmap);
                CloseClipboard();

                // clean up
                SelectObject(hDC, old_obj);
                DeleteDC(hDC);
                ReleaseDC(NULL, hScreen);
                DeleteObject(hBitmap);

                return 0;
			}
        }, {
            "GetImageDataFromClipboard", [](lua_State * L)
            {
                bool ok = false;

                if (IsClipboardFormatAvailable(CF_BITMAP) && OpenClipboard(nullptr))
                {
                    HDC hdc = GetDC(nullptr);

                    ok = GetBytesFromBitmap(L, hdc, (HBITMAP)GetClipboardData(CF_BITMAP));

                    CloseClipboard();
                    ReleaseDC(nullptr, hdc);
                }

                if (!ok) lua_pushboolean(L, 0);  // false

                return ok ? 3 : 1;
            }
        }, {
            "GetImageDataFromScreen", [](lua_State * L)
            {
                int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
                int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
                int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

                // copy screen to bitmap
                HDC     hScreen = GetDC(NULL);
                HDC     hDC = CreateCompatibleDC(hScreen);
                HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
                HGDIOBJ old_obj = SelectObject(hDC, hBitmap);
                BOOL    bRet = BitBlt(hDC, 0, 0, w, h, hScreen, x, y, SRCCOPY);

                bool ok = GetBytesFromBitmap(L, hDC, hBitmap);

                // clean up
                SelectObject(hDC, old_obj);
                DeleteDC(hDC);
                ReleaseDC(NULL, hScreen);
                DeleteObject(hBitmap);

                if (!ok) lua_pushboolean(L, 0);  // false

                return ok ? 3 : 1;
            }
        },
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);

	return 1;
}