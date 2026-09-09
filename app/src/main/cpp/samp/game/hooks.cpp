#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <atomic>
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <strings.h>
#include "../main.h"
#include "../vendor/armhook/patch.h"
#include "game.h"
#include "../net/netgame.h"
#include "../gui/gui.h"
#include "Textures/TextureDatabase.h"
#include "Textures/TextureDatabaseEntry.h"
#include "Textures/TextureDatabaseRuntime.h"
#include "Scene.h"
#include "sprite2d.h"
#include "Entity/PlayerPedGta.h"
#include "Pools.h"
#include "java/jniutil.h"
#include "game/Models/ModelInfo.h"
#include "MatrixLink.h"
#include "MatrixLinkList.h"
#include "game/Collision/Collision.h"
#include "TxdStore.h"
#include "util/CUtil.h"
#include "Coronas.h"
#include "multitouch.h"
#include "Streaming.h"
#include "References.h"
#include "VisibilityPlugins.h"
#include "game/Animation/AnimManager.h"
#include "FileLoader.h"
#include "Renderer.h"
#include "CrossHair.h"
#include "World.h"
#include "Core/Matrix.h"

extern UI* pUI;
extern CGame* pGame;
extern CNetGame *pNetGame;
extern MaterialTextGenerator* pMaterialTextGenerator;
extern CJavaWrapper* pJavaWrapper;
//extern CMatrix *pMatrix;
bool kuziak = false;
bool kuziaclose = false;
uint8_t byteInternalPlayer = 0;
CPedGTA* dwCurPlayerActor = 0;
uint8_t byteCurPlayer = 0;
uint8_t byteCurDriver = 0;

// O momento do bloqueio da campanha agora e controlado pelo game.cpp.
// Isso permite alguns frames extras de bootstrap do GTA depois que o
// CNetGame existe, sem deixar a campanha avancar ate a intro offline.
bool g_BlockGtaStoryScripts = false;

void (*CRunningScript__Process)(void* thiz);
void CRunningScript__Process_hook(void* thiz)
{
    if (g_BlockGtaStoryScripts)
    {
        static bool logged = false;
        if (!logged)
        {
            FLog("GTA story script threads blocked after post-CNetGame warm-up");
            logged = true;
        }
        return;
    }

    CRunningScript__Process(thiz);
}

extern "C" uintptr_t get_lib()
{
    return g_libGTASA;
}
// 0.3.7
PLAYERID FindPlayerIDFromGtaPtr(CEntityGTA* pEntity)
{
    if (pEntity == nullptr) return INVALID_PLAYER_ID;

    CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
    CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();

    PLAYERID PlayerID = pPlayerPool->FindRemotePlayerIDFromGtaPtr((CPedGTA*)pEntity);
    if (PlayerID != INVALID_PLAYER_ID) return PlayerID;

    VEHICLEID VehicleID = pVehiclePool->FindIDFromGtaPtr((CVehicleGTA*)pEntity);
    if (VehicleID != INVALID_VEHICLE_ID)
    {
        for (PLAYERID i = 0; i < MAX_PLAYERS; i++)
        {
            CRemotePlayer* pRemotePlayer = pPlayerPool->GetAt(i);
            if (pRemotePlayer && pRemotePlayer->CurrentVehicleID() == VehicleID) {
                return i;
            }
        }
    }

    return INVALID_PLAYER_ID;
}
// 0.3.7
PLAYERID FindActorIDFromGtaPtr(CPedGTA* pPed)
{
    if (pPed) {
        return pNetGame->GetActorPool()->FindIDFromGtaPtr(pPed);
    }

    return INVALID_PLAYER_ID;
}

/* =============================================================================== */

void RenderEffects() {
    static unsigned int v21EffectsSeq = 0;
    const unsigned int v21EffectsCurrent = ++v21EffectsSeq;
    const bool v21TraceEffects = v21EffectsCurrent <= 8;
    if (v21TraceEffects)
        FLog("V21 EFFECTS BEGIN | seq=%u", v21EffectsCurrent);

    if (pNetGame && pNetGame->GetGameState() == GAMESTATE_CONNECTED)
    {
        static bool loggedV12Effects = false;
        if (!loggedV12Effects)
        {
            CCamera& cameraProbe = *reinterpret_cast<CCamera*>(
                    g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
            FLog("V12: RenderEffects active | rwCamera=%p fading=%d interior=%u",
                 cameraProbe.m_pRwCamera,
                 cameraProbe.m_bFading ? 1 : 0,
                 (unsigned int)pGame->GetActiveInterior());
            loggedV12Effects = true;
        }
    }
//	RenderEffects();
    //RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
    //RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)5);
    //RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)6);
    //RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)1);
    //RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
    //RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)0);

    //((void(*)())(g_libGTASA + (VER_x32 ? 0x5C1280 + 1 : 0x6E5844) + 1))(); // 32 - 0x5C1280 // 64 - 0x6E5844
    //((void(*)())(g_libGTASA + (VER_x32 ? 0x5C1528 + 1 : 0x6E5A30) + 1))(); // 32 - 0x5C1528 // 64 - 0x6E5A30

    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x0059DA40 + 1 : 0x6C1D6C));
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005BE914 + 1 : 0x6E2FB4));
//    CRopes::Render();
//    CGlass::Render();
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005A6BC8 + 1 : 0x6CA5D0));
    CVisibilityPlugins::RenderReallyDrawLastObjects();
    CCoronas::Render();

    // FIXME
    CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
    auto g_fx = *(uintptr_t *) (g_libGTASA + (VER_x32 ? 0x00820520 : 0xA062A8));
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x00363DF0 + 1 : 0x433F54), &g_fx, TheCamera.m_pRwCamera, false);

    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005CBBAC + 1 : 0x6F054C));
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x0059BF84 + 1 : 0x6C0268));
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005A1C38 + 1 : 0x6C552C));
    //   CClouds::VolumetricCloudsRender();
////    if (CHeli::NumberOfSearchLights || CTheScripts::NumberOfScriptSearchLights) {
////        CHeli::Pre_SearchLightCone();
////        CHeli::RenderAllHeliSearchLights();
////        CTheScripts::RenderAllSearchLights();
////        CHeli::Post_SearchLightCone();
////    }
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005E3390 + 1 : 0x708DF0));
////    if (CReplay::Mode != MODE_PLAYBACK && !CPad::GetPad(0)->DisablePlayerControls) {
////        FindPlayerPed()->DrawTriangleForMouseRecruitPed();
////    }
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005C0B14 + 1 : 0x6E50CC));
//    //CVehicleRecording::Render();
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005B19D0 + 1 : 0x6D6068));
//    //CRenderer::RenderFirstPersonVehicle();
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005B5F78 + 1 : 0x6DA2B8));

    //DebugModules::Render3D();
    if (v21TraceEffects)
        FLog("V21 EFFECTS END | seq=%u", v21EffectsCurrent);
}

// =============================================================================
// V31: manter RenderEffects customizado fixo durante o teste do blit.
//
// A V30 provou que trocar para o RenderEffects original nao restaura o FBO0.
// Para a V31 medir somente a correcao de apresentacao, removemos a alternancia
// A/B e preservamos o comportamento customizado que vinha sendo usado antes.
// =============================================================================
static void (*RenderEffects_V30_Original)() = nullptr;
static std::atomic<unsigned int> g_v30EffectsConnectedCount{0};
static std::atomic<int> g_v30EffectsPhase{0}; // V31: permanece 0 (custom)

static void RenderEffects_V30_hook()
{
    const bool connected =
            pNetGame && pNetGame->GetGameState() == GAMESTATE_CONNECTED;

    if (connected)
    {
        const unsigned int n =
                g_v30EffectsConnectedCount.fetch_add(1, std::memory_order_relaxed) + 1;
        if (n == 1)
        {
            FLog("V31 EFFECTS MODE | CUSTOM_FIXED tid=%d ctx=%p original=%p",
                 (int)syscall(SYS_gettid), (void*)eglGetCurrentContext(),
                 (void*)RenderEffects_V30_Original);
        }
    }

    g_v30EffectsPhase.store(0, std::memory_order_relaxed);
    RenderEffects();
}

/*void MainLoop();
void(*Render2dStuff)();
void Render2dStuff_hook()
{
    Render2dStuff();
    if(pNetGame)
    {
        CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
        if(pTextDrawPool) pTextDrawPool->Draw();
    }
    if (pUI) pUI->render();
    return;
}*/

void ShowHud()
{
    static unsigned int v21HudSeq = 0;
    const unsigned int v21HudCurrent = ++v21HudSeq;
    const bool v21TraceHud = v21HudCurrent <= 8;

    if (v21TraceHud)
        FLog("V21 SHOWHUD BEGIN | seq=%u pNetGame=%p pGame=%p java=%p",
             v21HudCurrent, pNetGame, pGame, pJavaWrapper);

    CLocalPlayer *pLocalPlayer = nullptr;
    CPlayerPed *pPed = nullptr;
    CPlayerPool *pPlayerPool = nullptr;

    if (pNetGame)
    {
        pPlayerPool = pNetGame->GetPlayerPool();
        if (pPlayerPool)
            pLocalPlayer = pPlayerPool->GetLocalPlayer();
    }
    if (pGame)
        pPed = pGame->FindPlayerPed();

    if (v21TraceHud)
        FLog("V21 SHOWHUD PTRS | seq=%u pool=%p local=%p ped=%p gtaPed=%p",
             v21HudCurrent, pPlayerPool, pLocalPlayer, pPed, GamePool_FindPlayerPed());

    if(pGame && pNetGame)
    {
        if(pGame->FindPlayerPed() || GamePool_FindPlayerPed())
        {
            if (v21TraceHud)
                FLog("V21 SHOWHUD WEAPON BEGIN | seq=%u ped=%p", v21HudCurrent, pPed);

            CWeapon *pWeapon = pPed ? pPed->GetCurrentWeaponSlot() : nullptr;

            if (v21TraceHud)
                FLog("V21 SHOWHUD WEAPON END | seq=%u weapon=%p", v21HudCurrent, pWeapon);

            if(pPlayerPool && pPed && pWeapon && pJavaWrapper)
            {
                if (v21TraceHud)
                    FLog("V21 SHOWHUD JAVA BEGIN | seq=%u", v21HudCurrent);

                pJavaWrapper->UpdateHudInfo(
                        pGame->FindPlayerPed()->GetHealth(),
                        pGame->FindPlayerPed()->GetArmour(),
                        pWeapon->dwType,
                        pWeapon->dwAmmoInClip,
                        pWeapon->dwAmmo,
                        pGame->GetLocalMoney(),
                        pGame->GetWantedLevel()
                );

                if (v21TraceHud)
                    FLog("V21 SHOWHUD JAVA END | seq=%u", v21HudCurrent);
            }

            const uintptr_t hudFlagAddress =
                    g_libGTASA + (VER_x32 ? 0x00819D88 + 1 : 0x009ff3A8);

            if (v21TraceHud)
                FLog("V21 SHOWHUD FLAGWRITE BEGIN | seq=%u addr=%p x32=%d",
                     v21HudCurrent, (void*)hudFlagAddress, VER_x32 ? 1 : 0);

            *(uint8_t*)hudFlagAddress = 0;

            if (v21TraceHud)
                FLog("V21 SHOWHUD FLAGWRITE END | seq=%u", v21HudCurrent);
        }
    }

    if (v21TraceHud)
        FLog("V21 SHOWHUD END | seq=%u", v21HudCurrent);
}

#include "CSkyBox.h"
//extern CJavaWrapper* pJavaWrapper;

// =============================================================================
// V37 - SAFE 2D/PRESENT HANDSHAKE
//
// V36 proved that directly invoking RenderQueue::Flush() from the producer
// thread is unsafe in this build (SIGSEGV inside GTA's RenderQueue).
//
// V37 does NOT call any private RenderQueue method.
// It only tells the eglSwap/presentation thread whether Render2dStuff is still
// producing the 2D commands. If a swap overlaps 2D production, V31 defers its
// FBO2->FBO0 copy for that swap. On the following ordered swap, glFinish()
// completes the GLES work already executed by the graphics thread and V31 then
// copies the completed source framebuffer.
// =============================================================================
static std::atomic<bool> g_v37TwoDInProgress{false};
static std::atomic<unsigned int> g_v37TwoDCompleted{0};

// V56: handshake de apresentacao. O produtor 3D/2D roda separado da thread
// que possui o EGLSurface. Sem esperar o swap apresentar o 2D concluido,
// o proximo passe 3D pode sobrescrever o FBO antes do blit, deixando
// HUD/chat/radar/TextDraw invisiveis apesar de Draw() ser executado.
static std::atomic<unsigned int> g_v56PresentedTwoD{0};

// V38: real cross-thread exclusion. The producer owns this mutex while it
// submits the complete 2D pass. The EGL/swap thread only performs the V31
// copy when it can own the same mutex, which makes a mid-TextDraw/UI blit
// impossible.
static pthread_mutex_t g_v38TwoDPresentMutex = PTHREAD_MUTEX_INITIALIZER;

void Render2dStuff()
{
    static bool v35Full2DLogged = false;
    if (!v35Full2DLogged)
    {
        v35Full2DLogged = true;
        FLog("V38 FULL2D ACTIVE | tid=%d ctx=%p draw=%p",
             (int)syscall(SYS_gettid),
             (void*)eglGetCurrentContext(),
             (void*)eglGetCurrentSurface(EGL_DRAW));
        FLog("V42 RENDER CONTROL: GTA ORIGINAL ES2VertexBuffer + CRQ selector active");
    }

    static unsigned int v21TwoDSeq = 0;
    const unsigned int v21TwoDCurrent = ++v21TwoDSeq;
    const bool v21TraceTwoD = v21TwoDCurrent <= 8;

    pthread_mutex_lock(&g_v38TwoDPresentMutex);
    g_v37TwoDInProgress.store(true, std::memory_order_release);

    if (v21TraceTwoD)
        FLog("V38 2D MUTEX LOCK | seq=%u tid=%d",
             v21TwoDCurrent, (int)syscall(SYS_gettid));

    if (v21TraceTwoD)
    {
        FLog("V37 2D BEGIN | seq=%u tid=%d",
             v21TwoDCurrent, (int)syscall(SYS_gettid));
        FLog("V21 2D BEGIN | seq=%u", v21TwoDCurrent);
    }

    if (v21TraceTwoD)
        FLog("V21 2D SHOWHUD CALL BEGIN | seq=%u", v21TwoDCurrent);
    ShowHud();
    if (v21TraceTwoD)
        FLog("V21 2D SHOWHUD CALL END | seq=%u", v21TwoDCurrent);

    const uintptr_t v21AltTestTarget =
            g_libGTASA + (VER_x32 ? 0x001BB7F4 + 1 : 0x24EA90);

    if (v21TraceTwoD)
        FLog("V21 2D ALTTEST BEGIN | seq=%u target=%p x32=%d",
             v21TwoDCurrent, (void*)v21AltTestTarget, VER_x32 ? 1 : 0);

    bool v12AltRenderTarget = CHook::CallFunction<bool>(v21AltTestTarget);

    if (v21TraceTwoD)
        FLog("V21 2D ALTTEST END | seq=%u altRT=%d",
             v21TwoDCurrent, v12AltRenderTarget ? 1 : 0);

    if (pNetGame && pNetGame->GetGameState() == GAMESTATE_CONNECTED)
    {
        static bool loggedV12TwoD = false;
        if (!loggedV12TwoD)
        {
            CCamera& cameraProbe = *reinterpret_cast<CCamera*>(
                    g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));

            FLog("V12: Render2dStuff active | altRT=%d camera=%p sceneCamera=%p world=%p fading=%d interior=%u",
                 v12AltRenderTarget ? 1 : 0,
                 cameraProbe.m_pRwCamera,
                 Scene.m_pRwCamera,
                 Scene.m_pRpWorld,
                 cameraProbe.m_bFading ? 1 : 0,
                 (unsigned int)pGame->GetActiveInterior());
            loggedV12TwoD = true;
        }
    }

    if (v12AltRenderTarget)
    {
        const uintptr_t v21AltFlushTarget =
                g_libGTASA + (VER_x32 ? 0x001BC20C + 1 : 0x24F5B8);
        if (v21TraceTwoD)
            FLog("V21 2D ALTFLUSH BEGIN | seq=%u target=%p",
                 v21TwoDCurrent, (void*)v21AltFlushTarget);
        CHook::CallFunction<void>(v21AltFlushTarget);
        if (v21TraceTwoD)
            FLog("V21 2D ALTFLUSH END | seq=%u", v21TwoDCurrent);
    }

    if (v21TraceTwoD)
        FLog("V21 2D STATES BEGIN | seq=%u", v21TwoDCurrent);

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND, RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, RWRSTATE(rwRENDERSTATENARENDERSTATE));
    RwRenderStateSet(rwRENDERSTATECULLMODE, RWRSTATE(rwCULLMODECULLNONE));

    if (v21TraceTwoD)
        FLog("V21 2D STATES END | seq=%u", v21TwoDCurrent);

    if (v21TraceTwoD)
        FLog("V21 2D HUDDRAW BEGIN | seq=%u", v21TwoDCurrent);
    CHook::CallFunction<void>("_ZN4CHud4DrawEv");
    if (v21TraceTwoD)
        FLog("V21 2D HUDDRAW END | seq=%u", v21TwoDCurrent);

    if (v21TraceTwoD)
        FLog("V21 2D TOUCHDRAW1 BEGIN | seq=%u", v21TwoDCurrent);
    ((void(*)(bool) )(g_libGTASA + (VER_x32 ? 0x002B0BD8 + 1 : 0x36FB00)) )(false);
    if (v21TraceTwoD)
        FLog("V21 2D TOUCHDRAW1 END | seq=%u", v21TwoDCurrent);

    if (v21TraceTwoD)
        FLog("V21 2D GAMMA1 BEGIN | seq=%u", v21TwoDCurrent);
    CHook::CallFunction<void>("_Z12emu_GammaSeth", 1);
    if (v21TraceTwoD)
        FLog("V21 2D GAMMA1 END | seq=%u", v21TwoDCurrent);

    if (v21TraceTwoD)
        FLog("V21 2D MESSAGES BEGIN | seq=%u", v21TwoDCurrent);
    ((void (*)(bool)) (g_libGTASA + (VER_x32 ? 0x0054BDD4 + 1 : 0x66B678)))(1u);
    if (v21TraceTwoD)
        FLog("V21 2D MESSAGES END | seq=%u", v21TwoDCurrent);

    if (v21TraceTwoD)
        FLog("V21 2D FONT BEGIN | seq=%u", v21TwoDCurrent);
    ((void (*)(bool)) (g_libGTASA + (VER_x32 ? 0x005A9120 + 1 : 0x6CCEA0)))(1u);
    if (v21TraceTwoD)
        FLog("V21 2D FONT END | seq=%u", v21TwoDCurrent);

    CHook::CallFunction<void>("_Z12emu_GammaSeth", 0);

    if(pNetGame)
    {
        if (v21TraceTwoD)
            FLog("V21 2D TEXTDRAW BEGIN | seq=%u", v21TwoDCurrent);
        CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
        if(pTextDrawPool) pTextDrawPool->Draw();
        if (v21TraceTwoD)
            FLog("V21 2D TEXTDRAW END | seq=%u", v21TwoDCurrent);
    }

    if (v21TraceTwoD)
        FLog("V21 2D TOUCHDRAW2 BEGIN | seq=%u", v21TwoDCurrent);
    CHook::CallFunction<void>("_ZN15CTouchInterface7DrawAllEb", false);
    if (v21TraceTwoD)
        FLog("V21 2D TOUCHDRAW2 END | seq=%u", v21TwoDCurrent);

    if (v21TraceTwoD)
        FLog("V21 2D UI BEGIN | seq=%u ui=%p", v21TwoDCurrent, pUI);
    if (pUI) pUI->render();
    if (v21TraceTwoD)
        FLog("V21 2D UI END | seq=%u", v21TwoDCurrent);

    // V39: targeted render-target test.
    // The original pipeline flushes an active alternate render target BEFORE
    // drawing 2D. V38 proved ordering is no longer the problem, but the final
    // HUD/TextDraw/UI pixels still never reach the presented framebuffer.
    //
    // Re-check the engine's alt-target state AFTER the whole 2D pass. If it is
    // active again, flush it here so the 2D result is resolved before the V31
    // presentation copy. This uses the same GTA routine already proven safe at
    // the start of Render2dStuff; no private RenderQueue calls are used.
    const uintptr_t v39AltTestTarget =
            g_libGTASA + (VER_x32 ? 0x001BB7F4 + 1 : 0x24EA90);
    const uintptr_t v39AltFlushTarget =
            g_libGTASA + (VER_x32 ? 0x001BC20C + 1 : 0x24F5B8);

    const bool v39PostAltRenderTarget =
            CHook::CallFunction<bool>(v39AltTestTarget);

    if (v21TraceTwoD)
        FLog("V39 2D POST_ALTTEST | seq=%u altRT=%d test=%p",
             v21TwoDCurrent,
             v39PostAltRenderTarget ? 1 : 0,
             (void*)v39AltTestTarget);

    if (v39PostAltRenderTarget)
    {
        if (v21TraceTwoD)
            FLog("V39 2D POST_ALTFLUSH BEGIN | seq=%u target=%p",
                 v21TwoDCurrent, (void*)v39AltFlushTarget);

        CHook::CallFunction<void>(v39AltFlushTarget);

        if (v21TraceTwoD)
            FLog("V39 2D POST_ALTFLUSH END | seq=%u",
                 v21TwoDCurrent);
    }

    // Publish completion only after every HUD/TextDraw/UI command and the
    // optional post-2D alt-target resolve have been submitted.
    g_v37TwoDCompleted.store(v21TwoDCurrent, std::memory_order_release);
    g_v37TwoDInProgress.store(false, std::memory_order_release);

    if (v21TraceTwoD)
    {
        FLog("V38 2D COMPLETE | seq=%u tid=%d",
             v21TwoDCurrent, (int)syscall(SYS_gettid));
        FLog("V21 2D END | seq=%u", v21TwoDCurrent);
    }

    pthread_mutex_unlock(&g_v38TwoDPresentMutex);

    if (v21TraceTwoD)
        FLog("V38 2D MUTEX UNLOCK | seq=%u tid=%d",
             v21TwoDCurrent, (int)syscall(SYS_gettid));

    // V57: nao bloqueamos mais aqui. A V56 provou que esperar dentro
    // de Render2dStuff() sempre termina em TIMEOUT, porque o eglSwapBuffers
    // que pode apresentar o frame so acontece depois que esta funcao retorna.
    // A sincronizacao agora e feita na propria thread EGL, imediatamente
    // antes do glFinish/blit.
}

// =============================================================================
// V29 DIAGNOSTICO DO FRAME REAL / RENDER THREAD
//
// A V28 provou que:
//   - eglSwapBuffers() e a EGLSurface exibida pelo Android sao validos;
//   - FBO 0 / viewport 1525x720 sao apresentados normalmente;
//   - o clear magenta feito imediatamente antes do swap aparece na tela.
//
// Portanto a V29 NAO pinta mais a tela. Ela mede o caminho imediatamente
// anterior ao swap para descobrir qual destes casos ocorre:
//   A) CPU percorre o renderer, mas nenhum draw GL chega ao render thread;
//   B) draws ocorrem em outro FBO e o resultado nao chega ao FBO 0;
//   C) o frame e desenhado e depois limpo de preto antes do swap.
// =============================================================================

static inline int V29GetTid()
{
    return (int)syscall(SYS_gettid);
}

static std::atomic<unsigned int> g_v29DrawArraysCount{0};
static std::atomic<unsigned int> g_v29DrawElementsCount{0};
static std::atomic<unsigned int> g_v29FramebufferBindCount{0};
static std::atomic<unsigned int> g_v29ColorClearCount{0};
static std::atomic<unsigned int> g_v29BlackClearCount{0};
static std::atomic<unsigned int> g_v29Render2dCount{0};
static std::atomic<unsigned int> g_v29BarRoadsCount{0};
static std::atomic<unsigned int> g_v29EmuEndCount{0};
static std::atomic<int> g_v29LastGpuTid{-1};
static std::atomic<int> g_v29LastBoundFbo{-1};
static std::atomic<int> g_v29LastNonZeroFbo{-1};

void (*Render2dStuff_V26_Original)();

// =============================================================================
// V64 - RENDERQUEUE CONSUMER TRACE
//
// V63 proved that even a persistent, untextured RwIm2D primitive is submitted
// by the producer but is not visible in the frame acknowledged by V61.
//
// Important distinction:
//   producer "submitted 2D" != graphics thread "executed 2D".
//
// These hooks OBSERVE the GTA's existing RenderQueue command handlers. They do
// not call RenderQueue::Flush/Process and do not move rendering across threads.
// The goal is to correlate:
//   1) V63 probe submission on the producer thread;
//   2) rqDrawIndexed/rqDrawNonIndexed execution on GraphicsThread;
//   3) target/FBO selected there;
//   4) rqSwapBuffers / our EGL presentation.
//
// This is diagnostic-only and deliberately bounded to avoid log flooding.
// =============================================================================
static std::atomic<unsigned int> g_v64RqDrawExecTotal{0};
static std::atomic<unsigned int> g_v64RqDrawIndexedExec{0};
static std::atomic<unsigned int> g_v64RqDrawNonIndexedExec{0};
static std::atomic<unsigned int> g_v64RqTargetSelectExec{0};
static std::atomic<unsigned int> g_v64RqSwapExec{0};

static std::atomic<unsigned int> g_v64ProbeSeq{0};
static std::atomic<unsigned int> g_v64ProbeDrawBase{0};
static std::atomic<unsigned int> g_v64ProbeSwapBase{0};

static std::atomic<int> g_v64LastRqDrawTid{-1};
static std::atomic<int> g_v64LastRqDrawFbo{-1};
static std::atomic<int> g_v64LastRqTargetFbo{-1};

// =============================================================================
// V65 - PRECOMPOSE WORLD BEFORE RENDERQUEUE 2D
//
// V64 proved the real ordering on the GraphicsThread:
//   - scene/world commands execute in a non-zero FBO (for example FBO 16);
//   - the RenderQueue then switches to FBO0 and executes many 2D draws there;
//   - immediately before eglSwapBuffers the GTA binds the world FBO again;
//   - our old V31 final blit copied world FBO -> FBO0 at that point, erasing
//     the HUD/chat/TextDraw/UI that had JUST been rendered into FBO0.
//
// V65 fixes the order without calling private RenderQueue flush/process APIs:
//   1) remember the latest non-zero world FBO + viewport;
//   2) on the FIRST RenderQueue draw that reaches FBO0 for a 2D producer frame,
//      copy world -> FBO0 BEFORE that draw executes;
//   3) let GTA/SA-MP's queued 2D commands naturally draw over the world;
//   4) at eglSwapBuffers, preserve the already-composed FBO0 instead of doing
//      the old destructive final world blit.
// =============================================================================
static std::atomic<unsigned int> g_v65Producer2DSeq{0};
static std::atomic<unsigned int> g_v65Precomposed2DSeq{0};
static std::atomic<int> g_v65WorldFbo{-1};
static std::atomic<int> g_v65WorldVpX{0};
static std::atomic<int> g_v65WorldVpY{0};
static std::atomic<int> g_v65WorldVpW{0};
static std::atomic<int> g_v65WorldVpH{0};

// V66 - redirect the engine's default-framebuffer 2D phase into a private
// persistent FBO.  V65 proved that writing world/2D into FBO0 early is later
// discarded by this RenderQueue path.  We therefore compose offscreen and
// copy the finished frame to FBO0 only at the real EGL present.
static GLuint g_v66ComposeFbo = 0;
static GLuint g_v66ComposeTex = 0;
static GLint g_v66ComposeW = 0;
static GLint g_v66ComposeH = 0;
static std::atomic<unsigned int> g_v66ComposeSeq{0};
static std::atomic<unsigned int> g_v66RedirectCount{0};

static bool V66RedirectDefaultTargetToCompose(unsigned int producerSeq);
static bool V66PresentComposeToDefault(unsigned int swapSeq, int swapTid,
                                       unsigned int completedSeq,
                                       GLint restoreFbo,
                                       EGLint surfaceWidth, EGLint surfaceHeight);
static void V65ObserveAndMaybePrecompose(GLint currentFbo);

static void (*V64RQDrawIndexed_Original)(char*&) = nullptr;
static void (*V64RQDrawNonIndexed_Original)(char*&) = nullptr;
static void (*V64RQTargetSelect_Original)(char*&) = nullptr;
static void (*V64RQSwapBuffers_Original)(char*&) = nullptr;

static inline GLint V64CurrentFboIfPossible()
{
    if (eglGetCurrentContext() == EGL_NO_CONTEXT)
        return -1;

    GLint fbo = -1;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
    return fbo;
}

static inline bool V64ShouldTraceConsumer(
        unsigned int total,
        unsigned int probeSeq,
        unsigned int probeBase)
{
    if (probeSeq == 0)
        return false;

    // First connected/probe frames get a detailed trace. Afterwards only sparse
    // checkpoints are kept.
    if (probeSeq <= 16 && total <= probeBase + 80)
        return true;

    return (total % 1000u) == 0u;
}

