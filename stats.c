typedef struct{
	uint16_t wins;
	uint16_t losses;
	uint16_t streak_cur;
	uint16_t streak_max;
	uint16_t dist[6];
	uint16_t day;
} CURDLE_STATS_T;

CURDLE_STATS_T curdle_stats={0};

static OBJECT* form_stats;

static const char stats_filename[]=CURDLE_SAV;

#if !defined(__MINT__) && !defined(__WATCOMC__)
#include <stdio.h>
static void curdle_stats_load(){
	FILE* f=fopen (stats_filename,"r");
	if(!f) return;
	fread(&curdle_stats,sizeof(CURDLE_STATS_T),1,f);
	fclose(f);
}

static void curdle_stats_save(){
	FILE* f=fopen(stats_filename,"w");
	if(!f) return;
	fwrite(&curdle_stats,sizeof(CURDLE_STATS_T),1,f);
	fclose(f);
}
#endif

#ifdef __MINT__
static void curdle_stats_load(){
	int32_t f=Fopen(stats_filename,S_READ);
	if(f<0) return;
	Fread(f,sizeof(CURDLE_STATS_T),&curdle_stats);
	Fclose(f);
}

static void curdle_stats_save(){
	int32_t f=Fopen(stats_filename,S_WRITE);
	if(f<0) return;
	Fwrite(f,sizeof(CURDLE_STATS_T),&curdle_stats);
	Fclose(f);
}
#endif

#ifdef __WATCOMC__
#include <dos.h>
#include <fcntl.h>
static void curdle_stats_load(){
	unsigned read;
	int f;
	if(_dos_open(stats_filename,O_RDONLY,&f)) return;
	_dos_read(f,&curdle_stats,sizeof(CURDLE_STATS_T),&read);
	_dos_close(f);
}

static void curdle_stats_save(){
	unsigned wrote;
	int f;
	if(_dos_creat(stats_filename,_A_NORMAL,&f)) return;
	_dos_write(f,&curdle_stats,sizeof(CURDLE_STATS_T),&wrote);
	_dos_close(f);
}
#endif

static void curdle_stats_init(int colors){
	form_stats=rsrc_gaddr(R_TREE,CURDLE_STATS);

	// propagate bar fill attributes according to color depth
	OBJECT* ob=&form_stats[STATS_FIRST_BAR];
	if(colors>=4) ob+=2;
	assert(ob->type==G_BOXTEXT);
	int16_t color=((TEDINFO*)ob->spec)->color;
	for(int i=0;i<6;++i){
		ob=&form_stats[STATS_FIRST_BAR+i*2];
		assert(ob->type==G_BOXTEXT);
		((TEDINFO*)ob->spec)->color=color;
	}
	
	curdle_stats_load();
}

static void curdle_stats_update(int16_t win,int16_t guesses){
	if(win){
		++curdle_stats.wins;
		++curdle_stats.streak_cur;
		++curdle_stats.dist[guesses];
	}else{
		++curdle_stats.losses;
		curdle_stats.streak_cur=0;
	}
	if(curdle_stats.streak_cur>curdle_stats.streak_max){
		curdle_stats.streak_max=curdle_stats.streak_cur;
	}
	curdle_stats_save();
}

static char* strcpy(char* dst, const char* src){
	char* ptr=dst;
	while((*dst++=*src++));
	return ptr;
}

static void itoa_u16(char* dst,uint16_t n){
	char tmp[6];
	char* str=tmp+5;
	*str = 0;
	do{
		*--str = '0'+(n%10);
		n /= 10;
	} while (n > 0);
	strcpy(dst,str);
}

static void curdle_stats_show(){
	// we need to convert a lot of ints to strings
	char str[10][6];
	// compute and fill in basic game stats
	uint16_t played=curdle_stats.wins+curdle_stats.losses;
	uint16_t win_percent=0;
	if(played){
		win_percent=((uint32_t)curdle_stats.wins*100L)/(uint32_t)played;
	}
	itoa_u16(str[0],played);
	itoa_u16(str[1],win_percent);
	itoa_u16(str[2],curdle_stats.streak_cur);
	itoa_u16(str[3],curdle_stats.streak_max);
	for(int i=0;i<4;++i){
		ob_text_set(&form_stats[STATS_FIRST+i],str[i]);
	}
	
	// update bars
	uint16_t dist_max=1;
	for(int i=0;i<6;++i){
		dist_max=max(dist_max,curdle_stats.dist[i]);
		itoa_u16(str[i+4],curdle_stats.dist[i]);
		ob_text_set(&form_stats[STATS_FIRST_BAR+i*2],str[i+4]);
	}
	int16_t bar_maxlen=form_stats[STATS_FIRST_BAR].gr.w-16;
	for(int i=0;i<6;++i){
		int32_t tmp=((int32_t)curdle_stats.dist[i]*(int32_t)bar_maxlen)/
			((int32_t)dist_max);
		form_stats[STATS_FIRST_BAR+i*2].gr.w=tmp+16;
	}

	// display dialog
	GRECT c;
	form_center_gr(form_stats,&c);
	form_dial_gr(FMD_START,0,&c);
	objc_draw(form_stats,0,8,&c);
	int16_t obj=form_do(form_stats,0);
	ob_deselect(&form_stats[obj]);
	form_dial_gr(FMD_FINISH,0,&c);

	// restore for next time
	form_stats[STATS_FIRST_BAR].gr.w=bar_maxlen+16;
}
