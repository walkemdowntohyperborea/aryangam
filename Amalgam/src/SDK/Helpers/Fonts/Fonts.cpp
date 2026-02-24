#include "Fonts.h"
#include "../../Vars.h"

void CFonts::RemakeFonts()
{
	// variable fonts
	m_mFonts[FONT_ESP] = { Vars::Menu::Fonts::ESPName.Value.c_str(), Vars::Menu::Fonts::ESPSize.Value, Vars::Menu::Fonts::ESPFlags.Value, Vars::Menu::Fonts::ESPWeight.Value };
	m_mFonts[FONT_ESP_SMALL] = { Vars::Menu::Fonts::ESPSmallName.Value.c_str(), Vars::Menu::Fonts::ESPSmallSize.Value, Vars::Menu::Fonts::ESPSmallFlags.Value, Vars::Menu::Fonts::ESPSmallWeight.Value };
	m_mFonts[FONT_INDICATORS] = { Vars::Menu::Fonts::IndicatorsName.Value.c_str(), Vars::Menu::Fonts::IndicatorsSize.Value, Vars::Menu::Fonts::IndicatorsFlags.Value, Vars::Menu::Fonts::IndicatorsWeight.Value };

	// global fonts
	m_mFonts[FONT_RIJIN_DT] = { "Small Fonts", 9, FONTFLAG_NONE, 0 };
	m_mFonts[FONT_RIJIN] = { "Segoe UI", 16, FONTFLAG_ANTIALIAS, 400 };
	m_mFonts[FONT_RIJIN_MISC] = { "Tahoma", 13, FONTFLAG_NONE, 0 };
	m_mFonts[FONT_RIJIN_OLD] = { "Fixedsys", 10, FONTFLAG_NONE, 0 };
	m_mFonts[FONT_ATERIS] = { "Small Fonts", 11, FONTFLAG_NONE, 0 }; // also used for deadflag (same font)

	ReloadFonts();
}


void CFonts::ReloadFonts()
{
	for (auto& [_, fFont] : m_mFonts)
	{
		/*
		if (!fFont.m_dwFont || fFont.m_szName == nullptr)
		{
			RemakeFonts();
			continue;
		}*/

		if (fFont.m_dwFont = I::MatSystemSurface->CreateFont())
			I::MatSystemSurface->SetFontGlyphSet(fFont.m_dwFont, fFont.m_szName, fFont.m_nTall, fFont.m_nWeight, 0, 0, fFont.m_nFlags);
	}
}

const Font_t& CFonts::GetFont(EFonts eFont)
{
	return m_mFonts[eFont];
}