static void V64RQDrawIndexed_hook(char*& command)
{
    const unsigned int indexed =
            g_v64RqDrawIndexedExec.fetch_add(1, std::memory_order_relaxed) + 1;
    const unsigned int total =
            g_v64RqDrawExecTotal.fetch_add(1, std::memory_order_acq_rel) + 1;

    const unsigned int probeSeq =
            g_v64ProbeSeq.load(std::memory_order_acquire);
    const unsigned int probeBase =
            g_v64ProbeDrawBase.load(std::memory_order_acquire);

    const int tid = V29GetTid();
    const GLint fbo = V64CurrentFboIfPossible();
    V65ObserveAndMaybePrecompose(fbo);
    g_v64LastRqDrawTid.store(tid, std::memory_order_release);
    g_v64LastRqDrawFbo.store((int)fbo, std::memory_order_release);

    if (V64ShouldTraceConsumer(total, probeSeq, probeBase))
    {
        FLog("V64 RQ DRAWI EXEC | total=%u indexed=%u probe=%u base=%u delta=%u tid=%d ctx=%p fbo=%d cmd=%p",
             total, indexed, probeSeq, probeBase,
             total >= probeBase ? total - probeBase : 0u,
             tid, (void*)eglGetCurrentContext(), (int)fbo, (void*)command);
    }

    if (V64RQDrawIndexed_Original)
        V64RQDrawIndexed_Original(command);
}

static void V64RQDrawNonIndexed_hook(char*& command)
{
    const unsigned int nonIndexed =
            g_v64RqDrawNonIndexedExec.fetch_add(1, std::memory_order_relaxed) + 1;
    const unsigned int total =
            g_v64RqDrawExecTotal.fetch_add(1, std::memory_order_acq_rel) + 1;

    const unsigned int probeSeq =
            g_v64ProbeSeq.load(std::memory_order_acquire);
    const unsigned int probeBase =
            g_v64ProbeDrawBase.load(std::memory_order_acquire);

    const int tid = V29GetTid();
    const GLint fbo = V64CurrentFboIfPossible();
    V65ObserveAndMaybePrecompose(fbo);
    g_v64LastRqDrawTid.store(tid, std::memory_order_release);
    g_v64LastRqDrawFbo.store((int)fbo, std::memory_order_release);

    if (V64ShouldTraceConsumer(total, probeSeq, probeBase))
    {
        FLog("V64 RQ DRAWN EXEC | total=%u nonIndexed=%u probe=%u base=%u delta=%u tid=%d ctx=%p fbo=%d cmd=%p",
             total, nonIndexed, probeSeq, probeBase,
             total >= probeBase ? total - probeBase : 0u,
             tid, (void*)eglGetCurrentContext(), (int)fbo, (void*)command);
    }

    if (V64RQDrawNonIndexed_Original)
        V64RQDrawNonIndexed_Original(command);
}

static void V64RQTargetSelect_hook(char*& command)
{
    const unsigned int n =
            g_v64RqTargetSelectExec.fetch_add(1, std::memory_order_relaxed) + 1;

    if (V64RQTargetSelect_Original)
        V64RQTargetSelect_Original(command);

    GLint fboAfterOriginal = V64CurrentFboIfPossible();
    GLint effectiveFbo = fboAfterOriginal;

    // V66: when the real RenderQueue selects its default target for 2D, move
    // that phase to our persistent composition FBO.  The target command has
    // already applied all native state/viewport changes; we only replace the
    // framebuffer binding.
    const unsigned int producerSeq =
            g_v65Producer2DSeq.load(std::memory_order_acquire);
    if (fboAfterOriginal == 0 && producerSeq > 0)
    {
        if (V66RedirectDefaultTargetToCompose(producerSeq))
            effectiveFbo = V64CurrentFboIfPossible();
    }

    g_v64LastRqTargetFbo.store((int)effectiveFbo, std::memory_order_release);

    const unsigned int probeSeq =
            g_v64ProbeSeq.load(std::memory_order_acquire);

    if ((probeSeq > 0 && probeSeq <= 16 && n <= 160) || (n % 1000u) == 0u)
    {
        FLog("V66 RQ TARGET EXEC | n=%u probe=%u producer=%u tid=%d ctx=%p originalFbo=%d effectiveFbo=%d",
             n, probeSeq, producerSeq, V29GetTid(),
             (void*)eglGetCurrentContext(),
             (int)fboAfterOriginal, (int)effectiveFbo);
    }
}

static void V64RQSwapBuffers_hook(char*& command)
{
    const unsigned int n =
            g_v64RqSwapExec.fetch_add(1, std::memory_order_relaxed) + 1;

    const unsigned int probeSeq =
            g_v64ProbeSeq.load(std::memory_order_acquire);
    const unsigned int base =
            g_v64ProbeDrawBase.load(std::memory_order_acquire);
    const unsigned int now =
            g_v64RqDrawExecTotal.load(std::memory_order_acquire);

    if ((probeSeq > 0 && probeSeq <= 16) || (n % 120u) == 0u)
    {
        FLog("V64 RQ SWAP BEGIN | n=%u probe=%u rqNow=%u base=%u delta=%u tid=%d ctx=%p fbo=%d",
             n, probeSeq, now, base,
             now >= base ? now - base : 0u,
             V29GetTid(), (void*)eglGetCurrentContext(),
             (int)V64CurrentFboIfPossible());
    }

    if (V64RQSwapBuffers_Original)
        V64RQSwapBuffers_Original(command);

    if ((probeSeq > 0 && probeSeq <= 16) || (n % 120u) == 0u)
    {
        const unsigned int after =
                g_v64RqDrawExecTotal.load(std::memory_order_acquire);
        FLog("V64 RQ SWAP END | n=%u probe=%u rqAfter=%u base=%u delta=%u tid=%d ctx=%p",
             n, probeSeq, after, base,
             after >= base ? after - base : 0u,
             V29GetTid(), (void*)eglGetCurrentContext());
    }
}

// =============================================================================
// V63 - RW 2D PERSISTENT PRIMITIVE PROBE
//
// V61 proved that the next 3D pass can be held until the EGL/present thread has
// acknowledged the completed 2D frame, but HUD/chat/radar/TextDraws are still
// absent.  This probe submits one very simple, untextured RenderWare 2D quad
// through the SAME RwIm2DRenderIndexedPrimitive path used by the SA-MP ImGui
// renderer.  It does not touch private RenderQueue methods or EGL state.
//
// Expected visual result during the first ~720 2D frames after entering game:
// a large opaque magenta rectangle in the upper-left area.
//   - if it APPEARS: the common RW 2D primitive path works, so state/HUD/UI data
//     becomes the next target;
//   - if it DOES NOT appear: the problem is below HUD/TextDraw/UI, in the RW 2D
//     command/RenderQueue execution path or its render target.
// =============================================================================
static void V63SubmitRw2DProbe(unsigned int seq)
{
    // Keep the test bounded and give every submitted frame its OWN persistent
    // vertex storage.  On GTA SA Android RwIm2DRenderIndexedPrimitive may queue
    // work for the graphics thread; stack-local vertices can therefore be dead
    // before the queued command consumes them.
    if (seq == 0 || seq > 720)
        return;

    static RwIm2DVertex s_vertices[721][4]{};
    static RwImVertexIndex s_indices[6] = { 0, 1, 2, 0, 2, 3 };
    static bool s_ready[721]{};

    const RwReal nearScreenZ = CSprite2d::NearScreenZ;
    const RwReal recipNearClip = CSprite2d::RecipNearClip;

    const float x0 = 42.0f;
    const float y0 = 42.0f;
    const float x1 = 360.0f;
    const float y1 = 150.0f;

    RwIm2DVertex* v = s_vertices[seq];

    if (!s_ready[seq])
    {
        const float xs[4] = { x0, x1, x1, x0 };
        const float ys[4] = { y0, y0, y1, y1 };

        for (int i = 0; i < 4; ++i)
        {
            RwIm2DVertexSetScreenX(&v[i], xs[i]);
            RwIm2DVertexSetScreenY(&v[i], ys[i]);
            RwIm2DVertexSetScreenZ(&v[i], nearScreenZ);
            RwIm2DVertexSetRecipCameraZ(&v[i], recipNearClip);

            // ImGui's ImDrawVert::col uses the same packed RGBA convention on
            // this client.  This is opaque bright magenta.
            v[i].emissiveColor = 0xFFFF00FFu;

            RwIm2DVertexSetU(&v[i], 0.0f, recipNearClip);
            RwIm2DVertexSetV(&v[i], 0.0f, recipNearClip);
        }

        s_ready[seq] = true;
    }

    // Match the render state used by ImGuiWrapper::setupRenderState as closely
    // as possible, while keeping the probe untextured.
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)0);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)0);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
    RwRenderStateSet(rwRENDERSTATEBORDERCOLOR, (void*)0);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONGREATER);
    RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)2);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)0);

    // V64: snapshot the graphics-consumer progress immediately before the
    // unique last 2D probe is submitted.
    const unsigned int v64RqBase =
            g_v64RqDrawExecTotal.load(std::memory_order_acquire);
    const unsigned int v64SwapBase =
            g_v64RqSwapExec.load(std::memory_order_acquire);
    g_v64ProbeDrawBase.store(v64RqBase, std::memory_order_release);
    g_v64ProbeSwapBase.store(v64SwapBase, std::memory_order_release);
    g_v64ProbeSeq.store(seq, std::memory_order_release);

    const RwBool v64ProbeResult =
            RwIm2DRenderIndexedPrimitive(
                    rwPRIMTYPETRILIST, v, 4, s_indices, 6);

    if (seq <= 16 || (seq % 120) == 0)
    {
        FLog("V64 RW2D PROBE SUBMIT | seq=%u result=%d rqBase=%u swapBase=%u tid=%d v=%p idx=%p nearZ=%.6f recip=%.6f rect=%.0f,%.0f-%.0f,%.0f",
             seq, (int)v64ProbeResult, v64RqBase, v64SwapBase,
             V29GetTid(), (void*)v, (void*)s_indices,
             (double)nearScreenZ, (double)recipNearClip,
             (double)x0, (double)y0, (double)x1, (double)y1);
    }
}

void Render2dStuff_V26_hook()
{
    static unsigned int seq = 0;
    const unsigned int current = ++seq;
    g_v29Render2dCount.fetch_add(1, std::memory_order_relaxed);

    pthread_mutex_lock(&g_v38TwoDPresentMutex);
    g_v37TwoDInProgress.store(true, std::memory_order_release);

    if (current <= 16)
    {
        EGLContext ctx = eglGetCurrentContext();
        EGLSurface draw = eglGetCurrentSurface(EGL_DRAW);
        FLog("V58 ORIGINAL2D BEGIN | seq=%u tid=%d ctx=%p draw=%p",
             current, V29GetTid(), (void*)ctx, (void*)draw);
    }

    // V65: publish the producer frame before GTA/SA-MP enqueue their 2D work.
    // The GraphicsThread uses this sequence to precompose the world exactly
    // once, immediately before the first queued draw that reaches FBO0.
    g_v65Producer2DSeq.store(current, std::memory_order_release);

    // Deixa o proprio GTASA montar HUD/radar/mensagens usando a ordem e os
    // render-targets nativos desta build.
    Render2dStuff_V26_Original();

    if (current <= 16)
        FLog("V58 ORIGINAL2D END | seq=%u", current);

    // O GTA original nao conhece as camadas SA-MP. Recolocamos somente elas
    // DEPOIS do 2D original, sem chamar funcoes privadas do RenderQueue na
    // thread EGL.
    if (pNetGame)
    {
        CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
        if (pTextDrawPool)
        {
            pTextDrawPool->Draw();
            if (current <= 16)
                FLog("V58 TEXTDRAW DRAW | seq=%u pool=%p",
                     current, pTextDrawPool);
        }
    }

    if (pUI)
    {
        pUI->render();
        if (current <= 16)
            FLog("V58 UI RENDER | seq=%u ui=%p", current, pUI);
    }

    // V63 diagnostic: submit a persistent untextured RW 2D quad as the LAST 2D
    // command of this producer pass.  V61 then prevents the next 3D pass from
    // starting until the presentation thread acknowledges this completed 2D.
    V63SubmitRw2DProbe(current);

    g_v37TwoDCompleted.store(current, std::memory_order_release);
    g_v37TwoDInProgress.store(false, std::memory_order_release);

    pthread_mutex_unlock(&g_v38TwoDPresentMutex);

    if (current <= 16)
        FLog("V58 ORIGINAL2D COMPLETE | seq=%u", current);
}


// -----------------------------------------------------------------------------
// V29: interceptores GL leves. Nao alteram estado; apenas contam/observam.
// -----------------------------------------------------------------------------
static void (*glBindFramebuffer_V29_Original)(GLenum, GLuint) = nullptr;
static void (*glClearColor_V29_Original)(GLfloat, GLfloat, GLfloat, GLfloat) = nullptr;
static void (*glClear_V29_Original)(GLbitfield) = nullptr;
static void (*glDrawArrays_V29_Original)(GLenum, GLint, GLsizei) = nullptr;
static void (*glDrawElements_V29_Original)(GLenum, GLsizei, GLenum, const void*) = nullptr;

static void* g_v29BindFramebufferStub = nullptr;
static void* g_v29ClearColorStub = nullptr;
static void* g_v29ClearStub = nullptr;
static void* g_v29DrawArraysStub = nullptr;
static void* g_v29DrawElementsStub = nullptr;

static void glBindFramebuffer_V29_hook(GLenum target, GLuint framebuffer)
{
    if (!glBindFramebuffer_V29_Original)
        return;

    if (pNetGame)
    {
        const unsigned int n = g_v29FramebufferBindCount.fetch_add(1, std::memory_order_relaxed) + 1;
        g_v29LastGpuTid.store(V29GetTid(), std::memory_order_relaxed);

        if (target == GL_FRAMEBUFFER)
        {
            g_v29LastBoundFbo.store((int)framebuffer, std::memory_order_relaxed);
            if (framebuffer != 0)
                g_v29LastNonZeroFbo.store((int)framebuffer, std::memory_order_relaxed);
        }

        static std::atomic<unsigned int> totalTrace{0};
        const unsigned int t = totalTrace.fetch_add(1, std::memory_order_relaxed) + 1;
        if (t <= 24)
        {
            FLog("V29 GL BIND_FBO | call=%u frameCount=%u tid=%d target=0x%x fbo=%u",
                 t, n, V29GetTid(), (unsigned int)target, (unsigned int)framebuffer);
        }
    }

    glBindFramebuffer_V29_Original(target, framebuffer);
}

static void glClearColor_V29_hook(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    if (!glClearColor_V29_Original)
        return;

    if (pNetGame)
    {
        static std::atomic<unsigned int> totalTrace{0};
        const unsigned int t = totalTrace.fetch_add(1, std::memory_order_relaxed) + 1;
        g_v29LastGpuTid.store(V29GetTid(), std::memory_order_relaxed);

        if (t <= 16)
        {
            FLog("V29 GL CLEAR_COLOR | call=%u tid=%d rgba=%.3f,%.3f,%.3f,%.3f",
                 t, V29GetTid(), r, g, b, a);
        }
    }

    glClearColor_V29_Original(r, g, b, a);
}

static void glClear_V29_hook(GLbitfield mask)
{
    if (!glClear_V29_Original)
        return;

    if (pNetGame && (mask & GL_COLOR_BUFFER_BIT))
    {
        const unsigned int n = g_v29ColorClearCount.fetch_add(1, std::memory_order_relaxed) + 1;
        g_v29LastGpuTid.store(V29GetTid(), std::memory_order_relaxed);

        GLfloat cc[4] = {0.f, 0.f, 0.f, 0.f};
        GLint fbo = -1;
        glGetFloatv(GL_COLOR_CLEAR_VALUE, cc);
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);

        const bool black =
                cc[0] <= 0.01f &&
                cc[1] <= 0.01f &&
                cc[2] <= 0.01f;

        if (black)
            g_v29BlackClearCount.fetch_add(1, std::memory_order_relaxed);

        static std::atomic<unsigned int> totalTrace{0};
        const unsigned int t = totalTrace.fetch_add(1, std::memory_order_relaxed) + 1;
        if (t <= 24)
        {
            FLog("V29 GL CLEAR | call=%u frameCount=%u tid=%d mask=0x%x fbo=%d rgba=%.3f,%.3f,%.3f,%.3f black=%d",
                 t, n, V29GetTid(), (unsigned int)mask, (int)fbo,
                 cc[0], cc[1], cc[2], cc[3], black ? 1 : 0);
        }
    }

    glClear_V29_Original(mask);
}

static void glDrawArrays_V29_hook(GLenum mode, GLint first, GLsizei count)
{
    if (!glDrawArrays_V29_Original)
        return;

    if (pNetGame)
    {
        g_v29DrawArraysCount.fetch_add(1, std::memory_order_relaxed);
        g_v29LastGpuTid.store(V29GetTid(), std::memory_order_relaxed);
    }

    glDrawArrays_V29_Original(mode, first, count);
}

static void glDrawElements_V29_hook(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
    if (!glDrawElements_V29_Original)
        return;

    if (pNetGame)
    {
        g_v29DrawElementsCount.fetch_add(1, std::memory_order_relaxed);
        g_v29LastGpuTid.store(V29GetTid(), std::memory_order_relaxed);
    }

    glDrawElements_V29_Original(mode, count, type, indices);
}

// -----------------------------------------------------------------------------
// V29: acompanha troca de contexto EGL. Pode nao aparecer se o contexto tiver
// sido criado antes da instalacao dos hooks; nesse caso o EGLSWAP ainda informa
// o contexto/thread atual.
// -----------------------------------------------------------------------------
static EGLBoolean (*eglMakeCurrent_V29_Original)(
        EGLDisplay, EGLSurface, EGLSurface, EGLContext) = nullptr;
static void* g_v29EglMakeCurrentStub = nullptr;

static EGLBoolean eglMakeCurrent_V29_hook(
        EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
    if (!eglMakeCurrent_V29_Original)
        return EGL_FALSE;

    EGLBoolean result = eglMakeCurrent_V29_Original(dpy, draw, read, ctx);
    const EGLint makeCurrentErr = result ? EGL_SUCCESS : eglGetError();

    static std::atomic<unsigned int> totalTrace{0};
    const unsigned int t = totalTrace.fetch_add(1, std::memory_order_relaxed) + 1;
    if (t <= 24)
    {
        FLog("V29 EGL MAKECURRENT | call=%u tid=%d dpy=%p draw=%p read=%p ctx=%p result=%d err=0x%x",
             t, V29GetTid(), (void*)dpy, (void*)draw, (void*)read, (void*)ctx,
             (int)result, (unsigned int)makeCurrentErr);
    }

    return result;
}

// -----------------------------------------------------------------------------
// V29: hook da apresentacao final. Nao pinta nada.
// Resume contadores do frame e, em pontos selecionados, le 5 pixels do FBO 0.
// -----------------------------------------------------------------------------
static EGLBoolean (*eglSwapBuffers_V29_Original)(EGLDisplay, EGLSurface) = nullptr;
static void* g_v29EglSwapStub = nullptr;

static bool V29ShouldSamplePixels(unsigned int seq)
{
    return seq == 1 || seq == 30 || seq == 60 || seq == 120 ||
           seq == 180 || seq == 240 || seq == 360 || seq == 480;
}

static bool V29ShouldTraceSwap(unsigned int seq)
{
    return seq <= 60 || (seq <= 600 && (seq % 30) == 0);
}

// -----------------------------------------------------------------------------
// V30: auditoria do render target atualmente ligado.
//
// Quando o GTA entra no 3D, a V29 observou FBO 2 / viewport 960x452 antes do
// eglSwapBuffers. Agora lemos:
//   1) status + objeto do COLOR_ATTACHMENT0 do FBO nao-zero;
//   2) cinco pixels do proprio FBO interno;
//   3) cinco pixels do FBO 0 (Surface) no mesmo instante;
// e restauramos imediatamente o FBO original. Nenhum blit/clear e feito.
// -----------------------------------------------------------------------------
static std::atomic<unsigned int> g_v30NonZeroSamples{0};
static std::atomic<unsigned int> g_v30DefaultComparisons{0};

static void V30ReadFivePixels(GLint width, GLint height, GLubyte out[5][4], GLenum* outErr)
{
    memset(out, 0, 5 * 4 * sizeof(GLubyte));

    if (width <= 8 || height <= 8)
    {
        if (outErr) *outErr = GL_INVALID_VALUE;
        return;
    }

    const GLint xs[5] = {
            width / 2, width / 4, (width * 3) / 4,
            width / 4, (width * 3) / 4
    };
    const GLint ys[5] = {
            height / 2, height / 4, height / 4,
            (height * 3) / 4, (height * 3) / 4
    };

    while (glGetError() != GL_NO_ERROR) {}
    for (int i = 0; i < 5; ++i)
        glReadPixels(xs[i], ys[i], 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, out[i]);

    if (outErr) *outErr = glGetError();
}

static void V30ProbeCurrentRenderTarget(
        unsigned int swapSeq,
        int swapTid,
        GLint currentFbo,
        const GLint viewport[4],
        EGLint surfaceWidth,
        EGLint surfaceHeight)
{
    if (currentFbo <= 0 || viewport[2] <= 8 || viewport[3] <= 8)
        return;

    const unsigned int sampleNo =
            g_v30NonZeroSamples.fetch_add(1, std::memory_order_relaxed) + 1;

    // Limita o custo do glReadPixels. Queremos amostras suficientes antes e
    // depois do switch de RenderEffects, nao leitura em todos os frames.
    const int effectsPhase = g_v30EffectsPhase.load(std::memory_order_relaxed);
    const unsigned int effectsCalls =
            g_v30EffectsConnectedCount.load(std::memory_order_relaxed);

    static std::atomic<unsigned int> customSamples{0};
    static std::atomic<unsigned int> originalSamples{0};

    unsigned int phaseSample = 0;
    if (effectsPhase == 0)
        phaseSample = customSamples.fetch_add(1, std::memory_order_relaxed) + 1;
    else
        phaseSample = originalSamples.fetch_add(1, std::memory_order_relaxed) + 1;

    if (phaseSample > 6)
        return;

    while (glGetError() != GL_NO_ERROR) {}

    GLenum fbStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    GLint attachmentType = GL_NONE;
    GLint attachmentName = 0;

    glGetFramebufferAttachmentParameteriv(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &attachmentType);
    glGetFramebufferAttachmentParameteriv(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &attachmentName);
    GLenum metaErr = glGetError();

    GLubyte offscreenPx[5][4] = {};
    GLenum offscreenErr = GL_NO_ERROR;
    V30ReadFivePixels(viewport[2], viewport[3], offscreenPx, &offscreenErr);

    // Compara com o framebuffer padrao da Surface no MESMO swap.
    GLubyte defaultPx[5][4] = {};
    GLenum defaultErr = GL_NO_ERROR;
    bool defaultRead = false;

    if (surfaceWidth > 8 && surfaceHeight > 8)
    {
        if (glBindFramebuffer_V29_Original)
            glBindFramebuffer_V29_Original(GL_FRAMEBUFFER, 0);
        else
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

        GLint verifyDefault = -1;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &verifyDefault);

        if (verifyDefault == 0)
        {
            V30ReadFivePixels(surfaceWidth, surfaceHeight, defaultPx, &defaultErr);
            defaultRead = true;
            g_v30DefaultComparisons.fetch_add(1, std::memory_order_relaxed);
        }

        if (glBindFramebuffer_V29_Original)
            glBindFramebuffer_V29_Original(GL_FRAMEBUFFER, (GLuint)currentFbo);
        else
            glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)currentFbo);
    }

    GLint restoredFbo = -1;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &restoredFbo);
    GLenum restoreErr = glGetError();

    FLog("V30 FBO AUDIT | swap=%u sample=%u phase=%d phaseSample=%u effectsCalls=%u tid=%d fbo=%d viewport=%d,%d,%d,%d status=0x%x colorType=0x%x colorName=%d metaErr=0x%x restored=%d restoreErr=0x%x",
         swapSeq, sampleNo, effectsPhase, phaseSample, effectsCalls, swapTid,
         (int)currentFbo,
         (int)viewport[0], (int)viewport[1], (int)viewport[2], (int)viewport[3],
         (unsigned int)fbStatus, (unsigned int)attachmentType, (int)attachmentName,
         (unsigned int)metaErr, (int)restoredFbo, (unsigned int)restoreErr);

    FLog("V30 FBO PIXELS | swap=%u phase=%d fbo=%d p0=%u,%u,%u,%u p1=%u,%u,%u,%u p2=%u,%u,%u,%u p3=%u,%u,%u,%u p4=%u,%u,%u,%u err=0x%x",
         swapSeq, effectsPhase, (int)currentFbo,
         offscreenPx[0][0], offscreenPx[0][1], offscreenPx[0][2], offscreenPx[0][3],
         offscreenPx[1][0], offscreenPx[1][1], offscreenPx[1][2], offscreenPx[1][3],
         offscreenPx[2][0], offscreenPx[2][1], offscreenPx[2][2], offscreenPx[2][3],
         offscreenPx[3][0], offscreenPx[3][1], offscreenPx[3][2], offscreenPx[3][3],
         offscreenPx[4][0], offscreenPx[4][1], offscreenPx[4][2], offscreenPx[4][3],
         (unsigned int)offscreenErr);

    FLog("V30 FBO0 PIXELS | swap=%u phase=%d read=%d size=%dx%d p0=%u,%u,%u,%u p1=%u,%u,%u,%u p2=%u,%u,%u,%u p3=%u,%u,%u,%u p4=%u,%u,%u,%u err=0x%x",
         swapSeq, effectsPhase, defaultRead ? 1 : 0,
         (int)surfaceWidth, (int)surfaceHeight,
         defaultPx[0][0], defaultPx[0][1], defaultPx[0][2], defaultPx[0][3],
         defaultPx[1][0], defaultPx[1][1], defaultPx[1][2], defaultPx[1][3],
         defaultPx[2][0], defaultPx[2][1], defaultPx[2][2], defaultPx[2][3],
         defaultPx[3][0], defaultPx[3][1], defaultPx[3][2], defaultPx[3][3],
         defaultPx[4][0], defaultPx[4][1], defaultPx[4][2], defaultPx[4][3],
         (unsigned int)defaultErr);
}


#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER 0x8CA8
#endif
#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#endif
#ifndef GL_READ_FRAMEBUFFER_BINDING
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
#endif
#ifndef GL_DRAW_FRAMEBUFFER_BINDING
#define GL_DRAW_FRAMEBUFFER_BINDING 0x8CA6
#endif

typedef void (*V31BlitFramebufferFn)(
        GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
        GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
        GLbitfield mask, GLenum filter);

static V31BlitFramebufferFn g_v31BlitFramebuffer = nullptr;
static std::atomic<unsigned int> g_v31BlitAttempts{0};
static std::atomic<unsigned int> g_v31BlitSuccess{0};



static bool V66EnsureComposeTarget(GLint width, GLint height)
{
    if (width <= 8 || height <= 8)
        return false;

    if (g_v66ComposeFbo != 0 && g_v66ComposeTex != 0 &&
        g_v66ComposeW == width && g_v66ComposeH == height)
        return true;

    GLint savedTex = 0;
    GLint savedFbo = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &savedTex);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &savedFbo);
    while (glGetError() != GL_NO_ERROR) {}

    if (g_v66ComposeFbo != 0)
    {
        GLuint old = g_v66ComposeFbo;
        glDeleteFramebuffers(1, &old);
        g_v66ComposeFbo = 0;
    }
    if (g_v66ComposeTex != 0)
    {
        GLuint old = g_v66ComposeTex;
        glDeleteTextures(1, &old);
        g_v66ComposeTex = 0;
    }

    glGenTextures(1, &g_v66ComposeTex);
    glBindTexture(GL_TEXTURE_2D, g_v66ComposeTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glGenFramebuffers(1, &g_v66ComposeFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, g_v66ComposeFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, g_v66ComposeTex, 0);

    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    const GLenum err = glGetError();

    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)savedFbo);
    glBindTexture(GL_TEXTURE_2D, (GLuint)savedTex);

    if (status != GL_FRAMEBUFFER_COMPLETE || err != GL_NO_ERROR)
    {
        FLog("V66 COMPOSE CREATE FAIL | size=%dx%d fbo=%u tex=%u status=0x%x err=0x%x",
             (int)width, (int)height,
             (unsigned int)g_v66ComposeFbo,
             (unsigned int)g_v66ComposeTex,
             (unsigned int)status, (unsigned int)err);
        return false;
    }

    g_v66ComposeW = width;
    g_v66ComposeH = height;
    g_v66ComposeSeq.store(0, std::memory_order_release);

    FLog("V66 COMPOSE CREATE OK | size=%dx%d fbo=%u tex=%u",
         (int)width, (int)height,
         (unsigned int)g_v66ComposeFbo,
         (unsigned int)g_v66ComposeTex);
    return true;
}

