/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "menus.h"
#include "motd.h"
#include "voting.h"

#include <base/color.h>
#include <base/math.h>
#include <base/system.h>

#include <engine/console.h>
#include <engine/favorites.h>
#include <engine/font_icons.h>
#include <engine/friends.h>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <engine/shared/localization.h>
#include <engine/storage.h>
#include <engine/textrender.h>

#include <generated/client_data.h>
#include <generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/components/countryflags.h>
#include <game/client/components/touch_controls.h>
#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <game/client/ui_listbox.h>
#include <game/client/ui_scrollregion.h>
#include <game/localization.h>

#include <chrono>

using namespace std::chrono_literals;

void CMenus::RenderGame(CUIRect MainView)
{
	CUIRect Button, ButtonBars, ButtonBar, ButtonBar2;
	bool ShowDDRaceButtons = MainView.w > 855.0f;
	MainView.HSplitTop(45.0f + (g_Config.m_ClTouchControls ? 35.0f : 0.0f), &ButtonBars, &MainView);
	ButtonBars.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);
	ButtonBars.Margin(10.0f, &ButtonBars);
	ButtonBars.HSplitTop(25.0f, &ButtonBar, &ButtonBars);
	if(g_Config.m_ClTouchControls)
	{
		ButtonBars.HSplitTop(10.0f, nullptr, &ButtonBars);
		ButtonBars.HSplitTop(25.0f, &ButtonBar2, &ButtonBars);
	}

	ButtonBar.VSplitRight(120.0f, &ButtonBar, &Button);
	static CButtonContainer s_DisconnectButton;
	if(DoButton_Menu(&s_DisconnectButton, Localize("Disconnect"), 0, &Button))
	{
		if((GameClient()->CurrentRaceTime() / 60 >= g_Config.m_ClConfirmDisconnectTime && g_Config.m_ClConfirmDisconnectTime >= 0) ||
			GameClient()->m_TouchControls.HasEditingChanges() ||
			GameClient()->m_Menus.m_MenusIngameTouchControls.UnsavedChanges())
		{
			char aBuf[256] = {'\0'};
			if(GameClient()->CurrentRaceTime() / 60 >= g_Config.m_ClConfirmDisconnectTime && g_Config.m_ClConfirmDisconnectTime >= 0)
			{
				str_copy(aBuf, Localize("Are you sure that you want to disconnect?"));
			}
			if(GameClient()->m_TouchControls.HasEditingChanges() ||
				GameClient()->m_Menus.m_MenusIngameTouchControls.UnsavedChanges())
			{
				if(aBuf[0] != '\0')
				{
					str_append(aBuf, "\n\n");
				}
				str_append(aBuf, Localize("There's an unsaved change in the touch controls editor, you might want to save it."));
			}
			PopupConfirm(Localize("Disconnect"), aBuf, Localize("Yes"), Localize("No"), &CMenus::PopupConfirmDisconnect);
		}
		else
		{
			Client()->Disconnect();
			RefreshBrowserTab(true);
		}
	}

	ButtonBar.VSplitRight(5.0f, &ButtonBar, nullptr);
	ButtonBar.VSplitRight(170.0f, &ButtonBar, &Button);

	static CButtonContainer s_DummyButton;
	if(!Client()->DummyAllowed())
	{
		DoButton_Menu(&s_DummyButton, Localize("Connect Dummy"), 1, &Button);
		GameClient()->m_Tooltips.DoToolTip(&s_DummyButton, &Button, Localize("Dummy is not allowed on this server"));
	}
	else if(Client()->DummyConnectingDelayed())
	{
		DoButton_Menu(&s_DummyButton, Localize("Connect Dummy"), 1, &Button);
		GameClient()->m_Tooltips.DoToolTip(&s_DummyButton, &Button, Localize("Please wait…"));
	}
	else if(Client()->DummyConnecting())
	{
		DoButton_Menu(&s_DummyButton, Localize("Connecting dummy"), 1, &Button);
	}
	else if(DoButton_Menu(&s_DummyButton, Client()->DummyConnected() ? Localize("Disconnect Dummy") : Localize("Connect Dummy"), 0, &Button))
	{
		if(!Client()->DummyConnected())
		{
			Client()->DummyConnect();
		}
		else
		{
			if(GameClient()->CurrentRaceTime() / 60 >= g_Config.m_ClConfirmDisconnectTime && g_Config.m_ClConfirmDisconnectTime >= 0)
			{
				PopupConfirm(Localize("Disconnect Dummy"), Localize("Are you sure that you want to disconnect your dummy?"), Localize("Yes"), Localize("No"), &CMenus::PopupConfirmDisconnectDummy);
			}
			else
			{
				Client()->DummyDisconnect(nullptr);
				SetActive(false);
			}
		}
	}

	bool Paused = false;
	bool Spec = false;
	if(GameClient()->m_Snap.m_LocalClientId >= 0)
	{
		Paused = GameClient()->m_aClients[GameClient()->m_Snap.m_LocalClientId].m_Paused;
		Spec = GameClient()->m_aClients[GameClient()->m_Snap.m_LocalClientId].m_Spec;
	}

	if(GameClient()->m_Snap.m_pLocalInfo && GameClient()->m_Snap.m_pGameInfoObj && !Paused && !Spec)
	{
		if(GameClient()->m_Snap.m_pLocalInfo->m_Team != TEAM_SPECTATORS)
		{
			ButtonBar.VSplitLeft(120.0f, &Button, &ButtonBar);
			ButtonBar.VSplitLeft(5.0f, nullptr, &ButtonBar);
			static CButtonContainer s_SpectateButton;
			if(!Client()->DummyConnecting() && DoButton_Menu(&s_SpectateButton, Localize("Spectate"), 0, &Button))
			{
				if(g_Config.m_ClDummy == 0 || Client()->DummyConnected())
				{
					GameClient()->SendSwitchTeam(TEAM_SPECTATORS);
					SetActive(false);
				}
			}
		}

		if(GameClient()->IsTeamPlay())
		{
			if(GameClient()->m_Snap.m_pLocalInfo->m_Team != TEAM_RED)
			{
				ButtonBar.VSplitLeft(100.0f, &Button, &ButtonBar);
				ButtonBar.VSplitLeft(5.0f, nullptr, &ButtonBar);
				static CButtonContainer s_JoinRedButton;
				if(!Client()->DummyConnecting() && DoButton_Menu(&s_JoinRedButton, Localize("Join red"), 0, &Button))
				{
					GameClient()->SendSwitchTeam(TEAM_RED);
					SetActive(false);
				}
			}

			if(GameClient()->m_Snap.m_pLocalInfo->m_Team != TEAM_BLUE)
			{
				ButtonBar.VSplitLeft(100.0f, &Button, &ButtonBar);
				ButtonBar.VSplitLeft(5.0f, nullptr, &ButtonBar);
				static CButtonContainer s_JoinBlueButton;
				if(!Client()->DummyConnecting() && DoButton_Menu(&s_JoinBlueButton, Localize("Join blue"), 0, &Button))
				{
					GameClient()->SendSwitchTeam(TEAM_BLUE);
					SetActive(false);
				}
			}
		}
		else
		{
			if(GameClient()->m_Snap.m_pLocalInfo->m_Team != TEAM_GAME)
			{
				ButtonBar.VSplitLeft(120.0f, &Button, &ButtonBar);
				ButtonBar.VSplitLeft(5.0f, nullptr, &ButtonBar);
				static CButtonContainer s_JoinGameButton;
				if(!Client()->DummyConnecting() && DoButton_Menu(&s_JoinGameButton, Localize("Join game"), 0, &Button))
				{
					GameClient()->SendSwitchTeam(TEAM_GAME);
					SetActive(false);
				}
			}
		}

		if(GameClient()->m_Snap.m_pLocalInfo->m_Team != TEAM_SPECTATORS && (ShowDDRaceButtons || !GameClient()->IsTeamPlay()))
		{
			ButtonBar.VSplitLeft(65.0f, &Button, &ButtonBar);
			ButtonBar.VSplitLeft(5.0f, nullptr, &ButtonBar);

			static CButtonContainer s_KillButton;
			if(DoButton_Menu(&s_KillButton, Localize("Kill"), 0, &Button))
			{
				GameClient()->SendKill();
				SetActive(false);
			}
		}
	}

	if(GameClient()->m_ReceivedDDNetPlayer && GameClient()->m_Snap.m_pLocalInfo && (ShowDDRaceButtons || !GameClient()->IsTeamPlay()))
	{
		if(GameClient()->m_Snap.m_pLocalInfo->m_Team != TEAM_SPECTATORS || Paused || Spec)
		{
			ButtonBar.VSplitLeft((!Paused && !Spec) ? 65.0f : 120.0f, &Button, &ButtonBar);
			ButtonBar.VSplitLeft(5.0f, nullptr, &ButtonBar);

			static CButtonContainer s_PauseButton;
			if(DoButton_Menu(&s_PauseButton, (!Paused && !Spec) ? Localize("Pause") : Localize("Join game"), 0, &Button))
			{
				Console()->ExecuteLine("say /pause", IConsole::CLIENT_ID_UNSPECIFIED);
				SetActive(false);
			}
		}
	}

	if(GameClient()->m_Snap.m_pLocalInfo && (GameClient()->m_Snap.m_pLocalInfo->m_Team == TEAM_SPECTATORS || Paused || Spec))
	{
		ButtonBar.VSplitLeft(32.0f, &Button, &ButtonBar);
		ButtonBar.VSplitLeft(5.0f, nullptr, &ButtonBar);

		static CButtonContainer s_AutoCameraButton;

		bool Active = GameClient()->m_Camera.m_AutoSpecCamera && GameClient()->m_Camera.SpectatingPlayer() && GameClient()->m_Camera.CanUseAutoSpecCamera();
		bool Enabled = g_Config.m_ClSpecAutoSync;
		if(Ui()->DoButton_FontIcon(&s_AutoCameraButton, FontIcon::CAMERA, !Active, &Button, BUTTONFLAG_LEFT, IGraphics::CORNER_ALL, Enabled))
		{
			GameClient()->m_Camera.ToggleAutoSpecCamera();
		}
		GameClient()->m_Camera.UpdateAutoSpecCameraTooltip();
		GameClient()->m_Tooltips.DoToolTip(&s_AutoCameraButton, &Button, GameClient()->m_Camera.AutoSpecCameraTooltip());
	}

	if(g_Config.m_ClTouchControls)
	{
		ButtonBar2.VSplitLeft(200.0f, &Button, &ButtonBar2);
		static char s_TouchControlsEditCheckbox;
		if(DoButton_CheckBox(&s_TouchControlsEditCheckbox, Localize("Edit touch controls"), GameClient()->m_TouchControls.IsEditingActive(), &Button))
		{
			if(GameClient()->m_TouchControls.IsEditingActive() && m_MenusIngameTouchControls.UnsavedChanges())
			{
				m_MenusIngameTouchControls.m_pOldSelectedButton = GameClient()->m_TouchControls.SelectedButton();
				m_MenusIngameTouchControls.m_pNewSelectedButton = nullptr;
				PopupConfirm(Localize("Unsaved changes"), Localize("Save all changes before turning off the editor?"), Localize("Save"), Localize("Cancel"), &CMenus::PopupConfirmTurnOffEditor);
			}
			else
			{
				GameClient()->m_TouchControls.SetEditingActive(!GameClient()->m_TouchControls.IsEditingActive());
				if(GameClient()->m_TouchControls.IsEditingActive())
				{
					GameClient()->m_TouchControls.ResetVirtualVisibilities();
					m_MenusIngameTouchControls.m_EditElement = CMenusIngameTouchControls::EElementType::LAYOUT;
				}
				else
				{
					m_MenusIngameTouchControls.ResetButtonPointers();
				}
			}
		}

		ButtonBar2.VSplitRight(80.0f, &ButtonBar2, &Button);
		static CButtonContainer s_CloseButton;
		if(DoButton_Menu(&s_CloseButton, Localize("Close"), 0, &Button))
		{
			SetActive(false);
		}

		ButtonBar2.VSplitRight(5.0f, &ButtonBar2, nullptr);
		ButtonBar2.VSplitRight(160.0f, &ButtonBar2, &Button);
		static CButtonContainer s_RemoveConsoleButton;
		if(DoButton_Menu(&s_RemoveConsoleButton, Localize("Remote console"), 0, &Button))
		{
			Console()->ExecuteLine("toggle_remote_console", IConsole::CLIENT_ID_UNSPECIFIED);
		}

		ButtonBar2.VSplitRight(5.0f, &ButtonBar2, nullptr);
		ButtonBar2.VSplitRight(120.0f, &ButtonBar2, &Button);
		static CButtonContainer s_LocalConsoleButton;
		if(DoButton_Menu(&s_LocalConsoleButton, Localize("Console"), 0, &Button))
		{
			Console()->ExecuteLine("toggle_local_console", IConsole::CLIENT_ID_UNSPECIFIED);
		}
		// Only when these are all false, the preview page is rendered. Once the page is not rendered, update is needed upon next rendering.
		if(!GameClient()->m_TouchControls.IsEditingActive() || m_MenusIngameTouchControls.m_CurrentMenu != CMenusIngameTouchControls::EMenuType::MENU_BUTTONS || GameClient()->m_TouchControls.IsButtonEditing())
			m_MenusIngameTouchControls.m_NeedUpdatePreview = true;
		// Quit preview all buttons automatically.
		if(!GameClient()->m_TouchControls.IsEditingActive() || m_MenusIngameTouchControls.m_CurrentMenu != CMenusIngameTouchControls::EMenuType::MENU_PREVIEW)
			GameClient()->m_TouchControls.SetPreviewAllButtons(false);
		if(GameClient()->m_TouchControls.IsEditingActive())
		{
			// Resolve issues if needed before rendering, so the elements could have a correct value on this frame.
			// Issues need to be resolved before popup. So CheckCachedSettings could not be bad.
			m_MenusIngameTouchControls.ResolveIssues();
			// Do Popups if needed.
			CTouchControls::CPopupParam PopupParam = GameClient()->m_TouchControls.RequiredPopup();
			if(PopupParam.m_PopupType != CTouchControls::EPopupType::NUM_POPUPS)
			{
				m_MenusIngameTouchControls.DoPopupType(PopupParam);
				return;
			}
			if(m_MenusIngameTouchControls.m_FirstEnter)
			{
				m_MenusIngameTouchControls.m_ColorActive = color_cast<ColorHSLA>(GameClient()->m_TouchControls.BackgroundColorActive()).Pack(true);
				m_MenusIngameTouchControls.m_ColorInactive = color_cast<ColorHSLA>(GameClient()->m_TouchControls.BackgroundColorInactive()).Pack(true);
				m_MenusIngameTouchControls.m_FirstEnter = false;
			}
			// Their width is all 505.0f, height is adjustable, you can directly change its h value, so no need for changing where tab is.
			CUIRect SelectingTab;
			MainView.HSplitTop(40.0f, nullptr, &MainView);
			MainView.VMargin((MainView.w - CMenusIngameTouchControls::BUTTON_EDITOR_WIDTH) / 2.0f, &MainView);
			MainView.HSplitTop(25.0f, &SelectingTab, &MainView);

			m_MenusIngameTouchControls.RenderSelectingTab(SelectingTab);
			switch(m_MenusIngameTouchControls.m_CurrentMenu)
			{
			case CMenusIngameTouchControls::EMenuType::MENU_FILE: m_MenusIngameTouchControls.RenderTouchControlsEditor(MainView); break;
			case CMenusIngameTouchControls::EMenuType::MENU_BUTTONS: m_MenusIngameTouchControls.RenderTouchButtonEditor(MainView); break;
			case CMenusIngameTouchControls::EMenuType::MENU_SETTINGS: m_MenusIngameTouchControls.RenderConfigSettings(MainView); break;
			case CMenusIngameTouchControls::EMenuType::MENU_PREVIEW: m_MenusIngameTouchControls.RenderPreviewSettings(MainView); break;
			default: dbg_assert_failed("Unknown selected tab value = %d.", (int)m_MenusIngameTouchControls.m_CurrentMenu);
			}
		}
	}
}

