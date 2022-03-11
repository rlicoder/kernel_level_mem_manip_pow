#define ORIGIN 1
#define STEAM 2

#define VERSION STEAM

#if VERSION == STEAM

    #define OFFSET_GLOBAL_VARS		    0x1309840 //globalvars
    #define OFFSET_ENTITYLIST			0x1974ad8 //cl_entitylist
    #define OFFSET_LOCAL_ENT			0x1d243d8 //LocalPlayer
    #define OFFSET_NAME_LIST          	0xb9315d0 //namelist
    #define OFFSET_RENDER				0x7431238 //viewrender
    #define OFFSET_CAMERAPOS			0x1f20 //Cplayer camera_origin
    #define OFFSET_MATRIX				0x10e8d0 //viewmatrix

    #define OFFSET_TEAM					0x448 //m_iTeamNum
    #define OFFSET_HEALTH				0x438 //m_iHealth
    #define OFFSET_SHIELD				0x170 //m_shieldHealth
    #define OFFSET_NAME					0x589 //m_iName
    #define OFFSET_SIGN_NAME			0x580 //m_iSignifierName

    #define OFFSET_ZOOMING      		0x1c31 //m_bZooming

    #define OFFSET_LIFE_STATE			0x0798//m_lifeState, >0 = dead
    #define OFFSET_BLEED_OUT_STATE		0x2708 //m_bleedoutState, >0 = knocked

    #define OFFSET_BONES				0x0f38 //m_bConstrainBetweenEndpoints

    
    #define OFFSET_VIEWANGLES			0x257c - 0x14 //m_ammoPoolCapacity - 0x14
    #define OFFSET_BREATH_ANGLES		OFFSET_VIEWANGLES - 0x10

    

    #define OFFSET_WEAPON				0x1a8c //m_latestPrimaryWeapons
    #define OFFSET_ZOOM_FOV             0x1708 + 0xb8 //m_playerData + m_curZoomFOV

    //use casualx dumper https://casualhacks.net/apexstuff/apexdumper.html
    
    #define OFFSET_ABS_VELOCITY         0x140 //m_vecAbsVelocity not found yet
    #define OFFSET_VISIBLE_TIME         0x1af4 //CPlayer!lastVisibleTime
    #define OFFSET_BULLET_SPEED         0x1f18 //CWeaponX!m_flProjectileSpeed
    #define OFFSET_BULLET_SCALE         0x1f20//CWeaponX!m_flProappjectileScale
    #define OFFSET_AIMPUNCH				0x2480 //m_currentFrameLocalPlayer.m_vecPunchWeapon_Angle
    //make sure to use CBaseViewModel
    #define OFFSET_ORIGIN				0x014c //m_vecAbsOrigin

#endif
