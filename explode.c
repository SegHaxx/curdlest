// epic win
static void explode(void){
	OBJECT* grid=&curd_grid[guess_num*CURD_LEN];
	GRECT curd_gr[CURD_LEN];
	PXY curd_vel[CURD_LEN];
	MFDB curd_mfdb[CURD_LEN];
	for(short i=0;i<CURD_LEN;++i){
		GRECT* curd=&curd_gr[i];
		curd_get_grect(&grid[i],curd);
		--curd->x;
		--curd->y;
		++curd->w;
		++curd->h;
		rc_intersect(&desk,curd);
		curd_vel[i].x=(i-CURD_LEN/2)*2;
		curd_vel[i].y=-5;
		curd_vel[i].y-=curd->y/curd_gr->h;
		//MFDB mfdb={0};
		//vro_cpyfm(S_ONLY,(int16_t*)&vr,&mfdb,&curd_mfdb[i]);
	}
	for(short j=0;j<200;++j){
		for(short i=0;i<CURD_LEN;++i){
			GRECT* curd=&curd_gr[i];
			VRECT vr[2];
			vr[0].x1=curd->x;
			vr[0].y1=curd->y;
			vr[0].x2=curd->x+curd->w-1;
			vr[0].y2=curd->y+curd->h-1;

			curd->x+=curd_vel[i].x;

			int16_t vy=curd_vel[i].y;
			if((curd->y+curd->h)>=desk.y+desk.h){
				vy=(vy*15)/16;
				vy=vy>0?-vy:vy;
				//curd->y=desk.h-curd->h-5;
			}
			curd->y+=vy;
			curd_vel[i].y=vy+1;

			if(rc_intersect(&desk,curd)){
				vr[1].x1=curd->x;
				vr[1].y1=curd->y;
				vr[1].x2=curd->x+curd->w-1;
				vr[1].y2=curd->y+curd->h-1;
				MFDB mfdb={0};
				vro_cpyfm(S_XOR_D,(int16_t*)&vr,&mfdb,&mfdb);
			}
		}
	}
}