void CMenus::PopupConfirmDisconnect()
{
	Client()->Disconnect();
}

void CMenus::PopupConfirmDisconnectDummy()
{
	Client()->DummyDisconnect(nullptr);
	SetActive(false);
}

void CMenus::PopupConfirmDiscardTouchControlsChanges()
{
	if(GameClient()->m_TouchControls.LoadConfigurationFromFile(IStorage::TYPE_ALL))
	{
		m_MenusIngameTouchControls.m_ColorActive = color_cast<ColorHSLA>(GameClient()->m_TouchControls.BackgroundColorActive()).Pack(true);
		m_MenusIngameTouchControls.m_ColorInactive = color_cast<ColorHSLA>(GameClient()->m_TouchControls.BackgroundColorInactive()).Pack(true);
		GameClient()->m_TouchControls.SetEditingChanges(false);
	}
	else
	{
		SWarning Warning(Localize("Error loading touch controls"), Localize("Could not load touch controls from file. See local console for details."));
		Warning.m_AutoHide = false;
		Client()->AddWarning(Warning);
	}
}

void CMenus::PopupConfirmResetTouchControls()
{
	bool Success = false;
	for(int StorageType = IStorage::TYPE_SAVE + 1; StorageType < Storage()->NumPaths(); ++StorageType)
	{
		if(GameClient()->m_TouchControls.LoadConfigurationFromFile(StorageType))
		{
			Success = true;
			break;
		}
	}
	if(Success)
	{
		m_MenusIngameTouchControls.m_ColorActive = color_cast<ColorHSLA>(GameClient()->m_TouchControls.BackgroundColorActive()).Pack(true);
		m_MenusIngameTouchControls.m_ColorInactive = color_cast<ColorHSLA>(GameClient()->m_TouchControls.BackgroundColorInactive()).Pack(true);
		GameClient()->m_TouchControls.SetEditingChanges(true);
	}
	else
	{
		SWarning Warning(Localize("Error loading touch controls"), Localize("Could not load default touch controls from file. See local console for details."));
		Warning.m_AutoHide = false;
		Client()->AddWarning(Warning);
	}
}