static bool V66RedirectDefaultTargetToCompose(unsigned int producerSeq)
{
    if (eglGetCurrentContext() == EGL_NO_CONTEXT)
        return false;

    EGLDisplay dpy = eglGetCurrentDisplay();
    EGLSurface surf = eglGetCurrentSurface(EGL_DRAW);
    if (dpy == EGL_NO_DISPLAY || surf == EGL_NO_SURFACE)
        return false;

    EGLint dstW = 0, dstH = 0;
    if (!eglQuerySurface(dpy, surf, EGL_WIDTH, &dstW) ||
        !eglQuerySurface(dpy, surf, EGL_HEIGHT, &dstH) ||
        dstW <= 8 || dstH <= 8)
        return false;

    if (!V66EnsureComposeTarget((GLint)dstW, (GLint)dstH))
        return false;

    if (!g_v31BlitFramebuffer)
    {
        g_v31BlitFramebuffer = reinterpret_cast<V31BlitFramebufferFn>(
                eglGetProcAddress("glBlitFramebuffer"));
        if (!g_v31BlitFramebuffer)
            return false;
    }

    const unsigned int oldSeq =
            g_v66ComposeSeq.load(std::memory_order_acquire);

    GLint savedViewport[4] = {0,0,0,0};
    GLint savedScissor[4] = {0,0,0,0};
    const GLboolean scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
    glGetIntegerv(GL_VIEWPORT, savedViewport);
    glGetIntegerv(GL_SCISSOR_BOX, savedScissor);

    // First default-target selection for this 2D frame: seed our private
    // composition target with the completed world image.
    if (oldSeq != producerSeq)
    {
        const GLint worldFbo =
                (GLint)g_v65WorldFbo.load(std::memory_order_acquire);
        const GLint x = (GLint)g_v65WorldVpX.load(std::memory_order_acquire);
        const GLint y = (GLint)g_v65WorldVpY.load(std::memory_order_acquire);
        const GLint w = (GLint)g_v65WorldVpW.load(std::memory_order_acquire);
        const GLint h = (GLint)g_v65WorldVpH.load(std::memory_order_acquire);

        if (worldFbo <= 0 || w <= 8 || h <= 8)
            return false;

        while (glGetError() != GL_NO_ERROR) {}
        glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)worldFbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, g_v66ComposeFbo);

        const GLenum readStatus = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
        const GLenum drawStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

        glDisable(GL_SCISSOR_TEST);
        g_v31BlitFramebuffer(x, y, x + w, y + h,
                             0, 0, dstW, dstH,
                             GL_COLOR_BUFFER_BIT, GL_NEAREST);
        const GLenum blitErr = glGetError();

        // Keep the native viewport/scissor selected by the original target
        // command, but bind our offscreen frame instead of FBO0.
        glBindFramebuffer(GL_FRAMEBUFFER, g_v66ComposeFbo);
        glViewport(savedViewport[0], savedViewport[1],
                   savedViewport[2], savedViewport[3]);
        if (scissorEnabled) glEnable(GL_SCISSOR_TEST);
        else glDisable(GL_SCISSOR_TEST);
        glScissor(savedScissor[0], savedScissor[1],
                  savedScissor[2], savedScissor[3]);

        if (readStatus != GL_FRAMEBUFFER_COMPLETE ||
            drawStatus != GL_FRAMEBUFFER_COMPLETE ||
            blitErr != GL_NO_ERROR)
        {
            FLog("V66 COMPOSE SEED FAIL | seq=%u world=%d compose=%u read=0x%x draw=0x%x err=0x%x",
                 producerSeq, (int)worldFbo,
                 (unsigned int)g_v66ComposeFbo,
                 (unsigned int)readStatus,
                 (unsigned int)drawStatus,
                 (unsigned int)blitErr);
            return false;
        }

        g_v66ComposeSeq.store(producerSeq, std::memory_order_release);

        if (producerSeq <= 16 || (producerSeq % 120u) == 0u)
        {
            FLog("V66 COMPOSE SEED+REDIRECT | seq=%u tid=%d world=%d worldVp=%d,%d %dx%d compose=%u size=%dx%d viewport=%d,%d %dx%d",
                 producerSeq, V29GetTid(), (int)worldFbo,
                 (int)x, (int)y, (int)w, (int)h,
                 (unsigned int)g_v66ComposeFbo,
                 (int)dstW, (int)dstH,
                 savedViewport[0], savedViewport[1],
                 savedViewport[2], savedViewport[3]);
        }
    }
    else
    {
        glBindFramebuffer(GL_FRAMEBUFFER, g_v66ComposeFbo);
        glViewport(savedViewport[0], savedViewport[1],
                   savedViewport[2], savedViewport[3]);
        if (scissorEnabled) glEnable(GL_SCISSOR_TEST);
        else glDisable(GL_SCISSOR_TEST);
        glScissor(savedScissor[0], savedScissor[1],
                  savedScissor[2], savedScissor[3]);
    }

    g_v66RedirectCount.fetch_add(1, std::memory_order_relaxed);
    return true;
}

static bool V66PresentComposeToDefault(unsigned int swapSeq, int swapTid,
                                       unsigned int completedSeq,
                                       GLint restoreFbo,
                                       EGLint surfaceWidth, EGLint surfaceHeight)
{
    if (completedSeq == 0 ||
        g_v66ComposeSeq.load(std::memory_order_acquire) != completedSeq ||
        g_v66ComposeFbo == 0 || g_v66ComposeW <= 8 || g_v66ComposeH <= 8 ||
        !g_v31BlitFramebuffer)
        return false;

    GLint savedRead = restoreFbo;
    GLint savedDraw = restoreFbo;
    GLint savedViewport[4] = {0,0,0,0};
    GLint savedScissor[4] = {0,0,0,0};
    const GLboolean scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);

    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &savedRead);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &savedDraw);
    glGetIntegerv(GL_VIEWPORT, savedViewport);
    glGetIntegerv(GL_SCISSOR_BOX, savedScissor);
    while (glGetError() != GL_NO_ERROR) {}

    glBindFramebuffer(GL_READ_FRAMEBUFFER, g_v66ComposeFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    const GLenum readStatus = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
    const GLenum drawStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

    glDisable(GL_SCISSOR_TEST);
    g_v31BlitFramebuffer(0, 0, g_v66ComposeW, g_v66ComposeH,
                         0, 0, surfaceWidth, surfaceHeight,
                         GL_COLOR_BUFFER_BIT, GL_NEAREST);
    const GLenum err = glGetError();

    // Sample FBO0 after the composed copy for diagnosis.
    GLubyte px[4] = {};
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glReadPixels(surfaceWidth / 2, surfaceHeight / 2, 1, 1,
                 GL_RGBA, GL_UNSIGNED_BYTE, px);
    const GLenum readErr = glGetError();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)savedRead);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)savedDraw);
    glViewport(savedViewport[0], savedViewport[1],
               savedViewport[2], savedViewport[3]);
    if (scissorEnabled) glEnable(GL_SCISSOR_TEST);
    else glDisable(GL_SCISSOR_TEST);
    glScissor(savedScissor[0], savedScissor[1],
              savedScissor[2], savedScissor[3]);

    const bool ok = (readStatus == GL_FRAMEBUFFER_COMPLETE &&
                     drawStatus == GL_FRAMEBUFFER_COMPLETE &&
                     err == GL_NO_ERROR);

    if (completedSeq <= 16 || (completedSeq % 120u) == 0u)
    {
        FLog("V66 PRESENT COMPOSED | swap=%u tid=%d seq=%u compose=%u size=%dx%d dst=%dx%d read=0x%x draw=0x%x err=0x%x center=%u,%u,%u,%u readErr=0x%x ok=%d redirects=%u",
             swapSeq, swapTid, completedSeq,
             (unsigned int)g_v66ComposeFbo,
             (int)g_v66ComposeW, (int)g_v66ComposeH,
             (int)surfaceWidth, (int)surfaceHeight,
             (unsigned int)readStatus, (unsigned int)drawStatus,
             (unsigned int)err,
             px[0], px[1], px[2], px[3],
             (unsigned int)readErr, ok ? 1 : 0,
             g_v66RedirectCount.load(std::memory_order_relaxed));
    }

    return ok;
}

static void V65ObserveAndMaybePrecompose(GLint currentFbo)
{
    if (eglGetCurrentContext() == EGL_NO_CONTEXT)
        return;

    // V66 composition target is not a world target.  Never let the legacy
    // V65 observer replace the remembered scene FBO with our private FBO.
    if (g_v66ComposeFbo != 0 && currentFbo == (GLint)g_v66ComposeFbo)
        return;

    // Any non-zero target observed by the real RQ consumer is a candidate for
    // the offscreen world target. Keep the latest valid viewport with it.
    if (currentFbo > 0)
    {
        GLint vp[4] = {0, 0, 0, 0};
        glGetIntegerv(GL_VIEWPORT, vp);
        if (vp[2] > 8 && vp[3] > 8)
        {
            g_v65WorldFbo.store((int)currentFbo, std::memory_order_release);
            g_v65WorldVpX.store((int)vp[0], std::memory_order_release);
            g_v65WorldVpY.store((int)vp[1], std::memory_order_release);
            g_v65WorldVpW.store((int)vp[2], std::memory_order_release);
            g_v65WorldVpH.store((int)vp[3], std::memory_order_release);
        }
        return;
    }

    // Only the first draw after RQ has switched to the default framebuffer
    // needs the world precomposition. Every following FBO0 command is allowed
    // to draw naturally over that copied world image.
    if (currentFbo != 0)
        return;

    const unsigned int producerSeq =
            g_v65Producer2DSeq.load(std::memory_order_acquire);
    if (producerSeq == 0)
        return;

    if (g_v65Precomposed2DSeq.load(std::memory_order_acquire) == producerSeq)
        return;

    const GLint sourceFbo =
            (GLint)g_v65WorldFbo.load(std::memory_order_acquire);
    const GLint srcX =
            (GLint)g_v65WorldVpX.load(std::memory_order_acquire);
    const GLint srcY =
            (GLint)g_v65WorldVpY.load(std::memory_order_acquire);
    const GLint srcW =
            (GLint)g_v65WorldVpW.load(std::memory_order_acquire);
    const GLint srcH =
            (GLint)g_v65WorldVpH.load(std::memory_order_acquire);

    if (sourceFbo <= 0 || srcW <= 8 || srcH <= 8)
        return;

    EGLSurface drawSurface = eglGetCurrentSurface(EGL_DRAW);
    if (drawSurface == EGL_NO_SURFACE)
        return;

    EGLDisplay dpy = eglGetCurrentDisplay();
    if (dpy == EGL_NO_DISPLAY)
        return;

    EGLint dstW = 0;
    EGLint dstH = 0;
    if (!eglQuerySurface(dpy, drawSurface, EGL_WIDTH, &dstW) ||
        !eglQuerySurface(dpy, drawSurface, EGL_HEIGHT, &dstH) ||
        dstW <= 8 || dstH <= 8)
        return;

    if (!g_v31BlitFramebuffer)
    {
        g_v31BlitFramebuffer =
                reinterpret_cast<V31BlitFramebufferFn>(
                        eglGetProcAddress("glBlitFramebuffer"));
        if (!g_v31BlitFramebuffer)
        {
            static std::atomic<unsigned int> noProcLogs{0};
            if (noProcLogs.fetch_add(1, std::memory_order_relaxed) < 4)
            {
                FLog("V65 PRECOMPOSE SKIP | seq=%u reason=no_glBlitFramebuffer",
                     producerSeq);
            }
            return;
        }
    }

    GLint savedReadFbo = 0;
    GLint savedDrawFbo = 0;
    GLint savedViewport[4] = {0, 0, 0, 0};
    GLint savedScissorBox[4] = {0, 0, 0, 0};
    const GLboolean savedScissor = glIsEnabled(GL_SCISSOR_TEST);

    while (glGetError() != GL_NO_ERROR) {}
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &savedReadFbo);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &savedDrawFbo);
    glGetIntegerv(GL_VIEWPORT, savedViewport);
    glGetIntegerv(GL_SCISSOR_BOX, savedScissorBox);
    GLenum queryErr = glGetError();

    if (queryErr != GL_NO_ERROR)
    {
        // We are entering from an FBO0 draw handler, so 0 is the safest
        // fallback binding if split read/draw queries are unsupported.
        savedReadFbo = 0;
        savedDrawFbo = 0;
    }

    // Use the public GL API on the GraphicsThread and restore every modified
    // state before the original RQ draw handler executes.
    glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)sourceFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    const GLenum readStatus = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
    const GLenum drawStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    const GLenum statusErr = glGetError();

    bool ok = false;
    GLenum blitErr = GL_NO_ERROR;

    if (readStatus == GL_FRAMEBUFFER_COMPLETE &&
        drawStatus == GL_FRAMEBUFFER_COMPLETE &&
        statusErr == GL_NO_ERROR)
    {
        glDisable(GL_SCISSOR_TEST);

        g_v31BlitFramebuffer(
                srcX, srcY, srcX + srcW, srcY + srcH,
                0, 0, dstW, dstH,
                GL_COLOR_BUFFER_BIT, GL_NEAREST);

        blitErr = glGetError();
        ok = (blitErr == GL_NO_ERROR);
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)savedReadFbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)savedDrawFbo);
    glViewport(savedViewport[0], savedViewport[1],
               savedViewport[2], savedViewport[3]);

    if (savedScissor) glEnable(GL_SCISSOR_TEST);
    else glDisable(GL_SCISSOR_TEST);

    glScissor(savedScissorBox[0], savedScissorBox[1],
              savedScissorBox[2], savedScissorBox[3]);

    const GLenum restoreErr = glGetError();

    if (ok)
    {
        g_v65Precomposed2DSeq.store(
                producerSeq, std::memory_order_release);

        if (producerSeq <= 16 || (producerSeq % 120u) == 0u)
        {
            FLog("V65 PRECOMPOSE WORLD->FBO0 | seq=%u tid=%d srcFbo=%d src=%d,%d %dx%d dst=%dx%d read=0x%x draw=0x%x blitErr=0x%x restoreErr=0x%x",
                 producerSeq, V29GetTid(), (int)sourceFbo,
                 (int)srcX, (int)srcY, (int)srcW, (int)srcH,
                 (int)dstW, (int)dstH,
                 (unsigned int)readStatus, (unsigned int)drawStatus,
                 (unsigned int)blitErr, (unsigned int)restoreErr);
        }
    }
    else if (producerSeq <= 8)
    {
        FLog("V65 PRECOMPOSE FAIL | seq=%u tid=%d srcFbo=%d src=%dx%d dst=%dx%d read=0x%x draw=0x%x statusErr=0x%x blitErr=0x%x restoreErr=0x%x",
             producerSeq, V29GetTid(), (int)sourceFbo,
             (int)srcW, (int)srcH, (int)dstW, (int)dstH,
             (unsigned int)readStatus, (unsigned int)drawStatus,
             (unsigned int)statusErr, (unsigned int)blitErr,
             (unsigned int)restoreErr);
    }
}

// V34: controlled A/B test for the final scene blit.
// A: keep the proven V31 FBO2 -> FBO0 blit for 360 valid offscreen frames.
// B: skip ONLY that blit for the next 360 valid offscreen frames.
// C: restore the V31 blit indefinitely.
// Nothing else in the render, camera, network or TextDraw path is changed.
static std::atomic<unsigned int> g_v34EligiblePresentFrames{0};
static std::atomic<int> g_v34LastPhase{0};

static const char* V34PhaseName(int phase)
{
    switch (phase)
    {
        case 1: return "A_BLIT_ON";
        case 2: return "B_NO_BLIT";
        case 3: return "C_BLIT_ON";
        default: return "UNKNOWN";
    }
}

static int V34PhaseForEligibleFrame(unsigned int frame)
{
    if (frame <= 360) return 1;
    if (frame <= 720) return 2;
    return 3;
}

static void V34ProbeRawDefaultFbo(
        unsigned int swapSeq,
        unsigned int eligibleFrame,
        EGLint surfaceWidth,
        EGLint surfaceHeight)
{
    if (surfaceWidth <= 8 || surfaceHeight <= 8)
        return;

    GLint savedReadFbo = 0;
    while (glGetError() != GL_NO_ERROR) {}
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &savedReadFbo);
    GLenum queryErr = glGetError();

    auto BindFbo = [](GLenum target, GLuint fbo)
    {
        if (glBindFramebuffer_V29_Original)
            glBindFramebuffer_V29_Original(target, fbo);
        else
            glBindFramebuffer(target, fbo);
    };

    BindFbo(GL_READ_FRAMEBUFFER, 0);

    unsigned int nonBlack = 0;
    unsigned int maxRgb = 0;
    unsigned int alphaNonZero = 0;
    GLubyte px[4] = {};

    while (glGetError() != GL_NO_ERROR) {}
    for (int gy = 1; gy <= 5; ++gy)
    {
        for (int gx = 1; gx <= 5; ++gx)
        {
            const GLint x = (surfaceWidth * gx) / 6;
            const GLint y = (surfaceHeight * gy) / 6;
            px[0] = px[1] = px[2] = px[3] = 0;
            glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
            const unsigned int rgb = (unsigned int)px[0] + px[1] + px[2];
            if (rgb > 12) ++nonBlack;
            if (rgb > maxRgb) maxRgb = rgb;
            if (px[3] != 0) ++alphaNonZero;
        }
    }
    GLenum readErr = glGetError();

    BindFbo(GL_READ_FRAMEBUFFER, (GLuint)savedReadFbo);
    GLenum restoreErr = glGetError();

    FLog("V34 RAW FBO0 | swap=%u eligible=%u grid=25 nonBlack=%u maxRgb=%u alphaNonZero=%u queryErr=0x%x readErr=0x%x restoreErr=0x%x",
         swapSeq, eligibleFrame, nonBlack, maxRgb, alphaNonZero,
         (unsigned int)queryErr, (unsigned int)readErr, (unsigned int)restoreErr);
}

static bool V31PresentOffscreenToDefault(
        unsigned int swapSeq,
        int swapTid,
        GLint sourceFbo,
        const GLint sourceViewport[4],
        EGLint surfaceWidth,
        EGLint surfaceHeight)
{
    if (sourceFbo <= 0 ||
        sourceViewport[2] <= 8 || sourceViewport[3] <= 8 ||
        surfaceWidth <= 8 || surfaceHeight <= 8)
        return false;

    if (!g_v31BlitFramebuffer)
    {
        g_v31BlitFramebuffer =
                reinterpret_cast<V31BlitFramebufferFn>(
                        eglGetProcAddress("glBlitFramebuffer"));

        FLog("V31 BLIT PROC | ptr=%p tid=%d ctx=%p",
             (void*)g_v31BlitFramebuffer, swapTid, (void*)eglGetCurrentContext());

        if (!g_v31BlitFramebuffer)
            return false;
    }

    // O FBO atual (sourceFbo) foi confirmado pela V30 como o render target da cena.
    while (glGetError() != GL_NO_ERROR) {}

    GLenum sourceStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    GLenum statusErr = glGetError();

    if (sourceStatus != GL_FRAMEBUFFER_COMPLETE || statusErr != GL_NO_ERROR)
    {
        static unsigned int badStatusLogs = 0;
        if (badStatusLogs++ < 8)
        {
            FLog("V31 BLIT SKIP | seq=%u reason=source_status fbo=%d status=0x%x err=0x%x",
                 swapSeq, (int)sourceFbo,
                 (unsigned int)sourceStatus, (unsigned int)statusErr);
        }
        return false;
    }

    GLint savedReadFbo = sourceFbo;
    GLint savedDrawFbo = sourceFbo;
    GLint savedViewport[4] = {
            sourceViewport[0], sourceViewport[1],
            sourceViewport[2], sourceViewport[3]
    };
    GLboolean savedScissor = glIsEnabled(GL_SCISSOR_TEST);
    GLint savedScissorBox[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_SCISSOR_BOX, savedScissorBox);

    // ES3 separa READ e DRAW FBO. Se a consulta falhar, os defaults acima
    // ainda restauram o FBO observado imediatamente antes do swap.
    while (glGetError() != GL_NO_ERROR) {}
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &savedReadFbo);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &savedDrawFbo);
    GLenum bindingQueryErr = glGetError();
    if (bindingQueryErr != GL_NO_ERROR)
    {
        savedReadFbo = sourceFbo;
        savedDrawFbo = sourceFbo;
    }

    auto BindFbo = [](GLenum target, GLuint fbo)
    {
        if (glBindFramebuffer_V29_Original)
            glBindFramebuffer_V29_Original(target, fbo);
        else
            glBindFramebuffer(target, fbo);
    };

    const unsigned int attempt =
            g_v31BlitAttempts.fetch_add(1, std::memory_order_relaxed) + 1;

    while (glGetError() != GL_NO_ERROR) {}

    BindFbo(GL_READ_FRAMEBUFFER, (GLuint)sourceFbo);
    BindFbo(GL_DRAW_FRAMEBUFFER, 0);

    GLenum readStatus = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
    GLenum drawStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    GLenum preBlitErr = glGetError();

    if (readStatus != GL_FRAMEBUFFER_COMPLETE ||
        drawStatus != GL_FRAMEBUFFER_COMPLETE ||
        preBlitErr != GL_NO_ERROR)
    {
        if (attempt <= 12)
        {
            FLog("V31 BLIT SKIP | seq=%u attempt=%u reason=split_status src=%d read=0x%x draw=0x%x err=0x%x",
                 swapSeq, attempt, (int)sourceFbo,
                 (unsigned int)readStatus, (unsigned int)drawStatus,
                 (unsigned int)preBlitErr);
        }

        BindFbo(GL_READ_FRAMEBUFFER, (GLuint)savedReadFbo);
        BindFbo(GL_DRAW_FRAMEBUFFER, (GLuint)savedDrawFbo);
        glViewport(savedViewport[0], savedViewport[1],
                   savedViewport[2], savedViewport[3]);
        if (savedScissor) glEnable(GL_SCISSOR_TEST);
        else glDisable(GL_SCISSOR_TEST);
        glScissor(savedScissorBox[0], savedScissorBox[1],
                  savedScissorBox[2], savedScissorBox[3]);
        return false;
    }

    // Scissor pode limitar o destino do blit. Desativamos somente durante a
    // copia e restauramos logo em seguida.
    glDisable(GL_SCISSOR_TEST);

    g_v31BlitFramebuffer(
            0, 0, sourceViewport[2], sourceViewport[3],
            0, 0, surfaceWidth, surfaceHeight,
            GL_COLOR_BUFFER_BIT, GL_NEAREST);

    GLenum blitErr = glGetError();

    // Confirma alguns pixels do FBO0 apenas nos primeiros frames do teste.
    GLubyte defaultPx[5][4] = {};
    GLenum readbackErr = GL_NO_ERROR;
    bool didReadback = false;
    if (attempt <= 8)
    {
        // glReadPixels le o READ_FRAMEBUFFER em ES3. Troca apenas a leitura
        // para o FBO0 para confirmar que o blit realmente chegou na Surface.
        BindFbo(GL_READ_FRAMEBUFFER, 0);
        V30ReadFivePixels(surfaceWidth, surfaceHeight, defaultPx, &readbackErr);
        didReadback = true;
    }

    BindFbo(GL_READ_FRAMEBUFFER, (GLuint)savedReadFbo);
    BindFbo(GL_DRAW_FRAMEBUFFER, (GLuint)savedDrawFbo);
    glViewport(savedViewport[0], savedViewport[1],
               savedViewport[2], savedViewport[3]);
    if (savedScissor) glEnable(GL_SCISSOR_TEST);
    else glDisable(GL_SCISSOR_TEST);
    glScissor(savedScissorBox[0], savedScissorBox[1],
              savedScissorBox[2], savedScissorBox[3]);

    GLenum restoreErr = glGetError();

    if (blitErr == GL_NO_ERROR)
        g_v31BlitSuccess.fetch_add(1, std::memory_order_relaxed);

    if (attempt <= 12 || (swapSeq % 120) == 0)
    {
        FLog("V31 BLIT | seq=%u attempt=%u tid=%d srcFbo=%d src=%dx%d dstFbo=0 dst=%dx%d readStatus=0x%x drawStatus=0x%x blitErr=0x%x readback=%d p0=%u,%u,%u,%u p1=%u,%u,%u,%u p2=%u,%u,%u,%u p3=%u,%u,%u,%u p4=%u,%u,%u,%u readErr=0x%x restoreErr=0x%x successTotal=%u",
             swapSeq, attempt, swapTid, (int)sourceFbo,
             (int)sourceViewport[2], (int)sourceViewport[3],
             (int)surfaceWidth, (int)surfaceHeight,
             (unsigned int)readStatus, (unsigned int)drawStatus,
             (unsigned int)blitErr, didReadback ? 1 : 0,
             defaultPx[0][0], defaultPx[0][1], defaultPx[0][2], defaultPx[0][3],
             defaultPx[1][0], defaultPx[1][1], defaultPx[1][2], defaultPx[1][3],
             defaultPx[2][0], defaultPx[2][1], defaultPx[2][2], defaultPx[2][3],
             defaultPx[3][0], defaultPx[3][1], defaultPx[3][2], defaultPx[3][3],
             defaultPx[4][0], defaultPx[4][1], defaultPx[4][2], defaultPx[4][3],
             (unsigned int)readbackErr, (unsigned int)restoreErr,
             g_v31BlitSuccess.load(std::memory_order_relaxed));
    }

    return blitErr == GL_NO_ERROR;
}


// =============================================================================
// V40 - ONE-SHOT FBO ENUMERATION
//
// V39 proved the 2D pass finishes and emu_FlushAltRenderTarget() is executed
// after HUD/TextDraw/UI, but the visible result still contains only the 3D
// scene. The next question is whether the 2D pixels live in ANOTHER framebuffer
// object while V31 keeps copying FBO 2.
//
// This probe runs once on the EGL/swap thread (valid GL context), while the
// V38 mutex excludes the 2D producer. It does not clear, draw, delete, attach,
// or modify framebuffer storage. It only binds existing FBO ids temporarily,
// checks completeness, reads a few pixels, logs attachment metadata, and then
// restores the original framebuffer.
//
// No private RenderQueue calls are used.
// =============================================================================
static std::atomic<bool> g_v40FboEnumDone{false};

static void V40EnumerateFramebufferObjectsOnce(
        unsigned int swapSeq,
        int swapTid,
        GLint originalFbo)
{
    if (g_v40FboEnumDone.exchange(true, std::memory_order_acq_rel))
        return;

    GLint savedFbo = originalFbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &savedFbo);

    FLog("V40 FBO ENUM BEGIN | swap=%u tid=%d originalFbo=%d",
         swapSeq, swapTid, (int)savedFbo);

    for (GLuint id = 1; id <= 64; ++id)
    {
        if (glIsFramebuffer(id) != GL_TRUE)
            continue;

        while (glGetError() != GL_NO_ERROR) {}

        glBindFramebuffer(GL_FRAMEBUFFER, id);
        const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

        GLint colorType = 0;
        GLint colorName = 0;
        glGetFramebufferAttachmentParameteriv(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
                &colorType);
        glGetFramebufferAttachmentParameteriv(
                GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                &colorName);

        GLubyte p0[4] = {};
        GLubyte p1[4] = {};
        GLubyte p2[4] = {};
        GLenum readErr = GL_NO_ERROR;

        if (status == GL_FRAMEBUFFER_COMPLETE)
        {
            glReadPixels(1, 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p0);
            glReadPixels(32, 32, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p1);
            glReadPixels(128, 96, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, p2);
            readErr = glGetError();
        }
        else
        {
            readErr = glGetError();
        }

        FLog("V40 FBO ENUM | id=%u status=0x%x colorType=0x%x colorName=%d "
             "p0=%u,%u,%u,%u p1=%u,%u,%u,%u p2=%u,%u,%u,%u err=0x%x",
             (unsigned int)id,
             (unsigned int)status,
             (unsigned int)colorType,
             (int)colorName,
             p0[0], p0[1], p0[2], p0[3],
             p1[0], p1[1], p1[2], p1[3],
             p2[0], p2[1], p2[2], p2[3],
             (unsigned int)readErr);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)savedFbo);
    const GLenum restoreErr = glGetError();

    FLog("V40 FBO ENUM END | swap=%u tid=%d restored=%d err=0x%x",
         swapSeq, swapTid, (int)savedFbo, (unsigned int)restoreErr);
}


// =============================================================================
// V60 - PRESERVE FBO0 / NO-BLIT A/B TEST
//
// V59 ruled out "current FBO - 1" as the missing HUD/TextDraw source.
// New hypothesis: GTA/SA-MP 2D may already be drawn into FBO0 between swaps,
// while our full-screen V31 blit (FBO16 -> FBO0) overwrites those sparse 2D
// pixels immediately before presentation.
//
// This test is intentionally simple and safe:
//   phase A: normal V31 world blit (baseline)
//   phase B: NO manual blit; preserve FBO0 exactly as it is
//   phase C: normal V31 world blit again
//
// If phase B shows HUD/chat/TextDraw on a black/partial background, that proves
// the missing 2D already exists in FBO0 and is being erased by the world blit.
// No private RenderQueue functions are called.
// =============================================================================
static std::atomic<unsigned int> g_v60EligibleFrames{0};
static std::atomic<int> g_v60LastPhase{0};

static const char* V60PhaseName(int phase)
{
    switch (phase)
    {
        case 1: return "A_WORLD_BLIT";
        case 2: return "B_PRESERVE_FBO0";
        default: return "C_WORLD_BLIT";
    }
}

static int V60PhaseForFrame(unsigned int frame)
{
    if (frame <= 180) return 1;
    if (frame <= 480) return 2;
    return 3;
}

