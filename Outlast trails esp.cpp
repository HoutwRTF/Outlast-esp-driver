#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <winioctl.h>
#include <TlHelp32.h>
#include <dwmapi.h>
#include <d2d1.h>
#include <dwrite.h>
#include <cstdio>
#include <vector>
#include <cmath>
#include <algorithm>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

#define IOCTL_ATTACH  CTL_CODE(0x22, 0x696, 0, 0)
#define IOCTL_READ    CTL_CODE(0x22, 0x697, 0, 0)
#define IOCTL_PING    CTL_CODE(0x22, 0x699, 0, 0)
#define REQUEST_MAGIC 0xDEADC0DE13371337ULL

#pragma pack(push, 1)
struct REQUEST_DATA {
    ULONG64 Magic, ProcessId, Address, Buffer, Size, BaseAddress, Result;
};
#pragma pack(pop)

constexpr float PI = 3.14159265358979323846f;
constexpr float DEG2RAD = PI / 180.0f;

namespace Off {
    constexpr uintptr_t GWorld = 0x06B93870;
    constexpr uintptr_t Levels = 0x140;
    constexpr uintptr_t ActorArray = 0x98;
    constexpr uintptr_t GameInstance = 0x188;
    constexpr uintptr_t LocalPlayers = 0x38;
    constexpr uintptr_t PlayerController = 0x30;
    constexpr uintptr_t PlayerCameraManager = 0x2C8;
    constexpr uintptr_t CameraCache = 0x1C40;
    constexpr uintptr_t RootComponent = 0x140;
    constexpr uintptr_t RelativeLocation = 0x124;
    constexpr uintptr_t AttachParent = 0xC8;
    constexpr uintptr_t Pickup_InteractionZone = 0x0268;
    constexpr uintptr_t Pickup_ItemType = 0x0289;
    constexpr uintptr_t Pickup_ItemCategory = 0x0288;
    constexpr uintptr_t Pickup_bIsInHand = 0x0C20;
    constexpr uintptr_t Pickup_PawnOwner = 0x0900;
}

namespace PawnOff {
    constexpr uintptr_t CharLocation = 0x4AB0;
    constexpr uintptr_t Health = 0x5320;
    constexpr uintptr_t HealthF = 0x5324;
    constexpr uintptr_t bDead = 0x535C;
    constexpr uintptr_t MeshHolder = 0x55B8;
}

namespace NpcOff {
    constexpr uintptr_t NPCType = 0x57B0;
    constexpr uintptr_t NPCTags = 0x57B8;
    constexpr uintptr_t npcName = 0x5960;
    constexpr uintptr_t NPCId = 0x5970;
    constexpr uintptr_t BotActivity = 0x5E00;
    constexpr uintptr_t BotAwarenessState = 0x5E01;
    constexpr uintptr_t bIsInLimbo = 0x5E1A;
}

enum ENPCType : uint8_t {
    NPC_Grunt = 0, NPC_Ambient = 1, NPC_BigGrunt = 2, NPC_SleeperScreamer = 3,
    NPC_Pouncer = 4, NPC_Pusher = 5, NPC_GroundPitcher = 6, NPC_Jaeger = 7,
    NPC_Berserker = 8, NPC_Imposter = 9, NPC_NightHunter = 10, NPC_Spectre = 11,
    NPC_ThrowableTarget = 12, NPC_StalkerTarget = 13, NPC_Gooseberry = 14,
    NPC_Coyle = 15, NPC_Franco = 16, NPC_Otto = 17, NPC_Liliya = 18,
    NPC_Scientist = 19, NPC_Guard = 20, NPC_StageDefaultPrimeAsset = 21,
    NPC_StageNonDefaultPrimeAsset = 22, NPC_MAX = 23
};

enum ENPCCategory { NPCCAT_Dangerous, NPCCAT_Boss, NPCCAT_Passive, NPCCAT_Special };

enum EItemType : uint8_t {
    IT_HealthGain = 2, IT_HealthBoost = 3, IT_TempHealthGain = 4,
    IT_ReviveSyringe = 5, IT_RespawnConsumable = 6, IT_Bandage = 17,
    IT_HealthSicknessPill = 21, IT_PsychosisAntidote = 16,
    IT_StaminaRegen = 7, IT_MaxStaminaBoost = 8, IT_StaminaSicknessPill = 22,
    IT_SmallBattery = 9, IT_Battery = 10, IT_SuperBattery = 11,
    IT_LockPick = 12, IT_MasterKey = 13,
    IT_Bottle = 15, IT_Brick = 18, IT_GoreThrowable = 30, IT_AIAttractorItem = 41,
    IT_SkillCharge = 14, IT_TimeExtender = 20, IT_Decoder = 31,
    IT_ProximityDetector = 32, IT_MotionDetection = 37, IT_NoiseSuppression = 40,
    IT_BearTrap = 38, IT_ImposterTrap = 39,
    IT_Quest = 23, IT_ObjectiveThrowable = 29, IT_ObjectiveEquipable = 33,
    IT_ObjectiveCollectable = 34,
    IT_ActiveSkillController = 24, IT_ActiveSkillThrowable = 25,
    IT_ActiveSkillDeployable = 26, IT_ActiveSkillUsable = 27,
    IT_ActiveSkillAutoEquipable = 28,
    IT_CollectibleDocument = 19, IT_SpecialCollectibleDocument = 43,
    IT_BlacklightDetectableItem = 44,
    IT_ChainingUpgradePoint = 35, IT_AdvWeapon = 36,
    IT_ImposterStunThrowable = 42, IT_ImposterFakeItem = 45
};