void CMenus::PopupConfirmImportTouchControlsClipboard()
{
	if(GameClient()->m_TouchControls.LoadConfigurationFromClipboard())
	{
		m_MenusIngameTouchControls.m_ColorActive = color_cast<ColorHSLA>(GameClient()->m_TouchControls.BackgroundColorActive()).Pack(true);
		m_MenusIngameTouchControls.m_ColorInactive = color_cast<ColorHSLA>(GameClient()->m_TouchControls.BackgroundColorInactive()).Pack(true);
		GameClient()->m_TouchControls.SetEditingChanges(true);
	}
	else
	{
		SWarning Warning(Localize("Error loading touch controls"), Localize("Could not load touch controls from clipboard. See local console for details."));
		Warning.m_AutoHide = false;
		Client()->AddWarning(Warning);
	}
}

void CMenus::PopupConfirmDeleteButton()
{
	GameClient()->m_TouchControls.DeleteSelectedButton();
	m_MenusIngameTouchControls.ResetCachedSettings();
	GameClient()->m_TouchControls.SetEditingChanges(true);
}

void CMenus::PopupCancelDeselectButton()
{
	m_MenusIngameTouchControls.ResetButtonPointers();
	m_MenusIngameTouchControls.SetUnsavedChanges(false);
	m_MenusIngameTouchControls.ResetCachedSettings();
}

void CMenus::PopupConfirmSelectedNotVisible()
{
	if(m_MenusIngameTouchControls.UnsavedChanges())
	{
		// The m_pSelectedButton can't nullptr, because this function is triggered when selected button not visible.
		m_MenusIngameTouchControls.m_pOldSelectedButton = GameClient()->m_TouchControls.SelectedButton();
		m_MenusIngameTouchControls.m_pNewSelectedButton = nullptr;
		m_MenusIngameTouchControls.m_CloseMenu = true;
		m_MenusIngameTouchControls.ChangeSelectedButtonWhileHavingUnsavedChanges();
	}
	else
	{
		m_MenusIngameTouchControls.ResetButtonPointers();
		GameClient()->m_Menus.SetActive(false);
	}
}

void CMenus::PopupConfirmChangeSelectedButton()
{
	if(m_MenusIngameTouchControls.CheckCachedSettings())
	{
		GameClient()->m_TouchControls.SetSelectedButton(m_MenusIngameTouchControls.m_pNewSelectedButton);
		m_MenusIngameTouchControls.SaveCachedSettingsToTarget(m_MenusIngameTouchControls.m_pOldSelectedButton);
		// Update wild pointer.
		if(m_MenusIngameTouchControls.m_pNewSelectedButton != nullptr)
			m_MenusIngameTouchControls.m_pNewSelectedButton = GameClient()->m_TouchControls.SelectedButton();
		GameClient()->m_TouchControls.SetEditingChanges(true);
		m_MenusIngameTouchControls.SetUnsavedChanges(false);
		PopupCancelChangeSelectedButton();
	}
}

void CMenus::PopupCancelChangeSelectedButton()
{
	GameClient()->m_TouchControls.SetSelectedButton(m_MenusIngameTouchControls.m_pNewSelectedButton);
	m_MenusIngameTouchControls.CacheAllSettingsFromTarget(m_MenusIngameTouchControls.m_pNewSelectedButton);
	m_MenusIngameTouchControls.SetUnsavedChanges(false);
	if(m_MenusIngameTouchControls.m_pNewSelectedButton != nullptr)
	{
		m_MenusIngameTouchControls.UpdateSampleButton();
	}
	else
	{
		m_MenusIngameTouchControls.ResetButtonPointers();
	}
	if(m_MenusIngameTouchControls.m_CloseMenu)
		GameClient()->m_Menus.SetActive(false);
}

void CMenus::PopupConfirmTurnOffEditor()
{
	if(m_MenusIngameTouchControls.CheckCachedSettings())
	{
		m_MenusIngameTouchControls.SaveCachedSettingsToTarget(m_MenusIngameTouchControls.m_pOldSelectedButton);
		GameClient()->m_TouchControls.SetEditingActive(!GameClient()->m_TouchControls.IsEditingActive());
		m_MenusIngameTouchControls.ResetButtonPointers();
	}
}

void CMenus::PopupConfirmOpenWiki()
{
	Client()->ViewLink(Localize("https://wiki.ddnet.org/wiki/Touch_controls"));
}

