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

#include "common.h"
#include <windows.h>
#include <cstdlib>
#include <vector>

#define HWND_NAME "winmisc.window"

static HWND GetWindow (lua_State * L, int arg = 1)
{
    return *(HWND *)luaL_checkudata(L, arg, HWND_NAME);
}

template<typename T>
int Box (lua_State * L, T item, const char * name)
{
    if (item)
    {
        *static_cast<T *>(lua_newuserdata(L, sizeof(T))) = item; // ..., item

        luaL_newmetatable(L, HWND_NAME);// ..., item, mt
        lua_setmetatable(L, -2);// ..., item; item.metatable = mt
    }

    else lua_pushnil(L); // ..., nil

    return 1;
}

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

            BYTE * output = NULL;
            size_t size = count * 3;
            bool used_blob = false;
            int top = lua_gettop(L);

            if (BlobXS::IsBlob(L, top) && (BlobXS::IsResizable(L, top) || BlobXS::GetSize(L, top) >= size))
            {
                used_blob = true;

                if (BlobXS::GetSize(L,top) < size)
                {
                    used_blob = BlobXS::Resize(L, top, size);
                }

                if (used_blob) output = BlobXS::GetData(L, top);
            }

            std::vector<BYTE> out;

            if (!output)
            {
                out.resize(size);

                output = out.data();
            }

            LONG stride = (info.bmiHeader.biWidth * (info.bmiHeader.biBitCount / 8) + 3) & ~3;

            for (LONG y = 0; y < abs_height; ++y)
            {
                LONG base = y * stride;

                for (LONG x = 0; x < info.bmiHeader.biWidth; ++x)
                {
                    output[0] = input[base + 2];
                    output[1] = input[base + 1];
                    output[2] = input[base + 0];

                    output += 3;
                    base += 4;
                }
            }

            if (!used_blob) lua_pushlstring(L, reinterpret_cast<char *>(out.data()), out.size()); // [window, ]bad_blob, image

            lua_pushinteger(L, info.bmiHeader.biWidth); // [window, ][bad_blob, ]image, width
            lua_pushinteger(L, abs_height); // [window, ][bad_blob, ]image, width, height
        }
    }

    ReleaseDC(NULL, hdc);

    return ok;
}

static BOOL CALLBACK EnumWindowsProc (HWND window, LPARAM lparam)
{
    lua_State * L = reinterpret_cast<lua_State *>(lparam);

    lua_pushvalue(L, 1);   // enumerator, enumerator

    Box(L, window, HWND_NAME);  // enumerator, enumerator, window

    if (lua_pcall(L, 1, 1, 0) == 0)    // enumerator, result? / err
    {
        lua_pushliteral(L, "done");// enumerator, result?, "done"

        if (!lua_equal(L, 2, 3))
        {
            lua_pop(L, 2);  // enumerator

            return TRUE;
        }
    }

    return FALSE;
}

