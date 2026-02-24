#include "Spotify.h"
#include <windows.h>
#include <tlhelp32.h>

static std::string GetWindowTitle(HWND hwnd)
{
	char title[512];
	if (IsWindowVisible(hwnd) && GetWindowTextA(hwnd, title, sizeof(title)) > 0)
		return std::string(title);
	return std::string();
}

static std::vector<DWORD> GetProcessIdsByName(const std::wstring& exeName)
{
	std::vector<DWORD> pids;
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snap == INVALID_HANDLE_VALUE)
		return pids;

	PROCESSENTRY32W entry;
	entry.dwSize = sizeof(entry);

	if (Process32FirstW(snap, &entry)) 
	{
		do {
			if (_wcsicmp(entry.szExeFile, exeName.c_str()) == 0)
				pids.push_back(entry.th32ProcessID);
		} while (Process32NextW(snap, &entry));
	}

	CloseHandle(snap);
	return pids;
}

static HWND FindMainWindow(DWORD pid)
{
	struct EnumData {
		DWORD pid;
		HWND hwnd;
	} data{ pid, nullptr };

	auto enumProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
		EnumData* pData = reinterpret_cast<EnumData*>(lParam);
		DWORD windowPid = 0;
		if (!IsWindow(hwnd))
			return true;

		GetWindowThreadProcessId(hwnd, &windowPid);

		if (windowPid == pData->pid && IsWindowVisible(hwnd)) {
			std::string title = GetWindowTitle(hwnd);
			if (!title.empty()) {
				pData->hwnd = hwnd;
				return true;
			}
		}
		return true;
	};

	EnumWindows(enumProc, reinterpret_cast<LPARAM>(&data));
	return data.hwnd;
}

static std::string GetSpotifyTrackInfo()
{
	auto pids = GetProcessIdsByName(L"Spotify.exe");
	if (pids.empty())
		return "";

	for (auto pid : pids)
	{
		HWND hwnd = FindMainWindow(pid);
		if (hwnd)
		{
			std::string sTitle = GetWindowTitle(hwnd);
			if (sTitle.empty())
				continue;

			std::string sLowerTitle = sTitle;
			std::transform(sLowerTitle.begin(), sLowerTitle.end(), sLowerTitle.begin(), ::tolower);
			if (sLowerTitle.find("spotify") != std::string::npos)
				return "";

			return sLowerTitle;
		}
	}
	return "";
}

void CSpotifySong::Draw()
{
	static Timer tCheckName{};
	if (!(Vars::Menu::Indicators.Value & Vars::Menu::IndicatorsEnum::Spotify))
		return;

	static std::string sSpotifyTitle;
	if(tCheckName.Run(1.f))
		sSpotifyTitle = GetSpotifyTrackInfo();

	if (sSpotifyTitle.empty())
		return;

	const auto& fFont = H::Fonts.GetFont(FONT_INDICATORS);
	int x = Vars::Menu::SpotifySongDisplay.Value.x;
	int y = Vars::Menu::SpotifySongDisplay.Value.y;

	H::Draw.StringOutlined(fFont, x, y + 20, Vars::Menu::Theme::Accent.Value, Vars::Menu::Theme::Background.Value, ALIGN_CENTER, sSpotifyTitle.c_str());
}