enum EDisplayType {
    DISP_Health, DISP_Stamina, DISP_Battery, DISP_Key, DISP_Throwable,
    DISP_Trap, DISP_Quest, DISP_Skill, DISP_Document, DISP_Secret,
    DISP_Special, DISP_Antidote, DISP_Unknown,
    DISP_Enemy, DISP_EnemyBoss, DISP_EnemyPassive, DISP_EnemySpecial,
    DISP_EnemyAlert, DISP_EnemyChase
};

struct FVector { float X, Y, Z; };
struct FRotator { float Pitch, Yaw, Roll; };
struct CameraData { FVector Loc; FRotator Rot; float FOV; };
template<typename T> struct TArray { uintptr_t Data; int32_t Count, Max; };

bool g_ItemESPEnabled = true;
bool g_EnemyESPEnabled = true;
float g_ESPDistance = 50.0f;

class Memory {
    HANDLE hDriver = INVALID_HANDLE_VALUE;
    DWORD processId = 0;
    uintptr_t baseAddress = 0;
public:
    bool Initialize() {
        hDriver = CreateFileW(L"\\\\.\\KernelGDI", GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hDriver == INVALID_HANDLE_VALUE) return false;
        REQUEST_DATA req = {}; req.Magic = REQUEST_MAGIC; DWORD ret;
        DeviceIoControl(hDriver, IOCTL_PING, &req, sizeof(req), &req, sizeof(req), &ret, nullptr);
        return req.Result == 0x1337;
    }
    bool Attach(const wchar_t* name) {
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        PROCESSENTRY32W entry = { sizeof(entry) };
        if (Process32FirstW(snap, &entry)) do {
            if (_wcsicmp(entry.szExeFile, name) == 0) { processId = entry.th32ProcessID; break; }
        } while (Process32NextW(snap, &entry));
        CloseHandle(snap);
        if (!processId) return false;
        REQUEST_DATA req = {}; req.Magic = REQUEST_MAGIC; req.ProcessId = processId; DWORD ret;
        DeviceIoControl(hDriver, IOCTL_ATTACH, &req, sizeof(req), &req, sizeof(req), &ret, nullptr);
        baseAddress = req.BaseAddress;
        return req.Result == 1;
    }
    template<typename T> T Read(uintptr_t a) { T b{}; ReadRaw(a, &b, sizeof(T)); return b; }
    void ReadRaw(uintptr_t a, void* b, size_t s) {
        REQUEST_DATA req = {}; req.Magic = REQUEST_MAGIC; req.ProcessId = processId;
        req.Address = a; req.Buffer = (ULONG64)b; req.Size = s; DWORD ret;
        DeviceIoControl(hDriver, IOCTL_READ, &req, sizeof(req), &req, sizeof(req), &ret, nullptr);
    }
    uintptr_t Base() { return baseAddress; }
};

Memory g_Mem;

struct CachedItem { uintptr_t Actor, Root; const char* Name; EDisplayType DispType; bool IsImportant; };
struct CachedEnemy { uintptr_t Actor, Root; const char* Name; ENPCType Type; ENPCCategory Category; };

std::vector<CachedItem> g_Items;
std::vector<CachedEnemy> g_Enemies;
uintptr_t g_CamMgr = 0;

struct ScreenEntity {
    float X, Y, Dist;
    const char* Name;
    EDisplayType DispType;
    bool IsImportant;
    bool IsEnemy;
    uint8_t AwarenessState;
    ENPCCategory Category;
};

inline bool ValidPtr(uintptr_t p) { return p > 0x10000 && p < 0x7FFFFFFFFFFF; }
inline bool HeapPtr(uintptr_t p) { return p > 0x100000000 && p < 0x400000000000; }
inline bool ValidFloat(float f) { return !isnan(f) && !isinf(f) && fabsf(f) < 1e7; }
inline bool ValidPosition(FVector v) {
    return ValidFloat(v.X) && ValidFloat(v.Y) && ValidFloat(v.Z) && (v.X != 0 || v.Y != 0 || v.Z != 0);
}