static bool V60PresentPreserveAB(
        unsigned int swapSeq,
        int swapTid,
        GLint currentFbo,
        const GLint viewport[4],
        EGLint surfaceWidth,
        EGLint surfaceHeight)
{
    const unsigned int frame =
            g_v60EligibleFrames.fetch_add(1, std::memory_order_relaxed) + 1;
    const int phase = V60PhaseForFrame(frame);
    const int oldPhase =
            g_v60LastPhase.exchange(phase, std::memory_order_relaxed);

    if (phase != oldPhase)
    {
        FLog("V60 PRESENT PHASE | eligible=%u phase=%d name=%s current=%d",
             frame, phase, V60PhaseName(phase), (int)currentFbo);
    }

    // Phase B deliberately does not touch FBO0. The original eglSwapBuffers
    // below will present whatever 2D the engine has already placed there.
    if (phase == 2)
    {
        if (frame <= 190 || frame == 240 || frame == 360 || frame == 480)
        {
            GLint savedFbo = currentFbo;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &savedFbo);
            while (glGetError() != GL_NO_ERROR) {}

            if (glBindFramebuffer_V29_Original)
                glBindFramebuffer_V29_Original(GL_FRAMEBUFFER, 0);
            else
                glBindFramebuffer(GL_FRAMEBUFFER, 0);

            const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            const GLenum statusErr = glGetError();

            GLubyte px[5][4] = {};
            const GLint xs[5] = {
                    surfaceWidth / 2,
                    surfaceWidth / 10,
                    (surfaceWidth * 9) / 10,
                    surfaceWidth / 10,
                    (surfaceWidth * 9) / 10
            };
            const GLint ys[5] = {
                    surfaceHeight / 2,
                    surfaceHeight / 10,
                    surfaceHeight / 10,
                    (surfaceHeight * 9) / 10,
                    (surfaceHeight * 9) / 10
            };

            for (int i = 0; i < 5; ++i)
                glReadPixels(xs[i], ys[i], 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px[i]);

            const GLenum readErr = glGetError();

            if (glBindFramebuffer_V29_Original)
                glBindFramebuffer_V29_Original(GL_FRAMEBUFFER, (GLuint)savedFbo);
            else
                glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)savedFbo);

            glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
            const GLenum restoreErr = glGetError();

            FLog("V60 PRESERVE FBO0 | eligible=%u swap=%u tid=%d status=0x%x statusErr=0x%x "
                 "p0=%u,%u,%u,%u p1=%u,%u,%u,%u p2=%u,%u,%u,%u "
                 "p3=%u,%u,%u,%u p4=%u,%u,%u,%u readErr=0x%x restoreErr=0x%x",
                 frame, swapSeq, swapTid,
                 (unsigned int)status, (unsigned int)statusErr,
                 px[0][0],px[0][1],px[0][2],px[0][3],
                 px[1][0],px[1][1],px[1][2],px[1][3],
                 px[2][0],px[2][1],px[2][2],px[2][3],
                 px[3][0],px[3][1],px[3][2],px[3][3],
                 px[4][0],px[4][1],px[4][2],px[4][3],
                 (unsigned int)readErr, (unsigned int)restoreErr);
        }

        return true;
    }

    const bool ok = V31PresentOffscreenToDefault(
            swapSeq, swapTid, currentFbo, viewport,
            surfaceWidth, surfaceHeight);

    if (frame <= 12 || frame == 180 || frame == 181 ||
        frame == 480 || frame == 481 || (frame % 180) == 0)
    {
        FLog("V60 WORLD BLIT | eligible=%u swap=%u tid=%d phase=%s source=%d ok=%d",
             frame, swapSeq, swapTid, V60PhaseName(phase),
             (int)currentFbo, ok ? 1 : 0);
    }

    return ok;
}


// =============================================================================
// V61 - NEXT-3D GATE
//
// V60 proved FBO0 does not contain the missing HUD/TextDraw layer. The more
// important ordering visible in the log is:
//   swap/blit -> Render2dStuff -> next 3D pass
// so the next 3D frame can overwrite the offscreen target before the graphics
// thread presents the 2D commands.
//
// Do not wait inside Render2dStuff (V56 deadlocked/timed out there). Instead,
// after Render2dStuff has returned, stop at the FIRST 3D stage of the next
// frame until the EGL thread acknowledges that the latest completed 2D pass
// was presented. This leaves the render thread free to execute/swap the
// previous frame while preventing the producer from queueing the next 3D pass.
// =============================================================================
static std::atomic<unsigned int> g_v61GateWaits{0};
static std::atomic<unsigned int> g_v61GatePresented{0};
static std::atomic<unsigned int> g_v61GateTimeouts{0};

static void V61WaitForPrevious2DAtNext3D(unsigned int barSeq)
{
    const unsigned int target =
            g_v37TwoDCompleted.load(std::memory_order_acquire);
    if (target == 0)
        return;

    unsigned int presented =
            g_v56PresentedTwoD.load(std::memory_order_acquire);
    if (presented >= target)
        return;

    const unsigned int waitNo =
            g_v61GateWaits.fetch_add(1, std::memory_order_relaxed) + 1;

    if (waitNo <= 16 || (waitNo % 120) == 0)
    {
        FLog("V61 NEXT3D WAIT BEGIN | wait=%u barSeq=%u target2d=%u presented=%u tid=%d",
             waitNo, barSeq, target, presented, V29GetTid());
    }

    // Bounded wait: diagnostics must never be able to freeze the game.
    // Render2dStuff has already returned, so the graphics/EGL thread is free
    // to consume the queued 2D work and call eglSwapBuffers.
    unsigned int waitedMs = 0;
    while (waitedMs < 50)
    {
        usleep(1000);
        ++waitedMs;

        presented = g_v56PresentedTwoD.load(std::memory_order_acquire);
        if (presented >= target)
            break;
    }

    const bool ok = (presented >= target);
    if (ok)
        g_v61GatePresented.fetch_add(1, std::memory_order_relaxed);
    else
        g_v61GateTimeouts.fetch_add(1, std::memory_order_relaxed);

    if (waitNo <= 16 || !ok || (waitNo % 120) == 0)
    {
        FLog("V61 NEXT3D WAIT END | wait=%u barSeq=%u target2d=%u presented=%u waitedMs=%u result=%s",
             waitNo, barSeq, target, presented, waitedMs,
             ok ? "PRESENTED" : "TIMEOUT");
    }
}

static bool V61PresentCurrentWorld(
        unsigned int swapSeq,
        int swapTid,
        GLint currentFbo,
        const GLint viewport[4],
        EGLint surfaceWidth,
        EGLint surfaceHeight)
{
    const bool ok = V31PresentOffscreenToDefault(
            swapSeq, swapTid, currentFbo, viewport,
            surfaceWidth, surfaceHeight);

    static std::atomic<unsigned int> seq{0};
    const unsigned int n = seq.fetch_add(1, std::memory_order_relaxed) + 1;
    if (n <= 12 || (n % 120) == 0)
    {
        FLog("V61 PRESENT CURRENT | n=%u swap=%u tid=%d source=%d ok=%d completed2d=%u",
             n, swapSeq, swapTid, (int)currentFbo, ok ? 1 : 0,
             g_v37TwoDCompleted.load(std::memory_order_acquire));
    }
    return ok;
}

static EGLBoolean eglSwapBuffers_V29_hook(EGLDisplay dpy, EGLSurface surface)
{
    if (!eglSwapBuffers_V29_Original)
        return EGL_FALSE;

    if (!pNetGame)
        return eglSwapBuffers_V29_Original(dpy, surface);

    static unsigned int seq = 0;
    const unsigned int current = ++seq;
    const int swapTid = V29GetTid();

    // Snapshot at hook entry. If 2D changes while this swap hook is running,
    // this swap is considered concurrent/too early for the manual V31 copy.
    const bool v37In2DAtEntry =
            g_v37TwoDInProgress.load(std::memory_order_acquire);
    const unsigned int v37CompletedAtEntry =
            g_v37TwoDCompleted.load(std::memory_order_acquire);

    EGLDisplay currentDisplay = eglGetCurrentDisplay();
    EGLContext currentContext = eglGetCurrentContext();
    EGLSurface currentDraw = eglGetCurrentSurface(EGL_DRAW);

    EGLint width = -1;
    EGLint height = -1;
    EGLBoolean qWidth = EGL_FALSE;
    EGLBoolean qHeight = EGL_FALSE;

    if (dpy != EGL_NO_DISPLAY && surface != EGL_NO_SURFACE)
    {
        qWidth = eglQuerySurface(dpy, surface, EGL_WIDTH, &width);
        qHeight = eglQuerySurface(dpy, surface, EGL_HEIGHT, &height);
    }

    GLint fbo = -1;
    GLint viewport[4] = {-1, -1, -1, -1};
    GLenum glErrBefore = GL_NO_ERROR;

    if (currentContext != EGL_NO_CONTEXT)
    {
        while (glGetError() != GL_NO_ERROR) {}
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
        glGetIntegerv(GL_VIEWPORT, viewport);
        glErrBefore = glGetError();
    }

    const unsigned int drawsA =
            g_v29DrawArraysCount.exchange(0, std::memory_order_relaxed);
    const unsigned int drawsE =
            g_v29DrawElementsCount.exchange(0, std::memory_order_relaxed);
    const unsigned int binds =
            g_v29FramebufferBindCount.exchange(0, std::memory_order_relaxed);
    const unsigned int clears =
            g_v29ColorClearCount.exchange(0, std::memory_order_relaxed);
    const unsigned int blackClears =
            g_v29BlackClearCount.exchange(0, std::memory_order_relaxed);
    const unsigned int r2d =
            g_v29Render2dCount.exchange(0, std::memory_order_relaxed);
    const unsigned int bar =
            g_v29BarRoadsCount.exchange(0, std::memory_order_relaxed);
    const unsigned int emuEnd =
            g_v29EmuEndCount.exchange(0, std::memory_order_relaxed);

    const int lastGpuTid = g_v29LastGpuTid.load(std::memory_order_relaxed);
    const int lastBoundFbo = g_v29LastBoundFbo.load(std::memory_order_relaxed);
    const int lastNonZeroFbo = g_v29LastNonZeroFbo.exchange(-1, std::memory_order_relaxed);
    const int gameState = pNetGame ? (int)pNetGame->GetGameState() : -1;

    if (V29ShouldTraceSwap(current))
    {
        FLog("V30 FRAME | seq=%u state=%d effectsPhase=%d effectsCalls=%u swapTid=%d gpuTid=%d ctx=%p draw=%p size=%dx%d q=%d/%d fbo=%d viewport=%d,%d,%d,%d drawsA=%u drawsE=%u binds=%u lastFbo=%d nonZeroFbo=%d clears=%u blackClears=%u render2d=%u barroads=%u emuEnd=%u glErr=0x%x",
             current, gameState,
             g_v30EffectsPhase.load(std::memory_order_relaxed),
             g_v30EffectsConnectedCount.load(std::memory_order_relaxed),
             swapTid, lastGpuTid,
             (void*)currentContext, (void*)currentDraw,
             (int)width, (int)height, (int)qWidth, (int)qHeight,
             (int)fbo,
             (int)viewport[0], (int)viewport[1],
             (int)viewport[2], (int)viewport[3],
             drawsA, drawsE, binds, lastBoundFbo, lastNonZeroFbo,
             clears, blackClears, r2d, bar, emuEnd,
             (unsigned int)glErrBefore);
    }

    if (currentContext != EGL_NO_CONTEXT &&
        currentDraw != EGL_NO_SURFACE &&
        currentDraw == surface &&
        fbo > 0)
    {
        // V38: use a real mutex rather than racing atomic snapshots.
        // trylock means the swap thread never waits for the producer. If 2D is
        // active, skip the manual copy for this frame. If we get the lock, the
        // producer cannot begin a new TextDraw/UI pass until the blit is done.
        const int v38LockResult = pthread_mutex_trylock(&g_v38TwoDPresentMutex);
        const bool v38OwnsPresentLock = (v38LockResult == 0);
        const unsigned int v38Completed =
                g_v37TwoDCompleted.load(std::memory_order_acquire);

        if (!v38OwnsPresentLock)
        {
            if (current <= 40 || (current % 120) == 0)
            {
                FLog("V38 PRESENT DEFER | swap=%u tid=%d completed=%u mutex=%d reason=2D_BUSY",
                     current, swapTid, v38Completed, v38LockResult);
            }
        }
        else
        {
            V30ProbeCurrentRenderTarget(
                    current, swapTid, fbo, viewport, width, height);

            // V58: NUNCA chamar emu_IsAltRenderTarget/emu_FlushAltRenderTarget
            // dentro da thread EGL. A V57 mostrou dois sintomas de corrupcao:
            // hang logo no ALTTEST em uma execucao e SIGSEGV posterior em
            // RQ_Command_rqVertexBufferDelete em outra. Aqui fazemos apenas
            // sincronizacao GL segura + blit.
            // V64: compare producer-side probe submission with the commands
            // that the GTA GraphicsThread has ACTUALLY consumed before this
            // presentation.  This is the distinction V61 could not observe.
            const unsigned int v64ProbeSeq =
                    g_v64ProbeSeq.load(std::memory_order_acquire);
            const unsigned int v64RqBase =
                    g_v64ProbeDrawBase.load(std::memory_order_acquire);
            const unsigned int v64RqBeforeFinish =
                    g_v64RqDrawExecTotal.load(std::memory_order_acquire);
            const unsigned int v64RqSwapBase =
                    g_v64ProbeSwapBase.load(std::memory_order_acquire);
            const unsigned int v64RqSwapNow =
                    g_v64RqSwapExec.load(std::memory_order_acquire);

            if (v64ProbeSeq == v38Completed &&
                (v38Completed <= 16 || (v38Completed % 120) == 0))
            {
                FLog("V64 PROBE PRESENT PRE | completed=%u probe=%u rqNow=%u base=%u delta=%u rqSwapNow=%u swapBase=%u lastRqTid=%d lastRqFbo=%d lastTargetFbo=%d eglTid=%d eglFbo=%d",
                     v38Completed, v64ProbeSeq,
                     v64RqBeforeFinish, v64RqBase,
                     v64RqBeforeFinish >= v64RqBase
                         ? v64RqBeforeFinish - v64RqBase : 0u,
                     v64RqSwapNow, v64RqSwapBase,
                     g_v64LastRqDrawTid.load(std::memory_order_acquire),
                     g_v64LastRqDrawFbo.load(std::memory_order_acquire),
                     g_v64LastRqTargetFbo.load(std::memory_order_acquire),
                     swapTid, (int)fbo);
            }

            // The producer is excluded for the entire finish+copy window.
            glFinish();
            const GLenum v38FinishErr = glGetError();

            const unsigned int v64RqAfterFinish =
                    g_v64RqDrawExecTotal.load(std::memory_order_acquire);
            if (v64ProbeSeq == v38Completed &&
                (v38Completed <= 16 || (v38Completed % 120) == 0))
            {
                FLog("V64 PROBE PRESENT POSTFINISH | completed=%u probe=%u rqNow=%u base=%u delta=%u glFinishErr=0x%x",
                     v38Completed, v64ProbeSeq,
                     v64RqAfterFinish, v64RqBase,
                     v64RqAfterFinish >= v64RqBase
                         ? v64RqAfterFinish - v64RqBase : 0u,
                     (unsigned int)v38FinishErr);
            }

            // V40: once the queued work is complete, enumerate already-existing
            // framebuffer objects. This is diagnostic only and is intended to
            // reveal whether HUD/TextDraw/UI ended up in a framebuffer other
            // than the FBO 2 that V31 currently presents.
            if (v38Completed > 0)
            {
                V40EnumerateFramebufferObjectsOnce(
                        current, swapTid, fbo);
            }

            if (current <= 40 || (current % 120) == 0)
            {
                FLog("V38 PRESENT BLIT | swap=%u tid=%d completed=%u glFinishErr=0x%x mutex=OWNED",
                     current, swapTid, v38Completed,
                     (unsigned int)v38FinishErr);
            }

            // V66: the whole frame (world + queued GTA/SA-MP 2D) is composed
            // in a private FBO.  Copy that finished image to the real default
            // framebuffer only at the actual EGL present.
            const bool v66Presented = V66PresentComposeToDefault(
                    current, swapTid, v38Completed, fbo, width, height);

            if (!v66Presented)
            {
                // Startup/edge fallback: keep the previously proven world-only
                // path so a missing composition frame never kills 3D output.
                V61PresentCurrentWorld(
                        current, swapTid, fbo, viewport, width, height);

                if (v38Completed <= 8)
                {
                    FLog("V66 PRESENT FALLBACK WORLD | swap=%u tid=%d completed=%u composeSeq=%u composeFbo=%u source=%d",
                         current, swapTid, v38Completed,
                         g_v66ComposeSeq.load(std::memory_order_acquire),
                         (unsigned int)g_v66ComposeFbo, (int)fbo);
                }
            }

            // V56: a copia terminou enquanto a thread EGL possui contexto
            // e surface validos. Publica o numero do ultimo 2D realmente
            // apresentado para liberar o produtor antes do proximo 3D.
            if (v38Completed > 0)
            {
                g_v56PresentedTwoD.store(
                        v38Completed, std::memory_order_release);

                if (v38Completed <= 8 || (v38Completed % 120) == 0)
                {
                    FLog("V56 PRESENT ACK | swap=%u tid=%d completed=%u",
                         current, swapTid, v38Completed);
                }
            }

            pthread_mutex_unlock(&g_v38TwoDPresentMutex);
        }
    }

    if (V29ShouldSamplePixels(current) &&
        currentContext != EGL_NO_CONTEXT &&
        currentDraw != EGL_NO_SURFACE &&
        currentDraw == surface &&
        fbo == 0 &&
        width > 8 && height > 8)
    {
        const GLint xs[5] = {
                width / 2, width / 4, (width * 3) / 4,
                width / 4, (width * 3) / 4
        };
        const GLint ys[5] = {
                height / 2, height / 4, height / 4,
                (height * 3) / 4, (height * 3) / 4
        };

        GLubyte px[5][4] = {};
        while (glGetError() != GL_NO_ERROR) {}

        for (int i = 0; i < 5; ++i)
            glReadPixels(xs[i], ys[i], 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px[i]);

        GLenum readErr = glGetError();

        FLog("V29 PIXELS | seq=%u tid=%d p0=%u,%u,%u,%u p1=%u,%u,%u,%u p2=%u,%u,%u,%u p3=%u,%u,%u,%u p4=%u,%u,%u,%u err=0x%x",
             current, swapTid,
             px[0][0], px[0][1], px[0][2], px[0][3],
             px[1][0], px[1][1], px[1][2], px[1][3],
             px[2][0], px[2][1], px[2][2], px[2][3],
             px[3][0], px[3][1], px[3][2], px[3][3],
             px[4][0], px[4][1], px[4][2], px[4][3],
             (unsigned int)readErr);
    }
    else if (V29ShouldSamplePixels(current))
    {
        FLog("V29 PIXELS SKIP | seq=%u tid=%d ctx=%p draw=%p surface=%p fbo=%d size=%dx%d",
             current, swapTid, (void*)currentContext, (void*)currentDraw,
             (void*)surface, (int)fbo, (int)width, (int)height);
    }

    EGLBoolean result = eglSwapBuffers_V29_Original(dpy, surface);
    EGLint eglErr = result ? EGL_SUCCESS : eglGetError();

    if (V29ShouldTraceSwap(current))
    {
        FLog("V30 SWAP AFTER | seq=%u result=%d eglErr=0x%x curDpy=%p",
             current, (int)result, (unsigned int)eglErr, (void*)currentDisplay);
    }

    return result;
}

/* =============================================================================== */

