// Curdle for Atari ST and FreeGEM
// Copyright 2022 Seg <seg@haxxed.com>

#include "slimgem/gem.h"
#include <assert.h>

// skirt RSC endian issues with different names
#include "CURDLEST.H"
#ifdef __m68k__
static const char app_name[]="  CurdleST ";
#define CURDLE_RSC "CURDLEST.RSC"
#define CURDLE_SAV "CURDLEST.SAV"
#else
static const char app_name[]="  Curdled ";
#define CURDLE_RSC "CURDLED.RSC"
#define CURDLE_SAV "CURDLED.SAV"
#endif

static int16_t gl_wbox,gl_hbox; // UI cell sizes

static char secret_curd[5]="KINKS";
#include "curdle.c"

// Window stuff

static OBJECT* curdle_ui;
static OBJECT*	curdle_status;
static OBJECT* curd_box;
static OBJECT* curd_grid;
static OBJECT* curdle_kb;
static   GRECT desk,work;  // desktop and work areas
static int16_t wi_handle; // window handle
static int16_t fulled;   // current full state of window

static short guess_num;
static short guess_col;

#define CURD_LEN 5
#define GUESS_MAX 6

enum {WAITING,PLAYING,WON,LOST,STATS} ui_state = WAITING;

// walk the dirty rectangle list and redraw
static void window_do_redraw(GRECT* dirty){
	graf_mouse(M_OFF);
	wind_update(true);
	GRECT rect;
	wind_get_first(wi_handle,&rect);
	while (rect.w && rect.h) {
	  if (rc_intersect(dirty,&rect)) {
		 objc_draw(curdle_ui,0,8,&rect);
	  }
	  wind_get_next(wi_handle,&rect);
	}
	//if(win) explode();
	wind_update(false);
	graf_mouse(M_ON);
}

// send ourself a redraw message
SHL void window_redraw(GRECT* gr){
	int16_t msg[8];
	msg[0]=WM_REDRAW;
	msg[1]=gl_apid;
	//msg[2]=0;
	msg[3]=wi_handle;
	gr_copy(gr,&msg[4]);
	appl_write(gl_apid,16,&msg);
}

// need to byte align window on FreeGEM or else it will misalign text
static void window_align(GRECT* gr){
#ifdef _M_I86
	gr->x+=3;
	gr->x&=~7;
	gr->x+=3;
#endif
}

#define WI_KIND (NAME|CLOSER|MOVER)
#define NO_WINDOW (-1)

static void window_open(void){
	GRECT w;
	wind_calc(WC_BORDER,WI_KIND,&curdle_ui->gr,&w);
	w.x=desk.x+desk.w/2-w.w/2;
	w.y=desk.y+desk.h/2-w.h/2;
	window_align(&w);

	wi_handle=wind_create(WI_KIND,&w);
	wind_set_name(wi_handle,app_name+1);

	GRECT start;
	start.x=desk.x+desk.w/2;
	start.y=desk.y+desk.h/2;
	start.w=gl_wbox;
	start.h=gl_hbox;
	graf_growbox(&start,&w);

	wind_open(wi_handle,&w);
	wind_get_work(wi_handle,&work);
	curdle_ui->gr.x=work.x;
	curdle_ui->gr.y=work.y;
}

static void window_close(void){
	wind_close(wi_handle);
	GRECT end;
	end.x=work.x+work.w/2;
	end.y=work.y+work.h/2;
	end.w=gl_wbox;
	end.h=gl_hbox;
	graf_shrinkbox(&end,&work);
	wind_delete(wi_handle);
	wi_handle=NO_WINDOW;
	v_clsvwk();
}

uint16_t boxchar_state[4];
uint32_t boxchar_spec[4];

static void boxchar_set_gamestate(OBJECT* ob,int state){
	assert(ob->type==G_BOXCHAR);
	char c=((uint32_t)ob->spec>>24);
	ob->state=boxchar_state[state];
	ob->spec=(void*)(boxchar_spec[state]|(uint32_t)c<<24);
}

static char boxchar_get_char(OBJECT* ob){
	assert(ob->type==G_BOXCHAR);
	return ((uint32_t)ob->spec>>24);
}

static void boxchar_set_char(OBJECT* ob,char c){
	assert(ob->type==G_BOXCHAR);
	uint32_t spec=(uint32_t)ob->spec&0xFFFFFFL;
	ob->spec=(void*)(spec|(uint32_t)c<<24);
}

static void ob_text_set(OBJECT* ob,char* str){
	assert((ob->type==G_TEXT)||(ob->type==G_BOXTEXT));
	((TEDINFO*)ob->spec)->text=str;
}

#include "stats.c"

static void curdle_status_set(char* str){
	ob_text_set(curdle_status,str);
}

// get screen coordinates for a curd grid OBJECT
static void curd_get_grect(OBJECT* ob,GRECT* gr){
       gr->x=ob->gr.x+curd_box->gr.x+curdle_ui->gr.x;
       gr->y=ob->gr.y+curd_box->gr.y+curdle_ui->gr.y;
       gr->w=ob->gr.w;
       gr->h=ob->gr.h;
}