FVector GetWorldPosition(uintptr_t root) {
    FVector pos = g_Mem.Read<FVector>(root + Off::RelativeLocation);
    uintptr_t parent = g_Mem.Read<uintptr_t>(root + Off::AttachParent);
    int depth = 0;
    while (ValidPtr(parent) && depth < 10) {
        FVector p = g_Mem.Read<FVector>(parent + Off::RelativeLocation);
        pos.X += p.X; pos.Y += p.Y; pos.Z += p.Z;
        parent = g_Mem.Read<uintptr_t>(parent + Off::AttachParent);
        depth++;
    }
    return pos;
}

float Distance(FVector a, FVector b) {
    float dx = a.X - b.X, dy = a.Y - b.Y, dz = a.Z - b.Z;
    return sqrtf(dx * dx + dy * dy + dz * dz) / 100.0f;
}

ENPCCategory GetNPCCategory(ENPCType type) {
    switch (type) {
    case NPC_Gooseberry: case NPC_Coyle: case NPC_Franco: case NPC_Otto: case NPC_Liliya:
        return NPCCAT_Boss;
    case NPC_Ambient: case NPC_Scientist: case NPC_ThrowableTarget: case NPC_StalkerTarget:
    case NPC_StageDefaultPrimeAsset: case NPC_StageNonDefaultPrimeAsset:
        return NPCCAT_Passive;
    case NPC_Spectre: case NPC_Imposter: case NPC_NightHunter:
        return NPCCAT_Special;
    default:
        return NPCCAT_Dangerous;
    }
}

const char* GetNPCName(ENPCType type) {
    switch (type) {
    case NPC_Grunt: return "Grunt";
    case NPC_Ambient: return "Ambient";
    case NPC_BigGrunt: return "BigGrunt";
    case NPC_SleeperScreamer: return "Sleeper";
    case NPC_Pouncer: return "Pouncer";
    case NPC_Pusher: return "Pusher";
    case NPC_GroundPitcher: return "Pitcher";
    case NPC_Jaeger: return "Jaeger";
    case NPC_Berserker: return "Berserker";
    case NPC_Imposter: return "IMPOSTER";
    case NPC_NightHunter: return "NightHunter";
    case NPC_Spectre: return "SPECTRE";
    case NPC_Gooseberry: return "GOOSEBERRY";
    case NPC_Coyle: return "COYLE";
    case NPC_Franco: return "FRANCO";
    case NPC_Otto: return "OTTO";
    case NPC_Liliya: return "LILIYA";
    case NPC_Scientist: return "Scientist";
    case NPC_Guard: return "Guard";
    default: return "NPC";
    }
}

struct ItemInfo { const char* Name; EDisplayType DispType; bool IsImportant; };

ItemInfo GetItemInfo(uint8_t type) {
    switch (type) {
    case IT_HealthGain: return { "HP", DISP_Health, false };
    case IT_HealthBoost: return { "HP+", DISP_Health, false };
    case IT_TempHealthGain: return { "TempHP", DISP_Health, false };
    case IT_ReviveSyringe: return { "Syringe", DISP_Health, true };
    case IT_RespawnConsumable: return { "Respawn", DISP_Health, true };
    case IT_Bandage: return { "Bandage", DISP_Health, false };
    case IT_HealthSicknessPill: return { "HPill", DISP_Health, false };
    case IT_PsychosisAntidote: return { "Antidote", DISP_Antidote, false };
    case IT_StaminaRegen: return { "Stam", DISP_Stamina, false };
    case IT_MaxStaminaBoost: return { "Stam+", DISP_Stamina, false };
    case IT_StaminaSicknessPill: return { "SPill", DISP_Stamina, false };
    case IT_SmallBattery: return { "SmBat", DISP_Battery, false };
    case IT_Battery: return { "Batt", DISP_Battery, false };
    case IT_SuperBattery: return { "SBat", DISP_Battery, true };
    case IT_LockPick: return { "Pick", DISP_Key, false };
    case IT_MasterKey: return { "MKey", DISP_Key, true };
    case IT_Bottle: return { "Bottle", DISP_Throwable, false };
    case IT_Brick: return { "Brick", DISP_Throwable, false };
    case IT_GoreThrowable: return { "Gore", DISP_Throwable, false };
    case IT_AIAttractorItem: return { "Lure", DISP_Throwable, false };
    case IT_SkillCharge: return { "Charge", DISP_Skill, false };
    case IT_TimeExtender: return { "Time+", DISP_Special, true };
    case IT_Decoder: return { "Decoder", DISP_Key, true };
    case IT_ProximityDetector: return { "Prox", DISP_Special, false };
    case IT_MotionDetection: return { "Motion", DISP_Special, false };
    case IT_NoiseSuppression: return { "Quiet", DISP_Special, false };
    case IT_BearTrap: return { "TRAP!", DISP_Trap, true };
    case IT_ImposterTrap: return { "I-TRAP!", DISP_Trap, true };
    case IT_Quest: return { "QUEST", DISP_Quest, true };
    case IT_ObjectiveThrowable: return { "Q-Throw", DISP_Quest, true };
    case IT_ObjectiveEquipable: return { "Q-Equip", DISP_Quest, true };
    case IT_ObjectiveCollectable: return { "Q-Item", DISP_Quest, true };
    case IT_ActiveSkillController: return { "Skill", DISP_Skill, false };
    case IT_ActiveSkillThrowable: return { "SkillT", DISP_Skill, false };
    case IT_ActiveSkillDeployable: return { "SkillD", DISP_Skill, false };
    case IT_ActiveSkillUsable: return { "SkillU", DISP_Skill, false };
    case IT_ActiveSkillAutoEquipable: return { "SkillE", DISP_Skill, false };
    case IT_CollectibleDocument: return { "Doc", DISP_Document, true };
    case IT_SpecialCollectibleDocument: return { "SECRET", DISP_Secret, true };
    case IT_BlacklightDetectableItem: return { "BLight", DISP_Secret, true };
    case IT_ChainingUpgradePoint: return { "Upgrade", DISP_Special, true };
    case IT_AdvWeapon: return { "Weapon", DISP_Special, true };
    case IT_ImposterStunThrowable: return { "Stun", DISP_Throwable, false };
    case IT_ImposterFakeItem: return { "FAKE!", DISP_Trap, true };
    default: return { "Item", DISP_Unknown, false };
    }
}

