#include "proof_mode.h"

#include "editor.h"

void CProofMode::OnInit(CEditor *pEditor)
{
	CEditorComponent::OnInit(pEditor);
	OnReset();
}

void CProofMode::OnReset()
{
	m_ProofBorders = EProofBorder::OFF;
}

void CProofMode::OnMapLoad()
{
}

void CProofMode::RenderScreenSizes()
{
	const vec2 WorldOffset = Editor()->MapView()->GetWorldOffset();

	if(IsEnabled())
	{
		std::shared_ptr<CLayerGroup> pGameGroup = Map()->m_pGameGroup;
		pGameGroup->MapScreen();

		Graphics()->TextureClear();
		Graphics()->LinesBegin();

		float aLastPoints[4];
		float Start = 1.0f;
		float End = 16.0f / 9.0f;
		const int NumSteps = 20;
		for(int i = 0; i <= NumSteps; i++)
		{
			float aPoints[4];
			float Aspect = Start + (End - Start) * (i / (float)NumSteps);
			Graphics()->MapScreenToWorld(
				WorldOffset.x, WorldOffset.y,
				100.0f, 100.0f, 100.0f, 0.0f, 0.0f, Aspect, 1.0f, aPoints);

			if(i == 0)
			{
				IGraphics::CLineItem aArray[] = {
					IGraphics::CLineItem(aPoints[0], aPoints[1], aPoints[2], aPoints[1]),
					IGraphics::CLineItem(aPoints[0], aPoints[3], aPoints[2], aPoints[3])};
				Graphics()->LinesDraw(aArray, std::size(aArray));
			}

			if(i != 0)
			{
				IGraphics::CLineItem aArray[] = {
					IGraphics::CLineItem(aPoints[0], aPoints[1], aLastPoints[0], aLastPoints[1]),
					IGraphics::CLineItem(aPoints[2], aPoints[1], aLastPoints[2], aLastPoints[1]),
					IGraphics::CLineItem(aPoints[0], aPoints[3], aLastPoints[0], aLastPoints[3]),
					IGraphics::CLineItem(aPoints[2], aPoints[3], aLastPoints[2], aLastPoints[3])};
				Graphics()->LinesDraw(aArray, std::size(aArray));
			}

			if(i == NumSteps)
			{
				IGraphics::CLineItem aArray[] = {
					IGraphics::CLineItem(aPoints[0], aPoints[1], aPoints[0], aPoints[3]),
					IGraphics::CLineItem(aPoints[2], aPoints[1], aPoints[2], aPoints[3])};
				Graphics()->LinesDraw(aArray, std::size(aArray));
			}

			mem_copy(aLastPoints, aPoints, sizeof(aPoints));
		}
		Graphics()->LinesEnd();

		Graphics()->SetColor(1, 0, 0, 1);
		for(int Pass = 0; Pass < 2; Pass++)
		{
			float aPoints[4];
			const float aAspects[] = {4.0f / 3.0f, 16.0f / 10.0f, 5.0f / 4.0f, 16.0f / 9.0f};
			const ColorRGBA aColors[] = {ColorRGBA(1.0f, 0.0f, 0.0f, 1.0f), ColorRGBA(0.0f, 1.0f, 0.0f, 1.0f)};
			Graphics()->MapScreenToWorld(
				WorldOffset.x, WorldOffset.y,
				100.0f, 100.0f, 100.0f, 0.0f, 0.0f, aAspects[Pass], 1.0f, aPoints);

			CUIRect Rect;
			Rect.x = aPoints[0];
			Rect.y = aPoints[1];
			Rect.w = aPoints[2] - aPoints[0];
			Rect.h = aPoints[3] - aPoints[1];
			Rect.DrawOutline(aColors[Pass]);
		}

		Graphics()->TextureClear();
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0, 0, 1, 0.3f);
		Graphics()->DrawCircle(WorldOffset.x, WorldOffset.y - 3.0f, 20.0f, 32);
		Graphics()->QuadsEnd();
	}
}

bool CProofMode::IsEnabled() const
{
	return m_ProofBorders != EProofBorder::OFF;
}

bool CProofMode::IsModeIngame() const
{
	return m_ProofBorders == EProofBorder::INGAME;
}

void CProofMode::Toggle()
{
	m_ProofBorders = m_ProofBorders == EProofBorder::OFF ? EProofBorder::INGAME : EProofBorder::OFF;
}

void CProofMode::SetModeIngame()
{
	m_ProofBorders = EProofBorder::INGAME;
}