static void curd_redraw(OBJECT* ob){
	GRECT gr;
	curd_get_grect(ob,&gr);
	//obj_offset(curdle_ui,(PXY*)&gr,(ob-curdle_ui)/sizeof(OBJECT));
	//gr.w=ob->gr.w;
	//gr.h=ob->gr.h;
	window_redraw(&gr);
}

static int8_t curdle_kb_state[26];

static void curdle_ui_reset(void){
	curdle_status_set("NEXT CURD: HH:MM:SS");
	// Clear the grid
	guess_num=0;
	guess_col=0;
	for(int guess=0;guess<6;++guess){
		for(int i=0;i<5;++i){
			OBJECT* ob=&curd_grid[guess*5+i];
			boxchar_set_gamestate(ob,0);
			boxchar_set_char(ob,'\0');
		}
	}
	// Clear the keyboard
	for(int i=0;i<26;++i){
		curdle_kb_state[i]=0;
		boxchar_set_gamestate(&curdle_kb[i],0);
	}
}

static void curdle_ui_update_state(void){
	unsigned int current_day;
	switch(ui_state){
		case STATS:
			ui_state=WAITING;
			curdle_stats_show();
		case WAITING: // waiting for next game
			//current_day=curd_get_day();
			current_day=curdle_stats.day+1;
			if(current_day>curdle_stats.day){
				curdle_stats.day=current_day;
				secret_curd[0]=0;
				ui_state=PLAYING;
				curdle_ui_reset();
				curdle_status_set("Guess the secret word");
				boxchar_set_char(&curd_grid[0],'_'); // cursor
				window_redraw(&desk);
			}
		default:
			break;
	}
}

// check a guess and update ui
static void ui_do_guess(OBJECT* grid){
	char guess[CURD_LEN];
	for(int i=0;i<CURD_LEN;++i){
		guess[i]=boxchar_get_char(&grid[i]);
	}
	// 8088 PCs are slow, show an hour glass
	graf_mouse(BUSY_BEE);
	// and do a clever lazy curd pick
	if(secret_curd[0]==0){
		curd_pick_day(secret_curd,curdle_stats.day);
		curdle_status_set(secret_curd);
	}
	int8_t hint[5]={0};
	switch(curd_check(secret_curd,guess,hint)){
		case 0: // Incorrect
			++guess_num;
			if(guess_num>=GUESS_MAX){
				ui_state=LOST;
			}
			guess_col=0;
			break;
		case 1:
			curdle_status_set("Correct!");
			ui_state=WON;
			break;
		case 2:
			curdle_status_set("Not in word list");
			break;
	}
	if(ui_state!=PLAYING){
		// game ended, update stats
		curdle_stats_update(ui_state==WON?1:0,guess_num);
	}
	// update curd grid
	for(int i=0;i<CURD_LEN;++i){
		boxchar_set_gamestate(&grid[i],hint[i]);
	}
	// update keyboard
	for(int i=0;i<26;++i){
		OBJECT* ob=&curdle_kb[i];
		char c=boxchar_get_char(ob);
		for(int j=0;j<CURD_LEN;++j){
			if(guess[j]==c){
				if(curdle_kb_state[i]>=hint[j]) break;
				curdle_kb_state[i]=hint[j];
				boxchar_set_gamestate(&curdle_kb[i],hint[j]);
			}
		}
	}
	window_redraw(&desk);
	graf_mouse(ARROW);
}

// implement custom text entry etc
static void ui_handle_key(int16_t key){
	if(ui_state!=PLAYING){
		ui_state=STATS;
		return;
	}
	OBJECT* grid=&curd_grid[guess_num*CURD_LEN];
	switch(key>>8){ // high byte is scancode
		case 14: // Backspace
			curdle_status_set("");
			if(guess_col<CURD_LEN){
				boxchar_set_char(&grid[guess_col],'\0');
				curd_redraw(&grid[guess_col]);
			}
			guess_col=max(0,guess_col-1);
			boxchar_set_char(&grid[guess_col],'\0');
			break;
		case 28: // Return
			if(guess_col!=CURD_LEN) return;
			ui_do_guess(grid);
			break;
		default: // Alphabetics
			if(guess_col>=CURD_LEN) return;
			char c=key&0xff; // low byte is ascii
			if((c>='a')&&(c<='z')) c-='a'-'A'; // to uppercase
			if((c<'A')||(c>'Z')) return; // reject non-alpha
			boxchar_set_char(&grid[guess_col],c);
			curd_redraw(&grid[guess_col]);
			guess_col++;
	}
	if(guess_col>=CURD_LEN) return;
	if(ui_state!=PLAYING) return;
	grid=&curd_grid[guess_num*CURD_LEN];
	boxchar_set_char(&grid[guess_col],'_'); // display cursor
	curd_redraw(&grid[guess_col]);
}