bool ValidateNPC(uintptr_t actor, FVector actorPos, FVector& playerPos) {
    uint8_t bDead = g_Mem.Read<uint8_t>(actor + PawnOff::bDead);
    if (bDead != 0) return false;
    
    uint8_t inLimbo = g_Mem.Read<uint8_t>(actor + NpcOff::bIsInLimbo);
    if (inLimbo != 0) return false;
    
    uint8_t npcType = g_Mem.Read<uint8_t>(actor + NpcOff::NPCType);
    if (npcType >= NPC_MAX) return false;
    
    int32_t health = g_Mem.Read<int32_t>(actor + PawnOff::Health);
    if (health <= 0 || health > 5000) return false;
    
    FVector charLoc = g_Mem.Read<FVector>(actor + PawnOff::CharLocation);
    if (!ValidPosition(charLoc)) return false;
    
    float charLocDist = Distance(actorPos, charLoc);
    if (charLocDist > 10.0f) return false;
    
    float distToPlayer = Distance(actorPos, playerPos);
    if (distToPlayer < 0.3f) return false;
    
    uintptr_t meshHolder = g_Mem.Read<uintptr_t>(actor + PawnOff::MeshHolder);
    if (meshHolder != 0 && !HeapPtr(meshHolder)) return false;
    
    return true;
}

bool ScanAll() {
    g_Items.clear();
    g_Enemies.clear();

    uintptr_t base = g_Mem.Base();
    uintptr_t world = g_Mem.Read<uintptr_t>(base + Off::GWorld);
    if (!ValidPtr(world)) return false;

    uintptr_t gi = g_Mem.Read<uintptr_t>(world + Off::GameInstance);
    if (!ValidPtr(gi)) return false;
    uintptr_t lpData = g_Mem.Read<uintptr_t>(gi + Off::LocalPlayers);
    if (!ValidPtr(lpData)) return false;
    uintptr_t player = g_Mem.Read<uintptr_t>(lpData);
    if (!ValidPtr(player)) return false;
    uintptr_t controller = g_Mem.Read<uintptr_t>(player + Off::PlayerController);
    if (!ValidPtr(controller)) return false;
    g_CamMgr = g_Mem.Read<uintptr_t>(controller + Off::PlayerCameraManager);
    if (!ValidPtr(g_CamMgr)) return false;

    CameraData cam;
    g_Mem.ReadRaw(g_CamMgr + Off::CameraCache, &cam, sizeof(cam));
    FVector playerPos = cam.Loc;

    TArray<uintptr_t> levels = g_Mem.Read<TArray<uintptr_t>>(world + Off::Levels);
    if (levels.Count < 1 || levels.Count > 100) return false;

    std::vector<uintptr_t> lvlPtrs(levels.Count);
    g_Mem.ReadRaw(levels.Data, lvlPtrs.data(), levels.Count * 8);

    for (uintptr_t lvl : lvlPtrs) {
        if (!ValidPtr(lvl)) continue;
        TArray<uintptr_t> actors = g_Mem.Read<TArray<uintptr_t>>(lvl + Off::ActorArray);
        if (!ValidPtr(actors.Data) || actors.Count < 1 || actors.Count > 50000) continue;

        std::vector<uintptr_t> actorList(actors.Count);
        g_Mem.ReadRaw(actors.Data, actorList.data(), actors.Count * 8);

        for (uintptr_t actor : actorList) {
            if (!ValidPtr(actor)) continue;
            uintptr_t root = g_Mem.Read<uintptr_t>(actor + Off::RootComponent);
            if (!ValidPtr(root)) continue;
            FVector pos = GetWorldPosition(root);
            if (!ValidPosition(pos)) continue;
            float distToPlayer = Distance(pos, playerPos);
            if (distToPlayer > 150) continue;

            uintptr_t iz = g_Mem.Read<uintptr_t>(actor + Off::Pickup_InteractionZone);
            if (HeapPtr(iz)) {
                uint8_t type = g_Mem.Read<uint8_t>(actor + Off::Pickup_ItemType);
                uint8_t category = g_Mem.Read<uint8_t>(actor + Off::Pickup_ItemCategory);
                if (type >= 2 && type <= 45 && category <= 7) {
                    bool inHand = g_Mem.Read<bool>(actor + Off::Pickup_bIsInHand);
                    uintptr_t owner = g_Mem.Read<uintptr_t>(actor + Off::Pickup_PawnOwner);
                    if (!inHand && !ValidPtr(owner)) {
                        ItemInfo info = GetItemInfo(type);
                        if (info.Name) {
                            g_Items.push_back({ actor, root, info.Name, info.DispType, info.IsImportant });
                        }
                    }
                }
                continue;
            }

            if (ValidateNPC(actor, pos, playerPos)) {
                uint8_t npcType = g_Mem.Read<uint8_t>(actor + NpcOff::NPCType);
                ENPCType type = (ENPCType)npcType;
                ENPCCategory cat = GetNPCCategory(type);
                const char* name = GetNPCName(type);
                g_Enemies.push_back({ actor, root, name, type, cat });
            }
        }
    }

    printf("[+] Items: %zu, NPCs: %zu\n", g_Items.size(), g_Enemies.size());
    return g_Items.size() > 0 || g_Enemies.size() > 0;
}

