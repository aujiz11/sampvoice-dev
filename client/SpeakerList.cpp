/*
    This is a SampVoice project file
    Developer: CyberMor <cyber.mor.2020@gmail.ru>

    See more here https://github.com/CyberMor/sampvoice

    Copyright (c) Daniel (CyberMor) 2020 All rights reserved
*/

#include "SpeakerList.h"

#include <cassert>
#include <algorithm>

#include <game/CPed.h>
#include <game/CSprite.h>
#include <game/CCamera.h>
#include <util/ImGuiUtil.h>
#include <util/GameUtil.h>
#include <util/Logger.h>
#include <util/Render.h>

#include "PluginConfig.h"

bool SpeakerList::Init(IDirect3DDevice9* const pDevice, const AddressesBase& addrBase,
    const Resource& rSpeakerIcon, const Resource& rSpeakerFont) noexcept
{
    if (pDevice == nullptr)
        return false;

    if (SpeakerList::initStatus || !ImGuiUtil::IsInited())
        return false;

    try
    {
        SpeakerList::tSpeakerIcon = MakeTexture(pDevice, rSpeakerIcon);
    }
    catch (const std::exception& exception)
    {
        Logger::LogToFile("[sv:err:speakerlist:init] : failed to create speaker icon");
        SpeakerList::tSpeakerIcon.reset();
        return false;
    }

    Memory::ScopeExit iconResetScope { [] { SpeakerList::tSpeakerIcon.reset(); } };

    {
        float varFontSize { 0.f };

        if (!Render::ConvertBaseYValueToScreenYValue(kBaseFontSize, varFontSize))
        {
            Logger::LogToFile("[sv:err:speakerlist:init] : failed to convert font size");
            return false;
        }

        SpeakerList::pSpeakerFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(rSpeakerFont.GetDataPtr(),
            rSpeakerFont.GetDataSize(), varFontSize, nullptr, ImGui::GetIO().Fonts->GetGlyphRangesVietnamese());

        if (SpeakerList::pSpeakerFont == nullptr)
        {
            Logger::LogToFile("[sv:err:speakerlist:init] : failed to create speaker font");
            return false;
        }
    }

    if (!PluginConfig::IsSpeakerLoaded())
    {
        PluginConfig::SetSpeakerLoaded(true);
        SpeakerList::ResetConfigs();
    }

    iconResetScope.Release();

    SpeakerList::initStatus = true;
    SpeakerList::SyncConfigs();

    return true;
}

void SpeakerList::Free() noexcept
{
    if (!SpeakerList::initStatus)
        return;

    SpeakerList::tSpeakerIcon.reset();
    delete SpeakerList::pSpeakerFont;

    SpeakerList::initStatus = false;
}

void SpeakerList::Show() noexcept
{
    SpeakerList::showStatus = true;
}

bool SpeakerList::IsShowed() noexcept
{
    return SpeakerList::showStatus;
}

void SpeakerList::Hide() noexcept
{
    SpeakerList::showStatus = false;
}