void CMenus::RenderPlayers(CUIRect MainView)
{
	CUIRect Button, Button2, ButtonBar, PlayerList, Player;
	MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);

	// list background color
	MainView.Margin(10.0f, &PlayerList);
	PlayerList.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f), IGraphics::CORNER_ALL, 10.0f);
	PlayerList.Margin(10.0f, &PlayerList);

	// headline
	PlayerList.HSplitTop(34.0f, &ButtonBar, &PlayerList);
	ButtonBar.VSplitRight(231.0f, &Player, &ButtonBar);
	Ui()->DoLabel(&Player, Localize("Player"), 24.0f, TEXTALIGN_ML);

	ButtonBar.HMargin(1.0f, &ButtonBar);
	float Width = ButtonBar.h * 2.0f;
	ButtonBar.VSplitLeft(Width, &Button, &ButtonBar);
	RenderTools()->RenderIcon(IMAGE_GUIICONS, SPRITE_GUIICON_MUTE, &Button);

	ButtonBar.VSplitLeft(20.0f, nullptr, &ButtonBar);
	ButtonBar.VSplitLeft(Width, &Button, &ButtonBar);
	RenderTools()->RenderIcon(IMAGE_GUIICONS, SPRITE_GUIICON_EMOTICON_MUTE, &Button);

	ButtonBar.VSplitLeft(20.0f, nullptr, &ButtonBar);
	ButtonBar.VSplitLeft(Width, &Button, &ButtonBar);
	RenderTools()->RenderIcon(IMAGE_GUIICONS, SPRITE_GUIICON_FRIEND, &Button);

	int TotalPlayers = 0;
	for(const auto &pInfoByName : GameClient()->m_Snap.m_apInfoByName)
	{
		if(!pInfoByName)
			continue;

		int Index = pInfoByName->m_ClientId;

		if(Index == GameClient()->m_Snap.m_LocalClientId)
			continue;

		TotalPlayers++;
	}

	static CListBox s_ListBox;
	s_ListBox.DoStart(24.0f, TotalPlayers, 1, 3, -1, &PlayerList);

	// options
	static char s_aPlayerIds[MAX_CLIENTS][4] = {{0}};

	for(int i = 0, Count = 0; i < MAX_CLIENTS; ++i)
	{
		if(!GameClient()->m_Snap.m_apInfoByName[i])
			continue;

		int Index = GameClient()->m_Snap.m_apInfoByName[i]->m_ClientId;
		if(Index == GameClient()->m_Snap.m_LocalClientId)
			continue;

		CGameClient::CClientData &CurrentClient = GameClient()->m_aClients[Index];
		const CListboxItem Item = s_ListBox.DoNextItem(&CurrentClient);

		Count++;

		if(!Item.m_Visible)
			continue;

		CUIRect Row = Item.m_Rect;
		if(Count % 2 == 1)
			Row.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f), IGraphics::CORNER_ALL, 5.0f);
		Row.VSplitRight(s_ListBox.ScrollbarWidthMax() - s_ListBox.ScrollbarWidth(), &Row, nullptr);
		Row.VSplitRight(300.0f, &Player, &Row);

		// player info
		Player.VSplitLeft(28.0f, &Button, &Player);

		CTeeRenderInfo TeeInfo = CurrentClient.m_RenderInfo;
		TeeInfo.m_Size = Button.h;

		const CAnimState *pIdleState = CAnimState::GetIdle();
		vec2 OffsetToMid;
		CRenderTools::GetRenderTeeOffsetToRenderedTee(pIdleState, &TeeInfo, OffsetToMid);
		vec2 TeeRenderPos(Button.x + Button.h / 2, Button.y + Button.h / 2 + OffsetToMid.y);
		RenderTools()->RenderTee(pIdleState, &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), TeeRenderPos);
		Ui()->DoButtonLogic(&s_aPlayerIds[Index][3], 0, &Button, BUTTONFLAG_NONE);
		GameClient()->m_Tooltips.DoToolTip(&s_aPlayerIds[Index][3], &Button, CurrentClient.m_aSkinName);

		Player.HSplitTop(1.5f, nullptr, &Player);
		Player.VSplitMid(&Player, &Button);
		Row.VSplitRight(210.0f, &Button2, &Row);

		Ui()->DoLabel(&Player, CurrentClient.m_aName, 14.0f, TEXTALIGN_ML);
		Ui()->DoLabel(&Button, CurrentClient.m_aClan, 14.0f, TEXTALIGN_ML);

		GameClient()->m_CountryFlags.Render(CurrentClient.m_Country, ColorRGBA(1.0f, 1.0f, 1.0f, 0.5f),
			Button2.x, Button2.y + Button2.h / 2.0f - 0.75f * Button2.h / 2.0f, 1.5f * Button2.h, 0.75f * Button2.h);

		// ignore chat button
		Row.HMargin(2.0f, &Row);
		Row.VSplitLeft(Width, &Button, &Row);
		Button.VSplitLeft((Width - Button.h) / 4.0f, nullptr, &Button);
		Button.VSplitLeft(Button.h, &Button, nullptr);
		if(g_Config.m_ClShowChatFriends && !CurrentClient.m_Friend)
			DoButton_Toggle(&s_aPlayerIds[Index][0], 1, &Button, false);
		else if(DoButton_Toggle(&s_aPlayerIds[Index][0], CurrentClient.m_ChatIgnore, &Button, true))
			CurrentClient.m_ChatIgnore ^= 1;

		// ignore emoticon button
		Row.VSplitLeft(30.0f, nullptr, &Row);
		Row.VSplitLeft(Width, &Button, &Row);
		Button.VSplitLeft((Width - Button.h) / 4.0f, nullptr, &Button);
		Button.VSplitLeft(Button.h, &Button, nullptr);
		if(g_Config.m_ClShowChatFriends && !CurrentClient.m_Friend)
			DoButton_Toggle(&s_aPlayerIds[Index][1], 1, &Button, false);
		else if(DoButton_Toggle(&s_aPlayerIds[Index][1], CurrentClient.m_EmoticonIgnore, &Button, true))
			CurrentClient.m_EmoticonIgnore ^= 1;

		// friend button
		Row.VSplitLeft(10.0f, nullptr, &Row);
		Row.VSplitLeft(Width, &Button, &Row);
		Button.VSplitLeft((Width - Button.h) / 4.0f, nullptr, &Button);
		Button.VSplitLeft(Button.h, &Button, nullptr);
		if(DoButton_Toggle(&s_aPlayerIds[Index][2], CurrentClient.m_Friend, &Button, true))
		{
			if(CurrentClient.m_Friend)
				GameClient()->Friends()->RemoveFriend(CurrentClient.m_aName, CurrentClient.m_aClan);
			else
				GameClient()->Friends()->AddFriend(CurrentClient.m_aName, CurrentClient.m_aClan);

			GameClient()->Client()->ServerBrowserUpdate();
		}
	}

	s_ListBox.DoEnd();
}