void AutoScan() {
    printf("[*] Auto-scanning...\n");
    int attempts = 0;
    while (attempts < 30) {
        if (ScanAll()) { printf("[+] Done!\n"); return; }
        attempts++;
        Sleep(300);
        if (GetAsyncKeyState(VK_ESCAPE) & 1) { printf("[!] Cancelled\n"); return; }
    }
    printf("[!] Timeout\n");
}

std::vector<ScreenEntity> GetVisible(int W, int H) {
    std::vector<ScreenEntity> result;
    if (!ValidPtr(g_CamMgr)) return result;

    CameraData cam;
    g_Mem.ReadRaw(g_CamMgr + Off::CameraCache, &cam, sizeof(cam));
    if (cam.FOV < 1 || cam.FOV > 180) cam.FOV = 90;
    FVector playerPos = cam.Loc;

    float pr = cam.Rot.Pitch * DEG2RAD, yr = cam.Rot.Yaw * DEG2RAD, rr = cam.Rot.Roll * DEG2RAD;
    float sp = sinf(pr), cp = cosf(pr), sy = sinf(yr), cy = cosf(yr), sr = sinf(rr), cr = cosf(rr);
    FVector fwd = { cp * cy, cp * sy, sp };
    FVector right = { sr * sp * cy - cr * sy, sr * sp * sy + cr * cy, -sr * cp };
    FVector up = { -(cr * sp * cy + sr * sy), cy * sr - cr * sp * sy, cr * cp };
    float tanFov = tanf(cam.FOV * 0.5f * DEG2RAD);
    float aspect = (float)W / H;

    auto project = [&](FVector pos, float& sx, float& sy2) -> bool { //прототип от нейросети ( не забыть переписать, и удалить комментарий )
        float dx = pos.X - cam.Loc.X, dy = pos.Y - cam.Loc.Y, dz = pos.Z - cam.Loc.Z;
        float depth = dx * fwd.X + dy * fwd.Y + dz * fwd.Z;
        if (depth < 1) return false;
        float x = dx * right.X + dy * right.Y + dz * right.Z;
        float y = dx * up.X + dy * up.Y + dz * up.Z;
        sx = W * 0.5f + (x / (depth * tanFov)) * (W * 0.5f);
        sy2 = H * 0.5f - (y / (depth * tanFov / aspect)) * (H * 0.5f);
        return sx >= 0 && sx <= W && sy2 >= 0 && sy2 <= H;
    };

    if (g_ItemESPEnabled) {
        for (auto& item : g_Items) {
            bool inHand = g_Mem.Read<bool>(item.Actor + Off::Pickup_bIsInHand);
            if (inHand) continue;
            uintptr_t owner = g_Mem.Read<uintptr_t>(item.Actor + Off::Pickup_PawnOwner);
            if (ValidPtr(owner)) continue;
            FVector pos = GetWorldPosition(item.Root);
            float dist = Distance(pos, playerPos);
            float maxDist = item.IsImportant ? g_ESPDistance * 1.5f : g_ESPDistance;
            if (dist > maxDist) continue;
            float sx, sy2;
            if (project(pos, sx, sy2)) {
                result.push_back({ sx, sy2, dist, item.Name, item.DispType, item.IsImportant, false, 0, NPCCAT_Dangerous });
            }
        }
    }

    if (g_EnemyESPEnabled) {
        for (auto& enemy : g_Enemies) {
            uint8_t bDead = g_Mem.Read<uint8_t>(enemy.Actor + PawnOff::bDead);
            if (bDead != 0) continue;
            uint8_t inLimbo = g_Mem.Read<uint8_t>(enemy.Actor + NpcOff::bIsInLimbo);
            if (inLimbo != 0) continue;
            uint8_t awareness = g_Mem.Read<uint8_t>(enemy.Actor + NpcOff::BotAwarenessState);
            FVector pos = GetWorldPosition(enemy.Root);
            float dist = Distance(pos, playerPos);
            float maxDist = (enemy.Category == NPCCAT_Boss) ? g_ESPDistance * 2.0f : g_ESPDistance * 1.5f;
            if (dist > maxDist) continue;
            float sx, sy2;
            if (project(pos, sx, sy2)) {
                EDisplayType disp;
                switch (enemy.Category) {
                case NPCCAT_Boss: disp = DISP_EnemyBoss; break;
                case NPCCAT_Passive: disp = DISP_EnemyPassive; break;
                case NPCCAT_Special: disp = DISP_EnemySpecial; break;
                default: disp = DISP_Enemy; break;
                }
                if (awareness >= 3) disp = DISP_EnemyChase;
                else if (awareness >= 2) disp = DISP_EnemyAlert;
                result.push_back({ sx, sy2, dist, enemy.Name, disp, true, true, awareness, enemy.Category });
            }
        }
    }
    return result;
}

