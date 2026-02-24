#pragma once
#include "../../../Utils/Macros/Macros.h"
#include "../../Definitions/Interfaces/IMatSystemSurface.h"
#include <unordered_map>

enum EFonts
{
	FONT_INDICATORS,
	FONT_ESP,
	FONT_ESP_SMALL,
	FONT_RIJIN_DT,
	FONT_RIJIN,
	FONT_RIJIN_MISC,
	FONT_RIJIN_OLD,
	FONT_ATERIS
};

struct Font_t
{
	const char* m_szName;
	int m_nTall, m_nFlags, m_nWeight;
	unsigned long m_dwFont;
};

class CFonts
{
private:
	std::unordered_map<EFonts, Font_t> m_mFonts = {};

public:
	void RemakeFonts();
	void ReloadFonts();
	const Font_t& GetFont(EFonts eFont);
};

ADD_FEATURE_CUSTOM(CFonts, Fonts, H);