void CMenus::RenderServerInfo(CUIRect MainView)
{
	const float FontSizeTitle = 32.0f;
	const float FontSizeBody = 20.0f;

	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);

	CUIRect ServerInfo, GameInfo, Motd;
	MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);
	MainView.Margin(10.0f, &MainView);
	MainView.HSplitMid(&ServerInfo, &Motd, 10.0f);
	ServerInfo.VSplitMid(&ServerInfo, &GameInfo, 10.0f);

	ServerInfo.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f), IGraphics::CORNER_ALL, 10.0f);
	ServerInfo.Margin(10.0f, &ServerInfo);

	CUIRect Label;
	ServerInfo.HSplitTop(FontSizeTitle, &Label, &ServerInfo);
	ServerInfo.HSplitTop(5.0f, nullptr, &ServerInfo);
	Ui()->DoLabel(&Label, Localize("Server info"), FontSizeTitle, TEXTALIGN_ML);

	ServerInfo.HSplitTop(FontSizeBody, &Label, &ServerInfo);
	ServerInfo.HSplitTop(FontSizeBody, nullptr, &ServerInfo);
	Ui()->DoLabel(&Label, CurrentServerInfo.m_aName, FontSizeBody, TEXTALIGN_ML);

	ServerInfo.HSplitTop(FontSizeBody, &Label, &ServerInfo);
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Address"), CurrentServerInfo.m_aAddress);
	Ui()->DoLabel(&Label, aBuf, FontSizeBody, TEXTALIGN_ML);

	if(GameClient()->m_Snap.m_pLocalInfo)
	{
		ServerInfo.HSplitTop(FontSizeBody, &Label, &ServerInfo);
		str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Ping"), GameClient()->m_Snap.m_pLocalInfo->m_Latency);
		Ui()->DoLabel(&Label, aBuf, FontSizeBody, TEXTALIGN_ML);
	}

	ServerInfo.HSplitTop(FontSizeBody, &Label, &ServerInfo);
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Version"), CurrentServerInfo.m_aVersion);
	Ui()->DoLabel(&Label, aBuf, FontSizeBody, TEXTALIGN_ML);

	ServerInfo.HSplitTop(FontSizeBody, &Label, &ServerInfo);
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Password"), CurrentServerInfo.m_Flags & SERVER_FLAG_PASSWORD ? Localize("Yes") : Localize("No"));
	Ui()->DoLabel(&Label, aBuf, FontSizeBody, TEXTALIGN_ML);

	const CCommunity *pCommunity = ServerBrowser()->Community(CurrentServerInfo.m_aCommunityId);
	if(pCommunity != nullptr)
	{
		ServerInfo.HSplitTop(FontSizeBody, &Label, &ServerInfo);
		str_format(aBuf, sizeof(aBuf), "%s:", Localize("Community"));
		Ui()->DoLabel(&Label, aBuf, FontSizeBody, TEXTALIGN_ML);

		const CCommunityIcon *pIcon = m_CommunityIcons.Find(pCommunity->Id());
		if(pIcon != nullptr)
		{
			Label.VSplitLeft(TextRender()->TextWidth(FontSizeBody, aBuf) + 8.0f, nullptr, &Label);
			Label.VSplitLeft(2.0f * Label.h, &Label, nullptr);
			m_CommunityIcons.Render(pIcon, Label, true);
			static char s_CommunityTooltipButtonId;
			Ui()->DoButtonLogic(&s_CommunityTooltipButtonId, 0, &Label, BUTTONFLAG_NONE);
			GameClient()->m_Tooltips.DoToolTip(&s_CommunityTooltipButtonId, &Label, pCommunity->Name());
		}
	}

	// copy info button
	{
		CUIRect Button;
		ServerInfo.HSplitBottom(20.0f, &ServerInfo, &Button);
		Button.VSplitRight(200.0f, &ServerInfo, &Button);
		static CButtonContainer s_CopyButton;
		if(DoButton_Menu(&s_CopyButton, Localize("Copy info"), 0, &Button))
		{
			char aInfo[256];
			str_format(
				aInfo,
				sizeof(aInfo),
				"%s\n"
				"Address: ddnet://%s\n"
				"My IGN: %s\n",
				CurrentServerInfo.m_aName,
				CurrentServerInfo.m_aAddress,
				Client()->PlayerName());
			Input()->SetClipboardText(aInfo);
		}
	}

	// favorite checkbox
	{
		CUIRect Button;
		TRISTATE IsFavorite = Favorites()->IsFavorite(CurrentServerInfo.m_aAddresses, CurrentServerInfo.m_NumAddresses);
		ServerInfo.HSplitBottom(20.0f, &ServerInfo, &Button);
		static int s_AddFavButton = 0;
		if(DoButton_CheckBox(&s_AddFavButton, Localize("Favorite"), IsFavorite != TRISTATE::NONE, &Button))
		{
			if(IsFavorite != TRISTATE::NONE)
				Favorites()->Remove(CurrentServerInfo.m_aAddresses, CurrentServerInfo.m_NumAddresses);
			else
				Favorites()->Add(CurrentServerInfo.m_aAddresses, CurrentServerInfo.m_NumAddresses);
		}
	}

	GameInfo.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f), IGraphics::CORNER_ALL, 10.0f);
	GameInfo.Margin(10.0f, &GameInfo);

	GameInfo.HSplitTop(FontSizeTitle, &Label, &GameInfo);
	GameInfo.HSplitTop(5.0f, nullptr, &GameInfo);
	Ui()->DoLabel(&Label, Localize("Game info"), FontSizeTitle, TEXTALIGN_ML);

	GameInfo.HSplitTop(FontSizeBody, &Label, &GameInfo);
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Game type"), CurrentServerInfo.m_aGameType);
	Ui()->DoLabel(&Label, aBuf, FontSizeBody, TEXTALIGN_ML);

	GameInfo.HSplitTop(FontSizeBody, &Label, &GameInfo);
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Map"), CurrentServerInfo.m_aMap);
	Ui()->DoLabel(&Label, aBuf, FontSizeBody, TEXTALIGN_ML);

	const auto *pGameInfoObj = GameClient()->m_Snap.m_pGameInfoObj;
	if(pGameInfoObj)
	{
		if(pGameInfoObj->m_ScoreLimit)
		{
			GameInfo.HSplitTop(FontSizeBody, &Label, &GameInfo);
			str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), pGameInfoObj->m_ScoreLimit);
			Ui()->DoLabel(&Label, aBuf, FontSizeBody, TEXTALIGN_ML);
		}

		if(pGameInfoObj->m_TimeLimit)
		{
			GameInfo.HSplitTop(FontSizeBody, &Label, &GameInfo);
			str_format(aBuf, sizeof(aBuf), Localize("Time limit: %d min"), pGameInfoObj->m_TimeLimit);
			Ui()->DoLabel(&Label, aBuf, FontSizeBody, TEXTALIGN_ML);
		}

		if(pGameInfoObj->m_RoundCurrent && pGameInfoObj->m_RoundNum)
		{
			GameInfo.HSplitTop(FontSizeBody, &Label, &GameInfo);
			str_format(aBuf, sizeof(aBuf), Localize("Round %d/%d"), pGameInfoObj->m_RoundCurrent, pGameInfoObj->m_RoundNum);
			Ui()->DoLabel(&Label, aBuf, FontSizeBody, TEXTALIGN_ML);
		}
	}

	if(GameClient()->m_GameInfo.m_DDRaceTeam)
	{
		const char *pTeamMode = nullptr;
		switch(Config()->m_SvTeam)
		{
		case SV_TEAM_FORBIDDEN:
			pTeamMode = Localize("forbidden", "Team status");
			break;
		case SV_TEAM_ALLOWED:
			if(g_Config.m_SvSoloServer)
				pTeamMode = Localize("solo", "Team status");
			else
				pTeamMode = Localize("allowed", "Team status");
			break;
		case SV_TEAM_MANDATORY:
			pTeamMode = Localize("required", "Team status");
			break;
		case SV_TEAM_FORCED_SOLO:
			pTeamMode = Localize("solo", "Team status");
			break;
		default:
			dbg_assert_failed("unknown team mode");
		}
		if((Config()->m_SvTeam == SV_TEAM_ALLOWED || Config()->m_SvTeam == SV_TEAM_MANDATORY) && (Config()->m_SvMinTeamSize != DefaultConfig::SvMinTeamSize || Config()->m_SvMaxTeamSize != DefaultConfig::SvMaxTeamSize))
		{
			if(Config()->m_SvMinTeamSize != DefaultConfig::SvMinTeamSize && Config()->m_SvMaxTeamSize != DefaultConfig::SvMaxTeamSize)
				str_format(aBuf, sizeof(aBuf), "%s: %s (%s %d, %s %d)", Localize("Teams"), pTeamMode, Localize("minimum", "Team size"), Config()->m_SvMinTeamSize, Localize("maximum", "Team size"), Config()->m_SvMaxTeamSize);
			else if(Config()->m_SvMinTeamSize != DefaultConfig::SvMinTeamSize)
				str_format(aBuf, sizeof(aBuf), "%s: %s (%s %d)", Localize("Teams"), pTeamMode, Localize("minimum", "Team size"), Config()->m_SvMinTeamSize);
			else
				str_format(aBuf, sizeof(aBuf), "%s: %s (%s %d)", Localize("Teams"), pTeamMode, Localize("maximum", "Team size"), Config()->m_SvMaxTeamSize);
		}
		else
			str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Teams"), pTeamMode);
		GameInfo.HSplitTop(FontSizeBody, &Label, &GameInfo);
		Ui()->DoLabel(&Label, aBuf, FontSizeBody, TEXTALIGN_ML);
	}

	GameInfo.HSplitTop(FontSizeBody, &Label, &GameInfo);
	str_format(aBuf, sizeof(aBuf), "%s: %d/%d", Localize("Players"), GameClient()->m_Snap.m_NumPlayers, CurrentServerInfo.m_MaxClients);
	Ui()->DoLabel(&Label, aBuf, FontSizeBody, TEXTALIGN_ML);

	RenderServerInfoMotd(Motd);
}

