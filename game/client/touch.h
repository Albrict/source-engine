#include "utllinkedlist.h"
#include "vgui/VGUI.h"
#include <vgui_controls/Panel.h>
#include "cbase.h"
#include "kbutton.h"
#include "usercmd.h"

extern ConVar touch_enable;
extern "C" int getAssets();

#define GRID_COUNT touch_grid_count.GetInt()
#define GRID_COUNT_X (GRID_COUNT)
#define GRID_COUNT_Y (GRID_COUNT * screen_h / screen_w)
#define GRID_X (1.0/GRID_COUNT_X)
#define GRID_Y (screen_w/screen_h/GRID_COUNT_X)
#define GRID_ROUND_X(x) ((float)round( x * GRID_COUNT_X ) / GRID_COUNT_X)
#define GRID_ROUND_Y(x) ((float)round( x * GRID_COUNT_Y ) / GRID_COUNT_Y)

#define CMD_SIZE 64

#define TOUCH_FL_HIDE                   (1U << 0)
#define TOUCH_FL_NOEDIT                 (1U << 1)
#define TOUCH_FL_CLIENT                 (1U << 2)
#define TOUCH_FL_MP                             (1U << 3)
#define TOUCH_FL_SP                             (1U << 4)
#define TOUCH_FL_DEF_SHOW               (1U << 5)
#define TOUCH_FL_DEF_HIDE               (1U << 6)
#define TOUCH_FL_DRAW_ADDITIVE  (1U << 7)
#define TOUCH_FL_STROKE                 (1U << 8)
#define TOUCH_FL_PRECISION              (1U << 9)

enum ETouchButtonType
{
	touch_command = 0, // Tap button
	touch_move,    // Like a joystick stick.
	touch_joy,     // Like a joystick stick, centered.
	touch_dpad,    // Only two directions.
	touch_look,     // Like a touchpad.
	touch_key
};

enum ETouchState
{
	state_none = 0,
	state_edit,
	state_edit_move
};

enum ETouchRound
{
	round_none = 0,
	round_grid,
	round_aspect
};

struct rgba_t
{
	rgba_t(	unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255 ) : r( r ), g( g ), b( b ), a( a ) { }
	rgba_t() : r( 0 ), g( 0 ), b( 0 ), a( 0 ) { }
	rgba_t( unsigned char *x ) : r( x[0] ), g( x[1] ), b( x[2] ), a( x[3] ) { }

	operator unsigned char*() { return &r; }

	unsigned char r, g, b, a; 
};

struct event_clientcmd_t
{
	char buf[CMD_SIZE];
};

struct event_s
{
	int type;
	int x;
	int y;
	int fingerid;
} typedef touch_event_t;


class CTouchButton
{
public:
	// Touch button type: tap, stick or slider
	ETouchButtonType type;

	// Field of button in pixels
	float x1, y1, x2, y2;

	// Button texture
	int texture;

	rgba_t color;
	char texturefile[256];
	char command[256];
	char name[32];
	int finger;
	int flags;
	float fade;
	float fadespeed;
	float fadeend;
	float aspect;
	int textureID;
};

class CTouchPanel : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CTouchPanel, vgui::Panel );

public:
	CTouchPanel( vgui::VPANEL parent );
	virtual			~CTouchPanel( void ) {};

	virtual void	Paint();
};

abstract_class ITouchPanel
{
public:
	virtual void		Create( vgui::VPANEL parent ) = 0;
	virtual void		Destroy( void ) = 0;
};

class VTouchPanel : public ITouchPanel
{
private:
	CTouchPanel *touchPanel;
public:
	VTouchPanel( void )
	{
		touchPanel = NULL;
	}

	void Create( vgui::VPANEL parent )
	{
		touchPanel = new CTouchPanel( parent );
	}

	void Destroy( void )
	{
		if ( touchPanel )
		{
			touchPanel->SetParent( (vgui::Panel *)NULL );
			touchPanel->MarkForDeletion();
			touchPanel = NULL;
		}
	}
};


class CTouchControls
{
public:
	void Init( );
	void Shutdown( );

	void Paint( );
	void Frame( );
	
	void AddButton( const char *name, const char *texturefile, const char *command, float x1, float y1, float x2, float y2, rgba_t color = rgba_t(255, 255, 255, 255), int round = 2, float aspect = 1, int flags = 0 );
	void RemoveButton( const char *name );
	
	void HideButton( const char *name );
	void ShowButton( const char *name );
	
	
	CTouchButton *FindButton( const char *name );
//	bool FindNextButton( const char *name, CTouchButton &button );
	void SetTexture( const char *name, const char *file );
	void SetColor( const char *name, rgba_t color );
	void SetCommand( const char *name, const char *cmd );
	void SetFlags( const char *name, int flags );

	
	void IN_CheckCoords( float *x1, float *y1, float *x2, float *y2  );
	
	
	void Move( float frametime, CUserCmd *cmd );
	void IN_Look( );

	void ProcessEvent( touch_event_t *ev );
	void FingerPress( touch_event_t *ev );
	void FingerMotion( touch_event_t *ev );

	void EnableTouchEdit();
	
	CTouchPanel *touchPanel;
private:
	bool initialized;
	ETouchState state;
	CUtlLinkedList<CTouchButton*> btns;

	int look_finger, move_finger, wheel_finger;
	float forward, side, movecount;
	float yaw, pitch;
	CTouchButton *move_button;

	float move_start_x, move_start_y;
	float dx, dy;

	// editing
	CTouchButton *edit;
	CTouchButton *selection;
	int resize_finger;
	bool showbuttons;
	bool clientonly;
	rgba_t scolor;
	int swidth;
	bool precision;
	// textures
	int showtexture;
	int hidetexture;
	int resettexture;
	int closetexture;
	int joytexture; // touch indicator
	bool configchanged;
	vgui::HFont textfont;
	int mouse_events;

	int base_textureID;

	bool m_bHaveAssets;
	int screen_h, screen_w;
};

extern CTouchControls gTouch;
extern VTouchPanel *touch_panel;