static void window_init(int16_t colors){
	curdle_ui=rsrc_gaddr(R_TREE,CURDLE_UI);
	assert(curdle_ui);

	// status line
	curdle_status=rsrc_gaddr(R_OBJECT,CURDLE_STATUS);
	assert(curdle_status);

	// curd grid container
	curd_box=rsrc_gaddr(R_OBJECT,CURD_GRID);
	assert(curd_box);

	// the first BOXCHAR of the game grid
	curd_grid=rsrc_gaddr(R_OBJECT,CURD_GRID_FIRST);
	assert(curd_grid);

	// the first BOXCHAR of the keyboard
	curdle_kb=rsrc_gaddr(R_OBJECT,CURDLE_KB_FIRST);
	assert(curdle_kb);

	// read 4 BOXCHARs to get our display states
	// pick row according to color depth
	int i=0;
	if(colors>=4) i+=5;
	if(colors>=16) i+=5;
	OBJECT* states=&curd_grid[i];
	for(int i=0;i<4;++i){
		OBJECT* ob=&states[i];
		assert(ob->type==G_BOXCHAR);
		boxchar_state[i]=ob->state;
		boxchar_spec[i]=(uint32_t)ob->spec&0xFFFFFFL; // clear the char
	}

	curdle_ui_reset();

	wind_get_work(0,&desk);
	wi_handle=NO_WINDOW;
	fulled=false;
}

#define MIN_WIDTH  (2*gl_wbox)
#define MIN_HEIGHT (3*gl_hbox)

static int window_handle_event(int16_t* msg){
	GRECT* gr=(GRECT*)&msg[4];
	switch (msg[0]){
		case WM_REDRAW:
			window_do_redraw(gr);
			break;

		case WM_NEWTOP:
		case WM_TOPPED:
			wind_set_top(wi_handle);
			break;

		case WM_CLOSED:
			window_close();
			return 1;

		case WM_MOVED:
			window_align(gr);
			wind_set_current(wi_handle,gr);
			wind_get_work(wi_handle,&work);
			curdle_ui->gr.x=work.x;
			curdle_ui->gr.y=work.y;
			fulled=false;
			break;
	}
	return 0;
}

// returns colordepth
static int16_t open_vwork(void){
	int16_t dummy;
	int16_t phys_handle=graf_handle(&dummy,&dummy,&gl_wbox,&gl_hbox);

	int16_t in[11];
	for(int16_t i=0;i<10;in[i++]=1);
	in[10]=2;
	int16_t out[57];
	v_opnvwk(in,out,phys_handle);
	return out[13]; // colordepth
}

// Accessory stuff

int16_t acc_menu_id ;	// our menu id

static void accessory_handle_event(int16_t* msg){
	switch (msg[0]) {
		case AC_OPEN:
			if(msg[4]==acc_menu_id){
				if(wi_handle == NO_WINDOW){
					open_vwork();
					window_open();
				}
				else wind_set_top(wi_handle);}
			break;
		case AC_CLOSE:
			if((msg[3]==acc_menu_id)&&(wi_handle != NO_WINDOW)){
				v_clsvwk();
				wi_handle = NO_WINDOW;
			}
			break;
	}
}

// Dispatch events
// only returns on user quit
static void handle_aes_events(){
	EVMULT_IN in={0};
	EVMULT_OUT out;
	in.emi_flags=MU_MESAG|MU_KEYBD;
	while(1){
		curdle_ui_update_state();
		int16_t msg[8];
		int event=evnt_multi_fast(&in,msg,&out);
		if(event & MU_KEYBD){
			ui_handle_key(out.key);
		}
		if(event & MU_MESAG){
			if(msg[3]==wi_handle)
				if(window_handle_event(msg))
					if(_app) return;

			if(!_app) accessory_handle_event(msg);
		}
	}
}

static void curdle_about_show(void){
	OBJECT* form_about=rsrc_gaddr(R_TREE,CURDLE_ABOUT);
	GRECT c;
	form_center_gr(form_about,&c);
	form_dial_gr(FMD_START,0,&c);
	objc_draw(form_about,0,8,&c);
	int16_t obj=form_do(form_about,0);
	ob_deselect(&form_about[obj]);
	form_dial_gr(FMD_FINISH,0,&c);
}

int main(void){
	int16_t appid=appl_init();
	assert(appid>=0);
	if(appid<0) return 1;

	if(!rsrc_load(CURDLE_RSC)){
		form_alert(1,"[3][| |Loading " CURDLE_RSC " failed. ][ Quit ]");
		return 2;
	}

	// we need to open vwork to get color depth
	int colors=open_vwork();
	window_init(colors);

	curdle_stats_init(colors);
	//curdle_stats_show();

	//curdle_stats.day=0;
	//secret_curd[0]=0;
	ui_state=PLAYING;

	// libcmini sets _app if we are running as an application
	if(!_app){
		// we are a desk accessory
		v_clsvwk(); // we have to re-open when called anyway
		// register into the desk menu
		acc_menu_id=menu_register(gl_apid,app_name);
	}else{
		graf_mouse(ARROW);
		curdle_about_show();
		// open the window now
		window_open();
	}

	handle_aes_events();

	rsrc_free();
	appl_exit();
	return 0;
}