CORONA_EXPORT int luaopen_plugin_winmisc (lua_State* L)
{
	lua_newtable(L); // winmisc

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
                HDC     hScreen = GetDC(nullptr);
                HDC     hDC = CreateCompatibleDC(hScreen);
                HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
                HGDIOBJ old_obj = SelectObject(hDC, hBitmap);
                BOOL    bRet = BitBlt(hDC, 0, 0, w, h, hScreen, x, y, SRCCOPY);

                // save bitmap to clipboard
                OpenClipboard(nullptr);
                EmptyClipboard();
                SetClipboardData(CF_BITMAP, hBitmap);
                CloseClipboard();

                // clean up
                SelectObject(hDC, old_obj);
                DeleteDC(hDC);
                ReleaseDC(nullptr, hScreen);
                DeleteObject(hBitmap);

                return 0;
			}
        }, {
			"CopyWindowToClipboard", [](lua_State * L)
			{
                HWND    window = GetWindow(L);
                HDC     hScreen = GetDC(window);
                HDC     hDC = CreateCompatibleDC(hScreen);
               
                RECT rect;

                GetWindowRect(window, &rect);

                LONG w = rect.right - rect.left, h = rect.bottom - rect.top;
                HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
                HGDIOBJ old_obj = SelectObject(hDC, hBitmap);
                BOOL    bRet = BitBlt(hDC, 0, 0, w, h, hScreen, rect.left, rect.top, SRCCOPY);

                // save bitmap to clipboard
                OpenClipboard(nullptr);
                EmptyClipboard();
                SetClipboardData(CF_BITMAP, hBitmap);
                CloseClipboard();

                // clean up
                SelectObject(hDC, old_obj);
                DeleteDC(hDC);
                ReleaseDC(window, hScreen);
                DeleteObject(hBitmap);

                return 0;
			}
        }, {
            "EnumerateDesktops", [](lua_State * L)
            {
                lua_settop(L, 1);   // enumerator
                luaL_argcheck(L, lua_isfunction(L, 1), 1, "Non-function desktop enumerator");

                BOOL result = EnumDesktops(GetProcessWindowStation(), [](char * name, LPARAM state) {
                    lua_State * lstate = reinterpret_cast<lua_State *>(state);

                    lua_pushvalue(lstate, 1);   // enumerator, enumerator
                    lua_pushstring(lstate, name);   // enumerator, enumerator, name
                    
                    if (lua_pcall(lstate, 1, 1, 0) == 0)    // enumerator, result? / err
                    {
                        lua_pushliteral(lstate, "done");// enumerator, result?, "done"

                        if (!lua_equal(lstate, 2, 3))
                        {
                            lua_pop(lstate, 2);  // enumerator

                            return TRUE;
                        }
                    }

                    return FALSE;
                }, LONG_PTR(L));

                lua_pushboolean(L, result); // enumerator[, result[, "done"]], ok

                return 1;
            }
        }, {
            "EnumerateDesktopWindows", [](lua_State * L)
            {
                lua_settop(L, 2);   // name, enumerator

                const char * name = luaL_checkstring(L, 1);
                luaL_argcheck(L, lua_isfunction(L, 2), 2, "Non-function desktop window enumerator");

                HDESK desktop = OpenDesktop(name, 0, FALSE, GENERIC_READ);

                lua_remove(L, 1);   // enumerator

                BOOL result = !!desktop;

                if (desktop)
                {
                    result = EnumDesktopWindows(desktop, EnumWindowsProc, LONG_PTR(L));

                    CloseDesktop(desktop);
                }

                lua_pushboolean(L, result); // enumerator[, result[, "done"]], ok

                return 1;
            }
        }, {
            "EnumerateWindows", [](lua_State * L)
            {
                luaL_argcheck(L, lua_isfunction(L, 1), 1, "Non-function window enumerator");
                
                BOOL result = EnumWindows(EnumWindowsProc, LONG_PTR(L));

                lua_pushboolean(L, result); // enumerator[, result[, "done"]], ok

                return 1;
            }
        }, {
		    "GetClipboardText", [](lua_State * L)
		    {
			    lua_settop(L, 0);	// (empty)

			    BOOL ok = FALSE;

			    if (OpenClipboard(nullptr))
			    {
				    HANDLE hMem = GetClipboardData(CF_TEXT);

				    if (hMem)
				    {
					    void * data = GlobalLock(hMem);

					    if (data)
					    {
						    ok = TRUE;

						    lua_pushstring(L, (char *)data);// text

						    GlobalUnlock(hMem);
					    }
				    }

				    CloseClipboard();
			    }

			    lua_pushboolean(L, ok);	// [text, ]ok

			    if (ok) lua_insert(L, -2);	// ok, text

			    return lua_gettop(L);
		    }
        }, {
            "GetForegroundWindow", [](lua_State * L)
            {
                return Box(L, GetForegroundWindow(), HWND_NAME);
            }
        }, {
            "GetImageDataFromClipboard", [](lua_State * L)
            {
                lua_settop(L, 1); // bytes?

                bool ok = false;

                if (IsClipboardFormatAvailable(CF_BITMAP) && OpenClipboard(nullptr))
                {
                    HDC hdc = GetDC(nullptr);

                    ok = GetBytesFromBitmap(L, hdc, (HBITMAP)GetClipboardData(CF_BITMAP)); // bytes?[, data[, w, h]]

                    CloseClipboard();
                    ReleaseDC(nullptr, hdc);
                }

                if (!ok) lua_pushboolean(L, 0);  // bytes?, false

                return ok ? 3 : 1;
            }
        }, {
            "GetImageDataFromScreen", [](lua_State * L)
            {
                lua_settop(L, 1); // bytes?

                int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
                int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
                int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
                int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);

                // copy screen to bitmap
                HDC     hScreen = GetDC(nullptr);
                HDC     hDC = CreateCompatibleDC(hScreen);
                HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
                HGDIOBJ old_obj = SelectObject(hDC, hBitmap);
                BOOL    bRet = BitBlt(hDC, 0, 0, w, h, hScreen, x, y, SRCCOPY);

                bool ok = GetBytesFromBitmap(L, hDC, hBitmap); // bytes?[, data[, w, h]]

                // clean up
                SelectObject(hDC, old_obj);
                DeleteDC(hDC);
                ReleaseDC(nullptr, hScreen);
                DeleteObject(hBitmap);

                if (!ok) lua_pushboolean(L, 0);  // bytes?, false

                return ok ? 3 : 1;
            }
        }, {
            "GetImageDataFromWindow", [](lua_State * L)
            {
                lua_settop(L, 2); // window, bytes?

                HWND    window = GetWindow(L);
                HDC     hScreen = GetDC(window);
                HDC     hDC = CreateCompatibleDC(hScreen);
               
                RECT rect;

                GetWindowRect(window, &rect);

                LONG w = rect.right - rect.left, h = rect.bottom - rect.top;
                HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
                HGDIOBJ old_obj = SelectObject(hDC, hBitmap);
                BOOL    bRet = BitBlt(hDC, 0, 0, w, h, hScreen, 0, 0, SRCCOPY);

                bool ok = GetBytesFromBitmap(L, hDC, hBitmap); // window, bytes?[, data[, w, h]]

                // clean up
                SelectObject(hDC, old_obj);
                DeleteDC(hDC);
                ReleaseDC(window, hScreen);
                DeleteObject(hBitmap);

                if (!ok) lua_pushboolean(L, 0);  // bytes?, false

                return ok ? 3 : 1;
            }
        }, {
            "GetWindowText", [](lua_State * L)
            {
                HWND window = GetWindow(L);
                int len = GetWindowTextLength(window);

                std::vector<char> str(len + 1);

                GetWindowText(window, str.data(), len + 1);

                lua_pushlstring(L, str.data(), size_t(len));// window, text

                return 1;
            }
        }, {
            "IsWindowVisible", [](lua_State * L)
            {
                lua_pushboolean(L, IsWindowVisible(GetWindow(L)));  // window, is_visible

                return 1;
            }
        }, {
		    "SetClipboardText", [](lua_State * L)
		    {
			    BOOL ok = FALSE;

			    if(OpenClipboard(nullptr))
			    {
				    if(EmptyClipboard())
				    {
					    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, lua_objlen(L, 1) + 1);

					    memcpy(GlobalLock(hMem), lua_tostring(L, 1), lua_objlen(L, 1) + 1);

					    GlobalUnlock(hMem);

					    ok = SetClipboardData(CF_TEXT, hMem) != nullptr;
				    }
			
				    CloseClipboard();
			    }

			    lua_pushboolean(L, ok);	// text, ok

			    return 1;
		    }
	    },
		{ nullptr, nullptr }
	};

	luaL_register(L, nullptr, funcs);

    AddPDFImageConverter(L);
    AddScreenRecorder(L);

	return 1;
}