void CMenus::RenderServerInfoMotd(CUIRect Motd)
{
	const float MotdFontSize = 16.0f;
	Motd.Draw(ColorRGBA(1.0f, 1.0f, 1.0f, 0.25f), IGraphics::CORNER_ALL, 10.0f);
	Motd.Margin(10.0f, &Motd);

	CUIRect MotdHeader;
	Motd.HSplitTop(2.0f * MotdFontSize, &MotdHeader, &Motd);
	Motd.HSplitTop(5.0f, nullptr, &Motd);
	Ui()->DoLabel(&MotdHeader, Localize("MOTD"), 2.0f * MotdFontSize, TEXTALIGN_ML);

	if(!GameClient()->m_Motd.ServerMotd()[0])
		return;

	static CScrollRegion s_ScrollRegion;
	vec2 ScrollOffset(0.0f, 0.0f);
	CScrollRegionParams ScrollParams;
	ScrollParams.m_ScrollUnit = 5 * MotdFontSize;
	s_ScrollRegion.Begin(&Motd, &ScrollOffset, &ScrollParams);
	Motd.y += ScrollOffset.y;

	static float s_MotdHeight = 0.0f;
	static int64_t s_MotdLastUpdateTime = -1;
	if(!m_MotdTextContainerIndex.Valid() || s_MotdLastUpdateTime == -1 || s_MotdLastUpdateTime != GameClient()->m_Motd.ServerMotdUpdateTime())
	{
		CTextCursor Cursor;
		Cursor.m_FontSize = MotdFontSize;
		Cursor.m_LineWidth = Motd.w;
		TextRender()->RecreateTextContainer(m_MotdTextContainerIndex, &Cursor, GameClient()->m_Motd.ServerMotd());
		s_MotdHeight = Cursor.Height();
		s_MotdLastUpdateTime = GameClient()->m_Motd.ServerMotdUpdateTime();
	}

	CUIRect MotdTextArea;
	Motd.HSplitTop(s_MotdHeight, &MotdTextArea, &Motd);
	s_ScrollRegion.AddRect(MotdTextArea);

	if(m_MotdTextContainerIndex.Valid())
		TextRender()->RenderTextContainer(m_MotdTextContainerIndex, TextRender()->DefaultTextColor(), TextRender()->DefaultTextOutlineColor(), MotdTextArea.x, MotdTextArea.y);

	s_ScrollRegion.End();
}

bool CMenus::RenderServerControlServer(CUIRect MainView, bool UpdateScroll)
{
	CUIRect List = MainView;
	int NumVoteOptions = 0;
	int aIndices[MAX_VOTE_OPTIONS];
	int Selected = -1;
	int TotalShown = 0;

	int i = 0;
	for(const CVoteOptionClient *pOption = GameClient()->m_Voting.FirstOption(); pOption; pOption = pOption->m_pNext, i++)
	{
		if(!m_FilterInput.IsEmpty() && !str_utf8_find_nocase(pOption->m_aDescription, m_FilterInput.GetString()))
			continue;
		if(i == m_CallvoteSelectedOption)
			Selected = TotalShown;
		TotalShown++;
	}

	static CListBox s_ListBox;
	s_ListBox.DoStart(19.0f, TotalShown, 1, 3, Selected, &List);

	i = 0;
	for(const CVoteOptionClient *pOption = GameClient()->m_Voting.FirstOption(); pOption; pOption = pOption->m_pNext, i++)
	{
		if(!m_FilterInput.IsEmpty() && !str_utf8_find_nocase(pOption->m_aDescription, m_FilterInput.GetString()))
			continue;
		aIndices[NumVoteOptions] = i;
		NumVoteOptions++;

		const CListboxItem Item = s_ListBox.DoNextItem(pOption);
		if(!Item.m_Visible)
			continue;

		CUIRect Label;
		Item.m_Rect.VMargin(2.0f, &Label);
		Ui()->DoLabel(&Label, pOption->m_aDescription, 13.0f, TEXTALIGN_ML);
	}

	Selected = s_ListBox.DoEnd();
	if(UpdateScroll)
		s_ListBox.ScrollToSelected();
	m_CallvoteSelectedOption = Selected != -1 ? aIndices[Selected] : -1;
	return s_ListBox.WasItemActivated();
}

bool CMenus::RenderServerControlKick(CUIRect MainView, bool FilterSpectators, bool UpdateScroll)
{
	int NumOptions = 0;
	int Selected = -1;
	int aPlayerIds[MAX_CLIENTS];
	for(const auto &pInfoByName : GameClient()->m_Snap.m_apInfoByName)
	{
		if(!pInfoByName)
			continue;

		int Index = pInfoByName->m_ClientId;
		if(Index == GameClient()->m_Snap.m_LocalClientId || (FilterSpectators && pInfoByName->m_Team == TEAM_SPECTATORS))
			continue;

		if(!str_utf8_find_nocase(GameClient()->m_aClients[Index].m_aName, m_FilterInput.GetString()))
			continue;

		if(m_CallvoteSelectedPlayer == Index)
			Selected = NumOptions;
		aPlayerIds[NumOptions] = Index;
		NumOptions++;
	}

	static CListBox s_ListBox;
	s_ListBox.DoStart(24.0f, NumOptions, 1, 3, Selected, &MainView);

	for(int i = 0; i < NumOptions; i++)
	{
		const CListboxItem Item = s_ListBox.DoNextItem(&aPlayerIds[i]);
		if(!Item.m_Visible)
			continue;

		CUIRect TeeRect, Label;
		Item.m_Rect.VSplitLeft(Item.m_Rect.h, &TeeRect, &Label);

		CTeeRenderInfo TeeInfo = GameClient()->m_aClients[aPlayerIds[i]].m_RenderInfo;
		TeeInfo.m_Size = TeeRect.h;

		const CAnimState *pIdleState = CAnimState::GetIdle();
		vec2 OffsetToMid;
		CRenderTools::GetRenderTeeOffsetToRenderedTee(pIdleState, &TeeInfo, OffsetToMid);
		vec2 TeeRenderPos(TeeRect.x + TeeInfo.m_Size / 2, TeeRect.y + TeeInfo.m_Size / 2 + OffsetToMid.y);

		RenderTools()->RenderTee(pIdleState, &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), TeeRenderPos);

		Ui()->DoLabel(&Label, GameClient()->m_aClients[aPlayerIds[i]].m_aName, 16.0f, TEXTALIGN_ML);
	}

	Selected = s_ListBox.DoEnd();
	if(UpdateScroll)
		s_ListBox.ScrollToSelected();
	m_CallvoteSelectedPlayer = Selected != -1 ? aPlayerIds[Selected] : -1;
	return s_ListBox.WasItemActivated();
}