int (*CRadar__SetCoordBlip)(int r0, float X, float Y, float Z, int r4, int r5, char* name);
int CRadar__SetCoordBlip_hook(int r0, float X, float Y, float Z, int r4, int r5, char* name)
{
    if(pNetGame && !strncmp(name, "CODEWAY", 7))
    {
        float fFindZ = CWorld::FindGroundZForCoord(X, Y) + 1.5f;

        if(pNetGame->GetGameState() != GAMESTATE_CONNECTED) return 0;

        RakNet::BitStream bsSend;
        bsSend.Write(X);
        bsSend.Write(Y);
        bsSend.Write(fFindZ);
        pNetGame->GetRakClient()->RPC(&RPC_MapMarker, &bsSend, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
    }

    return CRadar__SetCoordBlip(r0, X, Y, Z, r4, r5, name);
}

/* =============================================================================== */

void(*CRadar_DrawRadarGangOverlay)(uint32_t unk);
void CRadar_DrawRadarGangOverlay_hook(uint32_t unk)
{
    if (pNetGame)
    {
        CGangZonePool *pGangZonePool = pNetGame->GetGangZonePool();
        if (pGangZonePool) {
            pGangZonePool->Draw(unk);
        }
    }
}

/* =============================================================================== */

typedef struct {
    CVector     vecPosObject;
    CQuaternion m_qRotation;
    int32       wModelIndex;
    union {
        struct { // CFileObjectInstanceType
            uint32 m_nAreaCode : 8;
            uint32 m_bRedundantStream : 1;
            uint32 m_bDontStream : 1; // Merely assumed, no countercheck possible.
            uint32 m_bUnderwater : 1;
            uint32 m_bTunnel : 1;
            uint32 m_bTunnelTransition : 1;
            uint32 m_nReserved : 19;
        };
        uint32 m_nInstanceType;
    };
    int32 m_nLodInstanceIndex; // -1 - without LOD model
} stLoadObjectInstance;
VALIDATE_SIZE(stLoadObjectInstance, (VER_x32 ? 0x28 : 0x28));

extern int iBuildingToRemoveCount;
extern REMOVEBUILDING_DATA BuildingToRemove[1000];

int (*CFileLoader__LoadObjectInstance)(stLoadObjectInstance *thiz);
int CFileLoader__LoadObjectInstance_hook(stLoadObjectInstance *thiz) {
    if (thiz) {
        if (iBuildingToRemoveCount >= 1) {
            for (int i = 0; i < iBuildingToRemoveCount; i++)
            {
                float fDistance = GetDistance(BuildingToRemove[i].vecPos, thiz->vecPosObject);
                if (fDistance <= BuildingToRemove[i].fRange) {
                    if (BuildingToRemove[i].dwModel == -1 || thiz->wModelIndex == (uint16_t) BuildingToRemove[i].dwModel) {
                        thiz->wModelIndex = 19300;
                        //thiz->vecPosObject = 0.0f;
                        break;
                    }
                }
            }
        }
    }

    return CFileLoader__LoadObjectInstance(thiz);
}

extern int iBuildingToRemoveCount;
extern std::list<REMOVE_BUILDING_DATA> RemoveBuildingData;
void (*CEntity_Render)(CEntityGTA* pEntity);
int g_iLastRenderedObject;
void CEntity_Render_hook(CEntityGTA* pEntity)
{
    if (Skybox::GetSkyObject()) {
        if (Skybox::GetSkyObject()->m_pEntity == pEntity && !Skybox::IsNeedRender()) {
            return;
        }
    }
    if(iBuildingToRemoveCount > 1)
    {
        if(pEntity && *(uintptr_t*)pEntity != g_libGTASA+(VER_x32 ? 0x667D18:0x8300A0) && !pNetGame->GetObjectPool()->GetObjectFromGtaPtr(pEntity))
        {
            for (auto &entry : RemoveBuildingData)
            {
                float fDistance = GetDistance(entry.vecPos, pEntity->GetMatrix().m_pos);
                if(fDistance <= entry.fRange)
                {
                    if(pEntity->GetModelId() == entry.usModelIndex)
                    {
                        pEntity->m_bUsesCollision = 0;
                        pEntity->m_bCollisionProcessed = 0;
                        return;
                    }
                }
            }
        }
    }

    // V18 DIAGNOSTICO DE RENDER:
    // A V17 provou que o modelo 1268 nao e a causa unica do SIGBUS:
    // mesmo pulando 1268, o crash continuou logo depois no pipeline 3D.
    //
    // Nesta versao NAO pulamos nenhum model ID. Em vez disso, registramos
    // cada entrada e cada retorno de CEntity::Render. O ultimo "BEGIN" sem
    // um "END" correspondente mostra exatamente qual entidade/modelo entrou
    // em CEntity::Render e provocou o SIGBUS.
    static unsigned int v18RenderSequence = 0;
    const unsigned int renderSequence = ++v18RenderSequence;
    const bool v29TraceLegacyRender = renderSequence <= 24;
    int modelId = -1;

    if (pEntity)
    {
        modelId = pEntity->GetModelId();
        g_iLastRenderedObject = modelId;

        if (v29TraceLegacyRender) FLog("V18 RENDER BEGIN | seq=%u model=%d entity=%p rwObject=%p",
             renderSequence,
             modelId,
             pEntity,
             pEntity->m_pRwObject);
    }
    else
    {
        if (v29TraceLegacyRender) FLog("V18 RENDER BEGIN | seq=%u model=-1 entity=null rwObject=null",
             renderSequence);
    }

    CEntity_Render(pEntity);

    if (v29TraceLegacyRender) FLog("V18 RENDER END | seq=%u model=%d entity=%p",
         renderSequence,
         modelId,
         pEntity);
}


// =============================================================================
// V19 DIAGNOSTICO POS-CEntity::Render
//
// A V18 confirmou que CEntity::Render() retorna normalmente para os primeiros
// objetos (incluindo o model 1268) e o SIGBUS acontece DEPOIS. O objetivo da
// V19 e isolar o trecho exato dentro de CRenderer::RenderOneNonRoad():
// SetupLighting -> Render -> RemoveLighting / limpeza de render-state.
// =============================================================================

// =============================================================================
// V20 DIAGNOSTICO DA LISTA VISIVEL
//
// A V19 mostrou que RenderOneNonRoad(), SetupLighting(), CEntity::Render() e
// RemoveLighting() retornam normalmente para o model 1268. O SIGBUS acontece
// logo APOS RenderOneNonRoad() devolver o controle para
// CRenderer::RenderEverythingBarRoads().
//
// Agora registramos o indice da entidade dentro de ms_aVisibleEntityPtrs,
// a quantidade de entidades visiveis e o ponteiro cru do proximo slot. Assim
// descobrimos se o renderer cai ao avancar para a proxima entrada da lista.
// Os offsets abaixo foram confirmados na libGTASA das duas ABIs deste projeto.
// =============================================================================

static inline int V20GetVisibleCount()
{
    const uintptr_t offset = VER_x32 ? 0x00960B64 : 0x00BCF8E4;
    return *reinterpret_cast<volatile int*>(g_libGTASA + offset);
}

static inline CEntityGTA** V20GetVisibleArray()
{
    const uintptr_t offset = VER_x32 ? 0x00960B80 : 0x00BCF900;
    return reinterpret_cast<CEntityGTA**>(g_libGTASA + offset);
}

static inline int V20FindVisibleIndex(CEntityGTA* entity)
{
    if (!entity)
        return -1;

    const int count = V20GetVisibleCount();
    if (count < 0 || count > 1000)
        return -2;

    CEntityGTA** list = V20GetVisibleArray();
    for (int i = 0; i < count; ++i)
    {
        if (list[i] == entity)
            return i;
    }
    return -1;
}

static inline CEntityGTA* V20GetVisibleRaw(int index)
{
    const int count = V20GetVisibleCount();
    if (index < 0 || count < 0 || count > 1000 || index >= count)
        return nullptr;

    return V20GetVisibleArray()[index];
}

static inline int V19GetModelId(CEntityGTA* entity)
{
    return entity ? entity->GetModelId() : -1;
}

void (*CRenderer__RenderOneNonRoad)(CEntityGTA* pEntity);
void CRenderer__RenderOneNonRoad_hook(CEntityGTA* pEntity)
{
    static unsigned int seq = 0;
    const unsigned int current = ++seq;
    const bool trace = current <= 24;
    const int modelId = V19GetModelId(pEntity);
    const int visibleCount = V20GetVisibleCount();
    const int visibleIndex = V20FindVisibleIndex(pEntity);
    CEntityGTA* nextRaw = V20GetVisibleRaw(visibleIndex + 1);

    if (trace) FLog("V20 NONROAD BEGIN | seq=%u model=%d entity=%p rwObject=%p visIndex=%d visCount=%d nextRaw=%p",
         current,
         modelId,
         pEntity,
         pEntity ? pEntity->m_pRwObject : nullptr,
         visibleIndex,
         visibleCount,
         nextRaw);

    CRenderer__RenderOneNonRoad(pEntity);

    if (trace) FLog("V20 NONROAD END | seq=%u model=%d entity=%p visIndex=%d visCount=%d nextRaw=%p",
         current,
         modelId,
         pEntity,
         visibleIndex,
         visibleCount,
         nextRaw);
}

bool (*CEntity__SetupLighting)(CEntityGTA* thiz);
bool CEntity__SetupLighting_hook(CEntityGTA* thiz)
{
    static unsigned int seq = 0; const bool trace = ++seq <= 16;
    const int modelId = V19GetModelId(thiz);
    if (trace) FLog("V19 ENTITY LIGHT SETUP BEGIN | model=%d entity=%p", modelId, thiz);
    const bool result = CEntity__SetupLighting(thiz);
    if (trace) FLog("V19 ENTITY LIGHT SETUP END | model=%d entity=%p result=%d",
         modelId, thiz, result ? 1 : 0);
    return result;
}

void (*CEntity__RemoveLighting)(CEntityGTA* thiz, bool setupResult);
void CEntity__RemoveLighting_hook(CEntityGTA* thiz, bool setupResult)
{
    static unsigned int seq = 0; const bool trace = ++seq <= 16;
    const int modelId = V19GetModelId(thiz);
    if (trace) FLog("V19 ENTITY LIGHT REMOVE BEGIN | model=%d entity=%p setup=%d",
         modelId, thiz, setupResult ? 1 : 0);
    CEntity__RemoveLighting(thiz, setupResult);
    if (trace) FLog("V19 ENTITY LIGHT REMOVE END | model=%d entity=%p", modelId, thiz);
}

bool (*CObject__SetupLighting)(CObjectGta* thiz);
bool CObject__SetupLighting_hook(CObjectGta* thiz)
{
    static unsigned int seq = 0; const bool trace = ++seq <= 16;
    CEntityGTA* entity = reinterpret_cast<CEntityGTA*>(thiz);
    const int modelId = V19GetModelId(entity);
    if (trace) FLog("V19 OBJECT LIGHT SETUP BEGIN | model=%d object=%p", modelId, thiz);
    const bool result = CObject__SetupLighting(thiz);
    if (trace) FLog("V19 OBJECT LIGHT SETUP END | model=%d object=%p result=%d",
         modelId, thiz, result ? 1 : 0);
    return result;
}

void (*CObject__RemoveLighting)(CObjectGta* thiz, bool setupResult);
void CObject__RemoveLighting_hook(CObjectGta* thiz, bool setupResult)
{
    static unsigned int seq = 0; const bool trace = ++seq <= 16;
    CEntityGTA* entity = reinterpret_cast<CEntityGTA*>(thiz);
    const int modelId = V19GetModelId(entity);
    if (trace) FLog("V19 OBJECT LIGHT REMOVE BEGIN | model=%d object=%p setup=%d",
         modelId, thiz, setupResult ? 1 : 0);
    CObject__RemoveLighting(thiz, setupResult);
    if (trace) FLog("V19 OBJECT LIGHT REMOVE END | model=%d object=%p", modelId, thiz);
}

bool (*CPed__SetupLighting)(CPedGTA* thiz);
bool CPed__SetupLighting_hook(CPedGTA* thiz)
{
    static unsigned int seq = 0; const bool trace = ++seq <= 16;
    CEntityGTA* entity = reinterpret_cast<CEntityGTA*>(thiz);
    const int modelId = V19GetModelId(entity);
    if (trace) FLog("V19 PED LIGHT SETUP BEGIN | model=%d ped=%p", modelId, thiz);
    const bool result = CPed__SetupLighting(thiz);
    if (trace) FLog("V19 PED LIGHT SETUP END | model=%d ped=%p result=%d",
         modelId, thiz, result ? 1 : 0);
    return result;
}

void (*CPed__RemoveLighting)(CPedGTA* thiz, bool setupResult);
void CPed__RemoveLighting_hook(CPedGTA* thiz, bool setupResult)
{
    static unsigned int seq = 0; const bool trace = ++seq <= 16;
    CEntityGTA* entity = reinterpret_cast<CEntityGTA*>(thiz);
    const int modelId = V19GetModelId(entity);
    if (trace) FLog("V19 PED LIGHT REMOVE BEGIN | model=%d ped=%p setup=%d",
         modelId, thiz, setupResult ? 1 : 0);
    CPed__RemoveLighting(thiz, setupResult);
    if (trace) FLog("V19 PED LIGHT REMOVE END | model=%d ped=%p", modelId, thiz);
}

bool (*CVehicle__SetupLighting)(CVehicleGTA* thiz);
bool CVehicle__SetupLighting_hook(CVehicleGTA* thiz)
{
    static unsigned int seq = 0; const bool trace = ++seq <= 16;
    CEntityGTA* entity = reinterpret_cast<CEntityGTA*>(thiz);
    const int modelId = V19GetModelId(entity);
    if (trace) FLog("V19 VEHICLE LIGHT SETUP BEGIN | model=%d vehicle=%p", modelId, thiz);
    const bool result = CVehicle__SetupLighting(thiz);
    if (trace) FLog("V19 VEHICLE LIGHT SETUP END | model=%d vehicle=%p result=%d",
         modelId, thiz, result ? 1 : 0);
    return result;
}

void (*CVehicle__RemoveLighting)(CVehicleGTA* thiz, bool setupResult);
void CVehicle__RemoveLighting_hook(CVehicleGTA* thiz, bool setupResult)
{
    static unsigned int seq = 0; const bool trace = ++seq <= 16;
    CEntityGTA* entity = reinterpret_cast<CEntityGTA*>(thiz);
    const int modelId = V19GetModelId(entity);
    if (trace) FLog("V19 VEHICLE LIGHT REMOVE BEGIN | model=%d vehicle=%p setup=%d",
         modelId, thiz, setupResult ? 1 : 0);
    CVehicle__RemoveLighting(thiz, setupResult);
    if (trace) FLog("V19 VEHICLE LIGHT REMOVE END | model=%d vehicle=%p", modelId, thiz);
}
/* =============================================================================== */

/* =============================================================================== */
bool m_bNeedRender = true;
CObject* m_pSkyObject = nullptr;

bool IsNeedRender()
{
    return m_bNeedRender;
}

CObject *GetSkyObject()
{
    return m_pSkyObject;
}

void (*CObject_Render)(CObjectGta* thiz);
void CObject_Render_hook(CObjectGta* thiz)
{
    CObjectGta *object = thiz;
    if(pNetGame && object != 0)
    {
        /*if (CSkyBox::GetSkyObject())
        {
            if (CSkyBox::GetSkyObject()->m_pEntity == thiz && !CSkyBox::IsNeedRender())
                return;
        }*/
        CObject *pObject = pNetGame->GetObjectPool()->FindObjectFromGtaPtr(object);
        if(pObject && pObject->m_pEntity)
        {
            RwObject* rwObject = (RwObject*)pObject->m_pEntity->m_pRwObject;
            if(rwObject)
            {
                // SetObjectMaterial
                if(pObject->m_bHasMaterial || pObject->m_bHasMaterialText)
                {
                    RwFrameForAllObjects((RwFrame*)rwObject->parent, (RwObject *(*)(RwObject *, void *))ObjectMaterialCallBack, pObject);
                    //RpAtomic* atomic = (RpAtomic*)object->m_pRwAtomic;
                    //RpGeometryForAllMaterials(atomic->geometry, ObjectMaterialCallBack, (void*)pObject);
                }
                // SetObjectMaterialText
                if(pObject->m_bHasMaterialText)
                {
                    RwFrameForAllObjects((RwFrame*)rwObject->parent, (RwObject *(*)(RwObject *, void *))ObjectMaterialTextCallBack, pObject);
                    //RpAtomic* atomic = (RpAtomic*)object->m_pRwAtomic;
                    //RpGeometryForAllMaterials(atomic->geometry, ObjectMaterialTextCallBack, (void*)pObject);
                }
            }


        }

        CObject_Render(object);
    }

    //((void (*)(void))(g_libGTASA + (VER_x32 ? 0x005D1F98 + 1 : 0x6F6664)))();
    //((void (*)(void))(g_libGTASA + 0x5D1F5C + 1))();
}

/*((void (*)(void))(g_libGTASA + 0x5D1F48 + 1))();
				CObject_Render(thiz);
				// ActivateDirectional
				((void (*)(void))(g_libGTASA + 0x5D1F5C + 1))();*/
/* =============================================================================== */

/* =============================================================================== */

bool NotifyEnterVehicle(CVehicleGTA *_pVehicle)
{
    if(!pNetGame) {
        return false;
    }

    CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
    if(!pVehiclePool) {
        return false;
    }

    CVehicle *pVehicle = nullptr;
    VEHICLEID VehicleID = pVehiclePool->FindIDFromGtaPtr(_pVehicle);

    if(VehicleID <= 0 || VehicleID >= MAX_VEHICLES) {
        return false;
    }

    if(!pVehiclePool->GetSlotState(VehicleID)) {
        return false;
    }

    pVehicle = pVehiclePool->GetAt(VehicleID);
    if(!pVehicle) {
        return false;
    }

    CLocalPlayer *pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();

    if(pLocalPlayer) {
        FLog("Vehicle ID: %d", VehicleID);
        pLocalPlayer->SendEnterVehicleNotification(VehicleID, false);
    }

    return true;
}

int (*TaskEnterVehicle)(uintptr_t a1, uintptr_t a2);
int TaskEnterVehicleHook(uintptr_t a1, uintptr_t a2)
{
    if(!NotifyEnterVehicle((CVehicleGTA*)a1)) {
        return false;
    }

    // CTask::operator new
    uintptr_t pTask = ((uintptr_t (*)(void))(g_libGTASA + (VER_x32 ? 0x4D6A70:0x5D7414)))();

    // CTaskComplexEnterCarAsDriver::CTaskComplexEnterCarAsDriver
    ((void (__fastcall *)(uintptr_t, uintptr_t))(g_libGTASA + (VER_x32 ? 0x4F6FE0:0x6007E0)))(pTask, a1);

    // CTaskManager::SetTask
    ((int (__fastcall *)(uintptr_t, uintptr_t, int, int))(g_libGTASA + (VER_x32 ? 0x53397A:0x64E084)))(a2, pTask, 3, 0);

    return true;
}

void (*CTaskComplexLeaveCar)(uintptr_t** thiz, CVehicleGTA* pVehicle, int iTargetDoor, int iDelayTime, bool bSensibleLeaveCar, bool bForceGetOut);
void CTaskComplexLeaveCar_hook(uintptr_t** thiz, CVehicleGTA* pVehicle, int iTargetDoor, int iDelayTime, bool bSensibleLeaveCar, bool bForceGetOut)
{
    uintptr_t dwRetAddr = 0;
    __asm__ volatile ("mov %0, lr" : "=r" (dwRetAddr));
    dwRetAddr -= g_libGTASA;

    if (dwRetAddr == 0x409A42+1 || dwRetAddr == 0x40A818+1)
    {
        if (pNetGame)
        {
            if ((CVehicleGTA*)GamePool_FindPlayerPed()->pVehicle == pVehicle)
            {
                CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();
                VEHICLEID VehicleID = pVehiclePool->FindIDFromGtaPtr((CVehicleGTA*)GamePool_FindPlayerPed()->pVehicle);
                if (VehicleID != INVALID_VEHICLE_ID)
                {
                    CVehicle* pVehicle = pVehiclePool->GetAt(VehicleID);
                    CLocalPlayer* pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
                    if (pVehicle && pLocalPlayer)
                    {
                        if (pVehicle->IsATrainPart())
                        {
                            RwMatrix mat = pVehicle->m_pVehicle->GetMatrix().ToRwMatrix();
                            pLocalPlayer->GetPlayerPed()->RemoveFromVehicleAndPutAt(mat.pos.x + 2.5f, mat.pos.y + 2.5f, mat.pos.z);
                        }
                        else
                        {
                            pLocalPlayer->SendExitVehicleNotification(VehicleID);
                        }
                    }
                }
            }
        }
    }

    (*CTaskComplexLeaveCar)(thiz, pVehicle, iTargetDoor, iDelayTime, bSensibleLeaveCar, bForceGetOut);
}

/* =============================================================================== */

uint32_t CRadar__GetRadarTraceColor(uint32_t color, uint8_t bright, uint8_t friendly)
{
    return TranslateColorCodeToRGBA(color);
}

#if VER_x32
uint32_t CHudColours__GetIntColour(uint32 colour_id)
{
	return TranslateColorCodeToRGBA(colour_id);
}
#else
uint32_t CHudColours__GetIntColour(uintptr* thiz, uint8 colour_id)
{
    return TranslateColorCodeToRGBA(colour_id);
}
#endif

/* =============================================================================== */

void (*AND_TouchEvent)(int type, int num, int posX, int posY);
void AND_TouchEvent_hook(int type, int num, int posX, int posY)
{
    // imgui
    //bool bRet = pUI->OnTouchEvent(type, num, posX, posY);

    if (pGame->IsGamePaused())
        return AND_TouchEvent(type, num, posX, posY);

    if (pUI != nullptr)
    {
        switch (type)
        {
            case 2: // push
                pUI->touchEvent(ImVec2(posX, posY), TouchType::push);
                break;

            case 3: // move
                pUI->touchEvent(ImVec2(posX, posY), TouchType::move);
                break;

            case 1: // pop
                pUI->touchEvent(ImVec2(posX, posY), TouchType::pop);
                break;
        }

        if (pUI->keyboard()->visible() || pUI->dialog()->visible()) {
            AND_TouchEvent(1, 0, 0, 0);
            return;
        }
        else
        {
            if (pNetGame && pNetGame->GetTextDrawPool())
            {
                if (!pNetGame->GetTextDrawPool()->onTouchEvent(type, num, posX, posY)) {
                    return AND_TouchEvent(1, 0, 0, 0);
                }
            }
        }
    }

    if (pGame->IsGameInputEnabled())
        AND_TouchEvent(type, num, posX, posY);
    else
        AND_TouchEvent(1, 0, 0, 0);
}

/* =============================================================================== */

/* =============================================================================== */

/* =============================================================================== */

uint32_t (*CPed__GetWeaponSkill)(CPedGTA *thiz);
uint32_t CPed__GetWeaponSkill_hook(CPedGTA *thiz)
{
    bool bWeaponSkillStored = false;

    dwCurPlayerActor = thiz;
    byteInternalPlayer = CWorld::PlayerInFocus;
    byteCurPlayer = FindPlayerNumFromPedPtr(dwCurPlayerActor);

    if(dwCurPlayerActor && byteCurPlayer != 0 && CWorld::PlayerInFocus == 0)
    {
        GameStoreLocalPlayerSkills();
        GameSetRemotePlayerSkills(byteCurPlayer);
        bWeaponSkillStored = true;
    }

    // CPed::GetWeaponSkill
    uint32_t result = (( uint32_t (*)(CPedGTA *, uint32_t))(g_libGTASA+0x4A55E2+1))(thiz, thiz->m_aWeapons[thiz->m_nActiveWeaponSlot].dwType);

    if(bWeaponSkillStored)
    {
        GameSetLocalPlayerSkills();
        bWeaponSkillStored = false;
    }

    return result;
}

/* =============================================================================== */

extern CPlayerPed* g_pCurrentFiredPed;
extern BULLET_DATA* g_pCurrentBulletData;

extern int g_iLagCompensationMode;

void SendBulletSync(CVector* vecOrigin, CVector* a2, CColPoint *colPoint, CEntityGTA** ppEntity)
{
    CMatrix mat1, mat2;

    static BULLET_DATA bulletData;
    memset(&bulletData, 0, sizeof(BULLET_DATA));

    bulletData.vecOrigin.x = vecOrigin->x;
    bulletData.vecOrigin.y = vecOrigin->y;
    bulletData.vecOrigin.z = vecOrigin->z;

    bulletData.vecPos.x = colPoint->m_vecPoint.x;
    bulletData.vecPos.y = colPoint->m_vecPoint.y;
    bulletData.vecPos.z = colPoint->m_vecPoint.z;

    if (ppEntity)
    {
        CEntityGTA* pEntity = *ppEntity;
        if (pEntity)
        {
            if (g_iLagCompensationMode != 0)
            {
                bulletData.vecOffset.x = colPoint->m_vecPoint.x - pEntity->m_matrix->m_pos.x;
                bulletData.vecOffset.y = colPoint->m_vecPoint.y - pEntity->m_matrix->m_pos.y;
                bulletData.vecOffset.z = colPoint->m_vecPoint.z - pEntity->m_matrix->m_pos.z;
            }
            else
            {
                memset(&mat1, 0, sizeof(CMatrix));
                memset(&mat2, 0, sizeof(CMatrix));
                // RwMatrixOrthoNormalize
                auto entMat = pEntity->GetMatrix().ToRwMatrix();
                RwMatrixOrthoNormalize(reinterpret_cast<RwMatrix *>(&mat2), &entMat);
                // RwMatrixInvert
                Invert(mat1, mat2);

                ProjectMatrix(&bulletData.vecOffset, &mat1, &colPoint->m_vecPoint);
            }

            bulletData.pEntity = pEntity;
        }
        else bulletData.vecOffset = 0;
    }

    pGame->FindPlayerPed()->ProcessBulletData(&bulletData);
}

extern bool g_customFire;
/* 0.3.7 */
uint32_t(*CWeapon__FireInstantHit)(CWeapon* thiz, CPedGTA* pFiringEntity, CVector* vecOrigin, CVector* muzzlePosn, CEntityGTA* targetEntity,
                                   CVector* target, CVector* originForDriveBy, bool arg6, bool muzzle);
uint32_t CWeapon__FireInstantHit_hook(CWeapon* thiz, CPedGTA* pFiringEntity, CVector* vecOrigin, CVector* muzzlePosn, CEntityGTA* targetEntity,
                                      CVector* target, CVector* originForDriveBy, bool arg6, bool muzzle)
{
    if (pNetGame && pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()->m_pPed)		// CWeapon::Fire
    {
        if(pFiringEntity != GamePool_FindPlayerPed())
            return muzzle;

        if(pNetGame)
        {
            pNetGame->GetPlayerPool()->ApplyCollisionChecking();
        }

        if(pGame)
        {
            CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
            if(pPlayerPed)
                pPlayerPed->FireInstant();
        }

        if(pNetGame)
        {
            pNetGame->GetPlayerPool()->ResetCollisionChecking();
        }

        return muzzle;
    }

    return CWeapon__FireInstantHit(thiz, pFiringEntity, vecOrigin, muzzlePosn, targetEntity,
                                   target, originForDriveBy, arg6, muzzle);
}

bool g_bForceWorldProcessLineOfSight = false;
uint32_t (*CWeapon__ProcessLineOfSight)(CVector *vecOrigin, CVector *vecEnd, CVector *vecPos, CPedGTA **ppEntity, CWeapon *pWeaponSlot, CPedGTA **ppEntity2, bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7);
uint32_t CWeapon__ProcessLineOfSight_hook(CVector *vecOrigin, CVector *vecEnd, CVector *vecPos, CPedGTA **ppEntity, CWeapon *pWeaponSlot, CPedGTA **ppEntity2, bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7)
{
    uintptr_t dwRetAddr = 0;
    GET_LR(dwRetAddr);

    FLog("dwRetAddr CWeapon__ProcessLineOfSight_hook 0x%llx", dwRetAddr);
#if VER_x32
    if(dwRetAddr >= 0x005DC178 && dwRetAddr <= 0x005DD684)
		g_bForceWorldProcessLineOfSight = true;
#else
    if(dwRetAddr >= 0x701494 && dwRetAddr <= 0x702B18)
        g_bForceWorldProcessLineOfSight = true;
#endif

    return CWeapon__ProcessLineOfSight(vecOrigin, vecEnd, vecPos, ppEntity, pWeaponSlot, ppEntity2, b1, b2, b3, b4, b5, b6, b7);
}

uint32_t(*CWorld__ProcessLineOfSight)(CVector*, CVector*, CColPoint *colPoint, CEntityGTA**, bool, bool, bool, bool, bool, bool, bool, bool);
uint32_t CWorld__ProcessLineOfSight_hook(CVector* vecOrigin, CVector* vecEnd, CColPoint *colPoint, CEntityGTA** ppEntity,
                                         bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7, bool b8)
{
    uintptr_t dwRetAddr = 0;
    GET_LR(dwRetAddr);

    if(dwRetAddr == (VER_x32 ? 0x005dd0b0 + 1 : 0x70253C) || g_bForceWorldProcessLineOfSight)
    {
        g_bForceWorldProcessLineOfSight = false;
        //LOGI("CWorld_ProcessLineOfSight iLagCompensationMode: %d", g_iLagCompensationMode);
        static CVector vecPosPlusOffset;

        if (g_iLagCompensationMode != 2)
        {
            if (g_pCurrentFiredPed != pGame->FindPlayerPed())
            {
                if (g_pCurrentBulletData && g_pCurrentBulletData->pEntity)
                {
                    if (*(uintptr_t*)(g_pCurrentBulletData->pEntity) != g_libGTASA+(VER_x32 ? 0x667D18:0x8300A0)) // CPlaceable
                    {
                        if (g_iLagCompensationMode)
                        {
                            vecPosPlusOffset.x = g_pCurrentBulletData->pEntity->GetPosition().x + g_pCurrentBulletData->vecOffset.x;
                            vecPosPlusOffset.y = g_pCurrentBulletData->pEntity->GetPosition().y + g_pCurrentBulletData->vecOffset.y;
                            vecPosPlusOffset.z = g_pCurrentBulletData->pEntity->GetPosition().z + g_pCurrentBulletData->vecOffset.z;
                        }
                        else
                        {
                            //FLog("vecPosPlusOffset %f %f %f", vecPosPlusOffset.x, vecPosPlusOffset.y, vecPosPlusOffset.z);
                            //FLog("pEntity->GetMatrix().m_up %f %f %f", g_pCurrentBulletData->pEntity->GetMatrix().m_up.x, g_pCurrentBulletData->pEntity->GetMatrix().m_up.y, g_pCurrentBulletData->pEntity->GetMatrix().m_up.z);
                            //FLog("g_pCurrentBulletData->vecOffset %f %f %f", g_pCurrentBulletData->vecOffset.x, g_pCurrentBulletData->vecOffset.y, g_pCurrentBulletData->vecOffset.z);
                            ProjectMatrix((CVector*)&vecPosPlusOffset, &g_pCurrentBulletData->pEntity->GetMatrix(), &g_pCurrentBulletData->vecOffset);
                            //vecPosPlusOffset.x = pEntity->GetMatrix().m_up.x * g_pCurrentBulletData->vecOffset.z + pEntity->GetMatrix().m_forward.x * g_pCurrentBulletData->vecOffset.y + pEntity->GetMatrix().m_right.x * g_pCurrentBulletData->vecOffset.x + pEntity->GetMatrix().m_pos.x;
                            //vecPosPlusOffset.y = pEntity->GetMatrix().m_up.y * g_pCurrentBulletData->vecOffset.z + pEntity->GetMatrix().m_forward.y * g_pCurrentBulletData->vecOffset.y + pEntity->GetMatrix().m_right.y * g_pCurrentBulletData->vecOffset.x + pEntity->GetMatrix().m_pos.y;
                            //vecPosPlusOffset.z = pEntity->GetMatrix().m_up.z * g_pCurrentBulletData->vecOffset.z + pEntity->GetMatrix().m_forward.z * g_pCurrentBulletData->vecOffset.y + pEntity->GetMatrix().m_right.z * g_pCurrentBulletData->vecOffset.x + pEntity->GetMatrix().m_pos.z;
                        }

                        vecEnd->x = vecPosPlusOffset.x - vecOrigin->x + vecPosPlusOffset.x;
                        vecEnd->y = vecPosPlusOffset.y - vecOrigin->y + vecPosPlusOffset.y;
                        vecEnd->z = vecPosPlusOffset.z - vecOrigin->z + vecPosPlusOffset.z;
                    }
                }
            }
        }

        uint32_t result = CWorld__ProcessLineOfSight(vecOrigin, vecEnd, colPoint, ppEntity, b1, b2, b3, b4, b5, b6, b7, b8);

        if (g_iLagCompensationMode == 2)
        {
            if (g_pCurrentFiredPed == pGame->FindPlayerPed()) {
                SendBulletSync(vecOrigin, vecEnd, colPoint, ppEntity);
            }
            return result;
        }

        if (g_pCurrentFiredPed)
        {
            if (g_pCurrentFiredPed != pGame->FindPlayerPed())
            {
                if (g_pCurrentBulletData)
                {
                    if (g_pCurrentBulletData->pEntity == nullptr)
                    {
                        CPedGTA* pLocalPed = GamePool_FindPlayerPed();
                        if (*ppEntity == GamePool_FindPlayerPed() ||
                            pLocalPed->IsInVehicle() && *ppEntity == pLocalPed->pVehicle)
                        {
                            result = 0;
                            *ppEntity = nullptr;
                            colPoint->m_vecPoint.x = 0.0f;
                            colPoint->m_vecPoint.y = 0.0f;
                            colPoint->m_vecPoint.z = 0.0f;
                            return result;
                        }
                    }
                }
            }
            else {
                SendBulletSync(vecOrigin, vecEnd, colPoint, ppEntity);
            }
        }

        return result;
    }

    return CWorld__ProcessLineOfSight(vecOrigin, vecEnd, colPoint, ppEntity, b1, b2, b3, b4, b5, b6, b7, b8);
}
// 0.3.7
uint32_t(*CWeapon__FireSniper)(CWeapon* thiz, CPedGTA* pFiringEntity, CEntityGTA* victim, CVector* target);
uint32_t CWeapon__FireSniper_hook(CWeapon* thiz, CPedGTA* pFiringEntity, CEntityGTA* victim, CVector* target)
{
    if (pFiringEntity == GamePool_FindPlayerPed())
    {
        if (pGame)
        {
            CPlayerPed* pPlayerPed = pGame->FindPlayerPed();
            if (pPlayerPed) {
                pPlayerPed->FireInstant();
            }
        }
    }

    return true;
}
// 0.3.7
bool(*CBulletInfo_AddBullet)(CEntityGTA* creator, int weaponType, CVector pos, CVector velocity);
bool CBulletInfo_AddBullet_hook(CEntityGTA* creator, int weaponType, CVector pos, CVector velocity)
{
    velocity.x *= 50.0f;
    velocity.y *= 50.0f;
    velocity.z *= 50.0f;

    CBulletInfo_AddBullet(creator, weaponType, pos, velocity);

    // CBulletInfo::Update
    CHook::CallFunction<void>("_ZN11CBulletInfo6UpdateEv");
    return true;
}

#pragma pack(push, 1)
struct CPedDamageResponseCalculator
{
    CPedGTA* m_pDamager;
    float m_fDamageFactor;
    int m_pedPieceType;
    int m_weaponType;
};
#pragma pack(pop)
// 0.3.7
bool ComputeDamageResponse(CPedDamageResponseCalculator* calculator, CPedGTA* pPed)
{
    CPedGTA* pGamePed = GamePool_FindPlayerPed();
    bool isLocalPed = false;

    if (!pNetGame) return false;

    CPedGTA* pDamager = calculator->m_pDamager;
    if (pDamager != pGamePed && IsValidGamePed(pGamePed)) /* CCivilianPed */
        return true;

    if (pPed == pGamePed) {
        isLocalPed = true;
    }
    else if (pDamager != pGamePed) {
        return false;
    }

    CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
    CLocalPlayer* pLocalPlayer = pPlayerPool->GetLocalPlayer();
    PLAYERID PlayerID;

    if (isLocalPed)
    {
        PlayerID = FindPlayerIDFromGtaPtr(pDamager);
        pLocalPlayer->SendTakeDamageEvent(PlayerID,
                                          calculator->m_fDamageFactor,
                                          calculator->m_weaponType,
                                          calculator->m_pedPieceType);
    }
    else
    {
        PlayerID = FindPlayerIDFromGtaPtr(pPed);
        if (PlayerID != INVALID_PLAYER_ID)
        {
            pLocalPlayer->SendGiveDamageEvent(PlayerID,
                                              calculator->m_fDamageFactor,
                                              calculator->m_weaponType,
                                              calculator->m_pedPieceType);
            if (pPlayerPool->GetAt(PlayerID)->IsNPC())
                return true;
        }
        else
        {
            PLAYERID ActorID = FindActorIDFromGtaPtr(pPed);
            if (ActorID != INVALID_PLAYER_ID) {
                pLocalPlayer->SendGiveDamageEvent(ActorID,
                                                  calculator->m_fDamageFactor,
                                                  calculator->m_weaponType,
                                                  calculator->m_pedPieceType);
                return true;
            }
        }
    }


    // :check_friendly_fire
    if (!pNetGame->m_pNetSet->bFriendlyFire)
        return false;
    uint8_t byteTeam = pPlayerPool->GetLocalPlayer()->m_byteTeam;
    if (byteTeam == NO_TEAM ||
        PlayerID == INVALID_PLAYER_ID ||
        pPlayerPool->GetAt(PlayerID)->m_byteTeam != byteTeam) {
        return false;
    }

    return true;
}

// 0.3.7
void (*CPedDamageResponseCalculator__ComputeDamageResponse)(CPedDamageResponseCalculator* thiz, CPedGTA* pPed, uintptr_t* a3, uint32_t a4);
void CPedDamageResponseCalculator__ComputeDamageResponse_hook(CPedDamageResponseCalculator* thiz, CPedGTA* pPed, uintptr_t *a3, uint32_t a4)
{
    if (thiz == nullptr || pPed == nullptr || a3 == nullptr) return;

    if (ComputeDamageResponse(thiz, pPed))
        return;

    CPedDamageResponseCalculator__ComputeDamageResponse(thiz, pPed, a3, a4);
}

void (*CRenderer__RenderEverythingBarRoads)();
void CRenderer__RenderEverythingBarRoads_hook() {
    g_v29BarRoadsCount.fetch_add(1, std::memory_order_relaxed);
    static unsigned int v20BarRoadsSeq = 0;
    unsigned int currentBarSeq = 0;
    bool v20TraceThisCall = false;

    if(pNetGame) {
        if (pNetGame->GetGameState() == GAMESTATE_CONNECTED)
        {
            static bool loggedV12Renderer = false;
            if (!loggedV12Renderer)
            {
                CCamera& cameraProbe = *reinterpret_cast<CCamera*>(
                        g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
                FLog("V29 3D RENDERER ACTIVE | tid=%d ctx=%p camera=%p sceneCamera=%p world=%p fading=%d interior=%u",
                     V29GetTid(), (void*)eglGetCurrentContext(),
                     cameraProbe.m_pRwCamera,
                     Scene.m_pRwCamera,
                     Scene.m_pRpWorld,
                     cameraProbe.m_bFading ? 1 : 0,
                     (unsigned int)pGame->GetActiveInterior());
                loggedV12Renderer = true;
            }

            currentBarSeq = ++v20BarRoadsSeq;

            // V61: Render2dStuff from the previous frame has already returned.
            // Gate the first 3D stage of this next frame until EGL confirms
            // that the latest completed 2D pass was actually presented.
            V61WaitForPrevious2DAtNext3D(currentBarSeq);

            v20TraceThisCall = currentBarSeq <= 12;
            if (v20TraceThisCall)
            {
                const int visibleCount = V20GetVisibleCount();
                FLog("V20 BARROADS BEGIN | seq=%u visCount=%d visibleArray=%p",
                     currentBarSeq,
                     visibleCount,
                     V20GetVisibleArray());

                if (visibleCount >= 0 && visibleCount <= 1000)
                {
                    const int dumpCount = visibleCount < 16 ? visibleCount : 16;
                    CEntityGTA** list = V20GetVisibleArray();
                    for (int i = 0; i < dumpCount; ++i)
                    {
                        FLog("V20 VIS SLOT | barSeq=%u idx=%d raw=%p",
                             currentBarSeq,
                             i,
                             list[i]);
                    }
                }
            }
        }

        Skybox::Process();
    }

    CRenderer__RenderEverythingBarRoads();

    if (v20TraceThisCall)
    {
        FLog("V20 BARROADS END | seq=%u visCountNow=%d",
             currentBarSeq,
             V20GetVisibleCount());
    }
}

// =============================================================================
// V22 DIAGNOSTICO POS-BARROADS
//
// A V20 confirmou que RenderEverythingBarRoads() retorna normalmente.
// O crash acontece logo depois. No RenderScene original, os próximos passos
// incluem BreakManager_c::Render(false), RenderFadingInUnderwaterEntities()
// e RenderFadingInEntities().
//
// Como ainda não temos o símbolo/offset do BreakManager::Render confirmado
// nesta build Android, não vamos inventar endereço. Em vez disso, marcamos
// as duas funções CRenderer exportadas seguintes. Assim:
// - se nenhum BEGIN aparecer após "V20 BARROADS END", o crash ocorreu antes
//   delas (forte candidato: BreakManager_c::Render(false));
// - se aparecer BEGIN sem END, achamos a função exata;
// - se ambas retornarem, seguimos para o próximo estágio do RenderScene.
// =============================================================================

void (*CRenderer__RenderFadingInUnderwaterEntities)();
void CRenderer__RenderFadingInUnderwaterEntities_hook()
{
    static unsigned int seq = 0;
    const unsigned int current = ++seq;
    const bool trace = current <= 16;

    if (trace) FLog("V22 UNDERWATER FADING BEGIN | seq=%u", current);
    CRenderer__RenderFadingInUnderwaterEntities();
    if (trace) FLog("V22 UNDERWATER FADING END | seq=%u", current);
}

void (*CRenderer__RenderFadingInEntities)();
void CRenderer__RenderFadingInEntities_hook()
{
    static unsigned int seq = 0;
    const unsigned int current = ++seq;
    const bool trace = current <= 16;

    if (trace) FLog("V22 FADING ENTITIES BEGIN | seq=%u", current);
    CRenderer__RenderFadingInEntities();
    if (trace) FLog("V22 FADING ENTITIES END | seq=%u", current);
}


// =============================================================================
// V23 DIAGNOSTICO / BYPASS TEMPORARIO DA AGUA
//
// O V22 mostrou:
//   UNDERWATER FADING BEGIN -> END
//   [SIGBUS]
//   FADING ENTITIES BEGIN nunca aparece.
//
// No RenderScene original, CWaterLevel::RenderWater() fica exatamente entre
// essas duas etapas. Para confirmar sem alterar o restante do pipeline 3D,
// esta versao pula SOMENTE o RenderWater.
//
// Resultado esperado:
// - Se o jogo passar dessa tela, a causa esta no caminho da agua/WaterShader.
// - A agua pode ficar invisivel nesta versao. Isto e apenas um teste.
// =============================================================================

void (*CWaterLevel__RenderWater)();
void CWaterLevel__RenderWater_hook()
{
    static unsigned int seq = 0;
    const unsigned int current = ++seq;
    const bool trace = current <= 16;

    if (trace)
        FLog("V25 WATER BEGIN | seq=%u", current);

    // V25: WaterShader customizado continua bypassado no emu_glEndInternal_hook,
    // mas a rotina ORIGINAL de agua do GTA volta a ser executada.
    CWaterLevel__RenderWater();

    if (trace)
        FLog("V25 WATER END | seq=%u", current);
}


#include "CFPSFix.h"
#include "ES2VertexBuffer.h"
#include "RQ_Commands.h"
#include "Pickups.h"
#include "TimeCycle.h"
#include "game/Pipelines/CustomCar/CustomCarEnvMapPipeline.h"
#include "game/Pipelines/CustomBuilding/CustomBuildingDNPipeline.h"
#include "COcclusion.h"
#include "RealTimeShadowManager.h"
#include "game/Widgets/WidgetGta.h"

CFPSFix g_fps;

void (*ANDRunThread)(void* a1);
void ANDRunThread_hook(void* a1)
{
    g_fps.PushThread(gettid());

    ANDRunThread(a1);
}

static constexpr float ar43 = 4.0f/3.0f;
float *ms_fAspectRatio;
void (*DrawCrosshair)(uintptr_t* thiz);
void DrawCrosshair_hook(uintptr_t* thiz)
{
    float save1 = CCamera::m_f3rdPersonCHairMultX;
    CCamera::m_f3rdPersonCHairMultX = 0.530f - (*ms_fAspectRatio - ar43) * 0.01125f;

    float save2 = CCamera::m_f3rdPersonCHairMultY;
    CCamera::m_f3rdPersonCHairMultY = 0.400f + (*ms_fAspectRatio - ar43) * 0.03600f;

    DrawCrosshair(thiz);

    CCamera::m_f3rdPersonCHairMultX = save1;
    CCamera::m_f3rdPersonCHairMultY = save2;
}

CVector& (*FindPlayerSpeed)(int a1);
CVector& FindPlayerSpeed_hook(int a1)
{
    uintptr_t dwRetAddr = 0;
    __asm__ volatile ("mov %0, lr":"=r" (dwRetAddr));
    dwRetAddr -= g_libGTASA;

    if(dwRetAddr == 0x43E1F6 + 1)
    {
        if(pNetGame)
        {
            CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
            if(pPlayerPed &&
               pPlayerPed->IsInVehicle() &&
               pPlayerPed->IsAPassenger())
            {
                CVector vec = CVector(-1.0f);
                return vec;
            }
        }
    }

    return FindPlayerSpeed(a1);
}

int (*RwFrameAddChild)(int a1, int a2);
int RwFrameAddChild_hook(int a1, int a2)
{
    if(a1 == 0 || a2 == 0) return 0;
    return RwFrameAddChild(a1, a2);
}

int iLastTouchedWidgetId = -1;

int iLastReleasedWidgetId = -1;

int (*CTouchInterface__IsReleased)(int iWidgetId, int iUnk, int iEnableWidget);
int CTouchInterface__IsReleased_hook(int iWidgetId, int iUnk, int iEnableWidget)
{
    uintptr_t dwRetAddr = 0;
    __asm__ volatile ("mov %0, lr" : "=r" (dwRetAddr));
    dwRetAddr -= g_libGTASA;

    int iReleased = CTouchInterface__IsReleased(iWidgetId, iUnk, iEnableWidget);
    if(iReleased && iEnableWidget)
    {
        iLastReleasedWidgetId = iWidgetId;
    }

    return iReleased;
}

int (*CTextureDatabaseRuntime__GetEntry)(uintptr_t thiz, const char* a2, bool* a3);
int CTextureDatabaseRuntime__GetEntry_hook(uintptr_t thiz, const char* a2, bool* a3)
{
    if (!thiz)
    {
        return -1;
    }
    return CTextureDatabaseRuntime__GetEntry(thiz, a2, a3);
}

uintptr_t (*CTxdStore__TxdStoreFindCB)(const char *a1);
uintptr_t CTxdStore__TxdStoreFindCB_hook(const char *a1)
{
    static char* texdbs[] = { "samp", "gta_int", "gta3" };
    for(auto &texdb : texdbs)
    {
        // TextureDatabaseRuntime::GetDatabase
        uintptr_t db_handle = ((uintptr_t (*)(const char *))(g_libGTASA+0x1EAC8C+1))(texdb);

        // TextureDatabaseRuntime::registered
        uint32_t unk_61B8D4 = *(uint32_t*)(g_libGTASA+0x6BD174+4);
        if(unk_61B8D4)
        {
            // TextureDatabaseRuntime::registered
            uintptr_t dword_61B8D8 = *(uintptr_t*)(g_libGTASA+0x6BD174+8);

            int index = 0;
            while(*(uint32_t*)(dword_61B8D8 + 4 * index) != db_handle)
            {
                if(++index >= unk_61B8D4)
                    goto GetTheTexture;
            }

            continue;
        }

        GetTheTexture:
        // TextureDatabaseRuntime::Register
        ((void (*)(int))(g_libGTASA+0x1E9BC8+1))(db_handle);

        // TextureDatabaseRuntime::GetTexture
        uintptr_t tex = ((uintptr_t (*)(const char *))(g_libGTASA+0x1E9C64+1))(a1);

        // TextureDatabaseRuntime::Unregister
        ((void (*)(int))(g_libGTASA+0x1E9C80+1))(db_handle);

        if(tex) return tex;
    }

    // RwTexDictionaryGetCurrent
    int current = ((int (*)(void))(g_libGTASA+0x1DBA64+1))();
    if(current)
    {
        while(true)
        {
            // RwTexDictionaryFindNamedTexture
            uintptr_t tex = ((int (*)(int, const char *))(g_libGTASA+0x1DB9B0+1))(current, a1);
            if(tex) return tex;

            // CTxdStore::GetTxdParent
            current = ((int (*)(int))(g_libGTASA+0x5D428C+1))(current);
            if(!current) return 0;
        }
    }

    return 0;
}

int (*CCustomRoadsignMgr_RenderRoadsignAtomic)(int a1, int a2);
int CCustomRoadsignMgr_RenderRoadsignAtomic_hook(int a1, int a2)
{
    if ( a1 )
        return CCustomRoadsignMgr_RenderRoadsignAtomic(a1, a2);
}

int (*_RwTextureDestroy)(int a1);
int _RwTextureDestroy_hook(int a1)
{
    int result; // r0

    if ( (unsigned int)(a1 + 1) >= 2 )
        result = _RwTextureDestroy(a1);
    else
        result = 0;
    return result;
}

int (*CPed_UpdatePosition)(CPedGTA* a1);
int CPed_UpdatePosition_hook(CPedGTA* a1)
{
    int result; // r0

    if ( GamePool_FindPlayerPed() == a1 )
        result = CPed_UpdatePosition(a1);
    return result;
}

void (*CCamera__Process)(uintptr_t thiz);
void CCamera__Process_hook(uintptr_t thiz)
{
    //if(pGame->GetCamera())
    //pGame->GetCamera()->Update();

    CCamera__Process(thiz);
}

//extern CJavaWrapper* pJavaWrapper;
void (*MainMenuScreen__OnExit)();
void MainMenuScreen__OnExit_hook()
{
    pGame->bIsGameExiting = true;

    pNetGame->GetRakClient()->Disconnect(0);

    pJavaWrapper->exitGame();
}

void (*rqVertexBufferSelect)(unsigned int **result);
void rqVertexBufferSelect_hook(unsigned int **result)
{
    uint32_t buffer = *(uint32_t *)*result;
    *result += 4;
    if ( buffer )
    {
        glBindBuffer(34962, *(uint32_t *)(buffer + 8));
        *(uint32_t*)(g_libGTASA + 0x6B8AF0) = 0;
    }
    else
    {
        glBindBuffer(34962, 0);
    }
}

uintptr_t* (*rpMaterialListDeinitialize)(RpMaterialList* matList);
uintptr_t* rpMaterialListDeinitialize_hook(RpMaterialList* matList)
{
    if(!matList || !matList->materials)
        return nullptr;

    return rpMaterialListDeinitialize(matList);
}

void (*rqVertexBufferDelete)(unsigned int **result);
void rqVertexBufferDelete_hook(unsigned int **result)
{
    uint32_t* buffer = *(uint32_t **)*result;
    *result += 4;
    glDeleteBuffers(1, reinterpret_cast<const GLuint *>(buffer + 2));
    buffer[2] = 0;
    if ( buffer )
        (*(void (**)(uint32_t *))(*buffer + 4))(buffer);
}

void rotate_ped_if_local(unsigned int *a1, unsigned int *a2)
{
    if ( GamePool_FindPlayerPed() == (CPedGTA*)a2 )
        *(uint32_t *)(a2 + 0x560) = *a1;
}

void (*player_control_zelda)(unsigned int *a2, unsigned int *a3);
void player_control_zelda_hook(unsigned int *a2, unsigned int *a3)
{
    rotate_ped_if_local(a2, a3);
}

// 006778B0
int (*rxOpenGLDefaultAllInOneRenderCB)(RwResEntry* resEntry, uintptr_t object, uint8_t type, uint32_t flags);
int rxOpenGLDefaultAllInOneRenderCB_hook(RwResEntry* resEntry, uintptr_t object, uint8_t type, uint32_t flags)
{
    if(!resEntry || !flags)
        return 0;

    return rxOpenGLDefaultAllInOneRenderCB(resEntry, object, type, flags);
}

// 00677CB4
int (*CCustomBuildingDNPipeline__CustomPipeRenderCB)(RwResEntry* resEntry, uintptr_t object, uint8_t type, uint32_t flags);
int CCustomBuildingDNPipeline__CustomPipeRenderCB_hook(RwResEntry* resEntry, uintptr_t object, uint8_t type, uint32_t flags)
{
    if(!resEntry || !flags)
        return 0;

    return CCustomBuildingDNPipeline__CustomPipeRenderCB(resEntry, object, type, flags);
}

int (*EmuShader_Select)(uintptr_t *result);
int EmuShader_Select_hook(uintptr_t *result)
{
    int result1;
    if ( *result >= 0x1000 )
        return EmuShader_Select(result);
    return 0;
}

float float_4DD9E8;
float ms_fTimeStep;
float fMagic = 50.0f / 30.0f;
void (*CTaskSimpleUseGun__SetMoveAnim)(uintptr_t *thiz, uintptr_t *a2);
void CTaskSimpleUseGun__SetMoveAnim_hook(uintptr_t *thiz, uintptr_t *a2)
{
    ms_fTimeStep = *(float*)(g_libGTASA + 0x96B500);
    float_4DD9E8 = *(float*)(g_libGTASA + 0x4DD9E8);
    float_4DD9E8 = (fMagic) * (0.1f / ms_fTimeStep);
    CTaskSimpleUseGun__SetMoveAnim(thiz, a2);
}

int (*CAnimManager_UncompressAnimation)(int result);
int CAnimManager_UncompressAnimation_hook(int result)
{
    if ( result )
        return CAnimManager_UncompressAnimation(result);
    return 0;
}

void readVehiclesAudioSettings();

void (*CVehicleModelInfo__SetupCommonData)();

void CVehicleModelInfo__SetupCommonData_hook() {
    CVehicleModelInfo__SetupCommonData();
    readVehiclesAudioSettings();
}

extern VehicleAudioPropertiesStruct VehicleAudioProperties[20000];
static uintptr_t addr_veh_audio = (uintptr_t) &VehicleAudioProperties[0];

void (*CAEVehicleAudioEntity__GetVehicleAudioSettings)(uintptr_t thiz, int16_t a2, int a3);

void CAEVehicleAudioEntity__GetVehicleAudioSettings_hook(uintptr_t dest, int16_t a2, int ID) {
    memcpy((void *) dest, &VehicleAudioProperties[(ID - 400)], sizeof(VehicleAudioPropertiesStruct));
}

void (*CRadar_ClearBlip)(uint32_t a2);
void CRadar_ClearBlip_hook(uint32_t a2)
{
    uintptr_t dwRetAddr = 0;
    GET_LR(dwRetAddr);

    //LOGI("[CRadar::ClearBlip]: %d called from 0x%X", (uint16_t)a2, dwRetAddr);

    if ( (uint16_t)a2 > 249 )
    {
        LOGI("[CRadar::ClearBlip]: Invalid blip ID (%d) called from 0x%X", (uint16_t)a2, dwRetAddr);
    }
    else
    {
        CRadar_ClearBlip(a2);
    }
}

/* =============================================================================== */

void InstallHuaweiCrashFixHooks()
{
    CHook::InstallPLT(g_libGTASA + 0x677498, (uintptr_t)rqVertexBufferSelect_hook, (uintptr_t*)&rqVertexBufferSelect);
    CHook::InstallPLT(g_libGTASA + 0x679B14, (uintptr_t)rqVertexBufferDelete_hook, (uintptr_t*)&rqVertexBufferDelete);
    //CHook::InstallPLT(g_libGTASA + 0x677B6C, (uintptr_t)rqSetAlphaTest_hook, (uintptr_t*)&rqSetAlphaTest);
}

void InstallCrashFixHooks()
{
    // some crashfixes
    CHook::InstallPLT(g_libGTASA + 0x66F5AC, (uintptr_t)CCustomRoadsignMgr_RenderRoadsignAtomic_hook, (uintptr_t*)&CCustomRoadsignMgr_RenderRoadsignAtomic);
    CHook::InstallPLT(g_libGTASA + 0x67332C, (uintptr_t)_RwTextureDestroy_hook, (uintptr_t*)&_RwTextureDestroy);
    CHook::InstallPLT(g_libGTASA + 0x671458, (uintptr_t)CPed_UpdatePosition_hook, (uintptr_t*)&CPed_UpdatePosition);
    CHook::InstallPLT(g_libGTASA + 0x675490, (uintptr_t)RwFrameAddChild_hook, (uintptr_t*)&RwFrameAddChild);
    CHook::InstallPLT(g_libGTASA + 0x672D14, (uintptr_t)CTextureDatabaseRuntime__GetEntry_hook, (uintptr_t*)&CTextureDatabaseRuntime__GetEntry);
    //CHook::InstallPLT(g_libGTASA + 0x66FBD0, (uintptr_t)RpClumpForAllAtomics_hook, (uintptr_t*)&RpClumpForAllAtomics);
    CHook::InstallPLT(g_libGTASA + 0x6730F0, (uintptr_t)rpMaterialListDeinitialize_hook, (uintptr_t*)&rpMaterialListDeinitialize);
    //CHook::InstallPLT(g_libGTASA + 0x6778B0, (uintptr_t)rxOpenGLDefaultAllInOneRenderCB_hook, (uintptr_t*)&rxOpenGLDefaultAllInOneRenderCB);
    //CHook::InstallPLT(g_libGTASA + 0x677CB4, (uintptr_t)CCustomBuildingDNPipeline__CustomPipeRenderCB_hook, (uintptr_t*)&CCustomBuildingDNPipeline__CustomPipeRenderCB);
    //CHook::InstallPLT(g_libGTASA + 0x66F9E8, (uintptr_t)EmuShader_Select_hook, (uintptr_t*)&EmuShader_Select);
    CHook::InstallPLT(g_libGTASA + 0x6750D4, (uintptr_t)CAnimManager_UncompressAnimation_hook, (uintptr_t*)&CAnimManager_UncompressAnimation);
    //CHook::InstallPLT(g_libGTASA + 0x670E1C, (uintptr_t)CStreaming__MakeSpaceFor_hook, (uintptr_t*)&CStreaming__MakeSpaceFor);
}

void InstallWeaponFireHooks()
{
    //CHook::InstallPLT(g_libGTASA + 0x6716D0, (uintptr_t)CWeapon_FireInstantHit_hook, (uintptr_t*)&CWeapon_FireInstantHit);
    //CHook::InstallPLT(g_libGTASA + 0x671F10, (uintptr_t)CWorld_ProcessLineOfSight_hook, (uintptr_t*)&CWorld_ProcessLineOfSight);
    //CHook::InstallPLT(g_libGTASA + 0x670A10, (uintptr_t)CWeapon_FireSniper_hook, (uintptr_t*)&CWeapon_FireSniper);
    //CHook::InstallPLT(g_libGTASA + 0x66EAC4, (uintptr_t)CBulletInfo_AddBullet_hook, (uintptr_t*)&CBulletInfo_AddBullet);
}

void InstallSAMPHooks()
{
    //CHook::InstallPLT(g_libGTASA + 0x677EA0, (uintptr_t)MainMenuScreen__OnExit_hook, (uintptr_t*)&MainMenuScreen__OnExit);
    // samp main loop
    //CHook::InstallPLT(g_libGTASA + 0x67589C, (uintptr_t)Render2dStuff_hook, (uintptr_t*)&Render2dStuff);
    // imgui
    //CHook::InstallPLT(g_libGTASA + 0x6710C4, (uintptr_t)Idle_hook, (uintptr_t*)&Idle);
    //CHook::InstallPLT(g_libGTASA + 0x675DE4, (uintptr_t)AND_TouchEvent_hook, (uintptr_t*)&AND_TouchEvent);
    // splashscreen
    //ARMHook::installHook(g_libGTASA + 0x43AF28, (uintptr_t)DisplayScreen_hook, (uintptr_t*)&DisplayScreen);
    // gangzones
    //CHook::InstallPLT(g_libGTASA + 0x67196C, (uintptr_t)CRadar_DrawRadarGangOverlay_hook, (uintptr_t*)&CRadar_DrawRadarGangOverlay);
    // radar
    //CHook::InstallPLT(g_libGTASA+0x675914, (uintptr_t)CRadar__SetCoordBlip_hook, (uintptr_t*)&CRadar__SetCoordBlip);
    // removebuilding
    //CHook::InstallPLT(g_libGTASA + 0x675E6C, (uintptr_t)CFileLoader__LoadObjectInstance_hook, (uintptr_t*)&CFileLoader__LoadObjectInstance);
    // obj material
    //ARMHook::installHook(g_libGTASA + 0x454EF0, (uintptr_t)CObject_Render_hook, (uintptr_t*)& CObject_Render);
    // textdraw models
    //CHook::InstallPLT(g_libGTASA + 0x66FE58, (uintptr_t)CGame_Process_hook, (uintptr_t*)& CGame_Process);
    // enter vehicle as driver
    //ARMHook::codeInject(g_libGTASA + 0x40AC28, (uintptr_t)TaskEnterVehicle_hook, 0);
    //CHook::InstallPLT(g_libGTASA+0x6733F0, (uintptr_t)TaskEnterVehicle_hook, (uintptr_t*)&TaskEnterVehicle);
    // radar color
    //CHook::InstallPLT(g_libGTASA + 0x673950, (uintptr_t)CHudColours__GetIntColour_hook, (uintptr_t*)& CHudColours__GetIntColour);
    // exit vehicle
    CHook::InstallPLT(g_libGTASA + 0x671984, (uintptr_t)CTaskComplexLeaveCar_hook, (uintptr_t*)& CTaskComplexLeaveCar);
    CHook::InstallPLT(g_libGTASA + 0x675320, (uintptr_t)CTaskComplexLeaveCar_hook, (uintptr_t*)& CTaskComplexLeaveCar);
    // attach obj to ped
    //CHook::InstallPLT(g_libGTASA + 0x675C68, (uintptr_t)CWorld_ProcessPedsAfterPreRender_Hook, (uintptr_t*)&CWorld_ProcessPedsAfterPreRender);
    // game pause
    //CHook::InstallPLT(g_libGTASA + 0x672644, (uintptr_t)CTimer_StartUserPause_hook, (uintptr_t*)&CTimer_StartUserPause);
    //CHook::InstallPLT(g_libGTASA + 0x67056C, (uintptr_t)CTimer_EndUserPause_hook, (uintptr_t*)&CTimer_EndUserPause);
    // aim
    // Crosshair Fix
    //ms_fAspectRatio = (float*)(g_libGTASA+(VER_x32 ? 0xA26A90:0xCC7F00));
    //CHook::InstallPLT(g_libGTASA + 0x672880, (uintptr_t)DrawCrosshair_hook, (uintptr_t*)&DrawCrosshair);

    // fix radar in passenger
    CHook::InstallPLT(g_libGTASA+0x671BBC, (uintptr_t)FindPlayerSpeed_hook, (uintptr_t*)&FindPlayerSpeed);

    // fix texture loading
    CHook::InstallPLT(g_libGTASA + 0x676034, (uintptr_t)CTxdStore__TxdStoreFindCB_hook, (uintptr_t*)&CTxdStore__TxdStoreFindCB);

    // interpolate camera fix
    CHook::InstallPLT(g_libGTASA + 0x6717BC, (uintptr_t)CCamera__Process_hook, (uintptr_t*)&CCamera__Process);

    // for surfing
    //CHook::InstallPLT(g_libGTASA + 0x66EAE8, (uintptr_t)CWorld_ProcessAttachedEntities_Hook, (uintptr_t*)&CWorld_ProcessAttachedEntities);

    //CHook::InstallPLT(g_libGTASA + 0x67193C, (uintptr_t)player_control_zelda_hook, (uintptr_t*)&player_control_zelda);

    //ARMHook::installHook(g_libGTASA + 0x4DD5E8, (uintptr_t)CTaskSimpleUseGun__SetMoveAnim_hook, (uintptr_t*)&CTaskSimpleUseGun__SetMoveAnim);

    // hueta ne rabotaet no pust budet (tipo ne kak v 1.08)
    CHook::InstallPLT(g_libGTASA + 0x674280, (uintptr_t) CVehicleModelInfo__SetupCommonData_hook, (uintptr_t*)&CVehicleModelInfo__SetupCommonData);
    CHook::InstallPLT(g_libGTASA + 0x06D008, (uintptr_t) CAEVehicleAudioEntity__GetVehicleAudioSettings_hook, (uintptr_t*)&CAEVehicleAudioEntity__GetVehicleAudioSettings);

    CHook::InstallPLT(g_libGTASA + 0x66FF0C, (uintptr_t)CRadar_ClearBlip_hook, (uintptr_t*)&CRadar_ClearBlip);

    // skills
    CHook::InstallPLT(g_libGTASA + 0x6749D0, (uintptr_t)CPed__GetWeaponSkill_hook, (uintptr_t*)&CPed__GetWeaponSkill);

    //InstallHuaweiCrashFixHooks();
    InstallCrashFixHooks();
    InstallWeaponFireHooks();
    HookCPad();
}

void ReadSettingFile();
void ApplyFPSPatch(uint8_t fps);
void (*NvUtilInit)();
void NvUtilInit_hook()
{
    FLog("NvUtilInit");

    NvUtilInit();

    g_pszStorage = (char*)(g_libGTASA + (VER_x32 ? 0x6D687C : 0x8B46A8)); // StorageRootBuffer

    ReadSettingFile();

    ApplyFPSPatch(120);
}

struct stFile
{
    int isFileExist;
    FILE *f;
};

char lastFile[123];
static const char* WIU_INTERNAL_ROOT = "/data/user/0/com.samp.mobile/files/";

static bool V48ResolveReadablePathCaseInsensitive(
        const char* baseDir,
        const char* relativePath,
        char* outPath,
        size_t outPathSize)
{
    if (!baseDir || !relativePath || !outPath || outPathSize == 0)
        return false;

    char current[255]{};
    snprintf(current, sizeof(current), "%s", baseDir);

    // Normaliza a base para nao terminar com "/" (exceto raiz).
    size_t currentLen = strlen(current);
    while (currentLen > 1 && current[currentLen - 1] == '/')
    {
        current[--currentLen] = '\0';
    }

    char relative[255]{};
    snprintf(relative, sizeof(relative), "%s", relativePath);

    char* savePtr = nullptr;
    char* part = strtok_r(relative, "/", &savePtr);

    while (part)
    {
        DIR* dir = opendir(current);
        if (!dir)
            return false;

        bool found = false;
        char realName[256]{};

        while (dirent* entry = readdir(dir))
        {
            if (!strcasecmp(entry->d_name, part))
            {
                snprintf(realName, sizeof(realName), "%s", entry->d_name);
                found = true;
                break;
            }
        }

        closedir(dir);

        if (!found)
            return false;

        const size_t need =
                strlen(current) + 1 + strlen(realName) + 1;

        if (need > sizeof(current))
            return false;

        strcat(current, "/");
        strcat(current, realName);

        part = strtok_r(nullptr, "/", &savePtr);
    }

    // Confirma leitura real; nao basta o nome existir.
    errno = 0;
    FILE* check = fopen(current, "rb");
    if (!check)
        return false;

    fclose(check);
    snprintf(outPath, outPathSize, "%s", current);
    return true;
}


stFile* NvFOpen(const char* r0, const char* r1, int r2, int r3)
{
    strcpy(lastFile, r1);

    static char path[255]{};
    memset(path, 0, sizeof(path));

    sprintf(path, "%s%s", g_pszStorage, r1);

    // V45: para arquivos espelhados pela BetaTesterData, preferimos primeiro
    // a área PRIVADA real do app. Arquivos que não existem lá continuam no
    // caminho externo antigo.
    {
        char internalMirror[255]{};
        snprintf(internalMirror, sizeof(internalMirror), "%s%s", WIU_INTERNAL_ROOT, r1);

        FILE* mirrorCheck = fopen(internalMirror, "rb");
        if (mirrorCheck)
        {
            fclose(mirrorCheck);
            snprintf(path, sizeof(path), "%s", internalMirror);
            FLog("V45 INTERNAL MIRROR | %s", path);
        }
    }

    // V49 - MODELS interno com resolucao case-insensitive.
    // O Android e case-sensitive e o GTA pode pedir, por exemplo,
    // MODELS/COLL/WEAPONS.COL enquanto a copia privada usa models/coll/...
    // Resolve qualquer arquivo sob MODELS/ dentro da area privada antes de
    // cair no caminho externo de Android/data (que nesta build retorna EACCES).
    if (!strncasecmp(r1, "MODELS/", 7))
    {
        char resolvedModels[255]{};

        if (V48ResolveReadablePathCaseInsensitive(
                WIU_INTERNAL_ROOT,
                r1,
                resolvedModels,
                sizeof(resolvedModels)))
        {
            snprintf(path, sizeof(path), "%s", resolvedModels);
            FLog("V49 MODELS selected | request=%s | path=%s", r1, path);
        }
        else
        {
            FLog("V49 MODELS no internal candidate | request=%s", r1);
        }
    }

    // V51 - TEXDB/*.IMG ja esta dentro de texdb_app/texdb.
    // A BetaTesterData do usuario possui as pastas de textura e os arquivos
    // gta3.img, gta_int.img, samp.img, SAMPCOL.img, cutscene.img, player.img
    // todos na MESMA pasta texdb. Portanto nao criamos uma segunda pasta.
    if (!strncasecmp(r1, "TEXDB/", 6))
    {
        char resolvedTexdbImg[255]{};
        const char* relativeTexdb = r1 + 6;

        if (V48ResolveReadablePathCaseInsensitive(
                "/data/user/0/com.samp.mobile/files/texdb_app/texdb/",
                relativeTexdb,
                resolvedTexdbImg,
                sizeof(resolvedTexdbImg)))
        {
            snprintf(path, sizeof(path), "%s", resolvedTexdbImg);
            FLog("V51 TEXDB IMG selected | request=%s | path=%s", r1, path);
        }
        else
        {
            FLog("V51 TEXDB IMG no internal candidate | request=%s", r1);
        }
    }

    // V44 - AMERICAN.GXT no armazenamento interno privado do app.
    //
    // O V43 confirmou EACCES em todos os caminhos dentro de Android/data.
    // Primeiro tentamos a cópia preparada pelo Java em /data/user/0/.../files.
    // Só depois mantemos os caminhos externos antigos como fallback diagnóstico.
    if (!strncmp(r1, "TEXT/AMERICAN.GXT", 17))
    {
        const char* absoluteCandidates[] = {
                "/data/user/0/com.samp.mobile/files/TEXT/AMERICAN.GXT",
                "/data/data/com.samp.mobile/files/TEXT/AMERICAN.GXT"
        };

        bool foundReadableAmerican = false;

        for (const char* candidate : absoluteCandidates)
        {
            errno = 0;
            FILE* check = fopen(candidate, "rb");

            if (check)
            {
                fclose(check);
                snprintf(path, sizeof(path), "%s", candidate);

                FLog("V44 AMERICAN INTERNAL selected | %s", path);
                foundReadableAmerican = true;
                break;
            }

            const int checkErr = errno;
            FLog("V44 AMERICAN INTERNAL FAIL | path=%s | errno=%d | %s",
                 candidate, checkErr, strerror(checkErr));
        }

        if (!foundReadableAmerican)
        {
            const char* suffixes[] = {
                    "AMERICAN_APP.GXT",
                    "Text/american.gxt",
                    "TEXT/AMERICAN.GXT",
                    "SAMP_app/AMERICAN.GXT",
                    "SAMP_app/american.gxt"
            };

            char candidate[255]{};

            for (const char* suffix : suffixes)
            {
                memset(candidate, 0, sizeof(candidate));
                snprintf(candidate, sizeof(candidate), "%s%s", g_pszStorage, suffix);

                errno = 0;
                FILE* check = fopen(candidate, "rb");

                if (check)
                {
                    fclose(check);
                    snprintf(path, sizeof(path), "%s", candidate);

                    FLog("V44 AMERICAN EXTERNAL fallback selected | %s", path);
                    foundReadableAmerican = true;
                    break;
                }

                const int checkErr = errno;
                FLog("V44 AMERICAN EXTERNAL FAIL | path=%s | errno=%d | %s",
                     candidate, checkErr, strerror(checkErr));
            }
        }

        if (!foundReadableAmerican)
        {
            snprintf(path, sizeof(path), "%s%s", g_pszStorage, r1);
            FLog("V44 AMERICAN no readable source | original=%s", path);
        }
    }

    // Todo acesso a texdb/ vai para a copia criada pelo proprio app.
    // A importacao criou a estrutura texdb_app/texdb/...
    // Ex.: texdb/menu/menu.txt -> texdb_app/texdb/menu/menu.txt
    if (!strncmp(r1, "texdb/", 6))
    {
        snprintf(path, sizeof(path), "%stexdb_app/texdb/%s", WIU_INTERNAL_ROOT, r1 + 6);
        FLog("V45 Redirecting TEXDB INTERNAL -> %s", path);
    }

    // V47 - DATA interno robusto + compatibilidade com nomes minusculos.
    //
    // A BetaTesterData usa nomes como "fonts.dat", enquanto o GTA pode pedir
    // "DATA/FONTS.DAT". O armazenamento interno do Android e case-sensitive,
    // entao testamos tanto o nome original quanto a versao em minusculas.
    if (!strncmp(r1, "data/", 5) || !strncmp(r1, "DATA/", 5))
    {
        const char* relativeName = r1 + 5;

        char lowerRelative[180]{};
        size_t relLen = strlen(relativeName);

        if (relLen >= sizeof(lowerRelative))
            relLen = sizeof(lowerRelative) - 1;

        for (size_t i = 0; i < relLen; ++i)
            lowerRelative[i] = (char)tolower((unsigned char)relativeName[i]);

        lowerRelative[relLen] = '\0';

        char candidates[6][255]{};

        // Estrutura V46 atual: conteudo de BetaTesterData/data -> data_app/
        snprintf(candidates[0], sizeof(candidates[0]),
                 "%sdata_app/%s", WIU_INTERNAL_ROOT, relativeName);
        snprintf(candidates[1], sizeof(candidates[1]),
                 "%sdata_app/%s", WIU_INTERNAL_ROOT, lowerRelative);

        // Compatibilidade com importacoes anteriores.
        snprintf(candidates[2], sizeof(candidates[2]),
                 "%sdata_app/data/%s", WIU_INTERNAL_ROOT, relativeName);
        snprintf(candidates[3], sizeof(candidates[3]),
                 "%sdata_app/data/%s", WIU_INTERNAL_ROOT, lowerRelative);
        snprintf(candidates[4], sizeof(candidates[4]),
                 "%sdata_app/data/data/%s", WIU_INTERNAL_ROOT, relativeName);
        snprintf(candidates[5], sizeof(candidates[5]),
                 "%sdata_app/data/data/%s", WIU_INTERNAL_ROOT, lowerRelative);

        bool foundData = false;

        for (int i = 0; i < 6; ++i)
        {
            errno = 0;
            FILE* dataCheck = fopen(candidates[i], "rb");

            if (dataCheck)
            {
                fclose(dataCheck);
                snprintf(path, sizeof(path), "%s", candidates[i]);

                FLog("V47 DATA selected | %s", path);
                foundData = true;
                break;
            }

            const int dataErr = errno;
            FLog("V47 DATA candidate FAIL | path=%s | errno=%d | %s",
                 candidates[i], dataErr, strerror(dataErr));
        }

        if (!foundData)
        {
            // Mantem o caminho em minusculas como melhor fallback para a Data atual.
            snprintf(path, sizeof(path), "%sdata_app/%s",
                     WIU_INTERNAL_ROOT, lowerRelative);

            FLog("V47 DATA no readable candidate | %s", path);
        }
    }


    // V48 - AUDIO robusto.
    //
    // O V47 provou que a Data/TEXDB privada esta funcional, mas o audio podia
    // existir apenas na copia externa ou com diferenca de maiusculas/minusculas
    // (ex.: BANKSLOT.DAT x BankSlot.dat). O bloco antigo apenas testava
    // audio_app/audio/... e depois FORCAVA audio_app/... sem confirmar leitura.
    //
    // Agora tentamos, em ordem:
    //   1) privado: audio_app/...
    //   2) privado legado: audio_app/audio/...
    //   3) externo: audio_app/...
    //   4) externo legado: audio_app/audio/...
    //   5) externo antigo: audio/...
    //   6) externo antigo: AUDIO/...
    //
    // Cada candidato e resolvido segmento por segmento ignorando case, mas o
    // caminho final preserva o nome real do arquivo no filesystem.
    if (!strncmp(r1, "AUDIO/", 6) || !strncmp(r1, "audio/", 6))
    {
        const char* relativeAudio = r1 + 6;

        char audioBases[6][255]{};
        snprintf(audioBases[0], sizeof(audioBases[0]),
                 "%saudio_app", WIU_INTERNAL_ROOT);
        snprintf(audioBases[1], sizeof(audioBases[1]),
                 "%saudio_app/audio", WIU_INTERNAL_ROOT);
        snprintf(audioBases[2], sizeof(audioBases[2]),
                 "%saudio_app", g_pszStorage);
        snprintf(audioBases[3], sizeof(audioBases[3]),
                 "%saudio_app/audio", g_pszStorage);
        snprintf(audioBases[4], sizeof(audioBases[4]),
                 "%saudio", g_pszStorage);
        snprintf(audioBases[5], sizeof(audioBases[5]),
                 "%sAUDIO", g_pszStorage);

        bool foundAudio = false;

        for (int i = 0; i < 6; ++i)
        {
            char resolvedAudio[255]{};

            if (V48ResolveReadablePathCaseInsensitive(
                    audioBases[i],
                    relativeAudio,
                    resolvedAudio,
                    sizeof(resolvedAudio)))
            {
                snprintf(path, sizeof(path), "%s", resolvedAudio);
                FLog("V48 AUDIO selected | candidate=%d | request=%s | path=%s",
                     i, r1, path);
                foundAudio = true;
                break;
            }
        }

        if (!foundAudio)
        {
            // Fallback diagnostico canonico: mantem o local privado esperado.
            // O NvFOpen abaixo vai registrar errno real no fopen final.
            snprintf(path, sizeof(path), "%saudio_app/%s",
                     WIU_INTERNAL_ROOT, relativeAudio);

            FLog("V48 AUDIO no readable candidate | request=%s | fallback=%s",
                 r1, path);
        }
    }

    // ----------------------------
    
    // stream.ini copiado pelo proprio app para evitar EACCES/Permission denied
    if (!strcmp(r1, "stream.ini") || !strcmp(r1, "STREAM.INI"))
    {
        snprintf(path, sizeof(path), "%sstream_app.ini", WIU_INTERNAL_ROOT);
        FLog("Redirecting STREAM.INI -> %s", path);
    }

if(!strncmp(r1+12, "mainV1.scm", 10))
    {
        // TEMP TEST:
        // Let this GTA 2.10 base load its matching mainV1.scm instead of
        // replacing it with the much smaller SA-MP main.scm.
        snprintf(path, sizeof(path), "%sSAMP_app/mainV1.scm", WIU_INTERNAL_ROOT);
        FLog("TEST: loading original mainV1.scm -> %s", path);
    }
    // ----------------------------
    if(!strncmp(r1+12, "SCRIPTV1.IMG", 12))
    {
        snprintf(path, sizeof(path), "%sSAMP_app/script.img", WIU_INTERNAL_ROOT);
        FLog("Loading script.img..");
    }
    // ----------------------------
    if(!strncmp(r1, "DATA/PEDS.IDE", 13))
    {
        snprintf(path, sizeof(path), "%sSAMP_app/peds.ide", WIU_INTERNAL_ROOT);
        FLog("Loading peds.ide..");
    }
    // ----------------------------
    if(!strncmp(r1, "DATA/VEHICLES.IDE", 17))
    {
        snprintf(path, sizeof(path), "%sSAMP_app/vehicles.ide", WIU_INTERNAL_ROOT);
        FLog("Loading vehicles.ide..");
    }

    if (!strncmp(r1, "DATA/GTA.DAT", 12))
    {
        snprintf(path, sizeof(path), "%sSAMP_app/gta.dat", WIU_INTERNAL_ROOT);
        FLog("Loading gta.dat..");
    }

    if (!strncmp(r1, "DATA/WEAPON.DAT", 15))
    {
        // V55 - o pacote atual nao possui SAMP_app/weapon.dat.
        // O V47 ja resolveu DATA/WEAPON.DAT para data_app/weapon.dat;
        // so usa a versao SAMP se ela realmente existir.
        char sampWeaponPath[512];
        snprintf(sampWeaponPath, sizeof(sampWeaponPath),
                 "%sSAMP_app/weapon.dat", WIU_INTERNAL_ROOT);

        if (access(sampWeaponPath, R_OK) == 0)
        {
            snprintf(path, sizeof(path), "%s", sampWeaponPath);
            FLog("V55 WEAPON selected SAMP override | %s", path);
        }
        else
        {
            FLog("V55 WEAPON SAMP override missing | keeping DATA resolved path | %s", path);
        }
    }

    // V54 - ANIM resolver robusto.
    // O Android e case-sensitive e a Data pode trazer PED.IFP/ped.ifp,
    // alem de existir uma camada anim/ extra em alguns pacotes.
    // Resolve o nome real antes de entregar o arquivo ao GTA.
    if (!strncmp(r1, "ANIM/", 5) || !strncmp(r1, "anim/", 5))
    {
        const char* relativeAnim = r1 + 5;

        char animBases[6][255]{};
        snprintf(animBases[0], sizeof(animBases[0]),
                 "%sanim_app", WIU_INTERNAL_ROOT);
        snprintf(animBases[1], sizeof(animBases[1]),
                 "%sanim_app/anim", WIU_INTERNAL_ROOT);
        snprintf(animBases[2], sizeof(animBases[2]),
                 "%sanim_app/ANIM", WIU_INTERNAL_ROOT);
        snprintf(animBases[3], sizeof(animBases[3]),
                 "%sanim_app", g_pszStorage);
        snprintf(animBases[4], sizeof(animBases[4]),
                 "%sanim_app/anim", g_pszStorage);
        snprintf(animBases[5], sizeof(animBases[5]),
                 "%sanim", g_pszStorage);

        bool foundAnim = false;

        for (int i = 0; i < 6; ++i)
        {
            char resolvedAnim[255]{};

            if (V48ResolveReadablePathCaseInsensitive(
                    animBases[i],
                    relativeAnim,
                    resolvedAnim,
                    sizeof(resolvedAnim)))
            {
                snprintf(path, sizeof(path), "%s", resolvedAnim);
                FLog("V54 ANIM selected | candidate=%d | request=%s | path=%s",
                     i, r1, path);
                foundAnim = true;
                break;
            }
        }

        if (!foundAnim)
        {
            snprintf(path, sizeof(path), "%sanim_app/%s",
                     WIU_INTERNAL_ROOT, relativeAnim);
            FLog("V54 ANIM no readable candidate | request=%s | fallback=%s",
                 r1, path);
        }
    }

    // V50 - BYPASS DO CINFO.BIN IMPORTADO.
    //
    // A V49 passou WEAPONS.COL e carregou os IDE/DAT do mapa, mas caiu em
    // CColStore::IncludeModelIndex -> CColAccel::cacheLoadCol logo depois de
    // abrir CINFO_APP.BIN. Isso indica que o cache importado pode nao
    // corresponder ao conjunto atual de gta.dat/IDE/COL.
    //
    // Para este teste, tratamos CINFO.BIN como inexistente. Assim o GTA nao
    // consome o cache importado/stale e segue pelo caminho normal de carga
    // dos COLs. Nao apagamos o arquivo; apenas deixamos de entrega-lo ao GTA.
    if (!strcmp(r1, "CINFO.BIN"))
    {
        FLog("V50 CINFO BYPASS | request=%s | imported cache intentionally ignored", r1);
        return nullptr;
    }

    // SAMP IDE - o GTA pede "SAMP/samp.IDE", mas o arquivo do pacote
    // está em SAMP_app/SAMP.ide. Redirecionamos explicitamente para
    // evitar o Permission denied da pasta SAMP antiga.
    if (!strcmp(r1, "SAMP/samp.IDE") ||
        !strcmp(r1, "SAMP/SAMP.IDE") ||
        !strcmp(r1, "SAMP/samp.ide") ||
        !strcmp(r1, "SAMP/SAMP.ide"))
    {
        snprintf(path, sizeof(path), "%sSAMP_app/SAMP.ide", WIU_INTERNAL_ROOT);
        FLog("Redirecting SAMP.IDE -> %s", path);
    }

#if VER_x32
    auto *st = (stFile*)malloc(8);
#else
    auto *st = (stFile*)malloc(0x10);
#endif
    st->isFileExist = false;

    errno = 0;
    FILE *f  = fopen(path, "rb");

    if(f)
    {
        st->isFileExist = true;
        st->f = f;
        FLog("NVFOpen OK | %s", path);
        return st;
    }
    else
    {
        int openErr = errno;
        FLog("NVFOpen FAIL | path=%s | errno=%d | %s", path, openErr, strerror(openErr));
        free(st);
        return nullptr;
    }
}

// V12: keep every normal GTA fade intact.
// Only neutralize the exact late V9 diagnostic Fade(0.5f, 1) after SA-MP is
// already connected, because V9 proved that this state makes the next frame
// fall into SIGBUS. Everything else goes to the original GTA function.
void (*CCamera__Fade_V12_Original)(CCamera* thiz, float duration, short fadeInOutFlag);

void CCamera__Fade_V12_hook(CCamera* thiz, float duration, short fadeInOutFlag)
{
    if (pNetGame &&
        pNetGame->GetGameState() == GAMESTATE_CONNECTED &&
        fadeInOutFlag == 1 &&
        duration >= 0.49f && duration <= 0.51f)
    {
        static bool logged = false;
        if (!logged)
        {
            FLog("V12: neutralized late V9 Fade(0.5,1); normal GTA fades remain untouched");
            logged = true;
        }
        return;
    }

    CCamera__Fade_V12_Original(thiz, duration, fadeInOutFlag);
}

bool g_bPlaySAMP = false;

void MainMenu_OnStartSAMP()
{
    if(g_bPlaySAMP) return;

    //InitInMenu();
    pGame->StartGame();

    // Deixa o GTA concluir a transicao New Game com os scripts normais ativos.
    // O teste anterior mostrou que bloquear os scripts ANTES daqui deixa a camera
    // sem target valido e causa SIGSEGV em CCamera::UpdateTargetEntity.
    FLog("WORLD INIT V2: calling OnNewGameCheck with story temporarily allowed");

    (( void (*)())(g_libGTASA + (VER_x32 ? 0x002A7270 + 1 : 0x365EA0)))();

    FLog("WORLD INIT V2: OnNewGameCheck returned; story will block when CNetGame exists");

    // Nao seta g_BlockGtaStoryScripts aqui.
    // O hook de CRunningScript::Process ja ativa o bloqueio automaticamente
    // assim que pNetGame for criado, antes da campanha offline avancar.
    //CHook::InlineHook(g_libGTASA, (VER_x32 ? 0x5A3E40 : , &DoSunAndMoon, &dword_67E048);

    g_bPlaySAMP = true;
}

unsigned int (*MainMenuScreen__Update)(uintptr_t thiz, float a2);
unsigned int MainMenuScreen__Update_hook(uintptr_t thiz, float a2)
{
    unsigned int ret = MainMenuScreen__Update(thiz, a2);

    // V13 DIAGNOSTICO:
    // O V12 mostrou SA-MP conectado e estavel, mas nenhum dos callbacks do
    // render 3D/RenderEffects/Render2dStuff instrumentados foi executado.
    // Agora confirmamos se o GTA continua preso no frontend/menu mesmo depois
    // de o SA-MP chegar ao estado CONNECTED.
    if (pNetGame &&
        pNetGame->GetGameState() == GAMESTATE_CONNECTED)
    {
        static bool loggedV13MenuStillUpdating = false;
        if (!loggedV13MenuStillUpdating)
        {
            FLog("V13: MainMenuScreen::Update still running while SA-MP is CONNECTED | g_bPlaySAMP=%d",
                 g_bPlaySAMP ? 1 : 0);
            loggedV13MenuStillUpdating = true;
        }
    }

    MainMenu_OnStartSAMP();
    return ret;
}

void (*StartGameScreen__OnNewGameCheck)();
void StartGameScreen__OnNewGameCheck_hook()
{
    // V13: registra se o frontend ainda tenta disparar NewGameCheck depois que
    // nossa inicializacao SA-MP ja marcou g_bPlaySAMP.
    if(g_bPlaySAMP)
    {
        static bool loggedV13NewGameSuppressed = false;
        if (!loggedV13NewGameSuppressed)
        {
            FLog("V13: StartGameScreen::OnNewGameCheck suppressed because g_bPlaySAMP=1");
            loggedV13NewGameSuppressed = true;
        }
        return;
    }

    StartGameScreen__OnNewGameCheck();
}

void (*CTaskSimpleUseGun__RemoveStanceAnims)(uintptr* thiz, void* ped, float a3);
void CTaskSimpleUseGun__RemoveStanceAnims_hook(uintptr* thiz, void* ped, float a3)
{
    if(!thiz)
        return;

    uintptr* m_pAnim = (uintptr*)(thiz + 0x2c);
    if(m_pAnim) {
        if (!((uintptr *)(m_pAnim + 0x14)))
            return;
    }
    CTaskSimpleUseGun__RemoveStanceAnims(thiz, ped, a3);
}

int (*CCollision__ProcessVerticalLine)(float *a1, float *a2, int a3, int a4, int *a5, int a6, int a7, int a8);
int CCollision__ProcessVerticalLine_hook(float *a1, float *a2, int a3, int a4, int *a5, int a6, int a7, int a8)
{
    int result; // r0

    if (a3)
        result = CCollision__ProcessVerticalLine(a1, a2, a3, a4, a5, a6, a7, a8);
    else
        result = 0;
    return result;
}

int(*CUpsideDownCarCheck__IsCarUpsideDown)(int, int);
int CUpsideDownCarCheck__IsCarUpsideDown_hook(int a1, int a2)
{
    /* Passengers leave the vehicle out of fear if it overturns */

//	if (*(uintptr_t*)(a2 + 20))
//	{
//		return CUpsideDownCarCheck__IsCarUpsideDown(a1, a2);
//	}
    return 0;
}

int (*CTaskSimpleGetUp__ProcessPed)(uintptr_t* thiz, CPedGTA* ped);
int CTaskSimpleGetUp__ProcessPed_hook(uintptr_t* thiz, CPedGTA* ped)
{
    //return false;
    if(!ped)return 0;
    int res = 0;
    try {
        res = CTaskSimpleGetUp__ProcessPed(thiz, ped);
    }
    catch(...) {
        return 0;
    }

    return res;
}

int64 getmip()
{
    return 1;
}

uint64_t* RQCommand_rqSetAlphaTest(uint64_t *result)
{
    *result += 8;
    return result;
}

int64 GetInputType(void)
{
    return 0LL;
}

int(*CAnimBlendNode__FindKeyFrame)(int, float, int, int);
int CAnimBlendNode__FindKeyFrame_hook(int a1, float a2, int a3, int a4)
{
    if (*(uintptr_t*)(a1 + 16))
    {
        return CAnimBlendNode__FindKeyFrame(a1, a2, a3, a4);
    }
    else return 0;
}

RwFrame* CClumpModelInfo_GetFrameFromId_Post(RwFrame* pFrameResult, RpClump* pClump, int id)
{
    if (pFrameResult)
        return pFrameResult;

    uintptr_t calledFrom = 0;
    __asm__ volatile ("mov %0, lr" : "=r" (calledFrom));
    calledFrom -= g_libGTASA;

    if (calledFrom == 0x00515708                // CVehicle::SetWindowOpenFlag
        || calledFrom == 0x00515730             // CVehicle::ClearWindowOpenFlag
        || calledFrom == 0x00338698             // CVehicleModelInfo::GetOriginalCompPosition
        || calledFrom == 0x00338B2C)            // CVehicleModelInfo::CreateInstance
        return nullptr;

    for (uint i = 2; i < 40; i++)
    {
        RwFrame* pNewFrameResult = nullptr;
        uint     uiNewId = id + (i / 2) * ((i & 1) ? -1 : 1);

        pNewFrameResult = ((RwFrame * (*)(RpClump * pClump, int id))(g_libGTASA + (VER_2_1 ? 0x003856D0 : 0x00335CC0) + 1))(pClump, i);

        if (pNewFrameResult)
        {
            return pNewFrameResult;
        }
    }

    return nullptr;
}
RwFrame* (*CClumpModelInfo_GetFrameFromId)(RpClump*, int);
RwFrame* CClumpModelInfo_GetFrameFromId_hook(RpClump* a1, int a2)
{
    return CClumpModelInfo_GetFrameFromId_Post(CClumpModelInfo_GetFrameFromId(a1, a2), a1, a2);
}

void (*FxEmitterBP_c__Render)(uintptr_t* a1, int a2, int a3, float a4, char a5);
void FxEmitterBP_c__Render_hook(uintptr_t* a1, int a2, int a3, float a4, char a5)
{
    if(!a1 || !a2) return;
    uintptr_t* temp = *((uintptr_t**)a1 + 3);
    if (!temp)
    {
        return;
    }
    FxEmitterBP_c__Render(a1, a2, a3, a4, a5);
}

bool (*RwResourcesFreeResEntry)(void* entry);
bool RwResourcesFreeResEntry_hook(void* entry)
{
    bool result;
    if (entry) result = RwResourcesFreeResEntry(entry);
    else result = false;
    return result;
}

// V53 - CINFO runtime write through stdio.
//
// V52 proved that the CINFO write request is intercepted, but passing an
// absolute /data/user/0/... path back into Rockstar's CFileMgr returns handle
// 0. CColAccel::endCache does not validate that handle before calling Write,
// so OS_FileWrite dereferences an invalid file and crashes at address 0x8.
//
// For the CINFO rebuild only, bypass Rockstar's writer completely:
//   OpenFileForWriting -> fopen() in app-private storage
//   CFileMgr::Write    -> fwrite() for a private sentinel handle
//   CFileMgr::CloseFile-> fclose() for that same sentinel
// All other files keep using the original CFileMgr functions.
uintptr_t (*CFileMgr_OpenFileForWriting_V53_Original)(const char* fileName);
int (*CFileMgr_Write_V53_Original)(uintptr_t file, char* buffer, int size);
int (*CFileMgr_CloseFile_V53_Original)(uintptr_t file);

static FILE* g_V53CinfoRuntimeFile = nullptr;
static const uintptr_t V53_CINFO_HANDLE = (uintptr_t)0x53C1F0u;
static unsigned long long g_V53CinfoBytesWritten = 0;

uintptr_t CFileMgr_OpenFileForWriting_V53_hook(const char* fileName)
{
    if (fileName)
    {
        const bool isCInfo =
                !strcasecmp(fileName, "MODELS\\CINFO.BIN") ||
                !strcasecmp(fileName, "MODELS/CINFO.BIN") ||
                !strcasecmp(fileName, "CINFO.BIN");

        if (isCInfo)
        {
            char privatePath[255]{};
            snprintf(privatePath, sizeof(privatePath),
                     "%sCINFO_RUNTIME.BIN", WIU_INTERNAL_ROOT);

            if (g_V53CinfoRuntimeFile)
            {
                fclose(g_V53CinfoRuntimeFile);
                g_V53CinfoRuntimeFile = nullptr;
            }

            errno = 0;
            g_V53CinfoRuntimeFile = fopen(privatePath, "wb");
            const int openErr = errno;
            g_V53CinfoBytesWritten = 0;

            if (!g_V53CinfoRuntimeFile)
            {
                FLog("V53 CINFO fopen FAIL | request=%s | path=%s | errno=%d | %s",
                     fileName, privatePath, openErr, strerror(openErr));
                return 0;
            }

            FLog("V53 CINFO fopen OK | request=%s | path=%s | sentinel=0x%llx",
                 fileName, privatePath,
                 (unsigned long long)V53_CINFO_HANDLE);

            return V53_CINFO_HANDLE;
        }
    }

    return CFileMgr_OpenFileForWriting_V53_Original(fileName);
}

int CFileMgr_Write_V53_hook(uintptr_t file, char* buffer, int size)
{
    if (file == V53_CINFO_HANDLE)
    {
        if (!g_V53CinfoRuntimeFile || !buffer || size < 0)
        {
            FLog("V53 CINFO fwrite INVALID | fp=%p | buffer=%p | size=%d",
                 g_V53CinfoRuntimeFile, buffer, size);
            return 0;
        }

        errno = 0;
        const size_t written = fwrite(buffer, 1, (size_t)size, g_V53CinfoRuntimeFile);
        const int writeErr = errno;
        g_V53CinfoBytesWritten += (unsigned long long)written;

        if (written != (size_t)size)
        {
            FLog("V53 CINFO fwrite SHORT | wanted=%d | wrote=%llu | errno=%d | %s",
                 size, (unsigned long long)written, writeErr, strerror(writeErr));
        }
        else
        {
            FLog("V53 CINFO fwrite OK | chunk=%d | total=%llu",
                 size, g_V53CinfoBytesWritten);
        }

        return (int)written;
    }

    return CFileMgr_Write_V53_Original(file, buffer, size);
}

int CFileMgr_CloseFile_V53_hook(uintptr_t file)
{
    if (file == V53_CINFO_HANDLE)
    {
        if (!g_V53CinfoRuntimeFile)
        {
            FLog("V53 CINFO fclose skipped | runtime file already null");
            return 0;
        }

        fflush(g_V53CinfoRuntimeFile);
        errno = 0;
        const int result = fclose(g_V53CinfoRuntimeFile);
        const int closeErr = errno;
        g_V53CinfoRuntimeFile = nullptr;

        FLog("V53 CINFO fclose | result=%d | total=%llu | errno=%d | %s",
             result, g_V53CinfoBytesWritten, closeErr, strerror(closeErr));

        return result;
    }

    return CFileMgr_CloseFile_V53_Original(file);
}

static uint32_t dwRLEDecompressSourceSize = 0;

size_t (*OS_FileRead)(OSFile a1, void *buffer, size_t numBytes);
size_t OS_FileRead_hook(OSFile a1, void *buffer, size_t numBytes)
{
    dwRLEDecompressSourceSize = numBytes;

    return OS_FileRead(a1, buffer, numBytes);
}

char g_iLastBlock[123];

int *(*LoadFullTexture)(TextureDatabaseRuntime *thiz, unsigned int a2);
int *LoadFullTexture_hook(TextureDatabaseRuntime *thiz, unsigned int a2)
{
	strcpy(g_iLastBlock, thiz->name);

    return LoadFullTexture(thiz, a2);
}

void (*RLEDecompress)(uint8_t* pDest, size_t uiDestSize, uint8_t const* pSrc, size_t uiSegSize, uint32_t uiEscape);
void RLEDecompress_hook(uint8_t* pDest, size_t uiDestSize, const uint8_t* pSrc, size_t uiSegSize, uint32_t uiEscape) {

    if (!pDest || !pSrc || uiDestSize == 0 || uiSegSize == 0) {
        // Обработка некорректных входных данных или размеров
        // Здесь можно сгенерировать исключение или вернуть код ошибки
        return;
    }

    const uint8_t* pTempSrc = pSrc;
    const uint8_t* const pEndOfDest = pDest + uiDestSize;
    const uint8_t* const pEndOfSrc = pSrc + dwRLEDecompressSourceSize; // Предполагается, что dwRLEDecompressSourceSize определено правильно

    try {
        while (pDest < pEndOfDest && pTempSrc < pEndOfSrc) {
            if (*pTempSrc == uiEscape) {
                if (pTempSrc + 1 >= pEndOfSrc || pTempSrc[1] == 0 || pTempSrc + 2 + uiSegSize > pEndOfSrc) {
                    // Обработка ошибки, неверное значение ucCurSeg или недостаточно данных в исходном буфере
                    throw std::runtime_error("rled error 1");
                }

                uint8_t ucCurSeg = pTempSrc[1];
                while (ucCurSeg--) {
                    if (pDest + uiSegSize > pEndOfDest) {
                        // Обработка ошибки, недостаточно места в целевом буфере
                        throw std::runtime_error("rled error 2");
                    }
                    memcpy(pDest, pTempSrc + 2, uiSegSize);
                    pDest += uiSegSize;
                }
                pTempSrc += 2 + uiSegSize;
            } else {
                if (pDest + uiSegSize > pEndOfDest || pTempSrc + uiSegSize > pEndOfSrc) {
                    // Обработка ошибки, недостаточно данных в исходном буфере или недостаточно места в целевом буфере
                    throw std::runtime_error("rled error 3");
                }
                memcpy(pDest, pTempSrc, uiSegSize);
                pDest += uiSegSize;
                pTempSrc += uiSegSize;
            }
        }

        dwRLEDecompressSourceSize = 0;
    } catch (const std::exception& e) {
        FLog("%s", e.what());
    }
}

void (*CGame_Process)();
void CGame_Process_hook()
{
    if(pGame->bIsGameExiting)return;

    CGame_Process();

    if (pNetGame)
    {
        if(pGame && pGame->FindPlayerPed() && pUI && pUI->buttonpanel() && pUI->buttonpanel()->m_bH)
        {
            if(pGame->FindPlayerPed()->IsInVehicle())
            {
                pUI->buttonpanel()->m_bH->setCaption("D/B");
            }
            else
                pUI->buttonpanel()->m_bH->setCaption("H");
        }

        CObjectPool* pObjectPool = pNetGame->GetObjectPool();
        if (pObjectPool) {
            pObjectPool->Process();
            pObjectPool->ProcessMaterialText();
        }

        CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
        if (pTextDrawPool) {
            pTextDrawPool->SnapshotProcess();
        }
    }
}

void(*CStreaming__Init2)();
void CStreaming__Init2_hook()
{
    CStreaming__Init2();
    *(uint32_t*)(g_libGTASA+(VER_x32 ? 0x00685FA0:0x85EBD8)) = 536870912;
}

void SetUpGLHooks();

int(*mpg123_param)(void* mh, int key, long val, int ZERO, double fval);

int mpg123_param_hook(void* mh, int key, long val, int ZERO, double fval)
{
    // 0x2000 = MPG123_SKIP_ID3V2
    // 0x200  = MPG123_FUZZY
    // 0x100  = MPG123_SEEKBUFFER
    // 0x40   = MPG123_GAPLESS
    return mpg123_param(mh, key, val | (0x2000 | 0x200 | 0x100 | 0x40), ZERO, fval);
}

#include "Widgets/TouchInterface.h"
void InjectHooks()
{
    FLog("InjectHooks");
    CHook::Write(g_libGTASA + (VER_x32 ? 0x678954 : 0x84F2D0), &Scene);

#if !VER_x32 // mb all.. wtf crash x64?
    CHook::RET("_ZN11CPlayerInfo14LoadPlayerSkinEv");
    CHook::RET("_ZN11CPopulation10InitialiseEv");
#endif
    CCustomCarEnvMapPipeline::InjectHooks();
    SetUpGLHooks();
    CCamera::InjectHooks(); //
    CReferences::InjectHooks(); //
    CModelInfo::injectHooks(); //
    CTimer::InjectHooks(); //
    //cTransmission::InjectHooks(); //
    CAnimBlendAssociation::InjectHooks(); //
    //cHandlingDataMgr::InjectHooks(); //
    CPools::InjectHooks(); //
    CVehicleGTA::InjectHooks(); //
    CMatrixLink::InjectHooks(); //
    CMatrixLinkList::InjectHooks(); //
    CStreaming::InjectHooks();
    CPlaceable::InjectHooks(); //
    CMatrix::InjectHooks(); //
    CCollision::InjectHooks(); //
    //CIdleCam::InjectHooks(); //
    CTouchInterface::InjectHooks(); //
    CWidgetGta::InjectHooks();
    CEntityGTA::InjectHooks(); //
    CPhysical::InjectHooks(); //
    CAnimManager::InjectHooks(); //
    //CCarEnterExit::InjectHooks();
    CPlayerPedGta::InjectHooks(); //
    CTaskManager::InjectHooks(); //
    //CPedIntelligence::InjectHooks(); //
    CWorld::InjectHooks(); //
    CGame::InjectHooks();
    // V42 CONTROLLED TEST:
    // Leave GTA's original ES2 vertex-buffer CPU state and RenderQueue
    // vertex-buffer selector untouched. V41 disabled only CRQ redirect;
    // V42 disables BOTH custom injections to isolate this entire layer.
    // ES2VertexBuffer::InjectHooks();
    // CRQ_Commands::InjectHooks();
    CTxdStore::InjectHooks();
    CVisibilityPlugins::InjectHooks();
    //CAdjustableHUD::InjectHooks();

    // new
    //CClouds::InjectHooks();
    //CWeather::InjectHooks();
    //RenderBuffer::InjectHooks();
    CTimeCycle::InjectHooks();
    CCoronas::InjectHooks();
    //CDraw::InjectHooks();
    //CClock::InjectHooks();
    //CBirds::Init();
    CVehicleModelInfo::InjectHooks();
    //CPathFind::InjectHooks();
    CSprite2d::InjectHooks();
    //CFileLoader::InjectHooks();
    //CShadows::InjectHooks();
    CPickups::InjectHooks();
    CRenderer::InjectHooks();
    CStreamingInfo::InjectHooks();
    TextureDatabase::InjectHooks();
    TextureDatabaseEntry::InjectHooks();
    TextureDatabaseRuntime::InjectHooks();
    CCustomBuildingDNPipeline::InjectHooks();
    //CWidgetRadar::InjectHooks();

    //CRealTimeShadowManager::InjectHooks();
    CHook::Write(g_libGTASA+(VER_x32 ? 0xA41140 : 0xCE3EE8), &COcclusion::aOccluders);
    CHook::Write(g_libGTASA+(VER_x32 ? 0xA45790:0xCE8538), &COcclusion::NumOccludersOnMap);
}

void InstallCutHooks()
{
// pvr
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E87A0 : 0x714003 ));
    *(char*)(g_libGTASA + (VER_x32 ? 0x1E87A0 : 0x714003 ) + 12) = 'd';
    *(char*)(g_libGTASA + (VER_x32 ? 0x1E87A0 : 0x714003 ) + 13) = 'x';
    *(char*)(g_libGTASA + (VER_x32 ? 0x1E87A0 : 0x714003 ) + 14) = 't';

    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8C04 : 0x71406F));
    *(char*)(g_libGTASA + (VER_x32 ? 0x1E8C04 : 0x71406F) + 12) = 'd';
    *(char*)(g_libGTASA + (VER_x32 ? 0x1E8C04 : 0x71406F) + 13) = 'x';
    *(char*)(g_libGTASA + (VER_x32 ? 0x1E8C04 : 0x71406F) + 14) = 't';

