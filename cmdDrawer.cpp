// Copyright 2026 MrSeagull. All Rights Reserved.
#include "cmdDrawer.h"
#include <iostream>
#include <algorithm>
#include <string>
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")

// ---------- 内部辅助结构 ----------
namespace {
	// 快速整数转字符串表（0-255）
	struct FastIntTable {
		char str[256][4];
		int len[256];
		FastIntTable() {
			for (int i = 0; i < 256; ++i) {
				if (i < 10) {
					str[i][0] = '0' + i;
					str[i][1] = '\0';
					len[i] = 1;
				}
				else if (i < 100) {
					str[i][0] = '0' + i / 10;
					str[i][1] = '0' + i % 10;
					str[i][2] = '\0';
					len[i] = 2;
				}
				else {
					str[i][0] = '0' + i / 100;
					str[i][1] = '0' + (i / 10) % 10;
					str[i][2] = '0' + i % 10;
					str[i][3] = '\0';
					len[i] = 3;
				}
			}
		}
	};
	const FastIntTable g_intTable;

	HANDLE g_hConsole = INVALID_HANDLE_VALUE;
	HANDLE g_hStdin = INVALID_HANDLE_VALUE;
	WORD g_originalAttributes = 0;
	DWORD g_originalInputMode = 0;
	bool g_initialized = false;
	Size2DInt g_maxCanvasSize = { 0, 0 };

	bool enableVTMode() {
		DWORD dwMode = 0;
		if (!GetConsoleMode(g_hConsole, &dwMode)) return false;
		dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		return SetConsoleMode(g_hConsole, dwMode);
	}

	bool isWindowsTerminal() {
		HWND hwnd = GetConsoleWindow();
		if (!hwnd) return false;
		DWORD pid = 0;
		GetWindowThreadProcessId(hwnd, &pid);
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
		if (!hProcess) return false;
		wchar_t path[MAX_PATH];
		if (GetModuleFileNameExW(hProcess, NULL, path, MAX_PATH)) {
			std::wstring procPath(path);
			size_t pos = procPath.find_last_of(L"\\");
			if (pos != std::wstring::npos) {
				std::wstring fileName = procPath.substr(pos + 1);
				if (_wcsicmp(fileName.c_str(), L"WindowsTerminal.exe") == 0) {
					CloseHandle(hProcess);
					return true;
				}
			}
		}
		CloseHandle(hProcess);
		return false;
	}
}

bool initializeConsoleDrawer() {
	if (g_initialized) return true;

	if (isWindowsTerminal()) return false;

	g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	g_hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (g_hConsole == INVALID_HANDLE_VALUE || g_hStdin == INVALID_HANDLE_VALUE) {
		std::string msg = "获取控制台句柄失败。错误码：" + std::to_string(GetLastError());
		OutputDebugStringA(msg.c_str());
		return false;
	}

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!GetConsoleScreenBufferInfo(g_hConsole, &csbi)) {
		OutputDebugStringA("获取控制台信息失败");
		return false;
	}
	g_originalAttributes = csbi.wAttributes;

	if (!GetConsoleMode(g_hStdin, &g_originalInputMode)) {
		OutputDebugStringA("获取输入模式失败");
		return false;
	}

	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof(CONSOLE_FONT_INFOEX);
	if (GetCurrentConsoleFontEx(g_hConsole, FALSE, &cfi)) {
		cfi.dwFontSize.X = 1;
		cfi.dwFontSize.Y = 1;
		cfi.FontFamily = FF_DONTCARE;
		cfi.FontWeight = FW_NORMAL;
		wcscpy_s(cfi.FaceName, L"Consolas");
		SetCurrentConsoleFontEx(g_hConsole, FALSE, &cfi);
	}

	HWND hwnd = GetConsoleWindow();
	if (hwnd) ShowWindow(hwnd, SW_MAXIMIZE);
	Sleep(100);

	if (hwnd) {
		LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
		style &= ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
		SetWindowLongPtr(hwnd, GWL_STYLE, style);
		SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	}

	GetConsoleScreenBufferInfo(g_hConsole, &csbi);
	SHORT windowWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
	SHORT windowHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	COORD newBufferSize = { windowWidth, windowHeight };
	SetConsoleScreenBufferSize(g_hConsole, newBufferSize);

	g_maxCanvasSize.x = windowWidth;
	g_maxCanvasSize.y = windowHeight;

	if (!enableVTMode()) {
		OutputDebugStringA("启用 VT 模式失败");
		return false;
	}

	DWORD newMode = g_originalInputMode;
	newMode |= ENABLE_EXTENDED_FLAGS;
	newMode &= ~ENABLE_QUICK_EDIT_MODE;
	SetConsoleMode(g_hStdin, newMode);

	g_initialized = true;
	return true;
}

Size2DInt getMaxCanvasSize() {
	return g_maxCanvasSize;
}

void drawFrame(const Frame& frame) {
	if (!g_initialized) {
		OutputDebugStringA("绘图器未初始化，请先调用 initializeConsoleDrawer()");
		return;
	}

	int drawWidth = frame.width;
	int drawHeight = frame.height;

	SetConsoleCursorPosition(g_hConsole, COORD{ 0, 0 });

	std::vector<char> lineBuffer(drawWidth * 20 + 2);
	char* lineBufferPtr = lineBuffer.data();

	for (int y = 0; y < drawHeight; ++y) {
		char* p = lineBufferPtr;
		int lastR = -1, lastG = -1, lastB = -1;

		for (int x = 0; x < drawWidth; ++x) {
			const auto& pixel = frame.pixels[y * drawWidth + x];

			if (pixel.Red != lastR || pixel.Green != lastG || pixel.Blue != lastB) {
				*p++ = '\x1b';
				*p++ = '[';
				*p++ = '4';
				*p++ = '8';
				*p++ = ';';
				*p++ = '2';
				*p++ = ';';

				memcpy(p, g_intTable.str[pixel.Red], g_intTable.len[pixel.Red]);
				p += g_intTable.len[pixel.Red];
				*p++ = ';';

				memcpy(p, g_intTable.str[pixel.Green], g_intTable.len[pixel.Green]);
				p += g_intTable.len[pixel.Green];
				*p++ = ';';

				memcpy(p, g_intTable.str[pixel.Blue], g_intTable.len[pixel.Blue]);
				p += g_intTable.len[pixel.Blue];

				*p++ = 'm';

				lastR = pixel.Red;
				lastG = pixel.Green;
				lastB = pixel.Blue;
			}
			*p++ = ' ';
		}
		*p++ = '\n';

		DWORD written;
		WriteFile(g_hConsole, lineBufferPtr, static_cast<DWORD>(p - lineBufferPtr), &written, nullptr);
	}

	printf("\x1b[0m");
	SetConsoleTextAttribute(g_hConsole, g_originalAttributes);
}

void shutdownConsoleDrawer() {
	if (g_initialized) {
		SetConsoleTextAttribute(g_hConsole, g_originalAttributes);
		SetConsoleMode(g_hStdin, g_originalInputMode);
		g_initialized = false;
	}
}