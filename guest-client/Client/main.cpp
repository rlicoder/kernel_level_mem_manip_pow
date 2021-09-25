#include "main.h"
#include <map>

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

//put your defaults here
//the first number in max dist is the # of meters you want to see ahead
//the 39.62 multiplier is the units to meters conversion
float max_dist = 200.0f * 39.62f; //read
float smooth = 50.0f;
float max_fov = 25.0f;
int bone = 2;

enum cfg {ESP, SMOOTH, BONE};
enum BONES {BODY = 2, HEAD = 8};
bool headbone = false;
int current_cfg = ESP;
int aim_key = VK_XBUTTON1;
bool use_nvidia = true;
bool active = true;
bool ready = false;
extern visuals v;
int spectators = 1; //write
bool aim = false; //read
bool esp = false; //read
int safe_level = 0; //read
bool item_glow = false;
bool player_glow = false;
bool aim_no_recoil = true;
bool aiming = false; //read
uint64_t g_Base = 0; //write
const float max_smooth = 175;
bool thirdperson = false;
typedef struct specname
{
	char name[33] = { 0 };
}specname;
specname specnames[100];
int numSpec = 0;

bool valid = false; //write
bool next = false; //read write

uint64_t add[16];

bool k_f5 = 0;
bool k_f6 = 0;
bool k_f7 = 0;

bool IsKeyDown(int vk)
{
	return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

player players[100];

void Overlay::RenderEsp()
{
	next = false;
	if (g_Base != 0 && esp)
	{
		memset(players, 0, sizeof(players));
		while (!next && esp)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		if (next && valid)
		{
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(ImVec2((float)getWidth(), (float)getHeight()));
			ImGui::Begin(XorStr("##esp"), (bool*)true, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);

			for (int i = 0; i < 100; i++)
			{
				if (players[i].health > 0)
				{
					std::string distance = std::to_string(players[i].dist / 39.62);
					distance = distance.substr(0, distance.find('.')) + "m(" + std::to_string(players[i].entity_team) + ")";
					if (v.box)
					{
						if (players[i].visible)
						{
							if (players[i].dist < 1600.0f)
								DrawBox(RED, players[i].boxMiddle, players[i].h_y, players[i].width, players[i].height); //BOX
							else
								DrawBox(ORANGE, players[i].boxMiddle, players[i].h_y, players[i].width, players[i].height); //BOX
						}
						else
						{
							DrawBox(WHITE, players[i].boxMiddle, players[i].h_y, players[i].width, players[i].height); //white if player not visible
						}
					}

					if (v.distance)
					{
						if (players[i].knocked)
							String(ImVec2(players[i].boxMiddle, (players[i].b_y + 1)), RED, distance.c_str());  //DISTANCE
						else
							String(ImVec2(players[i].boxMiddle, (players[i].b_y + 1)), GREEN, distance.c_str());  //DISTANCE
					}

					if(v.healthbar)
						ProgressBar((players[i].b_x - (players[i].width / 2.0f) - 4), (players[i].b_y - players[i].height), 3, players[i].height, players[i].health, 100); //health bar
					if (v.shieldbar)
						ProgressBar((players[i].b_x + (players[i].width / 2.0f) + 1), (players[i].b_y - players[i].height), 3, players[i].height, players[i].shield, 125); //shield bar

					if(v.name)
						String(ImVec2(players[i].boxMiddle, (players[i].b_y - players[i].height - 15)), WHITE, players[i].name);
				}
			}
			
			ImGui::End();
		}
	}
}

int main(int argc, char** argv)
{
	add[0] = (uintptr_t)&spectators;
	add[1] = (uintptr_t)&aim;
	add[2] = (uintptr_t)&esp;
	add[3] = (uintptr_t)&safe_level;
	add[4] = (uintptr_t)&aiming;
	add[5] = (uintptr_t)&g_Base;
	add[6] = (uintptr_t)&next;
	add[7] = (uintptr_t)&players[0];
	add[8] = (uintptr_t)&valid;
	add[9] = (uintptr_t)&max_dist;
	add[10] = (uintptr_t)&aim_no_recoil;
	add[11] = (uintptr_t)&smooth;
	add[12] = (uintptr_t)&max_fov;
	add[13] = (uintptr_t)&bone;
	add[14] = (uintptr_t)&specnames[0];
	add[15] = (uintptr_t)&numSpec;
	printf(XorStr("add offset: 0x%I64x\n"), (uint64_t)&add[0] - (uint64_t)GetModuleHandle(NULL));
	Overlay ov1 = Overlay();
	ov1.Start();
	printf(XorStr("Waiting for host process...\n"));
	while (spectators == 1)
	{
		if (IsKeyDown(VK_F4))
		{
			active = false;
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	if (active)
	{
		ready = true;
		printf(XorStr("Ready\n"));
	}
		
	while (active)
	{
		if (IsKeyDown(VK_UP))
		{
			if (current_cfg != ESP)
			{
				current_cfg--;
			}
			
			std::this_thread::sleep_for(std::chrono::milliseconds(135));
		}
		if (IsKeyDown(VK_DOWN))
		{
			if (current_cfg != BONE)
			{
				current_cfg++;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(135));
		}
		if (IsKeyDown(VK_F4))
		{
			active = false;
		}

		if (IsKeyDown(VK_F5) && k_f5 == 0)
		{
			k_f5 = 1;
			esp = !esp;
		}
		else if (!IsKeyDown(VK_F5) && k_f5 == 1)
		{
			k_f5 = 0;
		}

		if (IsKeyDown(VK_F6) && k_f6 == 0)
		{
			k_f6 = 1;
			aim = !aim;
		}
		else if (!IsKeyDown(VK_F6) && k_f6 == 1)
		{
			k_f6 = 0;
		}

		if (IsKeyDown(VK_LEFT))
		{
			switch (current_cfg)
			{
				case ESP:
					if (max_dist > 100.0f * 40.0f)
						max_dist -= 50.0f * 40.0f;
					break;
				case SMOOTH:
					if (smooth > 25)
						smooth -= 25;
					break;
				case BONE:
				{
					if (bone > 0)
					{
						bone--;
					}
					break;
				}
			}
			
			std::this_thread::sleep_for(std::chrono::milliseconds(135));
		}

		if (IsKeyDown(VK_RIGHT))
		{
			
			switch (current_cfg)
			{
				case ESP:
					if (max_dist < 800.0f * 40.0f)
						max_dist += 50.0f * 40.0f;
					break;
				case SMOOTH:
					if (smooth < max_smooth)
						smooth += 25;
					break;
				case BONE:
				{
					if (bone < 10)
					{
						bone++;
					}
					break;
				}
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(135));
		}

		if (IsKeyDown(aim_key))
			aiming = true;
		else
			aiming = false;
	}
	ready = false;
	ov1.Clear();
	if(!use_nvidia)
		system(XorStr("taskkill /F /T /IM overlay_ap.exe")); //custom overlay process name
	return 0;
}