// etc

    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E878C : 0x714017 ));
    *(char*)(g_libGTASA + (VER_x32 ? 0x1E878C : 0x714017 ) + 12) = 'd';
    *(char*)(g_libGTASA + (VER_x32 ? 0x1E878C : 0x714017 ) + 13) = 'x';
    *(char*)(g_libGTASA + (VER_x32 ? 0x1E878C : 0x714017 ) + 14) = 't';

    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E8BF4 : 0x71407F));
    *(char*)(g_libGTASA + (VER_x32 ? 0x1E8BF4 : 0x71407F) + 12) = 'd';
    *(char*)(g_libGTASA + (VER_x32 ? 0x1E8BF4 : 0x71407F) + 13) = 'x';
    *(char*)(g_libGTASA + (VER_x32 ? 0x1E8BF4 : 0x71407F) + 14) = 't';

// unc

    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x1E87F0 : 0x713FB3 ));
    *(char*)(g_libGTASA + (VER_x32 ? 0x1E87F0 : 0x713FB3 ) + 12) = 'd';
    *(char*)(g_libGTASA + (VER_x32 ? 0x1E87F0 : 0x713FB3 ) + 13) = 'x';
    *(char*)(g_libGTASA + (VER_x32 ? 0x1E87F0 : 0x713FB3 ) + 14) = 't';
}



