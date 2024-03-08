#define ORIGIN 1
#define STEAM 2
#define VERSION STEAM
#if VERSION == STEAM
#define OFFSET_ENTITYLIST		0x1db8548
#define OFFSET_GLOBAL_VARS		0x16db270
#define OFFSET_RENDER		0x73e0ad8
#define OFFSET_MATRIX		0x11a350
#define OFFSET_NAME_LIST    0xC5E1150
#define OFFSET_VISIBLE_TIME		0x1970
#define OFFSET_CAMERAPOS		0x1eb0
#define OFFSET_TEAM		0x0328
#define OFFSET_HEALTH		0x0318
#define OFFSET_SHIELD		0x01a0
#define OFFSET_NAME		0x0471
#define OFFSET_SIGN_NAME		0x0468
#define OFFSET_ZOOMING		0x1bb1
#define OFFSET_LIFE_STATE		0x0680
#define OFFSET_BLEED_OUT_STATE		0x26c0
#define OFFSET_FORCE_BONE		0x0d80
#define OFFSET_AMMO_POOL_CAPACITY		0x2514
#define OFFSET_WEAPON		0x1914
#define OFFSET_PLAYER_DATA		0x15b0
#define OFFSET_CURRENT_ZOOM		0x00b8
#define OFFSET_ABS_VELOCITY		0x0170
#define OFFSET_AIMPUNCH		0x2418
#define OFFSET_ORIGIN		0x017c
#define OFFSET_BONES		0x0d80 + 0x48
#define OFFSET_VIEWANGLES		0x2514 - 0x14
#define OFFSET_BREATH_ANGLES		OFFSET_VIEWANGLES - 0x10
#define OFFSET_ZOOM_FOV		OFFSET_PLAYER_DATA + OFFSET_CURRENT_ZOOM
#define OFFSET_LOCAL_ENT		0x2166fa8
//#define OFFSET_BULLET_SPEED		OFFSET_VISIBLE_TIME + 0x4cc
#define OFFSET_BULLET_SPEED		0x1e7c
//#define OFFSET_BULLET_SCALE		OFFSET_VISIBLE_TIME + 0x4d4
#define OFFSET_BULLET_SCALE		0x1e84
#endif