HWND g_Hwnd = nullptr, g_Target = nullptr;
ID2D1Factory* g_Factory = nullptr;
ID2D1HwndRenderTarget* g_RT = nullptr;
ID2D1SolidColorBrush* g_Brush = nullptr;
IDWriteFactory* g_WF = nullptr;
IDWriteTextFormat* g_TF = nullptr;
IDWriteTextFormat* g_TFBig = nullptr;
int g_W = 1920, g_H = 1080;
RECT g_LastRect = {};

LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    return m == WM_DESTROY ? (PostQuitMessage(0), 0) : DefWindowProcW(h, m, w, l);
}

bool InitOverlay() { 
    for (auto name : { L"The Outlast Trials  ", L"The Outlast Trials", L"UnrealWindow" }) {
        g_Target = FindWindowW(nullptr, name);
        if (g_Target) break;
    }
    if (!g_Target) return false;

    RECT r; GetClientRect(g_Target, &r); g_W = r.right; g_H = r.bottom;
    GetWindowRect(g_Target, &r);
    g_LastRect = r;

    WNDCLASSEXW wc = { sizeof(wc), 0, WndProc, 0, 0, GetModuleHandleW(0), 0, 0, 0, 0, L"E", 0 };
    RegisterClassExW(&wc);

    g_Hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        L"E", 0, WS_POPUP, r.left, r.top, g_W, g_H, 0, 0, wc.hInstance, 0);

    SetLayeredWindowAttributes(g_Hwnd, 0, 0, LWA_COLORKEY);
    MARGINS m = { -1 }; DwmExtendFrameIntoClientArea(g_Hwnd, &m);
    ShowWindow(g_Hwnd, SW_SHOWNOACTIVATE);

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &g_Factory);
    D2D1_HWND_RENDER_TARGET_PROPERTIES hp = D2D1::HwndRenderTargetProperties(g_Hwnd, D2D1::SizeU(g_W, g_H));
    hp.presentOptions = D2D1_PRESENT_OPTIONS_IMMEDIATELY;
    g_Factory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(), hp, &g_RT);
    g_RT->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    g_RT->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_ALIASED);
    g_RT->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1), &g_Brush);

    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&g_WF);
    g_WF->CreateTextFormat(L"Consolas", 0, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 11, L"", &g_TF);
    g_WF->CreateTextFormat(L"Consolas", 0, DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 14, L"", &g_TFBig);

    return true;
}