//skybox code

#include "hooks.h"

//WaterShader
#include "..//game/WaterShader.h"
uintptr_t* g_WaterShaderClass = nullptr;
void (*emu_glEndInternal)();

// =============================================================================
// V24 DIAGNOSTICO: desativar SOMENTE o WaterShader customizado.
//
// O SIGBUS continua sempre em libGTASA.so + 0x1BC269. Neste source existe
// uma chamada direta para 0x1BC20C (mesma regiao da libGTASA) e tambem um
// hook de emu_glEndInternal que troca o shader antes de devolver ao GTA.
//
// Para separar "engine original" de "shader customizado", a V24 mantem a
// chamada ORIGINAL emu_glEndInternal(), mas NAO chama temporariamente:
//   WaterShader::BuildShadersSource1(...)
//   WaterShader::EmuShader__Select3(...)
//
// Se GLEND BEGIN aparecer e GLEND END nao aparecer, o crash esta dentro do
// emu_glEndInternal original. Se os dois aparecerem e o jogo avancar, o
// WaterShader customizado era o gatilho.
// =============================================================================
void emu_glEndInternal_hook()
{
    if (pNetGame) g_v29EmuEndCount.fetch_add(1, std::memory_order_relaxed);
    static unsigned int v24ConnectedSeq = 0;
    static bool v24LoggedHookActive = false;

    if (!v24LoggedHookActive)
    {
        FLog("V29 GLEND HOOK ACTIVE | tid=%d ctx=%p", V29GetTid(), (void*)eglGetCurrentContext());
        v24LoggedHookActive = true;
    }

    const bool connected =
            pNetGame && pNetGame->GetGameState() == GAMESTATE_CONNECTED;

    unsigned int current = 0;
    bool trace = false;

    if (connected)
    {
        current = ++v24ConnectedSeq;
        trace = current <= 16;

        if (trace)
        {
            const uintptr_t emuFlags =
                    *(uintptr_t *)(g_libGTASA + (VER_x32 ? 0x006B7094 : 0x8944A8));

            FLog("V24 GLEND BEGIN | seq=%u flags=0x%llx x32=%d",
                 current,
                 (unsigned long long)emuFlags,
                 VER_x32 ? 1 : 0);

            FLog("V24 WATERSHADER BYPASS | seq=%u", current);
        }
    }

    // V24: propositalmente NAO selecionar o WaterShader customizado.
    // A rotina original do GTA continua sendo chamada normalmente.
    emu_glEndInternal();

    if (trace)
        FLog("V24 GLEND END | seq=%u", current);
}

