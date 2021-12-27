#include <stdio.h>
#include <set>
#include <string>
#include <map>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <random>
#include <chrono>
#include <iostream>
#include <cfloat>
#include "Game.h"
#include <thread>

Memory apex_mem;
Memory client_mem;

const int client_screen_width = 1920;
const int client_screen_height = 1080;
bool debug = false;

bool firing_range = false;
bool active = true;
uintptr_t aimentity = 0;
uintptr_t tmp_aimentity = 0;
uintptr_t lastaimentity = 0;
float max = 999.0f;
float max_dist = 200.0f*40.0f;
int team_player = 0;
float max_fov = 30;
const int toRead = 100;
bool aim = false;
bool esp = false;
extern bool aim_no_recoil;
int safe_level = 0;
bool aiming = false;
float smooth;
int bone;

bool actions_t = false;
bool esp_t = false;
bool aim_t = false;
bool vars_t = false;
uint64_t g_Base;
uint64_t c_Base;
bool next = false;
bool valid = false;
bool lock = false;

uint64_t speclist_addr = 0;
uint64_t numSpec_addr = 0;

typedef struct player
{
    float dist = 0;
    int entity_team = 0;
    float boxMiddle = 0;
    float h_y = 0;
    float width = 0;
    float height = 0;
    float b_x = 0;
    float b_y = 0;
    bool knocked = false;
    bool visible = false;
    int health = 0;
    int shield = 0;
    char name[33] = { 0 };
}player;

typedef struct specname
{
    char name[33] = { 0 };
}spacname;

struct Matrix
{
    float matrix[16];
};

float lastvis_esp[toRead];
float lastvis_aim[toRead];

//////////////////////////////////////////////////////////////////////////////////////////////////

void ProcessPlayer(Entity& LPlayer, Entity& target, uint64_t entitylist, int index)
{
    int entity_team = target.getTeamId();
    Vector EntityPosition = target.getPosition();
    Vector LocalPlayerPosition = LPlayer.getPosition();
    float dist = LocalPlayerPosition.DistTo(EntityPosition);
    if (dist > max_dist) return;

    if (!target.isAlive())
        return;

    if(!firing_range)
        if (entity_team < 0 || entity_team>50 || entity_team == team_player) return;

    float fov = CalculateFov(LPlayer, target);
    if (fov < max)
    {
        max = fov;
        tmp_aimentity = target.ptr;
    }
}

void DoActions()
{
    actions_t = true;
    while (actions_t)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        while (g_Base!=0 && c_Base!=0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));	
            uint64_t LocalPlayer = 0;
            apex_mem.Read<uint64_t>(g_Base + OFFSET_LOCAL_ENT, LocalPlayer);

            if (LocalPlayer == 0) 
            {
                //if (debug)
                    //std::cout << "failed localplayer = 0 in doactions" << std::endl;
                continue;
            }

            Entity LPlayer = getEntity(LocalPlayer);

            team_player = LPlayer.getTeamId();
            if (team_player < 0 || team_player>50)
            {
                //if (debug)
                    //std::cout << "failed team player check in doactions" << std::endl;
                continue;
            }
            uint64_t entitylist = g_Base + OFFSET_ENTITYLIST;

            uint64_t baseent = 0;
            apex_mem.Read<uint64_t>(entitylist, baseent);
            if (baseent == 0)
            {
                //if (debug)
                    //std::cout << "failed baseent check in doactions" << std::endl;
                continue;
            }

            max = 999.0f;
            tmp_aimentity = 0;
            if(firing_range)
            {
                int c=0;
                for (int i = 0; i < 10000; i++)
                {
                    uint64_t centity = 0;
                    apex_mem.Read<uint64_t>(entitylist + ((uint64_t)i << 5), centity);
                    if (centity == 0) continue;
                    if (LocalPlayer == centity) continue;

                    Entity Target = getEntity(centity);
                    if (!Target.isDummy())
                    {
                        continue;
                    }

                    ProcessPlayer(LPlayer, Target, entitylist, c);
                    c++;
                }
            }
            else
            {
                for (int i = 0; i < toRead; i++)
                {
                    uint64_t centity = 0;
                    apex_mem.Read<uint64_t>(entitylist + ((uint64_t)i << 5), centity);
                    if (centity == 0) 
                    {
                        //if (debug)
                            //std::cout << "failed centity = 0 check in doactions" << std::endl;
                        continue;
                    }
                    if (LocalPlayer == centity) 
                    {
                        //if (debug)
                            //std::cout << "failed localplayer == centity check in doactions" << std::endl;
                        continue;
                    }
                    Entity Target = getEntity(centity);
                    if (!Target.isPlayer())
                    {
                        //if (debug)
                            //std::cout << "failed player check in doactions" << std::endl;
                        continue;
                    }

                    ProcessPlayer(LPlayer, Target, entitylist, i);

                    int entity_team = Target.getTeamId();
                    if (entity_team == team_player)
                    {
                        //if (debug)
                            //std::cout << "failed team mutual check in doactions" << std::endl;
                        continue;
                    }

                }
            }
            if(!lock)
                aimentity = tmp_aimentity;
            else
                aimentity = lastaimentity;
        }
    }
    actions_t = false;
}

