#ifndef GAME_EDITOR_PROOF_MODE_H
#define GAME_EDITOR_PROOF_MODE_H

#include "component.h"

#include <base/vmath.h>

#include <vector>

class CProofMode : public CEditorComponent
{
public:
	void OnInit(CEditor *pEditor) override;
	void OnReset() override;
	void OnMapLoad() override;
	void RenderScreenSizes();

	bool IsEnabled() const;
	bool IsModeIngame() const;
	void Toggle();
	void SetModeIngame();

private:
	enum class EProofBorder
	{
		OFF,
		INGAME,
		MENU,
	};
	EProofBorder m_ProofBorders;
};

#endif