D2D1_COLOR_F GetColor(EDisplayType type) {
    switch (type) {
    case DISP_Health: return D2D1::ColorF(0.2f, 1.0f, 0.4f);
    case DISP_Stamina: return D2D1::ColorF(0.3f, 0.7f, 1.0f);
    case DISP_Battery: return D2D1::ColorF(1.0f, 1.0f, 0.0f);
    case DISP_Key: return D2D1::ColorF(0.0f, 1.0f, 1.0f);
    case DISP_Throwable: return D2D1::ColorF(1.0f, 0.5f, 0.0f);
    case DISP_Trap: return D2D1::ColorF(1.0f, 0.0f, 0.0f);
    case DISP_Quest: return D2D1::ColorF(1.0f, 0.0f, 1.0f);
    case DISP_Skill: return D2D1::ColorF(1.0f, 0.85f, 0.0f);
    case DISP_Document: return D2D1::ColorF(1.0f, 1.0f, 1.0f);
    case DISP_Secret: return D2D1::ColorF(0.7f, 0.3f, 1.0f);
    case DISP_Special: return D2D1::ColorF(0.8f, 0.8f, 0.95f);
    case DISP_Antidote: return D2D1::ColorF(1.0f, 0.4f, 0.8f);
    case DISP_Enemy: return D2D1::ColorF(1.0f, 0.3f, 0.3f);
    case DISP_EnemyBoss: return D2D1::ColorF(0.7f, 0.0f, 0.3f);
    case DISP_EnemyPassive: return D2D1::ColorF(1.0f, 0.9f, 0.2f);
    case DISP_EnemySpecial: return D2D1::ColorF(0.8f, 0.2f, 1.0f);
    case DISP_EnemyAlert: return D2D1::ColorF(1.0f, 0.6f, 0.0f);
    case DISP_EnemyChase: return D2D1::ColorF(1.0f, 0.0f, 0.0f);
    default: return D2D1::ColorF(0.6f, 0.6f, 0.6f);
    }
}

