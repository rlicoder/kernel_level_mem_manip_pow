#include "overlay.h"

extern bool aim;
extern bool esp;
extern bool item_glow;
extern bool player_glow;
extern bool aim_no_recoil;
extern bool ready;
extern bool use_nvidia;
extern int safe_level;
extern int spectators;
extern int allied_spectators;
extern float max_dist;
extern float smooth;
extern int numSpec;
extern int current_cfg;
extern float max_fov;
extern int bone;
extern bool thirdperson;
int width;
int height;
bool k_leftclick = false;
bool k_ins = false;
bool show_menu = false;

typedef struct specname
{
	char name[33] = { 0 };
}specname;
extern specname specnames[100];

visuals v;

enum cfg { ESP, SMOOTH, BONE };
enum BONES { BODY = 2, HEAD = 8 };

extern bool IsKeyDown(int vk);

LONG nv_default = WS_POPUP | WS_CLIPSIBLINGS;
LONG nv_default_in_game = nv_default | WS_DISABLED;
LONG nv_edit = nv_default_in_game | WS_VISIBLE;

LONG nv_ex_default = WS_EX_TOOLWINDOW;
LONG nv_ex_edit = nv_ex_default | WS_EX_LAYERED | WS_EX_TRANSPARENT;
LONG nv_ex_edit_menu = nv_ex_default | WS_EX_TRANSPARENT;

static DWORD WINAPI StaticMessageStart(void* Param)
{
	Overlay* ov = (Overlay*)Param;
	ov->CreateOverlay();
	return 0;
}

BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam)
{
	wchar_t className[255] = L"";
	GetClassName(hwnd, className, 255);
	if (use_nvidia)
	{
		if (wcscmp(XorStrW(L"CEF-OSC-WIDGET"), className) == 0) //Nvidia overlay
		{
			HWND* w = (HWND*)lParam;
			if (GetWindowLong(hwnd, GWL_STYLE) != nv_default && GetWindowLong(hwnd, GWL_STYLE) != nv_default_in_game)
				return TRUE;
			*w = hwnd;
			return TRUE;
		}
	}
	else
	{
		if (wcscmp(XorStrW(L"overlay"), className) == 0) //Custom overlay
		{
			HWND* w = (HWND*)lParam;
			*w = hwnd;
			return TRUE;
		}
	}
	return TRUE;
}

// Data
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();

void Overlay::RenderInfo()
{
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(150, 120 + (20 * numSpec)));
	ImGui::Begin(XorStr("##info"), (bool*)true, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
	switch (safe_level)
	{
	case 0:
		DrawLine(ImVec2(9, 5), ImVec2(17, 5), RED, 2);
		break;
	case 1:
		DrawLine(ImVec2(9, 5), ImVec2(17, 5), ORANGE, 2);
		break;
	case 2:
		DrawLine(ImVec2(9, 5), ImVec2(17, 5), GREEN, 2);
		break;
	default:
		break;
	}
	std::string curEdit;
	std::string curVal;
	switch (current_cfg)
	{
		case BONE:
			curEdit = "BONE";
			curVal = std::to_string(bone);
			break;
		case ESP:
			curEdit = "ESP";
			curVal = std::to_string(max_dist / 39.62);
			break;
		case SMOOTH:
			curEdit = "SMOOTH";
			curVal = std::to_string(smooth);
			break;
	}
	
	ImGui::TextColored((aim ? GREEN : RED), "%s\n", "AIM");
	ImGui::TextColored((esp ? GREEN : RED), "%s\n\n", "ESP");

	ImGui::TextColored(YELLOW, "%s\n", curEdit);
	ImGui::TextColored(WHITE, "%s\n\n", curVal);

	ImGui::TextColored((numSpec == 0 ? GREEN : RED), "%s: %d\n", "SPECTATORS", numSpec);
	for (int i = 0; i < numSpec; i++)
	{
		String(ImVec2(10, 120 + (20 * i)), WHITE, specnames[i].name);
    }

	ImGui::SameLine();
	ImGui::End();
}

void Overlay::ClickThrough(bool v)
{
	if (v)
	{
		nv_edit = nv_default_in_game | WS_VISIBLE;
		if (GetWindowLong(overlayHWND, GWL_EXSTYLE) != nv_ex_edit)
			SetWindowLong(overlayHWND, GWL_EXSTYLE, nv_ex_edit);
	}
	else
	{
		nv_edit = nv_default | WS_VISIBLE;
		if (GetWindowLong(overlayHWND, GWL_EXSTYLE) != nv_ex_edit_menu)
			SetWindowLong(overlayHWND, GWL_EXSTYLE, nv_ex_edit_menu);
	}
}

