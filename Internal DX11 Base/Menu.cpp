#include "pch.h"
#include "Engine.h"
#include "Menu.h"

#include "Features/ESP.h"
#include "Features/MagicBullet.h"
#include "Features/Misc.h"
#include "Features/Noclip.h"
#include "Game/Game.h"
#include "Utils/Settings.h"
#include "Utils/SettingsManager.h"
#include <algorithm>

const char* GetTargetModeName() {
    switch (g_Settings.Targeting.Mode) {
        case ETargetingMode::FOV: return "FOV";
        case ETargetingMode::SmartFOV: return "Smart FOV";
        case ETargetingMode::Distance: return "Distance";
        case ETargetingMode::Health: return "Health";
        default: return "Unknown";
    }
}

const char* GetKeyName(int key) {
    switch (key) {
        case VK_LBUTTON: return "Left Mouse";
        case VK_RBUTTON: return "Right Mouse";
        case VK_MBUTTON: return "Middle Mouse";
        case VK_XBUTTON1: return "Mouse 4";
        case VK_XBUTTON2: return "Mouse 5";
        case VK_LSHIFT: return "Left Shift";
        case VK_RSHIFT: return "Right Shift";
        case VK_LCONTROL: return "Left Ctrl";
        case VK_RCONTROL: return "Right Ctrl";
        case VK_LMENU: return "Left Alt";
        case VK_RMENU: return "Right Alt";
        case VK_SPACE: return "Space";
        case VK_TAB: return "Tab";
        case VK_CAPITAL: return "Caps Lock";
        case VK_RETURN: return "Enter";
        case VK_BACK: return "Backspace";
        case VK_DELETE: return "Delete";
        case VK_INSERT: return "Insert";
        case VK_HOME: return "Home";
        case VK_END: return "End";
        case VK_PRIOR: return "Page Up";
        case VK_NEXT: return "Page Down";
        case VK_UP: return "Up Arrow";
        case VK_DOWN: return "Down Arrow";
        case VK_LEFT: return "Left Arrow";
        case VK_RIGHT: return "Right Arrow";
        case VK_ESCAPE: return "Escape";
        case VK_LWIN: return "Left Win";
        case VK_RWIN: return "Right Win";
        case VK_APPS: return "Menu Key";
        case VK_SNAPSHOT: return "Print Screen";
        case VK_SCROLL: return "Scroll Lock";
        case VK_PAUSE: return "Pause";
        case VK_NUMLOCK: return "Num Lock";
        case VK_DIVIDE: return "Num /";
        case VK_MULTIPLY: return "Num *";
        case VK_SUBTRACT: return "Num -";
        case VK_ADD: return "Num +";
        case VK_DECIMAL: return "Num .";
        case VK_NUMPAD0: return "Num 0";
        case VK_NUMPAD1: return "Num 1";
        case VK_NUMPAD2: return "Num 2";
        case VK_NUMPAD3: return "Num 3";
        case VK_NUMPAD4: return "Num 4";
        case VK_NUMPAD5: return "Num 5";
        case VK_NUMPAD6: return "Num 6";
        case VK_NUMPAD7: return "Num 7";
        case VK_NUMPAD8: return "Num 8";
        case VK_NUMPAD9: return "Num 9";
        default:
            if (key >= 'A' && key <= 'Z') {
                static char keyName[2] = {0};
                keyName[0] = (char)key;
                return keyName;
            }
            if (key >= '0' && key <= '9') {
                static char keyName[2] = {0};
                keyName[0] = (char)key;
                return keyName;
            }
            if (key >= VK_F1 && key <= VK_F12) {
                static char keyName[4] = {0};
                sprintf_s(keyName, "F%d", key - VK_F1 + 1);
                return keyName;
            }
            if (key >= VK_F13 && key <= VK_F24) {
                static char keyName[5] = {0};
                sprintf_s(keyName, "F%d", key - VK_F1 + 1);
                return keyName;
            }
            return "Unknown";
    }
}

ImGuiWindowFlags Flags = 0;

namespace DX11Base {

	void Menu::Render() 
	{
		if (g_Engine->bShowMenu)
		{
			DrawMenu();
		}
	}

