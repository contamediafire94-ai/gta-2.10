#include "../main.h"
#include "../game/game.h"
#include "../net/netgame.h"
#include "gui.h"
#include "../playertags.h"
#include "../net/playerbubblepool.h"
#include "vendor/str_obfuscator/str_obfuscator.hpp"
// voice
#include "../voice_new/Plugin.h"
#include "../voice_new/MicroIcon.h"
#include "../voice_new/SpeakerList.h"
#include "../voice_new/Network.h"

#include "../gui/samp_widgets/voicebutton.h"
#include "game/Textures/TextureDatabaseRuntime.h"
#include "game/Streaming.h"
#include "game/Pools.h"

extern CNetGame* pNetGame;
extern CPlayerTags* pPlayerTags;
extern UI* pUI;
//RwTexture* logo = nullptr;
UI::UI(const ImVec2& display_size, const std::string& font_path)
        : Widget(), ImGuiWrapper(display_size, font_path)
{
    UISettings::Initialize(display_size);
    this->setFixedSize(display_size);

    //logo = (RwTexture*)CUtil::LoadTextureFromDB("mobile", "logobrmoscow");
}

bool UI::initialize()
{
    if (!ImGuiWrapper::initialize()) return false;

    m_splashScreen = new SplashScreen();
    this->addChild(m_splashScreen);
    m_splashScreen->setFixedSize(size());
    m_splashScreen->setPosition(ImVec2(0.0f, 0.0f));
    m_splashScreen->setVisible(true);

    m_chat = new Chat();
    this->addChild(m_chat);
    m_chat->setFixedSize(UISettings::chatSize());
    m_chat->setPosition(UISettings::chatPos());
    m_chat->setItemSize(UISettings::chatItemSize());
    m_chat->setVisible(false);

    m_buttonPanel = new ButtonPanel();
    this->addChild(m_buttonPanel);
    m_buttonPanel->setFixedSize(UISettings::buttonPanelSize());
    m_buttonPanel->setPosition(UISettings::buttonPanelPos());
    m_buttonPanel->setVisible(false);

    m_voiceButton = new VoiceButton();
    this->addChild(m_voiceButton);
    m_voiceButton->setFixedSize(UISettings::buttonVoiceSize());
    m_voiceButton->setPosition(UISettings::buttonVoicePos());
    m_voiceButton->setVisible(false);

    m_spawn = new Spawn();
    this->addChild(m_spawn);
    m_spawn->setFixedSize(UISettings::spawnSize());
    m_spawn->setPosition(UISettings::spawnPos());
    m_spawn->setVisible(false);

    m_dialog = new Dialog();
    this->addChild(m_dialog);
    m_dialog->setVisible(false);
    m_dialog->setMinSize(UISettings::dialogMinSize());
    m_dialog->setMaxSize(UISettings::dialogMaxSize());

    m_keyboard = new Keyboard();
    this->addChild(m_keyboard);
    m_keyboard->setFixedSize(UISettings::keyboardSize());
    m_keyboard->setPosition(UISettings::keyboardPos());
    m_keyboard->setVisible(false);

    m_playerTabList = new PlayerTabList();
    this->addChild(m_playerTabList);
    m_playerTabList->setMinSize(UISettings::dialogMinSize());
    m_playerTabList->setMaxSize(UISettings::dialogMaxSize());
    m_playerTabList->setVisible(false);

    label = new Label("0 FPS", ImColor(0.92f, 0.96f, 1.0f, 1.0f), false, UISettings::fontSize() / 2.7f);
    pUI->addChild(label);

    label2 = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    pUI->addChild(label2);

    label3 = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    pUI->addChild(label3);

    label4 = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    pUI->addChild(label4);

    // WIU UI CLEAN: remove internal architecture/debug watermark from gameplay.
    // The old "Client TGRP | arm64-v8a" label was development-only.



    //InitHudLogo();
    // ==== version ==== //
    //d_label = new Label("", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    //this->addChild(d_label);
    // d_label->setPosition(ImVec2(3.0, 55.0));

    return true;
}

void UI::render()
{
    ImGuiWrapper::render();

    // WIU UI CLEAN V2: keep only a compact, clean FPS counter.
    renderDebug();

    ProcessPushedTextdraws();

    if (m_bNeedClearMousePos) {
        ImGuiIO& io = ImGui::GetIO();
        io.MousePos = ImVec2(-1, -1);
        m_bNeedClearMousePos = false;
    }

   //DrawServerTexture(); // render sprite
}

void UI::shutdown()
{
    ImGuiWrapper::shutdown();
}

void UI::drawList()
{
    if (!visible()) return;

    /*Label* label;
    label = new Label("1.0.11", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    label->setPosition(ImVec2(0.0, 0.0));
    this->addChild(label);*/

    if (pPlayerTags) pPlayerTags->Render(renderer());
    if (pNetGame && pNetGame->GetTextLabelPool()) pNetGame->GetTextLabelPool()->Render(renderer());
    if (pNetGame && pNetGame->GetPlayerBubblePool()) pNetGame->GetPlayerBubblePool()->Render(renderer());

    draw(renderer());
}