void CMenus::RenderServerControl(CUIRect MainView)
{
	enum class EServerControlTab
	{
		SETTINGS,
		KICKVOTE,
		SPECVOTE,
	};
	static EServerControlTab s_ControlPage = EServerControlTab::SETTINGS;

	// render background
	CUIRect Bottom, RconExtension, TabBar, Button;
	MainView.HSplitTop(20.0f, &Bottom, &MainView);
	Bottom.Draw(ms_ColorTabbarActive, IGraphics::CORNER_NONE, 0.0f);
	MainView.HSplitTop(20.0f, &TabBar, &MainView);
	MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);
	MainView.Margin(10.0f, &MainView);

	if(Client()->RconAuthed())
		MainView.HSplitBottom(90.0f, &MainView, &RconExtension);

	// tab bar
	TabBar.VSplitLeft(TabBar.w / 3, &Button, &TabBar);
	static CButtonContainer s_Button0;
	if(DoButton_MenuTab(&s_Button0, Localize("Change settings"), s_ControlPage == EServerControlTab::SETTINGS, &Button, IGraphics::CORNER_NONE))
		s_ControlPage = EServerControlTab::SETTINGS;

	TabBar.VSplitMid(&Button, &TabBar);
	static CButtonContainer s_Button1;
	if(DoButton_MenuTab(&s_Button1, Localize("Kick player"), s_ControlPage == EServerControlTab::KICKVOTE, &Button, IGraphics::CORNER_NONE))
		s_ControlPage = EServerControlTab::KICKVOTE;

	static CButtonContainer s_Button2;
	if(DoButton_MenuTab(&s_Button2, Localize("Move player to spectators"), s_ControlPage == EServerControlTab::SPECVOTE, &TabBar, IGraphics::CORNER_NONE))
		s_ControlPage = EServerControlTab::SPECVOTE;

	// render page
	MainView.HSplitBottom(ms_ButtonHeight + 5 * 2, &MainView, &Bottom);
	Bottom.HMargin(5.0f, &Bottom);
	Bottom.HSplitTop(5.0f, nullptr, &Bottom);

	// render quick search
	CUIRect QuickSearch;
	Bottom.VSplitLeft(5.0f, nullptr, &Bottom);
	Bottom.VSplitLeft(250.0f, &QuickSearch, &Bottom);
	if(m_ControlPageOpening)
	{
		m_ControlPageOpening = false;
		Ui()->SetActiveItem(&m_FilterInput);
		m_FilterInput.SelectAll();
	}
	bool Searching = Ui()->DoEditBox_Search(&m_FilterInput, &QuickSearch, 14.0f, !Ui()->IsPopupOpen() && !GameClient()->m_GameConsole.IsActive());

	// vote menu
	bool Call = false;
	if(s_ControlPage == EServerControlTab::SETTINGS)
		Call = RenderServerControlServer(MainView, Searching);
	else if(s_ControlPage == EServerControlTab::KICKVOTE)
		Call = RenderServerControlKick(MainView, false, Searching);
	else if(s_ControlPage == EServerControlTab::SPECVOTE)
		Call = RenderServerControlKick(MainView, true, Searching);

	// call vote
	Bottom.VSplitRight(10.0f, &Bottom, nullptr);
	Bottom.VSplitRight(120.0f, &Bottom, &Button);

	static CButtonContainer s_CallVoteButton;
	if(DoButton_Menu(&s_CallVoteButton, Localize("Call vote"), 0, &Button) || Call)
	{
		if(s_ControlPage == EServerControlTab::SETTINGS)
		{
			if(0 <= m_CallvoteSelectedOption && m_CallvoteSelectedOption < GameClient()->m_Voting.NumOptions())
			{
				GameClient()->m_Voting.CallvoteOption(m_CallvoteSelectedOption, m_CallvoteReasonInput.GetString());
				if(g_Config.m_UiCloseWindowAfterChangingSetting)
					SetActive(false);
			}
		}
		else if(s_ControlPage == EServerControlTab::KICKVOTE)
		{
			if(m_CallvoteSelectedPlayer >= 0 && m_CallvoteSelectedPlayer < MAX_CLIENTS &&
				GameClient()->m_Snap.m_apPlayerInfos[m_CallvoteSelectedPlayer])
			{
				GameClient()->m_Voting.CallvoteKick(m_CallvoteSelectedPlayer, m_CallvoteReasonInput.GetString());
				SetActive(false);
			}
		}
		else if(s_ControlPage == EServerControlTab::SPECVOTE)
		{
			if(m_CallvoteSelectedPlayer >= 0 && m_CallvoteSelectedPlayer < MAX_CLIENTS &&
				GameClient()->m_Snap.m_apPlayerInfos[m_CallvoteSelectedPlayer])
			{
				GameClient()->m_Voting.CallvoteSpectate(m_CallvoteSelectedPlayer, m_CallvoteReasonInput.GetString());
				SetActive(false);
			}
		}
		m_CallvoteReasonInput.Clear();
	}

	// render kick reason
	CUIRect Reason;
	Bottom.VSplitRight(20.0f, &Bottom, nullptr);
	Bottom.VSplitRight(200.0f, &Bottom, &Reason);
	const char *pLabel = Localize("Reason:");
	Ui()->DoLabel(&Reason, pLabel, 14.0f, TEXTALIGN_ML);
	float w = TextRender()->TextWidth(14.0f, pLabel, -1, -1.0f);
	Reason.VSplitLeft(w + 10.0f, nullptr, &Reason);
	if(Input()->KeyPress(KEY_R) && Input()->ModifierIsPressed())
	{
		Ui()->SetActiveItem(&m_CallvoteReasonInput);
		m_CallvoteReasonInput.SelectAll();
	}
	Ui()->DoEditBox(&m_CallvoteReasonInput, &Reason, 14.0f);

	// vote option loading indicator
	if(s_ControlPage == EServerControlTab::SETTINGS && GameClient()->m_Voting.IsReceivingOptions())
	{
		CUIRect Spinner, LoadingLabel;
		Bottom.VSplitLeft(20.0f, nullptr, &Bottom);
		Bottom.VSplitLeft(16.0f, &Spinner, &Bottom);
		Bottom.VSplitLeft(5.0f, nullptr, &Bottom);
		Bottom.VSplitRight(10.0f, &LoadingLabel, nullptr);
		Ui()->RenderProgressSpinner(Spinner.Center(), 8.0f);
		Ui()->DoLabel(&LoadingLabel, Localize("Loading…"), 14.0f, TEXTALIGN_ML);
	}

	// extended features (only available when authed in rcon)
	if(Client()->RconAuthed())
	{
		// background
		RconExtension.HSplitTop(10.0f, nullptr, &RconExtension);
		RconExtension.HSplitTop(20.0f, &Bottom, &RconExtension);
		RconExtension.HSplitTop(5.0f, nullptr, &RconExtension);

		// force vote
		Bottom.VSplitLeft(5.0f, nullptr, &Bottom);
		Bottom.VSplitLeft(120.0f, &Button, &Bottom);

		static CButtonContainer s_ForceVoteButton;
		if(DoButton_Menu(&s_ForceVoteButton, Localize("Force vote"), 0, &Button))
		{
			if(s_ControlPage == EServerControlTab::SETTINGS)
			{
				GameClient()->m_Voting.CallvoteOption(m_CallvoteSelectedOption, m_CallvoteReasonInput.GetString(), true);
			}
			else if(s_ControlPage == EServerControlTab::KICKVOTE)
			{
				if(m_CallvoteSelectedPlayer >= 0 && m_CallvoteSelectedPlayer < MAX_CLIENTS &&
					GameClient()->m_Snap.m_apPlayerInfos[m_CallvoteSelectedPlayer])
				{
					GameClient()->m_Voting.CallvoteKick(m_CallvoteSelectedPlayer, m_CallvoteReasonInput.GetString(), true);
					SetActive(false);
				}
			}
			else if(s_ControlPage == EServerControlTab::SPECVOTE)
			{
				if(m_CallvoteSelectedPlayer >= 0 && m_CallvoteSelectedPlayer < MAX_CLIENTS &&
					GameClient()->m_Snap.m_apPlayerInfos[m_CallvoteSelectedPlayer])
				{
					GameClient()->m_Voting.CallvoteSpectate(m_CallvoteSelectedPlayer, m_CallvoteReasonInput.GetString(), true);
					SetActive(false);
				}
			}
			m_CallvoteReasonInput.Clear();
		}

		if(s_ControlPage == EServerControlTab::SETTINGS)
		{
			// remove vote
			Bottom.VSplitRight(10.0f, &Bottom, nullptr);
			Bottom.VSplitRight(120.0f, nullptr, &Button);
			static CButtonContainer s_RemoveVoteButton;
			if(DoButton_Menu(&s_RemoveVoteButton, Localize("Remove"), 0, &Button))
				GameClient()->m_Voting.RemovevoteOption(m_CallvoteSelectedOption);

			// add vote
			RconExtension.HSplitTop(20.0f, &Bottom, &RconExtension);
			Bottom.VSplitLeft(5.0f, nullptr, &Bottom);
			Bottom.VSplitLeft(250.0f, &Button, &Bottom);
			Ui()->DoLabel(&Button, Localize("Vote description:"), 14.0f, TEXTALIGN_ML);

			Bottom.VSplitLeft(20.0f, nullptr, &Button);
			Ui()->DoLabel(&Button, Localize("Vote command:"), 14.0f, TEXTALIGN_ML);

			static CLineInputBuffered<VOTE_DESC_LENGTH> s_VoteDescriptionInput;
			static CLineInputBuffered<VOTE_CMD_LENGTH> s_VoteCommandInput;
			RconExtension.HSplitTop(20.0f, &Bottom, &RconExtension);
			Bottom.VSplitRight(10.0f, &Bottom, nullptr);
			Bottom.VSplitRight(120.0f, &Bottom, &Button);
			static CButtonContainer s_AddVoteButton;
			if(DoButton_Menu(&s_AddVoteButton, Localize("Add"), 0, &Button))
				if(!s_VoteDescriptionInput.IsEmpty() && !s_VoteCommandInput.IsEmpty())
					GameClient()->m_Voting.AddvoteOption(s_VoteDescriptionInput.GetString(), s_VoteCommandInput.GetString());

			Bottom.VSplitLeft(5.0f, nullptr, &Bottom);
			Bottom.VSplitLeft(250.0f, &Button, &Bottom);
			Ui()->DoEditBox(&s_VoteDescriptionInput, &Button, 14.0f);

			Bottom.VMargin(20.0f, &Button);
			Ui()->DoEditBox(&s_VoteCommandInput, &Button, 14.0f);
		}
	}
}