void InstallSpecialHooks()
{
    InjectHooks();

	// change etc/unc/pvr -> dxt
    //InstallCutHooks(); //If u use CRMP cache, u need to use this

    CHook::Redirect("_ZN5CGame20InitialiseRenderWareEv", &CGame::InitialiseRenderWare);
    CHook::InstallPLT(g_libGTASA + (VER_x32 ? 0x6785FC : 0x84EC20), &StartGameScreen__OnNewGameCheck_hook, &StartGameScreen__OnNewGameCheck);

    CHook::InlineHook("_Z10NvUtilInitv", &NvUtilInit_hook, &NvUtilInit);

    CHook::RET("_ZN12CCutsceneMgr16LoadCutsceneDataEPKc"); // LoadCutsceneData - crashfix apos conectar
  //  CHook::RET("_ZN12CCutsceneMgr10InitialiseEv");			// CCutsceneMgr::Initialise

    CHook::Redirect("_Z7NvFOpenPKcS0_bb", &NvFOpen);

    CHook::InlineHook("_ZN14MainMenuScreen6UpdateEf", &MainMenuScreen__Update_hook, &MainMenuScreen__Update);

    CHook::RET("_ZN4CPed31RemoveWeaponWhenEnteringVehicleEi"); // CPed::RemoveWeaponWhenEnteringVehicle

//    CHook::InstallPLT(g_libGTASA + (VER_x32 ? 0x6701D4 : 0x840708), &RLEDecompress_hook, &RLEDecompress); // comment fix bug with widgets, cause hook was written by ChatGPT (not mine code)
	CHook::InlineHook("_ZN22TextureDatabaseRuntime15LoadFullTextureEj", &LoadFullTexture_hook, &LoadFullTexture);

    CHook::InlineHook("_ZN8CFileMgr18OpenFileForWritingEPKc",
                      &CFileMgr_OpenFileForWriting_V53_hook,
                      &CFileMgr_OpenFileForWriting_V53_Original);

#if VER_x32
    CHook::InlineHook("_ZN8CFileMgr5WriteEiPci",
                      &CFileMgr_Write_V53_hook,
                      &CFileMgr_Write_V53_Original);
    CHook::InlineHook("_ZN8CFileMgr9CloseFileEi",
                      &CFileMgr_CloseFile_V53_hook,
                      &CFileMgr_CloseFile_V53_Original);
#else
    CHook::InlineHook("_ZN8CFileMgr5WriteEyPci",
                      &CFileMgr_Write_V53_hook,
                      &CFileMgr_Write_V53_Original);
    CHook::InlineHook("_ZN8CFileMgr9CloseFileEy",
                      &CFileMgr_CloseFile_V53_hook,
                      &CFileMgr_CloseFile_V53_Original);
#endif

    CHook::InlineHook("_Z11OS_FileReadPvS_i", &OS_FileRead_hook, &OS_FileRead);

    CHook::InlineHook("_Z32_rxOpenGLDefaultAllInOneRenderCBP10RwResEntryPvhj", &rxOpenGLDefaultAllInOneRenderCB_hook, &rxOpenGLDefaultAllInOneRenderCB);
    CHook::InlineHook("_ZN25CCustomBuildingDNPipeline18CustomPipeRenderCBEP10RwResEntryPvhj", &CCustomBuildingDNPipeline__CustomPipeRenderCB_hook, &CCustomBuildingDNPipeline__CustomPipeRenderCB);
}


#include <GLES2/gl2.h>   // If using OpenGL ES 2.0 or 3.0

void InstallHooks()
{
    CHook::InlineHook("_ZN7CCamera4FadeEfs", &CCamera__Fade_V12_hook, &CCamera__Fade_V12_Original);
    CHook::InlineHook("_ZN14CRunningScript7ProcessEv", &CRunningScript__Process_hook, &CRunningScript__Process);
    // V58: voltar ao Render2dStuff ORIGINAL do GTA e adicionar somente
    // as camadas SA-MP depois dele. O custom Render2dStuff estava rodando em
    // thread sem EGL context; a V57 provou que mover funcoes privadas do
    // RenderQueue para a thread EGL corrompe a fila. O original preserva a
    // ordem/render-target que a propria versao do GTASA espera.
    CHook::InlineHook("_Z13Render2dStuffv",
                      &Render2dStuff_V26_hook,
                      &Render2dStuff_V26_Original);
    CHook::InlineHook("_Z13RenderEffectsv",
                      &RenderEffects_V30_hook,
                      &RenderEffects_V30_Original);
    FLog("V31 EFFECTS HOOK | mode=CUSTOM_FIXED original=%p", (void*)RenderEffects_V30_Original);
    CHook::InlineHook("_Z14AND_TouchEventiiii", &AND_TouchEvent_hook, &AND_TouchEvent);
	
    CHook::Redirect("_ZN11CHudColours12GetIntColourEh", &CHudColours__GetIntColour); // dangerous
    CHook::Redirect("_ZN6CRadar19GetRadarTraceColourEjhh", &CRadar__GetRadarTraceColor); // dangerous
    CHook::InlineHook("_ZN6CRadar12SetCoordBlipE9eBlipType7CVectorj12eBlipDisplayPc", &CRadar__SetCoordBlip_hook, &CRadar__SetCoordBlip);
    CHook::InlineHook("_ZN6CRadar20DrawRadarGangOverlayEb", &CRadar_DrawRadarGangOverlay_hook, &CRadar_DrawRadarGangOverlay);

    CHook::Redirect("_Z10GetTexturePKc", &CUtil::GetTexture);

    CHook::InlineHook("_ZN14MainMenuScreen6OnExitEv", &MainMenuScreen__OnExit_hook, &MainMenuScreen__OnExit);

    CHook::InlineHook("_ZN17CTaskSimpleUseGun17RemoveStanceAnimsEP4CPedf", &CTaskSimpleUseGun__RemoveStanceAnims_hook, &CTaskSimpleUseGun__RemoveStanceAnims);

    // Bullet sync
    CHook::InlineHook("_ZN7CWeapon14FireInstantHitEP7CEntityP7CVectorS3_S1_S3_S3_bb", &CWeapon__FireInstantHit_hook, &CWeapon__FireInstantHit);
    CHook::InlineHook("_ZN7CWeapon10FireSniperEP4CPedP7CEntityP7CVector", &CWeapon__FireSniper_hook, &CWeapon__FireSniper);
    CHook::InlineHook("_ZN6CWorld18ProcessLineOfSightERK7CVectorS2_R9CColPointRP7CEntitybbbbbbbb", &CWorld__ProcessLineOfSight_hook, &CWorld__ProcessLineOfSight);
    CHook::InlineHook("_ZN28CPedDamageResponseCalculator21ComputeDamageResponseEP4CPedR18CPedDamageResponseb", &CPedDamageResponseCalculator__ComputeDamageResponse_hook, &CPedDamageResponseCalculator__ComputeDamageResponse);
    CHook::InlineHook("_ZN7CWeapon18ProcessLineOfSightERK7CVectorS2_R9CColPointRP7CEntity11eWeaponTypeS6_bbbbbbb", &CWeapon__ProcessLineOfSight_hook, &CWeapon__ProcessLineOfSight);
    CHook::InlineHook("_ZN11CBulletInfo9AddBulletEP7CEntity11eWeaponType7CVectorS3_", &CBulletInfo_AddBullet_hook, &CBulletInfo_AddBullet);

    //CHook::InlineHook("_ZN11CFileLoader18LoadObjectInstanceEPKc", &CFileLoader__LoadObjectInstance_hook, &CFileLoader__LoadObjectInstance);

    CHook::InlineHook("_ZN6CRadar9ClearBlipEi", &CRadar_ClearBlip_hook, &CRadar_ClearBlip);

    CHook::InlineHook("_ZN10CCollision19ProcessVerticalLineERK8CColLineRK7CMatrixR9CColModelR9CColPointRfbbP15CStoredCollPoly", &CCollision__ProcessVerticalLine_hook, &CCollision__ProcessVerticalLine);

    CHook::InlineHook("_ZN19CUpsideDownCarCheck15IsCarUpsideDownEPK8CVehicle", &CUpsideDownCarCheck__IsCarUpsideDown_hook, &CUpsideDownCarCheck__IsCarUpsideDown);

    CHook::InlineHook("_ZN16CTaskSimpleGetUp10ProcessPedEP4CPed", &CTaskSimpleGetUp__ProcessPed_hook, &CTaskSimpleGetUp__ProcessPed); // CTaskSimpleGetUp::ProcessPed
    CHook::InlineHook("_ZN7CObject6RenderEv", &CObject_Render_hook, & CObject_Render);

    CHook::Redirect("_Z19PlayerIsEnteringCarv", &PlayerIsEnteringCar);
    if(*(uint8_t *)(g_libGTASA + (VER_x32 ? 0x6B8B9C:0x896135)))
    {
        CHook::Redirect("_ZNK14TextureListing11GetMipCountEv", &getmip);
    }

    if (!eglGetProcAddress("glAlphaFuncQCOM")) {
        // If "glAlphaFuncQCOM" is not available, try "glAlphaFunc"

        if (eglGetProcAddress("glAlphaFunc")) {
            // If "glAlphaFunc" is found, store the address in the global library
            *((void**)(g_libGTASA + (VER_x32 ? 0x6BCBF8:0x89A1B0))) = (void*)eglGetProcAddress("glAlphaFunc");
        } else {
            // If neither function is available, hook the fallback symbol
            CHook::Redirect("_Z25RQ_Command_rqSetAlphaTestRPc", &RQCommand_rqSetAlphaTest);
        }
    }

    // V64: observe the existing RenderQueue consumer.  Unlike V36/V57 these
    // hooks NEVER invoke RenderQueue::Flush/Process or move commands between
    // threads; they simply wrap handlers that the GraphicsThread already calls.
    CHook::InlineHook("_Z24RQ_Command_rqDrawIndexedRPc",
                      &V64RQDrawIndexed_hook,
                      &V64RQDrawIndexed_Original);
    CHook::InlineHook("_Z27RQ_Command_rqDrawNonIndexedRPc",
                      &V64RQDrawNonIndexed_hook,
                      &V64RQDrawNonIndexed_Original);
    CHook::InlineHook("_Z25RQ_Command_rqTargetSelectRPc",
                      &V64RQTargetSelect_hook,
                      &V64RQTargetSelect_Original);
    CHook::InlineHook("_Z24RQ_Command_rqSwapBuffersRPc",
                      &V64RQSwapBuffers_hook,
                      &V64RQSwapBuffers_Original);

    FLog("V66 INSTALL: OFFSCREEN_WORLD_2D_COMPOSE + TARGET0_REDIRECT + FINAL_COMPOSE_BLIT + V64_TRACE + V61_GATE");

    g_v29EglSwapStub = shadowhook_hook_sym_name(
            "libEGL.so",
            "eglSwapBuffers",
            (void*)eglSwapBuffers_V29_hook,
            (void**)&eglSwapBuffers_V29_Original);

    g_v29EglMakeCurrentStub = shadowhook_hook_sym_name(
            "libEGL.so",
            "eglMakeCurrent",
            (void*)eglMakeCurrent_V29_hook,
            (void**)&eglMakeCurrent_V29_Original);

    // O projeto usa GLES3. Se algum simbolo nao estiver em libGLESv3,
    // fazemos fallback para libGLESv2.
    g_v29BindFramebufferStub = shadowhook_hook_sym_name(
            "libGLESv3.so", "glBindFramebuffer",
            (void*)glBindFramebuffer_V29_hook,
            (void**)&glBindFramebuffer_V29_Original);
    if (!g_v29BindFramebufferStub || !glBindFramebuffer_V29_Original)
        g_v29BindFramebufferStub = shadowhook_hook_sym_name(
                "libGLESv2.so", "glBindFramebuffer",
                (void*)glBindFramebuffer_V29_hook,
                (void**)&glBindFramebuffer_V29_Original);

    g_v29ClearColorStub = shadowhook_hook_sym_name(
            "libGLESv3.so", "glClearColor",
            (void*)glClearColor_V29_hook,
            (void**)&glClearColor_V29_Original);
    if (!g_v29ClearColorStub || !glClearColor_V29_Original)
        g_v29ClearColorStub = shadowhook_hook_sym_name(
                "libGLESv2.so", "glClearColor",
                (void*)glClearColor_V29_hook,
                (void**)&glClearColor_V29_Original);

    g_v29ClearStub = shadowhook_hook_sym_name(
            "libGLESv3.so", "glClear",
            (void*)glClear_V29_hook,
            (void**)&glClear_V29_Original);
    if (!g_v29ClearStub || !glClear_V29_Original)
        g_v29ClearStub = shadowhook_hook_sym_name(
                "libGLESv2.so", "glClear",
                (void*)glClear_V29_hook,
                (void**)&glClear_V29_Original);

    g_v29DrawArraysStub = shadowhook_hook_sym_name(
            "libGLESv3.so", "glDrawArrays",
            (void*)glDrawArrays_V29_hook,
            (void**)&glDrawArrays_V29_Original);
    if (!g_v29DrawArraysStub || !glDrawArrays_V29_Original)
        g_v29DrawArraysStub = shadowhook_hook_sym_name(
                "libGLESv2.so", "glDrawArrays",
                (void*)glDrawArrays_V29_hook,
                (void**)&glDrawArrays_V29_Original);

    g_v29DrawElementsStub = shadowhook_hook_sym_name(
            "libGLESv3.so", "glDrawElements",
            (void*)glDrawElements_V29_hook,
            (void**)&glDrawElements_V29_Original);
    if (!g_v29DrawElementsStub || !glDrawElements_V29_Original)
        g_v29DrawElementsStub = shadowhook_hook_sym_name(
                "libGLESv2.so", "glDrawElements",
                (void*)glDrawElements_V29_hook,
                (void**)&glDrawElements_V29_Original);

    FLog("V29 HOOKS | swap=%p/%p makeCurrent=%p/%p bind=%p/%p clearColor=%p/%p clear=%p/%p drawA=%p/%p drawE=%p/%p installTid=%d ctx=%p",
         g_v29EglSwapStub, (void*)eglSwapBuffers_V29_Original,
         g_v29EglMakeCurrentStub, (void*)eglMakeCurrent_V29_Original,
         g_v29BindFramebufferStub, (void*)glBindFramebuffer_V29_Original,
         g_v29ClearColorStub, (void*)glClearColor_V29_Original,
         g_v29ClearStub, (void*)glClear_V29_Original,
         g_v29DrawArraysStub, (void*)glDrawArrays_V29_Original,
         g_v29DrawElementsStub, (void*)glDrawElements_V29_Original,
         V29GetTid(), (void*)eglGetCurrentContext());

    CHook::InlineHook("_Z17emu_glEndInternalv", (uintptr_t)emu_glEndInternal_hook, (uintptr_t*)&emu_glEndInternal); // V24 diagnostic

    CHook::Redirect("_ZN4CHID12GetInputTypeEv", &GetInputType);

#if VER_x32
    CHook::InlineHook("_ZN14CAnimBlendNode12FindKeyFrameEf", &CAnimBlendNode__FindKeyFrame_hook, &CAnimBlendNode__FindKeyFrame);
    CHook::InlineHook("_ZN15CClumpModelInfo14GetFrameFromIdEP7RpClumpi", &CClumpModelInfo_GetFrameFromId_hook, &CClumpModelInfo_GetFrameFromId);
#endif

    CHook::InlineHook("_ZN13FxEmitterBP_c6RenderEP8RwCamerajfh", &FxEmitterBP_c__Render_hook, &FxEmitterBP_c__Render);
    CHook::InlineHook("_Z23RwResourcesFreeResEntryP10RwResEntry", &RwResourcesFreeResEntry_hook, &RwResourcesFreeResEntry);

    CHook::InlineHook("_ZN9CRenderer24RenderEverythingBarRoadsEv", &CRenderer__RenderEverythingBarRoads_hook, &CRenderer__RenderEverythingBarRoads);

    // V22: isolate the exact RenderScene stage immediately after BARROADS.
    CHook::InlineHook("_ZN9CRenderer32RenderFadingInUnderwaterEntitiesEv",
                      &CRenderer__RenderFadingInUnderwaterEntities_hook,
                      &CRenderer__RenderFadingInUnderwaterEntities);

    CHook::InlineHook("_ZN9CRenderer22RenderFadingInEntitiesEv",
                      &CRenderer__RenderFadingInEntities_hook,
                      &CRenderer__RenderFadingInEntities);


    // V23: RenderWater fica entre UNDERWATER FADING e FADING ENTITIES.
    // Bypass temporario para confirmar se o SIGBUS vem do caminho da agua.
    CHook::InlineHook("_ZN11CWaterLevel11RenderWaterEv",
                      &CWaterLevel__RenderWater_hook,
                      &CWaterLevel__RenderWater);

    ms_fAspectRatio = (float*)(g_libGTASA+(VER_x32 ? 0xA26A90:0xCC7F00));
    CHook::InlineHook("_ZN4CHud14DrawCrossHairsEv", &DrawCrosshair_hook, &DrawCrosshair);

    // V20: trace visible-list index/raw next slot around CRenderer::RenderOneNonRoad
    CHook::InlineHook("_ZN9CRenderer16RenderOneNonRoadEP7CEntity",
                      &CRenderer__RenderOneNonRoad_hook,
                      &CRenderer__RenderOneNonRoad);

    CHook::InlineHook("_ZN7CEntity13SetupLightingEv",
                      &CEntity__SetupLighting_hook,
                      &CEntity__SetupLighting);
    CHook::InlineHook("_ZN7CEntity14RemoveLightingEb",
                      &CEntity__RemoveLighting_hook,
                      &CEntity__RemoveLighting);

    CHook::InlineHook("_ZN7CObject13SetupLightingEv",
                      &CObject__SetupLighting_hook,
                      &CObject__SetupLighting);
    CHook::InlineHook("_ZN7CObject14RemoveLightingEb",
                      &CObject__RemoveLighting_hook,
                      &CObject__RemoveLighting);

    CHook::InlineHook("_ZN4CPed13SetupLightingEv",
                      &CPed__SetupLighting_hook,
                      &CPed__SetupLighting);
    CHook::InlineHook("_ZN4CPed14RemoveLightingEb",
                      &CPed__RemoveLighting_hook,
                      &CPed__RemoveLighting);

    CHook::InlineHook("_ZN8CVehicle13SetupLightingEv",
                      &CVehicle__SetupLighting_hook,
                      &CVehicle__SetupLighting);
    CHook::InlineHook("_ZN8CVehicle14RemoveLightingEb",
                      &CVehicle__RemoveLighting_hook,
                      &CVehicle__RemoveLighting);

    // retexture
    CHook::InlineHook("_ZN7CEntity6RenderEv", &CEntity_Render_hook, &CEntity_Render);

#if VER_x32
    CHook::UnFuck(g_libGTASA + 0x4DD9E8);
    *(float*)(g_libGTASA + 0x4DD9E8) = 0.015f;
#else
    CHook::Write(g_libGTASA + 0x5DF790, 0x90000AA9);
    CHook::Write(g_libGTASA + 0x5DF794, 0xBD48D521);
#endif

    CHook::InlineHook("_ZN10CStreaming5Init2Ev", &CStreaming__Init2_hook, &CStreaming__Init2);
HookCPad();
}