void UI::touchEvent(const ImVec2& pos, TouchType type)
{
    if(!visible()) return;
    /*
        ? ??????? ??????????
        1 - ??????????
        2 - ??????
        3 - ???
    */

    if (m_keyboard->visible() && m_keyboard->contains(pos))
    {
        m_keyboard->touchEvent(pos, type);
        return;
    }

    if (m_dialog->visible() && m_dialog->contains(pos))
    {
        m_dialog->touchEvent(pos, type);
        return;
    }

    Widget::touchEvent(pos, type);
}

enum eTouchType
{
    TOUCH_POP = 1,
    TOUCH_PUSH = 2,
    TOUCH_MOVE = 3
};

bool UI::OnTouchEvent(int type, bool multi, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();

    /*
    switch (type)
    {
    case 1://TOUCH_PUSH:
        io.MousePos = ImVec2(x, y);
        io.MouseDown[0] = true;
        MyLog2("TOUCH_PUSH");
        break;

    case 2://TOUCH_POP:
        io.MouseDown[0] = false;
        m_bNeedClearMousePos = true;
        MyLog2("TOUCH_POP");
        break;

    case 3://TOUCH_MOVE:
        io.MousePos = ImVec2(x, y);
        MyLog2("TOUCH_MOVE");
        break;
    }*/
    VoiceButton* vbutton = pUI->voicebutton();
    switch (type)
    {
        case TOUCH_PUSH:
            io.MousePos = ImVec2(x, y);
            io.MouseDown[0] = true;
            break;

        case TOUCH_POP:
            io.MouseDown[0] = false;
            m_bNeedClearMousePos = true;
            break;

        case TOUCH_MOVE:
            io.MousePos = ImVec2(x, y);
            //if (vbutton->countdown > 50) vbutton->countdown = 20;
            break;
    }

    return true;
}

#include "../settings.h"
extern CGame *pGame;
extern CSettings* pSettings;
void UI::renderDebug()
{
    // WIU UI CLEAN V2: only the FPS counter stays on screen.
    // MEM / POS / Build / architecture labels remain removed.
    if (!pSettings || !pSettings->Get().iFPSCounter)
    {
        label->setText(" " );
        return;
    }

    static float fps = 60.0f;
    static auto lastTick = CTimer::m_snTimeInMillisecondsNonClipped;

    if (CTimer::m_snTimeInMillisecondsNonClipped - lastTick > 500)
    {
        lastTick = CTimer::m_snTimeInMillisecondsNonClipped;
        fps = std::clamp(CTimer::game_FPS, 1.0f, 240.0f);
    }

    char fpsText[24];
    snprintf(fpsText, sizeof(fpsText), "%.0f FPS", fps);
    label->setText(fpsText);

    // Compact top-right placement. The label has no heavy outline, which
    // removes the old debug-looking font effect.
    ImGuiIO& io = ImGui::GetIO();
    const float rightMargin = pUI->ScaleX(18.0f);
    const float topMargin = pUI->ScaleY(12.0f);
    const float estimatedWidth = pUI->ScaleX(78.0f);

    label->setPosition(ImVec2(
            io.DisplaySize.x - estimatedWidth - rightMargin,
            topMargin
    ));
}

void UI::PushToBufferedQueueTextDrawPressed(uint16_t textdrawId)
{
    BUFFERED_COMMAND_TEXTDRAW* pCmd = m_BufferedCommandTextdraws.WriteLock();

    pCmd->textdrawId = textdrawId;

    m_BufferedCommandTextdraws.WriteUnlock();
}

void UI::ProcessPushedTextdraws()
{
    BUFFERED_COMMAND_TEXTDRAW* pCmd = nullptr;
    while (pCmd = m_BufferedCommandTextdraws.ReadLock())
    {
        RakNet::BitStream bs;
        bs.Write(pCmd->textdrawId);
        pNetGame->GetRakClient()->RPC(&RPC_ClickTextDraw, &bs, HIGH_PRIORITY, RELIABLE_SEQUENCED, 0, false, UNASSIGNED_NETWORK_ID, 0);
        m_BufferedCommandTextdraws.ReadUnlock();
    }
}

#include "..//game/sprite2d.h"
void UI::DrawServerTexture() {
    ImGuiIO& io = ImGui::GetIO();
    float displayWidth = io.DisplaySize.x;
    float displayHeight = io.DisplaySize.y;

    float textureWidth = 175.0f;
    float textureHeight = 120.0f;
    float x = 10.0f;
    float y = displayHeight - textureHeight - 2.0f;


    CSprite2d* server = new CSprite2d();
    server->m_pRwTexture = (RwTexture*)CUtil::LoadTextureFromDB("samp", "serverlogo");

    CRGBA color {255, 255, 255, 255};

    server->Draw(x, y, textureWidth, textureHeight, &color);
}