	void Menu::DrawMenu() 
	{
		// Set initial window size and position for better first-time experience
		static bool firstTime = true;
		if (firstTime) {
			ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
			firstTime = false;
		}
		
		if (ImGui::Begin("Planetside 2 Cheat", NULL, Flags))
		{
			if (ImGui::BeginTabBar("MainTabs")) {
				
				// ESP Tab
				if (ImGui::BeginTabItem("ESP")) {
					ImGui::Checkbox("Enable ESP", &g_Settings.ESP.bEnabled);
					ImGui::Separator();
					
					ImGui::Text("ESP Features:");
					ImGui::Checkbox("Boxes", &g_Settings.ESP.bBoxes);
					ImGui::Checkbox("Skeletons", &g_Settings.ESP.bSkeletons);
					ImGui::Checkbox("Health Bars", &g_Settings.ESP.bHealthBars);
					ImGui::Checkbox("Tracers", &g_Settings.ESP.bTracers);
					ImGui::Checkbox("Names", &g_Settings.ESP.bNames);
					ImGui::Checkbox("Show Type", &g_Settings.ESP.bShowType);
					ImGui::Checkbox("Distance", &g_Settings.ESP.bShowDistance);
					ImGui::Checkbox("Bullet ESP", &g_Settings.ESP.bBulletESP);
					ImGui::Checkbox("Team ESP", &g_Settings.ESP.bTeamESP);
					ImGui::Checkbox("View Direction", &g_Settings.ESP.bViewDirection);
					ImGui::Separator();
					ImGui::Text("Entity Filter:");
					ImGui::Checkbox("Infantry", &g_Settings.ESP.bShowInfantry);
					ImGui::SameLine(); ImGui::Checkbox("MAX", &g_Settings.ESP.bShowMAX);
					ImGui::Checkbox("Ground Vehicles", &g_Settings.ESP.bShowGroundVehicles);
					ImGui::Checkbox("Air Vehicles", &g_Settings.ESP.bShowAirVehicles);
					ImGui::Checkbox("Turrets", &g_Settings.ESP.bShowTurrets);
					ImGui::Checkbox("Others (Mines/Utility)", &g_Settings.ESP.bShowOthers);

					ImGui::Separator();
					ImGui::SliderFloat("Max Distance", &g_Settings.ESP.fMaxDistance, 50.0f, 1000.0f, "%.0f m");
					
					
					ImGui::SeparatorText("Team Colors");
					ImGui::ColorEdit4("Vanu Sovereignty", g_Settings.ESP.Colors.VS);
					ImGui::ColorEdit4("New Conglomerate", g_Settings.ESP.Colors.NC);
					ImGui::ColorEdit4("Terran Republic", g_Settings.ESP.Colors.TR);
					ImGui::ColorEdit4("Nanite Systems", g_Settings.ESP.Colors.NSO);
					
					ImGui::EndTabItem();
				}
				
				// Targeting Tab
				if (ImGui::BeginTabItem("Targeting")) {
					ImGui::Checkbox("Enable Targeting System", &g_Settings.Targeting.bEnabled);
					ImGui::Separator();
					
					ImGui::Text("Targeting Mode:");
					int currentMode = static_cast<int>(g_Settings.Targeting.Mode);
					
					if (ImGui::RadioButton("FOV", &currentMode, 0)) {
						g_Settings.Targeting.Mode = ETargetingMode::FOV;
					}
					ImGui::SameLine();
					ImGui::TextDisabled("(?)");
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Targets closest enemy to crosshair within FOV");
					}
					
					if (ImGui::RadioButton("Smart FOV", &currentMode, 1)) {
						g_Settings.Targeting.Mode = ETargetingMode::SmartFOV;
					}
					ImGui::SameLine();
					ImGui::TextDisabled("(?)");
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("From enemies in FOV, targets closest to YOU.\nPrevents targeting through enemies.");
					}
					
					if (ImGui::RadioButton("Distance (360°)", &currentMode, 2)) {
						g_Settings.Targeting.Mode = ETargetingMode::Distance;
					}
					ImGui::SameLine();
					ImGui::TextDisabled("(?)");
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Rage mode. Targets absolute closest enemy.");
					}
					
					if (ImGui::RadioButton("Health (360°)", &currentMode, 3)) {
						g_Settings.Targeting.Mode = ETargetingMode::Health;
					}
					ImGui::SameLine();
					ImGui::TextDisabled("(?)");
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Targets enemy with lowest health + shield.");
					}
					
					ImGui::Separator();
					ImGui::Text("Settings:");
					
					if (g_Settings.Targeting.Mode == ETargetingMode::FOV || 
						g_Settings.Targeting.Mode == ETargetingMode::SmartFOV) {
						ImGui::SliderFloat("FOV Radius", &g_Settings.Targeting.fFOV, 10.0f, 500.0f, "%.0f px");
					}
					