void SpeakerList::Render()
{
    if (!SpeakerList::initStatus || !SpeakerList::showStatus)
        return;

    const auto pNetGame = SAMP::pNetGame();
    if (pNetGame == nullptr) return;

    const auto pPlayerPool = pNetGame->GetPlayerPool();
    if (pPlayerPool == nullptr) return;

    float vLeftIndent, vScrWidth, vScrHeight;

    if (!Render::ConvertBaseXValueToScreenXValue(kBaseLeftIndent, vLeftIndent)) return;
    if (!Render::GetScreenSize(vScrWidth, vScrHeight)) return;

    ImGuiUtil::BeginRender();

    const ImVec2 vWindowPadding { 4, 8 };
    const ImVec2 vFramePadding { 4, 8 };
    const ImVec2 vItemPadding { 4, 8 };

    ImGui::PushFont(SpeakerList::pSpeakerFont);

    const float vWidth = vScrWidth / 5.f - vLeftIndent;
    const float vHeight = 5.f * vWindowPadding.y + (kBaseLinesCount *
        (ImGui::GetTextLineHeight() + vFramePadding.y));

    const float vNickWidth = 0.5f * vWidth;
    const float vStreamsWidth = 0.20f * vWidth;

    const float vPosX = vLeftIndent;
    const float vPosY = (vScrHeight - vHeight) / 1.4f;

    ImGui::SetNextWindowPos({ vPosX, vPosY });
    ImGui::SetNextWindowSize({ vWidth, vHeight });

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, vFramePadding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, vWindowPadding);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, vItemPadding);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, vItemPadding);

    if (ImGui::Begin("speakerListWindow", nullptr,
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoInputs))
    {
        ImGui::Columns(2, nullptr, false);

        ImGui::SetColumnWidth(0, vNickWidth);
        ImGui::SetColumnWidth(1, vStreamsWidth);

        ImGui::SetColumnOffset(0, vWindowPadding.x);
        ImGui::SetColumnOffset(1, vWindowPadding.x + vNickWidth + vFramePadding.x);

        int curTextLine{ 0 };

        for (WORD playerId { 0 }; playerId < MAX_PLAYERS; ++playerId)
        {
            if (curTextLine >= kBaseLinesCount) break;

            bool dontRenderText{ true };
            
            if (const auto playerName = pPlayerPool->GetName(playerId); playerName != nullptr)
            {
                if (!SpeakerList::playerStreams[playerId].empty())
                {
                    for (const auto& playerStream : SpeakerList::playerStreams[playerId])
                    {
                        if (playerStream.second.GetType() == StreamType::LocalStreamAtPlayer)
                        {
                            if (GameUtil::IsPlayerVisible(playerId))
                            {
                                if (const auto pPlayer = pPlayerPool->GetPlayer(playerId); pPlayer != nullptr)
                                {
                                    if (const auto pPlayerPed = pPlayer->m_pPed; pPlayerPed != nullptr)
                                    {
                                        if (const auto pGamePed = pPlayerPed->m_pGamePed; pGamePed != nullptr)
                                        {
                                            /*const float distanceToCamera = (TheCamera.GetPosition() - pGamePed->GetPosition()).Magnitude();
						
						                    if (playerStream.second.GetType() == StreamType::LocalStreamAtPlayer)
                            			    {
                                                if (distanceToCamera > playerStream.second.GetDistance())
                                                {
                                                    dontRenderText = true;
                                                    break;
                                                }
                            			    }*/

                                            float vSpeakerIconSize { 0.f };

                                            if (Render::ConvertBaseYValueToScreenYValue(kBaseIconSize, vSpeakerIconSize))
                                            {
                                                const float distanceToCamera = (TheCamera.GetPosition() - pGamePed->GetPosition()).Magnitude();

                                                if (distanceToCamera < playerStream.second.GetDistance())
                                                {
                                                    dontRenderText = false;

                                                    vSpeakerIconSize *= PluginConfig::GetSpeakerIconScale();
                                                    vSpeakerIconSize *= 5 / distanceToCamera;

                                                    float width, height;
                                                    RwV3d playerPos, screenPos;

                                                    pGamePed->GetBonePosition(playerPos, 1, false);
                                                    playerPos.z += 1;

                                                    if (CSprite::CalcScreenCoors(playerPos, &screenPos, &width, &height, true, true))
                                                    {
                                                        screenPos.x -= vSpeakerIconSize / 2;
                                                        screenPos.y -= vSpeakerIconSize / 2;

                                                        const float addX = PluginConfig::GetSpeakerIconOffsetX() * 5 / distanceToCamera;
                                                        const float addY = PluginConfig::GetSpeakerIconOffsetY() * 5 / distanceToCamera;

                                                        SpeakerList::tSpeakerIcon->Draw(screenPos.x + addX, screenPos.y + addY,
                                                            vSpeakerIconSize, vSpeakerIconSize, -1, 0.f);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            break;
                        }
                    }

                    if (dontRenderText == true) continue;

                    ImGui::PushID(playerId);

                    const auto alphaLevel = static_cast<DWORD>((1.f - static_cast<float>(curTextLine) /
                        static_cast<float>(kBaseLinesCount)) * 255.f) << 24;

                    const auto color = ImGui::ColorConvertU32ToFloat4(0x00ffffff | alphaLevel);

                    ImGui::TextColored(color, "%s (%hu)", playerName, playerId);

                    ImGui::NextColumn();

                    for (const auto& streamInfo : SpeakerList::playerStreams[playerId])
                    {
                        if (streamInfo.second.GetColor() == NULL)
                            continue;
                        
                        if (streamInfo.second.GetType() != StreamType::LocalStreamAtPlayer)
                            continue;

                        ImGui::PushID(&streamInfo);
                        ImGui::Image(SpeakerList::tSpeakerIcon->GetTexture(), { ImGui::GetTextLineHeight(),
                            ImGui::GetTextLineHeight() });
                        ImGui::SameLine();
                        ImGui::PopID();
                    }

                    ImGui::NextColumn();
                    ImGui::PopID();

                    ++curTextLine;
                }
            }
        }

        ImGui::End();
    }

    ImGui::PopStyleVar(4);
    ImGui::PopFont();

    ImGuiUtil::EndRender();
}

int SpeakerList::GetSpeakerIconOffsetX() noexcept
{
    return PluginConfig::GetSpeakerIconOffsetX();
}

int SpeakerList::GetSpeakerIconOffsetY() noexcept
{
    return PluginConfig::GetSpeakerIconOffsetY();
}

float SpeakerList::GetSpeakerIconScale() noexcept
{
    return PluginConfig::GetSpeakerIconScale();
}

void SpeakerList::SetSpeakerIconOffsetX(const int speakerIconOffsetX) noexcept
{
    PluginConfig::SetSpeakerIconOffsetX(std::clamp(speakerIconOffsetX, -500, 500));
}

void SpeakerList::SetSpeakerIconOffsetY(const int speakerIconOffsetY) noexcept
{
    PluginConfig::SetSpeakerIconOffsetY(std::clamp(speakerIconOffsetY, -500, 500));
}

void SpeakerList::SetSpeakerIconScale(const float speakerIconScale) noexcept
{
    PluginConfig::SetSpeakerIconScale(std::clamp(speakerIconScale, 0.2f, 2.f));
}

void SpeakerList::SyncConfigs() noexcept
{
    SpeakerList::SetSpeakerIconOffsetX(PluginConfig::GetSpeakerIconOffsetX());
    SpeakerList::SetSpeakerIconOffsetY(PluginConfig::GetSpeakerIconOffsetY());
    SpeakerList::SetSpeakerIconScale(PluginConfig::GetSpeakerIconScale());
}

void SpeakerList::ResetConfigs() noexcept
{
    PluginConfig::SetSpeakerIconOffsetX(PluginConfig::kDefValSpeakerIconOffsetX);
    PluginConfig::SetSpeakerIconOffsetY(PluginConfig::kDefValSpeakerIconOffsetY);
    PluginConfig::SetSpeakerIconScale(PluginConfig::kDefValSpeakerIconScale);
}

void SpeakerList::OnSpeakerPlay(const Stream& stream, const WORD speaker) noexcept
{
    if (speaker != std::clamp<WORD>(speaker, 0, MAX_PLAYERS - 1))
        return;

    SpeakerList::playerStreams[speaker][(Stream*)(&stream)] = stream.GetInfo();
}

void SpeakerList::OnSpeakerStop(const Stream& stream, const WORD speaker) noexcept
{
    if (speaker != std::clamp<WORD>(speaker, 0, MAX_PLAYERS - 1))
        return;

    SpeakerList::playerStreams[speaker].erase((Stream*)(&stream));
}

bool SpeakerList::initStatus { false };
bool SpeakerList::showStatus { false };

ImFont* SpeakerList::pSpeakerFont { nullptr };
TexturePtr SpeakerList::tSpeakerIcon { nullptr };

std::array<std::unordered_map<Stream*, StreamInfo>, MAX_PLAYERS> SpeakerList::playerStreams;