void CMenus::RenderInGameNetwork(CUIRect MainView)
{
	MainView.Draw(ms_ColorTabbarActive, IGraphics::CORNER_B, 10.0f);

	CUIRect TabBar, Button;
	MainView.HSplitTop(24.0f, &TabBar, &MainView);

	int NewPage = g_Config.m_UiPage;

	TextRender()->SetFontPreset(EFontPreset::ICON_FONT);
	TextRender()->SetRenderFlags(ETextRenderFlags::TEXT_RENDER_FLAG_ONLY_ADVANCE_WIDTH | ETextRenderFlags::TEXT_RENDER_FLAG_NO_X_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_Y_BEARING | ETextRenderFlags::TEXT_RENDER_FLAG_NO_PIXEL_ALIGNMENT | ETextRenderFlags::TEXT_RENDER_FLAG_NO_OVERSIZE);

	TabBar.VSplitLeft(75.0f, &Button, &TabBar);
	static CButtonContainer s_InternetButton;
	if(DoButton_MenuTab(&s_InternetButton, FontIcon::EARTH_AMERICAS, g_Config.m_UiPage == PAGE_INTERNET, &Button, IGraphics::CORNER_NONE))
	{
		NewPage = PAGE_INTERNET;
	}
	GameClient()->m_Tooltips.DoToolTip(&s_InternetButton, &Button, Localize("Internet"));

	TabBar.VSplitLeft(75.0f, &Button, &TabBar);
	static CButtonContainer s_LanButton;
	if(DoButton_MenuTab(&s_LanButton, FontIcon::NETWORK_WIRED, g_Config.m_UiPage == PAGE_LAN, &Button, IGraphics::CORNER_NONE))
	{
		NewPage = PAGE_LAN;
	}
	GameClient()->m_Tooltips.DoToolTip(&s_LanButton, &Button, Localize("LAN"));

	TabBar.VSplitLeft(75.0f, &Button, &TabBar);
	static CButtonContainer s_FavoritesButton;
	if(DoButton_MenuTab(&s_FavoritesButton, FontIcon::STAR, g_Config.m_UiPage == PAGE_FAVORITES, &Button, IGraphics::CORNER_NONE))
	{
		NewPage = PAGE_FAVORITES;
	}
	GameClient()->m_Tooltips.DoToolTip(&s_FavoritesButton, &Button, Localize("Favorites"));

	size_t FavoriteCommunityIndex = 0;
	static CButtonContainer s_aFavoriteCommunityButtons[5];
	static_assert(std::size(s_aFavoriteCommunityButtons) == (size_t)PAGE_FAVORITE_COMMUNITY_5 - PAGE_FAVORITE_COMMUNITY_1 + 1);
	for(const CCommunity *pCommunity : ServerBrowser()->FavoriteCommunities())
	{
		TabBar.VSplitLeft(75.0f, &Button, &TabBar);
		const int Page = PAGE_FAVORITE_COMMUNITY_1 + FavoriteCommunityIndex;
		if(DoButton_MenuTab(&s_aFavoriteCommunityButtons[FavoriteCommunityIndex], FontIcon::ELLIPSIS, g_Config.m_UiPage == Page, &Button, IGraphics::CORNER_NONE, nullptr, nullptr, nullptr, nullptr, 10.0f, m_CommunityIcons.Find(pCommunity->Id())))
		{
			NewPage = Page;
		}
		GameClient()->m_Tooltips.DoToolTip(&s_aFavoriteCommunityButtons[FavoriteCommunityIndex], &Button, pCommunity->Name());

		++FavoriteCommunityIndex;
		if(FavoriteCommunityIndex >= std::size(s_aFavoriteCommunityButtons))
			break;
	}

	TextRender()->SetRenderFlags(0);
	TextRender()->SetFontPreset(EFontPreset::DEFAULT_FONT);

	if(NewPage != g_Config.m_UiPage)
	{
		SetMenuPage(NewPage);
	}

	RenderServerbrowser(MainView);
}

void CMenus::RenderIngameHint()
{
	// With touch controls enabled there is a Close button in the menu and usually no Escape key available.
	if(g_Config.m_ClTouchControls)
		return;

	float Width = 300 * Graphics()->ScreenAspect();
	Graphics()->MapScreen(0, 0, Width, 300);
	TextRender()->TextColor(1, 1, 1, 1);
	TextRender()->Text(5, 280, 5, Localize("Menu opened. Press Esc key again to close menu."), -1.0f);
	Ui()->MapScreen();
}