					ImGui::SliderFloat("Max Distance", &g_Settings.Targeting.fMaxDistance, 50.0f, 1000.0f, "%.0f m");
					ImGui::SameLine();
					ImGui::TextDisabled("(?)");
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Maximum distance for targeting (entities beyond this won't be targeted)");
					}
					
					ImGui::Separator();
					ImGui::Text("Target Filters:");
					ImGui::Checkbox("Target Team", &g_Settings.Targeting.bTargetTeam);
					ImGui::SameLine();
					ImGui::TextDisabled("(?)");
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Allow targeting of teammates (normally only enemies are targeted)");
					}
					
					ImGui::Checkbox("Ignore MAX Units", &g_Settings.Targeting.bIgnoreMaxUnits);
					ImGui::SameLine();
					ImGui::TextDisabled("(?)");
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Don't target MAX units (Heavy Assault MAX)");
					}
					
					ImGui::Checkbox("Ignore Vehicles", &g_Settings.Targeting.bIgnoreVehicles);
					ImGui::SameLine();
					ImGui::TextDisabled("(?)");
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Don't target vehicles (tanks, aircraft, etc.)");
					}
					
					ImGui::Checkbox("Don't Target NS", &g_Settings.Targeting.bIgnoreNS);
					ImGui::SameLine();
					ImGui::TextDisabled("(?)");
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Don't target NSO (Nanite Systems) players");
					}
					
					ImGui::Separator();
					ImGui::Text("Target Priority:");
					ImGui::Checkbox("Target Specific Player", &g_Settings.Targeting.bTargetName);
					ImGui::SameLine();
					ImGui::TextDisabled("(?)");
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Prioritize targeting a specific player by name");
					}
					
					if (g_Settings.Targeting.bTargetName) {
						ImGui::Indent();
						
					// Input field for player name
					static char playerNameBuffer[256] = "";
					
					// Initialize buffer with current value on first time
					static bool bufferInitialized = false;
					if (!bufferInitialized) {
						strncpy_s(playerNameBuffer, sizeof(playerNameBuffer), g_Settings.Targeting.sTargetName.c_str(), _TRUNCATE);
						bufferInitialized = true;
					}
					
					if (ImGui::InputText("Player Name (contains)", playerNameBuffer, sizeof(playerNameBuffer))) {
						g_Settings.Targeting.sTargetName = std::string(playerNameBuffer);
					}
					
					// Continuous search checkbox
					ImGui::Checkbox("Continuous Search", &g_Settings.Targeting.bContinuousSearch);
					ImGui::SameLine();
					ImGui::TextDisabled("(?)");
					if (ImGui::IsItemHovered()) {
						ImGui::SetTooltip("Keep searching for the player even if not currently visible");
					}
					
					// Show available players (suggestions)
					if (!g_Settings.Targeting.availablePlayers.empty()) {
						ImGui::Text("Available Players (%d):", (int)g_Settings.Targeting.availablePlayers.size());
						ImGui::Indent();
						
						// Show only players that contain the search term
						std::string searchLower = g_Settings.Targeting.sTargetName;
						std::transform(searchLower.begin(), searchLower.end(), searchLower.begin(), ::tolower);
						
						int matchingPlayers = 0;
						for (const auto& player : g_Settings.Targeting.availablePlayers) {
							std::string playerLower = player;
							std::transform(playerLower.begin(), playerLower.end(), playerLower.begin(), ::tolower);
							
							if (searchLower.empty() || playerLower.find(searchLower) != std::string::npos) {
								ImGui::Text("• %s", player.c_str());
								matchingPlayers++;
							}
						}
						
						if (matchingPlayers == 0 && !searchLower.empty()) {
							ImGui::TextColored(ImVec4(1, 1, 0, 1), "No matching players found");
						}
						
						ImGui::Unindent();
					} else {
						ImGui::Text("No players available");
					}
						
						ImGui::Unindent();
					}
					
					ImGui::Separator();
					ImGui::Text("Visualization:");
					ImGui::Checkbox("Show Target Info Panel", &g_Settings.Targeting.bShowTargetInfo);
					ImGui::Checkbox("Show Tracer to Target", &g_Settings.Targeting.bShowTracer);
					ImGui::Checkbox("Highlight Target in ESP", &g_Settings.ESP.bHighlightTarget);
					
					ImGui::EndTabItem();
				}
				
				
				// Magic Bullet Tab
				if (ImGui::BeginTabItem("Magic Bullet")) {
					ImGui::TextColored(ImVec4(1,0,0,1), "WARNING: High detection risk!");
					ImGui::Separator();
					
					ImGui::Checkbox("Enable Magic Bullet", &g_Settings.MagicBullet.bEnabled);
					ImGui::Separator();
                    ImGui::Separator();
					if (g_MagicBullet) {
						ImGui::Text("Active Bullets: %d", g_MagicBullet->GetActiveBullets());
						ImGui::Text("Update Time: %.3fms", g_MagicBullet->GetUpdateTime());
					} else {
						ImGui::Text("Active Bullets: 0 (Disabled)");
						ImGui::Text("Update Time: 0.000ms (Disabled)");
					}
					
					ImGui::EndTabItem();
				}
				
				// Misc Tab
				if (ImGui::BeginTabItem("Misc")) {
					ImGui::Text("Visual Features:");
					ImGui::Checkbox("Show FPS", &g_Settings.Misc.bShowFPS);
					ImGui::Checkbox("Show FOV Circle", &g_Settings.Misc.bShowFOVCircle);
					ImGui::Checkbox("Show Crosshair", &g_Settings.Misc.bShowCrosshair);
					
					ImGui::Separator();
					ImGui::Text("Game Features:");
					ImGui::Checkbox("No Recoil", &g_Settings.Misc.NoRecoil.bEnabled);
					if (g_Settings.Misc.NoRecoil.bEnabled) {
						ImGui::SliderInt("Strength", &g_Settings.Misc.NoRecoil.iStrength, 1, 100, "%d");
					}

					ImGui::Separator();
					ImGui::Text("Radar/Minimap:");
					ImGui::Checkbox("Show Radar (top-right)", &g_Settings.Misc.bShowRadar);
					if (g_Settings.Misc.bShowRadar) {
						ImGui::SliderFloat("Size", &g_Settings.Misc.fRadarSize, 120.0f, 300.0f, "%.0f px");
						ImGui::SliderFloat("Zoom", &g_Settings.Misc.fRadarZoom, 0.5f, 3.0f, "%.1f x");
					}
					
					ImGui::EndTabItem();
				}
				
				// Aimbot Tab
				if (ImGui::BeginTabItem("Aimbot")) {
					ImGui::Checkbox("Enable Aimbot", &g_Settings.Aimbot.bEnabled);
					ImGui::Separator();
					
					ImGui::Text("Targeting Mode:");
					ImGui::TextDisabled("Uses the global targeting system");
					ImGui::Text("Current: %s", GetTargetModeName());
					
					ImGui::Separator();
					ImGui::Text("Smoothing:");
					ImGui::SliderFloat("Smoothness", &g_Settings.Aimbot.fSmoothing, 1.0f, 20.0f, "%.1f");
					
					ImGui::Separator();
					ImGui::Text("Activation:");
					ImGui::Checkbox("Auto Aim (when shooting)", &g_Settings.Aimbot.bAutoAim);
					ImGui::Checkbox("Use Hotkey", &g_Settings.Aimbot.bUseHotkey);
					if (g_Settings.Aimbot.bUseHotkey) {
						// Hotkey selector
						ImGui::Text("Hotkey: %s", GetKeyName(g_Settings.Aimbot.iHotkey));
						if (ImGui::Button("Change Hotkey")) {
							g_Settings.Aimbot.bWaitingForHotkey = true;
						}
						
						if (g_Settings.Aimbot.bWaitingForHotkey) {
							ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Press any key...");
							// Check for any key press
							for (int i = 1; i < 256; i++) {
								if (GetAsyncKeyState(i) & 0x8000) {
									g_Settings.Aimbot.iHotkey = i;
									g_Settings.Aimbot.bWaitingForHotkey = false;
									break;
								}
							}
						}
					}
					
					ImGui::EndTabItem();
				}
				
				// Noclip Tab
				if (ImGui::BeginTabItem("Noclip")) {
					ImGui::TextColored(ImVec4(1,1,0,1), "Use at your own risk!");
					ImGui::Separator();
					
					ImGui::Checkbox("Enable Noclip/Flight", &g_Settings.Noclip.bEnabled);
					ImGui::Separator();
					
					ImGui::Text("Activation:");
					ImGui::Checkbox("Use Hotkey", &g_Settings.Noclip.bUseHotkey);
					if (g_Settings.Noclip.bUseHotkey) {
						// Hotkey selector
						ImGui::Text("Hotkey: %s", GetKeyName(g_Settings.Noclip.iHotkey));
						if (ImGui::Button("Change Hotkey")) {
							g_Settings.Noclip.bWaitingForHotkey = true;
						}
						
						if (g_Settings.Noclip.bWaitingForHotkey) {
							ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Press any key...");
							// Check for any key press
							for (int i = 1; i < 256; i++) {
								if (GetAsyncKeyState(i) & 0x8000) {
									g_Settings.Noclip.iHotkey = i;
									g_Settings.Noclip.bWaitingForHotkey = false;
									break;
								}
							}
						}
					}
					
					ImGui::Separator();
					ImGui::Text("Controls:");
					ImGui::BulletText("W/A/S/D - Move Forward/Left/Backward/Right");
					ImGui::BulletText("CAPSLOCK - Move Up");
					ImGui::BulletText("SHIFT - Move Down");
					
					ImGui::Separator();
					ImGui::SliderFloat("Movement Speed", &g_Settings.Noclip.fSpeed, 0.01f, 2.0f, "%.1f");
					
					ImGui::Separator();
					ImGui::TextColored(ImVec4(0,1,0,1), "Direct Memory Access:");
					ImGui::Text("Reading/Writing Double values directly");
					ImGui::Text("No coordinate conversion needed!");
					
					ImGui::Separator();
					ImGui::Text("Current Position:");
					if (g_Noclip) {
						auto worldPos = g_Noclip->GetWorldPosition();
						if (worldPos.x != 0 || worldPos.y != 0 || worldPos.z != 0) {
							ImGui::Text("Direct Memory: %.2f, %.2f, %.2f", worldPos.x, worldPos.y, worldPos.z);
						} else {
							ImGui::TextDisabled("Failed to read position");
						}
					} else {
						ImGui::TextDisabled("Noclip not initialized");
					}
					
					ImGui::EndTabItem();
				}
				
				// Info Tab
				if (ImGui::BeginTabItem("Info")) {
					ImGui::Text("Planetside 2 DX11 Internal Cheat");
					ImGui::Text("Version: 1.0.0");
					ImGui::Separator();
					
					ImGui::Text("Controls:");
					ImGui::Text("[INSERT] - Toggle Menu");
					ImGui::Text("[END] - Unload Cheat");
					ImGui::Separator();
					
					ImGui::Text("Performance:");
					if (g_Game) {
						ImGui::Text("Entities: %d", g_Game->GetEntityCount());
						ImGui::Text("Game Update: %.2f ms", g_Game->GetUpdateTime());
						
						// LocalPlayer position display
						auto worldSnapshot = g_Game->GetWorldSnapshot();
						if (worldSnapshot->localPlayer.isValid && worldSnapshot->localPlayer.isAlive) {
							ImGui::Separator();
							ImGui::Text("LocalPlayer Position:");
							ImGui::Text("X: %.2f", worldSnapshot->localPlayer.position.x);
							ImGui::Text("Y: %.2f", worldSnapshot->localPlayer.position.y);
							ImGui::Text("Z: %.2f", worldSnapshot->localPlayer.position.z);
						} else {
							ImGui::Text("LocalPlayer: Not Alive");
						}
					}
					if (g_ESP) {
						ImGui::Text("ESP Render: %.2f ms", g_ESP->GetRenderTime());
						ImGui::Text("ESP Entities: %d", g_ESP->GetRenderedEntities());
					}
					if (g_Misc) {
						ImGui::Text("FPS: %.1f", g_Misc->GetFPS());
					}
					
					ImGui::Separator();
					ImGui::Text("Settings Management:");
					if(ImGui::Button("Save Settings", ImVec2(150, 30))) {
						SettingsManager::SaveSettings();
					}
					ImGui::SameLine();
					if(ImGui::Button("Load Settings", ImVec2(150, 30))) {
						SettingsManager::LoadSettings();
					}
					if(ImGui::Button("Reset to Defaults", ImVec2(150, 30))) {
						SettingsManager::ResetToDefaults();
					}
					ImGui::SameLine();
					if(ImGui::Button("Uninject DLL", ImVec2(150, 30))) {
						g_Running = false;
					}
					
					ImGui::EndTabItem();
				}
				
				ImGui::EndTabBar();
			}
			ImGui::End();
		}
	}

	void Menu::Loops()
	{

	}
}