// /////////////////////////////////////////////////////////////////////////////////////////////////////

player players[toRead];

static void EspLoop()
{
    esp_t = true;
    while(esp_t)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        while(g_Base!=0 && c_Base!=0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (esp)
            {
                valid = false;

                uint64_t LocalPlayer = 0;
                apex_mem.Read<uint64_t>(g_Base + OFFSET_LOCAL_ENT, LocalPlayer);
                if (LocalPlayer == 0)
                {
                    next = true;
                    while(next && g_Base!=0 && c_Base!=0 && esp)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                    continue;
                }
                Entity LPlayer = getEntity(LocalPlayer);
                int team_player = LPlayer.getTeamId();
                if (team_player < 0 || team_player>50)
                {
                    next = true;
                    while(next && g_Base!=0 && c_Base!=0 && esp)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                    continue;
                }
                Vector LocalPlayerPosition = LPlayer.getPosition();

                uint64_t viewRenderer = 0;
                apex_mem.Read<uint64_t>(g_Base + OFFSET_RENDER, viewRenderer);
                uint64_t viewMatrix = 0;
                apex_mem.Read<uint64_t>(viewRenderer + OFFSET_MATRIX, viewMatrix);
                Matrix m = {};
                apex_mem.Read<Matrix>(viewMatrix, m);

                uint64_t entitylist = g_Base + OFFSET_ENTITYLIST;

                memset(players,0,sizeof(players));
                if(firing_range)
                {
                    int c=0;
                    for (int i = 0; i < 10000; i++)
                    {
                        uint64_t centity = 0;
                        apex_mem.Read<uint64_t>( entitylist + ((uint64_t)i << 5), centity);
                        if (centity == 0 || LocalPlayer == centity)
                        {
                            continue;
                        }		

                        Entity Target = getEntity(centity);

                        if (!Target.isDummy() || !Target.isPlayer() || !Target.isAlive())
                        {
                            continue;
                        }
                        int entity_team = Target.getTeamId();

                        Vector EntityPosition = Target.getPosition();
                        float dist = LocalPlayerPosition.DistTo(EntityPosition);
                        if (dist > max_dist || dist < 50.0f)
                        {	
                            continue;
                        }

                        Vector bs = Vector();
                        WorldToScreen(EntityPosition, m.matrix, client_screen_width, client_screen_height, bs);
                        if (bs.x > 0 && bs.y > 0)
                        {
                            Vector hs = Vector();
                            Vector HeadPosition = Target.getBonePosition(8);
                            WorldToScreen(HeadPosition, m.matrix, client_screen_width, client_screen_height, hs);
                            float height = abs(abs(hs.y) - abs(bs.y));
                            float width = height / 2.0f;
                            float boxMiddle = bs.x - (width / 2.0f);
                            int health = Target.getHealth();
                            int shield = Target.getShield();
                            players[c] = 
                            {
                                dist,
                                entity_team,
                                boxMiddle,
                                hs.y,
                                width,
                                height,
                                bs.x,
                                bs.y,
                                0,
                                (Target.lastVisTime() > lastvis_esp[c]),
                                health,
                                shield	
                            };
                            Target.get_name(g_Base, i-1, &players[c].name[0]);
                            lastvis_esp[c] = Target.lastVisTime();
                            c++;
                            valid = true;
                        }
                    }
                }	
                else
                {
                    for (int i = 0; i < toRead; i++)
                    {
                        uint64_t centity = 0;
                        apex_mem.Read<uint64_t>( entitylist + ((uint64_t)i << 5), centity);
                        if (centity == 0 || LocalPlayer == centity)
                        {
                            continue;
                        }

                        Entity Target = getEntity(centity);

                        if (!Target.isPlayer() || !Target.isAlive())
                        {
                            continue;
                        }

                        int entity_team = Target.getTeamId();
                        if (entity_team < 0 || entity_team>50 || entity_team == team_player)
                        {
                            continue;
                        }

                        Vector EntityPosition = Target.getPosition();
                        float dist = LocalPlayerPosition.DistTo(EntityPosition);
                        if (dist > max_dist || dist < 50.0f)
                        {	
                            continue;
                        }

                        Vector bs = Vector();
                        WorldToScreen(EntityPosition, m.matrix, client_screen_width, client_screen_height, bs);
                        if (bs.x > 0 && bs.y > 0)
                        {
                            Vector hs = Vector();
                            Vector HeadPosition = Target.getBonePosition(8);
                            WorldToScreen(HeadPosition, m.matrix, client_screen_width, client_screen_height, hs);
                            float height = abs(abs(hs.y) - abs(bs.y));
                            float width = height / 2.0f;
                            float boxMiddle = bs.x - (width / 2.0f);
                            int health = Target.getHealth();
                            int shield = Target.getShield();

                            players[i] = 
                            {
                                dist,
                                entity_team,
                                boxMiddle,
                                hs.y,
                                width,
                                height,
                                bs.x,
                                bs.y,
                                Target.isKnocked(),
                                (Target.lastVisTime() > lastvis_esp[i]),
                                health,
                                shield
                            };
                            Target.get_name(g_Base, i-1, &players[i].name[0]);
                            lastvis_esp[i] = Target.lastVisTime();
                            valid = true;
                        }
                    }
                }

                next = true;
                while(next && g_Base!=0 && c_Base!=0 && esp)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        }
    }
    esp_t = false;
}