void Render(const std::vector<ScreenEntity>& entities) {
    RECT r; GetWindowRect(g_Target, &r);
    if (r.left != g_LastRect.left || r.top != g_LastRect.top) {
        SetWindowPos(g_Hwnd, HWND_TOPMOST, r.left, r.top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        g_LastRect = r;
    }

    g_RT->BeginDraw();
    g_RT->Clear();

    wchar_t status[200];
    swprintf(status, 200, L"[F1]Items:%ls [F2]NPC:%ls [F5/F6]%.0fm | I:%zu N:%zu",
        g_ItemESPEnabled ? L"ON" : L"OFF",
        g_EnemyESPEnabled ? L"ON" : L"OFF",
        g_ESPDistance, g_Items.size(), g_Enemies.size());

    g_Brush->SetColor(D2D1::ColorF(0, 0, 0, 0.7f));
    g_RT->FillRectangle(D2D1::RectF(5, 5, 420, 25), g_Brush);
    g_Brush->SetColor(D2D1::ColorF(1, 1, 1));
    g_RT->DrawText(status, (UINT32)wcslen(status), g_TF, D2D1::RectF(10, 7, 420, 25), g_Brush);

    int textCount = 0;

    for (auto& e : entities) {
        if (e.IsImportant || e.IsEnemy) continue;
        D2D1_COLOR_F col = GetColor(e.DispType);
        float sz = 3.0f + (1.0f - e.Dist / g_ESPDistance) * 3.0f;
        g_Brush->SetColor(D2D1::ColorF(0, 0, 0));
        g_RT->FillEllipse(D2D1::Ellipse(D2D1::Point2F(e.X, e.Y), sz + 1.5f, sz + 1.5f), g_Brush);
        g_Brush->SetColor(col);
        g_RT->FillEllipse(D2D1::Ellipse(D2D1::Point2F(e.X, e.Y), sz, sz), g_Brush);
        if (e.Dist < 25 && textCount < 35) {
            wchar_t buf[24]; swprintf(buf, 24, L"%hs %.0fm", e.Name, e.Dist);
            g_Brush->SetColor(D2D1::ColorF(0, 0, 0));
            g_RT->DrawText(buf, (UINT32)wcslen(buf), g_TF, D2D1::RectF(e.X + sz + 4, e.Y - 6, e.X + 110, e.Y + 14), g_Brush);
            g_Brush->SetColor(col);
            g_RT->DrawText(buf, (UINT32)wcslen(buf), g_TF, D2D1::RectF(e.X + sz + 3, e.Y - 7, e.X + 110, e.Y + 13), g_Brush);
            textCount++;
        }
    }

    for (auto& e : entities) {
        if (!e.IsImportant || e.IsEnemy) continue;
        D2D1_COLOR_F col = GetColor(e.DispType);
        float sz = 5.0f + (1.0f - e.Dist / (g_ESPDistance * 1.5f)) * 5.0f;
        g_Brush->SetColor(D2D1::ColorF(0, 0, 0));
        g_RT->FillEllipse(D2D1::Ellipse(D2D1::Point2F(e.X, e.Y), sz + 3.5f, sz + 3.5f), g_Brush);
        g_Brush->SetColor(D2D1::ColorF(1, 1, 1));
        g_RT->FillEllipse(D2D1::Ellipse(D2D1::Point2F(e.X, e.Y), sz + 2.0f, sz + 2.0f), g_Brush);
        g_Brush->SetColor(col);
        g_RT->FillEllipse(D2D1::Ellipse(D2D1::Point2F(e.X, e.Y), sz, sz), g_Brush);
        wchar_t buf[32];
        if (e.DispType == DISP_Trap) swprintf(buf, 32, L"[!] %hs %.0fm", e.Name, e.Dist);
        else if (e.DispType == DISP_Quest) swprintf(buf, 32, L">>> %hs %.0fm", e.Name, e.Dist);
        else swprintf(buf, 32, L"* %hs %.0fm", e.Name, e.Dist);
        g_Brush->SetColor(D2D1::ColorF(0, 0, 0));
        g_RT->DrawText(buf, (UINT32)wcslen(buf), g_TFBig, D2D1::RectF(e.X + sz + 6, e.Y - 8, e.X + 160, e.Y + 18), g_Brush);
        g_Brush->SetColor(col);
        g_RT->DrawText(buf, (UINT32)wcslen(buf), g_TFBig, D2D1::RectF(e.X + sz + 5, e.Y - 9, e.X + 160, e.Y + 17), g_Brush);
    }

    for (auto& e : entities) {
        if (!e.IsEnemy) continue;
        D2D1_COLOR_F col = GetColor(e.DispType);
        float sz = 8.0f + (1.0f - e.Dist / (g_ESPDistance * 1.5f)) * 6.0f;
        g_Brush->SetColor(D2D1::ColorF(0, 0, 0));
        g_RT->FillRectangle(D2D1::RectF(e.X - sz - 2, e.Y - sz - 2, e.X + sz + 2, e.Y + sz + 2), g_Brush);
        g_Brush->SetColor(D2D1::ColorF(1, 1, 1));
        g_RT->FillRectangle(D2D1::RectF(e.X - sz - 1, e.Y - sz - 1, e.X + sz + 1, e.Y + sz + 1), g_Brush);
        g_Brush->SetColor(col);
        g_RT->FillRectangle(D2D1::RectF(e.X - sz, e.Y - sz, e.X + sz, e.Y + sz), g_Brush);
        const wchar_t* stateStr = L"";
        if (e.AwarenessState >= 3) stateStr = L" CHASE!";
        else if (e.AwarenessState >= 2) stateStr = L" ALERT";
        wchar_t buf[48]; swprintf(buf, 48, L"%hs%ls %.0fm", e.Name, stateStr, e.Dist);
        g_Brush->SetColor(D2D1::ColorF(0, 0, 0));
        g_RT->DrawText(buf, (UINT32)wcslen(buf), g_TFBig, D2D1::RectF(e.X + sz + 6, e.Y - 8, e.X + 200, e.Y + 18), g_Brush);
        g_Brush->SetColor(col);
        g_RT->DrawText(buf, (UINT32)wcslen(buf), g_TFBig, D2D1::RectF(e.X + sz + 5, e.Y - 9, e.X + 200, e.Y + 17), g_Brush);
    }

    g_RT->EndDraw();
}

int main() {
    SetConsoleTitleW(L"Outlast Trials ESP");
    printf("==============================================\n");
    printf("         OUTLAST TRIALS ESP \n"); 
    printf("==============================================\n\n");

    printf("CONTROLS:\n");
    printf("  F1 = Toggle Items ESP\n");
    printf("  F2 = Toggle NPC ESP\n");
    printf("  F5/F6 = Distance -/+\n");
    printf("  F9 = Scan\n");
    printf("  END = Exit\n\n");

    if (!g_Mem.Initialize()) { printf("[!] Driver failed\n"); return 1; }
    printf("[+] Driver OK\n");

    printf("[*] Waiting for game...\n");
    while (!g_Mem.Attach(L"TOTClient-Win64-Shipping.exe")) Sleep(500);
    printf("[+] Attached\n");

    if (!InitOverlay()) { printf("[!] Overlay failed\n"); return 1; }
    printf("[+] Overlay ready\n\n");

    printf(">>> Press F9 to scan! <<<\n\n");

    while (IsWindow(g_Target)) {
        MSG msg; while (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) DispatchMessageW(&msg);

        if (GetAsyncKeyState(VK_END) & 1) break;
        if (GetAsyncKeyState(VK_F1) & 1) {
            g_ItemESPEnabled = !g_ItemESPEnabled;
            printf("[*] Items: %s\n", g_ItemESPEnabled ? "ON" : "OFF");
        }
        if (GetAsyncKeyState(VK_F2) & 1) {
            g_EnemyESPEnabled = !g_EnemyESPEnabled;
            printf("[*] NPC: %s\n", g_EnemyESPEnabled ? "ON" : "OFF");
        }
        if (GetAsyncKeyState(VK_F5) & 1 && g_ESPDistance > 10) {
            g_ESPDistance -= 5;
            printf("[*] Dist: %.0fm\n", g_ESPDistance);
        }
        if (GetAsyncKeyState(VK_F6) & 1 && g_ESPDistance < 100) {
            g_ESPDistance += 5;
            printf("[*] Dist: %.0fm\n", g_ESPDistance);
        }
        if (GetAsyncKeyState(VK_F9) & 1) { printf("\n"); AutoScan(); printf("\n"); }

        Render(GetVisible(g_W, g_H));

        Sleep(16);
    }

    printf("\n[*] Goodbye!\n");
    return 0;
}
