// curdle: (intransitive verb)
// To become spoiled or transformed into something bad

// Copyright 2022 Seg <seg@haxxed.com>

#include "portable/print.h"
#include "portable/date.h"
#include "portable/xorshift128.h"

#include "wordlist.h"

static void curd_unpack(const unsigned char* curd,char* word){
	uint32_t base26=((uint32_t)curd[0]<<16)+
		((uint32_t)curd[1]<<8)+(uint32_t)curd[2];
	char* tmp=word+5;do{*--tmp='A'+(base26%26);base26/=26;}
	while(tmp>word);}

static void curd_pick(char* word,unsigned int n){
	unsigned char curd[3];
	curd[0]=curdlist0[n];
	curd[1]=curdlist1[n];
	curd[2]=curdlist2[n];
	curd_unpack(curd,word);
}

static void curd_pack(char* word,unsigned char* curd){
	uint32_t base26=0L;
	for(int i=0;i<5;++i){
		base26*=26;base26+=((uint32_t)(word[i]&=0xdf))-'A';}
	curd[0]=base26>>16;curd[1]=base26>>8;curd[2]=base26;}

static int curd_compare(unsigned char* a,unsigned char* b){
	if(a[0]!=b[0]) return 1;if(a[1]!=b[1]) return 1;
	if(a[2]!=b[2]) return 1;return 0;}

static int curdlist_search(char* guess){
	unsigned char pguess[3];curd_pack(guess,pguess);
	for(int i=0;i<curdlist_count;++i){
		unsigned char curd[3];
		curd[0]=curdlist0[i];
		curd[1]=curdlist1[i];
		curd[2]=curdlist2[i];
		if(!curd_compare(pguess,curd)) return 1;}
	return 0;}

// check a guess
// returns: 0 incorrect, 1 correct, 2 not in list
static int curd_check(char* secret,char* guess,int8_t* hint){
	if(!curdlist_search(guess)){
		return 2;
	}
	for(int i=0;i<5;++i)
		hint[i]=1; // nope
	int win=1;
	for(int i=0;i<5;++i){
		if(secret[i]==guess[i]){
			hint[i]=3; // correct
			continue;
		}
		win=0;
		for(int j=0;j<5;++j)
			if((secret[i]==guess[j])&&(hint[j]==1)){
				hint[j]=2; // wrong spot
				break;
			}
	}
	return win;
}

#if defined(__GNUC__) && defined(__OPTIMIZE_SIZE__)
__attribute__ ((noinline)) // no gcc don't inline it
#endif
static int is_ly(int y){
	return(!(y%400)||((y%100)&&!(y%4)))?1:0;
}

static unsigned int date_in_days(int year,int month,int day){
	int d=-73;
	for(int y=1981;y<year;++y)
		d+=365+is_ly(y);
	if(--month>0)
		d+=is_ly(year);
	static char monthadj[12]={0,4,5,7,8,10,11,12,14,15,17,18};
	d+=month*32-monthadj[month];
	return d+day-1;
}

static unsigned int curd_get_day(void){
	int y,m,d;
	get_date(&y,&m,&d);
	//print(NL "Today is ");
	//printi(y);printc('-');printi(m);printc('-');printi(d);
	unsigned int curd=date_in_days(y,m,d);
	//print(NL "Daily Curd #");printi(curd);print(NL NL);
	return curd;
}

static uint32_t rng_state[4];

#if defined(__GNUC__) && defined(__OPTIMIZE_SIZE__)
__attribute__ ((noinline))
#endif
static void rng_init(void){
#ifdef __m68k__	
	rng_state[0]=0x7b0f680d;
	rng_state[1]=0x4502e955;
	rng_state[2]=0x81300422;
	rng_state[3]=0x90f3122e;
#else
	rng_state[0]=0x5b0b680d;
	rng_state[1]=0xa5dff057;
	rng_state[2]=0x827b0548;
	rng_state[3]=0x801f610e;
#endif
}

#include "portable/timer.h"

static void rng_seed(void){
	long time=get_ticks();
	//printl(time);print(NL);
	rng_state[3]^=~(uint32_t)time;
}

static void curd_pick_rng(char* secret){
	unsigned int n=rng_xor128(rng_state,curdlist_count);
	curd_pick(secret,n);
}

static void curd_pick_day(char* secret, unsigned int day){
	rng_init();
	for(unsigned int i=0;i<day;++i)
		rng_xor128_u32(rng_state);
	curd_pick_rng(secret);
	rng_seed();
}
