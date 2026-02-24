#include "../SDK/SDK.h"

#include "../Features/Visuals/Materials/Materials.h"

MAKE_HOOK(IMatSystemSurface_OnScreenSizeChanged, U::Memory.GetVirtual(I::MatSystemSurface, 111), void,
	void* rcx, int nOldWidth, int nOldHeight)
{
	DEBUG_RETURN(rcx, nOldWidth, nOldHeight);

	CALL_ORIGINAL(rcx, nOldWidth, nOldHeight);

	H::Fonts.ReloadFonts();
	F::Materials.ReloadMaterials();
}