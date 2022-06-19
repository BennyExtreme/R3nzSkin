#pragma warning(disable : 26451)

#include <fstream>
#include <string>

#include "Json/json.hpp"
#include "fnv_hash.hpp"
#include "imgui/imgui.h"

#include "SDK/Spell.hpp"
#include "SDK/SpellSlot.hpp"

#include "CheatManager.hpp"
#include "GUI.hpp"
#include "Memory.hpp"
#include "SkinDatabase.hpp"
#include "Utils.hpp"

__forceinline static void footer() noexcept
{
	ImGui::Separator();
	ImGui::textUnformattedCentered((std::string("Last Build: ") + __DATE__ + " - " + __TIME__).c_str());
	ImGui::textUnformattedCentered("Copyright (C) 2021-2022 R3nzTheCodeGOD");
}

__forceinline static void infoText(const char* desc) noexcept
{
	if (ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void GUI::render() noexcept
{
	static const auto player{ cheatManager.memory->localPlayer };
	static const auto heroes{ cheatManager.memory->heroList };
	static const std::uintptr_t gametimePtr{ cheatManager.memory->getLeagueModule() + offsets::global::GameTime };
	static const auto my_team{ player ? player->get_team() : 100 };

	static const auto vector_getter_skin = [](void* vec, std::int32_t idx, const char** out_text) noexcept {
		const auto& vector{ *static_cast<std::vector<SkinDatabase::skin_info>*>(vec) };
		if (idx < 0 || idx > static_cast<std::int32_t>(vector.size())) return false;
		*out_text = idx == 0 ? "Default" : vector.at(idx - 1).skin_name.c_str();
		return true;
	};

	static const auto vector_getter_ward_skin = [](void* vec, std::int32_t idx, const char** out_text) noexcept {
		const auto& vector{ *static_cast<std::vector<std::pair<std::int32_t, std::string>>*>(vec) };
		if (idx < 0 || idx > static_cast<std::int32_t>(vector.size())) return false;
		*out_text = idx == 0 ? "Default" : vector.at(idx - 1).second.c_str();
		return true;
	};

	static auto vector_getter_default = [](void* vec, std::int32_t idx, const char** out_text) noexcept {
		const auto& vector{ *static_cast<std::vector<std::string>*>(vec) };
		if (idx < 0 || idx > static_cast<std::int32_t>(vector.size())) return false;
		*out_text = idx == 0 ? "Default" : vector.at(idx - 1).c_str();
		return true;
	};

	ImGui::Begin("R3nzSkin", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysAutoResize);
	{
		ImGui::rainbowText();
		if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll | ImGuiTabBarFlags_NoTooltip)) {
			if (player) {
				if (ImGui::BeginTabItem("LocalPlayer")) {
					auto& values{ cheatManager.database->champions_skins[fnv::hash_runtime(player->get_character_data_stack()->base_skin.model.str)] };
					ImGui::Text("Player Skins Settings:");

					if (ImGui::Combo("Current Skin", &cheatManager.config->current_combo_skin_index, vector_getter_skin, static_cast<void*>(&values), values.size() + 1))
						if (cheatManager.config->current_combo_skin_index > 0)
							player->change_skin(values[cheatManager.config->current_combo_skin_index - 1].model_name.c_str(), values[cheatManager.config->current_combo_skin_index - 1].skin_id);

					if (ImGui::Combo("Current Ward Skin", &cheatManager.config->current_combo_ward_index, vector_getter_ward_skin, static_cast<void*>(&cheatManager.database->wards_skins), cheatManager.database->wards_skins.size() + 1))
						cheatManager.config->current_ward_skin_index = cheatManager.config->current_combo_ward_index == 0 ? -1 : cheatManager.database->wards_skins.at(cheatManager.config->current_combo_ward_index - 1).first;
					footer();
					ImGui::EndTabItem();
				}
			}

			if (ImGui::BeginTabItem("Other Champs")) {
				ImGui::Text("Other Champs Skins Settings:");
				std::int32_t last_team{ 0 };
				for (auto i{ 0u }; i < heroes->length; ++i) {
					const auto hero{ heroes->list[i] };

					if (hero == player)
						continue;

					const auto champion_name_hash{ fnv::hash_runtime(hero->get_character_data_stack()->base_skin.model.str) };
					if (champion_name_hash == FNV("PracticeTool_TargetDummy"))
						continue;

					const auto hero_team{ hero->get_team() };
					const auto is_enemy{ hero_team != my_team };

					if (last_team == 0 || hero_team != last_team) {
						if (last_team != 0)
							ImGui::Separator();
						if (is_enemy)
							ImGui::Text(" Enemy champions");
						else
							ImGui::Text(" Ally champions");
						last_team = hero_team;
					}

					auto& config_array{ is_enemy ? cheatManager.config->current_combo_enemy_skin_index : cheatManager.config->current_combo_ally_skin_index };
					const auto config_entry{ config_array.insert({ champion_name_hash, 0 }) };

					snprintf(this->str_buffer, sizeof(this->str_buffer), cheatManager.config->heroName ? "HeroName: [ %s ]##%X" : "PlayerName: [ %s ]##%X", cheatManager.config->heroName ? hero->get_character_data_stack()->base_skin.model.str : hero->get_name().c_str(), reinterpret_cast<std::uintptr_t>(hero));

					auto& values{ cheatManager.database->champions_skins[champion_name_hash] };
					if (ImGui::Combo(str_buffer, &config_entry.first->second, vector_getter_skin, static_cast<void*>(&values), values.size() + 1))
						if (config_entry.first->second > 0)
							hero->change_skin(values[config_entry.first->second - 1].model_name.c_str(), values[config_entry.first->second - 1].skin_id);
				}
				footer();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Global Skins")) {
				ImGui::Text("Global Skins Settings:");
				if (ImGui::Combo("Minion Skins:", &cheatManager.config->current_combo_minion_index, vector_getter_default, static_cast<void*>(&cheatManager.database->minions_skins), cheatManager.database->minions_skins.size() + 1))
					cheatManager.config->current_minion_skin_index = cheatManager.config->current_combo_minion_index - 1;
				ImGui::Separator();
				ImGui::Text("Jungle Mobs Skins Settings:");
				for (auto& it : cheatManager.database->jungle_mobs_skins) {
					snprintf(str_buffer, 256, "Current %s skin", it.name.c_str());
					const auto config_entry{ cheatManager.config->current_combo_jungle_mob_skin_index.insert({ it.name_hashes.front(),0 }) };
					if (ImGui::Combo(str_buffer, &config_entry.first->second, vector_getter_default, static_cast<void*>(&it.skins), it.skins.size() + 1))
						for (const auto& hash : it.name_hashes)
							cheatManager.config->current_combo_jungle_mob_skin_index[hash] = config_entry.first->second;
				}
				footer();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Spells Cooldowns")) {
				static const auto drawTimer = [](const SpellSlot* spell, const char slotName, const bool sameLine) noexcept
				{
					const float gametime{ *reinterpret_cast<float*>(gametimePtr) };

					if (spell->level <= 0)
						ImGui::TextColored(ImVec4(1, 0, 0, 1), " %c: Close", slotName);
					else if (spell->level > 0 && spell->time > gametime)
						ImGui::TextColored(ImVec4(1, 1, 0, 1), " %c: %.1f", slotName, static_cast<float>(spell->time - gametime));
					else
						ImGui::TextColored(ImVec4(0, 1, 0, 1), " %c: Ready", slotName);
					if (sameLine)
						ImGui::SameLine();
				};

				for (auto i{ 0u }; i < heroes->length; ++i) {
					const auto hero{ heroes->list[i] };

					// just draw cooldowns of enemy champions.
					if (hero == player || hero->get_team() == my_team)
						continue;

					ImGui::Text(hero->get_character_data_stack()->base_skin.model.str);
					drawTimer(hero->getSpellSlot(Spell::Q), 'Q', true);
					drawTimer(hero->getSpellSlot(Spell::W), 'W', true);
					drawTimer(hero->getSpellSlot(Spell::E), 'E', true);
					drawTimer(hero->getSpellSlot(Spell::R), 'R', false);
					drawTimer(hero->getSpellSlot(Spell::D), 'D', true);
					drawTimer(hero->getSpellSlot(Spell::F), 'F', false);
					ImGui::Separator();
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Extras")) {
				ImGui::hotkey("Menu Key", cheatManager.config->menuKey);
				ImGui::Checkbox(cheatManager.config->heroName ? "HeroName based" : "PlayerName based", &cheatManager.config->heroName);
				ImGui::Checkbox("Rainbow Text", &cheatManager.config->rainbowText);
				ImGui::Checkbox("Quick Skin Change", &cheatManager.config->quickSkinChange);
				infoText("It allows you to change skin without opening the menu with the key you assign from the keyboard.");

				if (cheatManager.config->quickSkinChange) {
					ImGui::Separator();
					ImGui::hotkey("Previous Skin Key", cheatManager.config->previousSkinKey);
					ImGui::hotkey("Next Skin Key", cheatManager.config->nextSkinKey);
					ImGui::Separator();
				}

				if (ImGui::Button("No Skins")) {
					cheatManager.config->current_combo_skin_index = 1;
					player->change_skin(player->get_character_data_stack()->base_skin.model.str, 0);

					for (auto& enemy : cheatManager.config->current_combo_enemy_skin_index)
						enemy.second = 1;

					for (auto& ally : cheatManager.config->current_combo_ally_skin_index)
						ally.second = 1;

					for (auto i{ 0u }; i < heroes->length; ++i) {
						const auto hero{ heroes->list[i] };
						hero->change_skin(hero->get_character_data_stack()->base_skin.model.str, 0);
					}
				} infoText("Defaults the skin of all champions.");

				if (ImGui::Button("Random Skins")) {
					for (auto i{ 0u }; i < heroes->length; ++i) {
						const auto hero{ heroes->list[i] };
						const auto championHash{ fnv::hash_runtime(hero->get_character_data_stack()->base_skin.model.str) };
						const auto skinCount{ cheatManager.database->champions_skins[championHash].size() };
						auto& skinDatabase{ cheatManager.database->champions_skins[championHash] };
						auto& config{ (hero->get_team() != my_team) ? cheatManager.config->current_combo_enemy_skin_index : cheatManager.config->current_combo_ally_skin_index };

						if (championHash == FNV("PracticeTool_TargetDummy"))
							continue;

						if (hero == player) {
							cheatManager.config->current_combo_skin_index = random(1u, skinCount);
							hero->change_skin(skinDatabase[cheatManager.config->current_combo_skin_index - 1].model_name.c_str(), skinDatabase[cheatManager.config->current_combo_skin_index - 1].skin_id);
						}
						else {
							auto& data{ config[championHash] };
							data = random(1u, skinCount);
							hero->change_skin(skinDatabase[data - 1].model_name.c_str(), skinDatabase[data - 1].skin_id);
						}
					}
				} infoText("Randomly changes the skin of all champions.");

				if (ImGui::Button("Force Close"))
					cheatManager.hooks->uninstall();
				infoText("You will be returned to the reconnect screen.");
				ImGui::Text("FPS: %.0f FPS", ImGui::GetIO().Framerate);
				footer();
				ImGui::EndTabItem();
			}
		}
	}
	ImGui::End();
}