DWORD Overlay::CreateOverlay()
{
	EnumWindows(EnumWindowsCallback, (LPARAM)&overlayHWND);
	Sleep(300);
	if (overlayHWND == 0)
	{
		printf(XorStr("Can't find the overlay\n"));
		Sleep(1000);
		exit(0);
	}

	HDC hDC = ::GetWindowDC(NULL);
	width = ::GetDeviceCaps(hDC, HORZRES);
	height = ::GetDeviceCaps(hDC, VERTRES);
		
	running = true;

	// Initialize Direct3D
	if (!CreateDeviceD3D(overlayHWND))
	{
		CleanupDeviceD3D();
		return 1;
	}

	// Show the window
	::ShowWindow(overlayHWND, SW_SHOWDEFAULT);
	::UpdateWindow(overlayHWND);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplWin32_Init(overlayHWND);
	ImGui_ImplDX9_Init(g_pd3dDevice);

	ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);

	// Main loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	ClickThrough(true);
	while (running)
	{
		HWND wnd = GetWindow(GetForegroundWindow(), GW_HWNDPREV);
		if (use_nvidia)
		{
			if (GetWindowLong(overlayHWND, GWL_STYLE) != nv_edit)
				SetWindowLong(overlayHWND, GWL_STYLE, nv_edit);
			if (show_menu)
			{
				ClickThrough(false);
			}
			else
			{
				if (GetWindowLong(overlayHWND, GWL_EXSTYLE) != nv_ex_edit)
					SetWindowLong(overlayHWND, GWL_EXSTYLE, nv_ex_edit);
				ClickThrough(true);
			}
		}
		if (wnd != overlayHWND)
		{
			SetWindowPos(overlayHWND, wnd, 0, 0, 0, 0, SWP_ASYNCWINDOWPOS | SWP_NOMOVE | SWP_NOSIZE);
			::UpdateWindow(overlayHWND);
		}

		if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			continue;
		}

		// Start the Dear ImGui frame
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		if (IsKeyDown(VK_LBUTTON) && !k_leftclick)
		{
			io.MouseDown[0] = true;
			k_leftclick = true;
		}
		else if (!IsKeyDown(VK_LBUTTON) && k_leftclick)
		{
			io.MouseDown[0] = false;
			k_leftclick = false;
		}

		if (IsKeyDown(VK_INSERT) && !k_ins && ready)
		{
			show_menu = !show_menu;
			ClickThrough(!show_menu);
			k_ins = true;
		}
		else if (!IsKeyDown(VK_INSERT) && k_ins)
		{	
			k_ins = false;
		}
		
		RenderInfo();
		RenderEsp();
		// Rendering
		ImGui::EndFrame();
		g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * 255.0f), (int)(clear_color.y * 255.0f), (int)(clear_color.z * 255.0f), (int)(clear_color.w * 255.0f));
		g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
		if (g_pd3dDevice->BeginScene() >= 0)
		{
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			g_pd3dDevice->EndScene();
		}
		HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

		// Handle loss of D3D9 device
		if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
			ResetDevice();

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ClickThrough(true);

	CleanupDeviceD3D();
	::DestroyWindow(overlayHWND);
	return 0;
}

void Overlay::Start()
{
	DWORD ThreadID;
	CreateThread(NULL, 0, StaticMessageStart, (void*)this, 0, &ThreadID);
}

void Overlay::Clear()
{
	running = 0;
	Sleep(50);
	if (use_nvidia)
	{
		SetWindowLong(overlayHWND, GWL_STYLE, nv_default);
		SetWindowLong(overlayHWND, GWL_EXSTYLE, nv_ex_default);
	}
}

int Overlay::getWidth()
{
	return width;
}

int Overlay::getHeight()
{
	return height;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
	if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return false;

	// Create the D3DDevice
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	g_d3dpp.hDeviceWindow = hWnd;
	g_d3dpp.MultiSampleQuality = D3DMULTISAMPLE_NONE;
	g_d3dpp.BackBufferCount = 1;
	g_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
	g_d3dpp.BackBufferWidth = 0;
	g_d3dpp.BackBufferHeight = 0;
	if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
		return false;

	return true;
}

void CleanupDeviceD3D()
{
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
	if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
	if (hr == D3DERR_INVALIDCALL)
		IM_ASSERT(0);
	ImGui_ImplDX9_CreateDeviceObjects();
}

void Overlay::DrawLine(ImVec2 a, ImVec2 b, ImColor color, float width)
{
	ImGui::GetWindowDrawList()->AddLine(a, b, color, width);
}

void Overlay::DrawBox(ImColor color, float x, float y, float w, float h)
{
	DrawLine(ImVec2(x, y), ImVec2(x + w, y), color, 1.0f);
	DrawLine(ImVec2(x, y), ImVec2(x, y + h), color, 1.0f);
	DrawLine(ImVec2(x + w, y), ImVec2(x + w, y + h), color, 1.0f);
	DrawLine(ImVec2(x, y + h), ImVec2(x + w, y + h), color, 1.0f);
}

void Overlay::Text(ImVec2 pos, ImColor color, const char* text_begin, const char* text_end, float wrap_width, const ImVec4* cpu_fine_clip_rect)
{
	ImGui::GetWindowDrawList()->AddText(ImGui::GetFont(), ImGui::GetFontSize(), pos, color, text_begin, text_end, wrap_width, cpu_fine_clip_rect);
}

void Overlay::String(ImVec2 pos, ImColor color, const char* text)
{
	Text(pos, color, text, text + strlen(text), 200, 0);
}

void Overlay::RectFilled(float x0, float y0, float x1, float y1, ImColor color, float rounding, int rounding_corners_flags)
{
	ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), color, rounding, rounding_corners_flags);
}

void Overlay::ProgressBar(float x, float y, float w, float h, int value, int v_max)
{
	ImColor barColor = ImColor(
		min(510 * (v_max - value) / 100, 255),
		min(510 * value / 100, 255),
		25,
		255
	);
	
	RectFilled(x, y, x + w, y + ((h / float(v_max)) * (float)value), barColor, 0.0f, 0);
}