specname speclist[100];

static void SpecList()
{
    while (true)
    {
        std::map<int,bool> teamalive;
        uint64_t entitylist = g_Base + OFFSET_ENTITYLIST;
        std::set<std::string> s;
        int cur = 0;
        for (int i = 0; i < toRead; i++)
        {
            uint64_t centity = 0;
            apex_mem.Read<uint64_t>(entitylist + ((uint64_t)i << 5), centity);
            if (centity == 0) continue;
            Entity target = getEntity(centity);
            if (!target.isPlayer()) continue;
            bool alive = target.isAlive();
            int teamid = target.getTeamId();
            teamalive[teamid] |= alive;
        }
        int c = 0;
        for (int i = 0; i < toRead; i++)
        {
            uint64_t centity = 0;
            apex_mem.Read<uint64_t>(entitylist + ((uint64_t)i << 5), centity);
            if (centity == 0) continue;
            Entity target = getEntity(centity);
            if (!target.isPlayer()) continue;
            bool alive = target.isAlive();
            int teamid = target.getTeamId();
            char name[33];
            if (!alive && !teamalive[teamid])
            {
                target.get_name(g_Base, i-1, &speclist[c++].name[0]);
                cur++;
            }
        }
        client_mem.Write<int>(numSpec_addr, cur);
        client_mem.WriteArray<specname>(speclist_addr, speclist, cur);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

static void AimbotLoop()
{
    aim_t = true;
    while (aim_t)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        while (g_Base!=0 && c_Base!=0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (aim>0)
            {
                if (aimentity == 0 || !aiming)
                {
                    //if (debug)
                        //std::cout << "failed aimentity or aiming check in aimbotloop" << std::endl;
                    lock=false;
                    lastaimentity=0;
                    continue;
                }
                lock=true;
                lastaimentity = aimentity;
                uint64_t LocalPlayer = 0;
                apex_mem.Read<uint64_t>(g_Base + OFFSET_LOCAL_ENT, LocalPlayer);
                if (LocalPlayer == 0)
                {
                    //if (debug)
                        //std::cout << "failed local player read in aim" << std::endl;
                    continue;
                }

                Entity LPlayer = getEntity(LocalPlayer);
                QAngle Angles = CalculateBestBoneAim(LPlayer, aimentity, max_fov, smooth, bone);
                if (Angles.x == 0 && Angles.y == 0)
                {
                    //if (debug)
                        //std::cout << "failed angle check" << std::endl;
                    lock=false;
                    lastaimentity=0;
                    continue;
                }
                float current_time;
                float lastvis = getEntity(aimentity).lastVisTime();
                apex_mem.Read<float>(g_Base + OFFSET_GLOBAL_VARS + 0x10, current_time);
                bool visible = lastvis > 0.0f && fabsf(lastvis - current_time) < 0.07f;
                if (!visible)
                {
                    //if (debug)
                        //std::cout << "failed visibility check in aimbotloop" << std::endl;
                    continue;
                }
                LPlayer.SetViewAngles(Angles);
                //if (debug)
                    //std::cout << Angles.x << " " << Angles.y << std::endl;
            }
        }
    }
    aim_t = false;
}

static void set_vars(uint64_t add_addr)
{
    printf("Reading client vars...\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    //Get addresses of client vars
    uint64_t spec_addr = 0;
    client_mem.Read<uint64_t>(add_addr, spec_addr);
    uint64_t aim_addr = 0;
    client_mem.Read<uint64_t>(add_addr + sizeof(uint64_t)*1, aim_addr);
    uint64_t esp_addr = 0;
    client_mem.Read<uint64_t>(add_addr + sizeof(uint64_t)*2, esp_addr);
    uint64_t safe_lev_addr = 0;
    client_mem.Read<uint64_t>(add_addr + sizeof(uint64_t)*3, safe_lev_addr);
    uint64_t aiming_addr = 0;
    client_mem.Read<uint64_t>(add_addr + sizeof(uint64_t)*4, aiming_addr);
    uint64_t g_Base_addr = 0;
    client_mem.Read<uint64_t>(add_addr + sizeof(uint64_t)*5, g_Base_addr);
    uint64_t next_addr = 0;
    client_mem.Read<uint64_t>(add_addr + sizeof(uint64_t)*6, next_addr);
    uint64_t player_addr = 0;
    client_mem.Read<uint64_t>(add_addr + sizeof(uint64_t)*7, player_addr);
    uint64_t valid_addr = 0;
    client_mem.Read<uint64_t>(add_addr + sizeof(uint64_t)*8, valid_addr);
    uint64_t max_dist_addr = 0;
    client_mem.Read<uint64_t>(add_addr + sizeof(uint64_t)*9, max_dist_addr);
    uint64_t aim_no_recoil_addr = 0;
    client_mem.Read<uint64_t>(add_addr + sizeof(uint64_t)*10, aim_no_recoil_addr);
    uint64_t smooth_addr = 0;
    client_mem.Read<uint64_t>(add_addr + sizeof(uint64_t)*11, smooth_addr);
    uint64_t max_fov_addr = 0;
    client_mem.Read<uint64_t>(add_addr + sizeof(uint64_t)*12, max_fov_addr);
    uint64_t bone_addr = 0;
    client_mem.Read<uint64_t>(add_addr + sizeof(uint64_t)*13, bone_addr);
    client_mem.Read<uint64_t>(add_addr + sizeof(uint64_t)*14, speclist_addr);
    client_mem.Read<uint64_t>(add_addr + sizeof(uint64_t)*15, numSpec_addr);

    int tmp = 0;
    client_mem.Read<int>(spec_addr, tmp);

    if(tmp != 1)
    {
        printf("Incorrect values read. Check if the add_off is correct. Quitting.\n");
        active = false;
        return;
    }
    vars_t = true;
    while(vars_t)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if(c_Base!=0 && g_Base!=0)
            printf("\nReady\n");

        int temp = 0;
        while(c_Base!=0 && g_Base!=0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            client_mem.Write<uint64_t>(g_Base_addr, g_Base);
            //lmao...
            uint64_t a = 0;
            client_mem.Write<uint64_t>(spec_addr, a);

            client_mem.Read<bool>(aim_addr, aim);
            client_mem.Read<bool>(esp_addr, esp);
            client_mem.Read<int>(safe_lev_addr, safe_level);
            client_mem.Read<bool>(aiming_addr, aiming);
            client_mem.Read<float>(max_dist_addr, max_dist);
            client_mem.Read<bool>(aim_no_recoil_addr, aim_no_recoil);
            client_mem.Read<float>(smooth_addr, smooth);
            //client_mem.Read<float>(max_fov_addr, max_fov);

            client_mem.Read<int>(bone_addr, bone);

            if(esp && next)
            {
                if(valid)
                    client_mem.WriteArray<player>(player_addr, players, toRead);
                client_mem.Write<bool>(valid_addr, valid);
                client_mem.Write<bool>(next_addr, true); //next

                bool next_val = false;
                do
                {
                    client_mem.Read<bool>(next_addr, next_val);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                } while (next_val && g_Base!=0 && c_Base!=0);

                next = false;
            }
        }
    }
    vars_t = false;
}

int main(int argc, char *argv[])
{
    if(geteuid() != 0)
    {
        printf("Error: %s is not running as root\n", argv[0]);
        return 0;
    }

    const char* cl_proc = "client_ap.exe";
    const char* ap_proc = "R5Apex.exe";
    //const char* ap_proc = "EasyAntiCheat_launcher.exe";

    //Client "add" offset
    uint64_t add_off = 0x2e880;

    std::thread aimbot_thr;
    std::thread esp_thr;
    std::thread actions_thr;
    std::thread vars_thr;
    std::thread spec_thr;
    while(active)
    {
        if(apex_mem.get_proc_status() != process_status::FOUND_READY)
        {
            if(aim_t)
            {
                aim_t = false;
                esp_t = false;
                actions_t = false;
                g_Base = 0;

                aimbot_thr.~thread();
                esp_thr.~thread();
                actions_thr.~thread();
                spec_thr.~thread();
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
            printf("Searching for apex process...\n");

            apex_mem.open_proc(ap_proc);

            if(apex_mem.get_proc_status() == process_status::FOUND_READY)
            {
                g_Base = apex_mem.get_proc_baseaddr();
                printf("\nApex process found\n");
                printf("Base: %lx\n", g_Base);

                aimbot_thr = std::thread(AimbotLoop);
                esp_thr = std::thread(EspLoop);
                actions_thr = std::thread(DoActions);
                spec_thr = std::thread(SpecList);
                aimbot_thr.detach();
                esp_thr.detach();
                actions_thr.detach();
                spec_thr.detach();
            }
        }
        else
        {
            apex_mem.check_proc();
        }

        if(client_mem.get_proc_status() != process_status::FOUND_READY)
        {
            if(vars_t)
            {
                vars_t = false;
                c_Base = 0;

                vars_thr.~thread();
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
            printf("Searching for client process...\n");

            client_mem.open_proc(cl_proc);

            if(client_mem.get_proc_status() == process_status::FOUND_READY)
            {
                c_Base = client_mem.get_proc_baseaddr();
                printf("\nClient process found\n");
                printf("Base: %lx\n", c_Base);

                vars_thr = std::thread(set_vars, c_Base + add_off);
                vars_thr.detach();
            }
        }
        else
        {
            client_mem.check_proc();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
