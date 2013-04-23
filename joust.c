#include <SDL/SDL.h>
#include <stdlib.h>
#include "bitmask/bitmask.c"
#include <time.h>
#include <string.h>

#define MAX_SPRITES 200
#define MAX_PLAYERS 4
#define MAX_SPEED 10

#define ROOFLEVEL 0 
#define REALHEIGHT 800
#define PANELPLACE REALHEIGHT-100
#define GROUNDLEVEL REALHEIGHT-132

#define REALWIDTH 1200
#define WRAPWIDTH 1178

#define NUM_STEEDS 11

typedef struct playertruc {
        int dx, dy, x, y, width, height, dir, oldx, oldy;
        int initframe,framenum,wingnum,ridernum;
	int computer;
        int playeron,dying,lifetime,cod,isgoal,plat,platnum,wasonplat,birdtype,flap;
	SDLKey lkey,rkey,ukey,skey;
	int lkeyon,rkeyon,ukeyon,skeyon;
} player_t;

#define MAX_PLATFORMS 150
typedef struct platinfo_t {
  int x,y,width,height,dx,dy,xlim1,xlim2,ylim1,ylim2;
  char type,temptype,effecttype,fieldtype,extraeffects[20];      
  int active;
  int length;
  int dir;
  int appearance;
  signed int oncycles,offcycles,count,numcycles,cycletype;
  bitmask *bitmasks;
} platinfo;
platinfo platforms[MAX_PLATFORMS];
int numplatforms;

int halfgrav=0,numplayer=4,gravity=1,phase=0;

int debug=0;

const char maxspeedx[NUM_STEEDS]={20,24,20,20, 12,20,22, 12,22,18,0};
const char maxspeedy[NUM_STEEDS]={24,16,16,32,22,16,18,16,24,24,0};

char accelx[NUM_STEEDS] =  { 3, 2, 3, 1, 2, 2, 3, 4, 1 ,3, 0};
char accely[NUM_STEEDS] =  { 4, 4, 2, 3, 5, 2, 3 ,2, 3, 4, 0};
char masses[NUM_STEEDS]={2,3,3,4,2,3,3,2,5,2,0};

const char *prefixes[NUM_STEEDS] = {"CLASSIC","PAR","HORS","BUFF","MOTH","BANA","CAT","BIKE","DINO","FISH"};
#define PLATFORMFILENAME "images/PLATSOLO.bmp"
#define PLATLEFTFILENAME "images/PLATLEFT.bmp"
#define PLATMIDFILENAME "images/PLATMID.bmp"
#define PLATRIGHTFILENAME "images/PLATRIGH.bmp"
#define PLATVERTFILENAME "images/PLATVERT.bmp"

char *backfile;

const char spechigh = 9;
const char specrate[10]={0,60,60,5,40,10,50,10,20,30};
const char minenum[10]={'0','1','2','3','4','5','6','7','8','9'};

static player_t player[MAX_PLAYERS];

static SDL_Surface *screen;
static SDL_Surface *sprite_d[MAX_SPRITES];

SDL_Surface *background, *panel;
int bgon=0,panon=0,lava=0;
Uint16 backcolor,pancolor;

static bitmask *bitmasks[MAX_SPRITES]; 

/* 1 2 3 4 5 W1 W2 */
/* 0-99 facing forward */
/* 100-199 facing backward */
/* 70/71 riders */

#define RIDER 70
#define PLATFORM 80
#define PLATLEFT 81
#define PLATMID 82
#define PLATRIGHT 83
#define PLATVERT 84

#define FIRESPRITE 85
#define FIREMAX 89

#define BURNSPRITE 90
#define BURNMAX 94

/* Causes of death (also used for other things) */
#define LANCE 1
#define FIRE 2
#define LAVA 3

/* Other flags */
#define HORIZONTAL 0
#define VERTICAL 1
#define BLOCK 1

int min(int a, int b)
{
  return ((a < b) ? a : b);
}

int max(int a, int b)
{
  return ((a > b) ? a : b);
}


static void init_platforms()
{
  int i,spr_draw;

  for (i=0;i<numplatforms;i++) {
	platforms[i].active = 1;

        spr_draw = PLATMID; 

        if (platforms[i].dir == VERTICAL)
          {
	    spr_draw = PLATVERT;
            platforms[i].width = sprite_d[spr_draw]->w;
            platforms[i].height = platforms[i].length*sprite_d[spr_draw]->h;
          }
        else
          {
	    platforms[i].width = platforms[i].length*sprite_d[spr_draw]->w;
	    platforms[i].height = sprite_d[spr_draw]->h;
          }
  }

  if (debug) printf("\nPlatform Init OK\n");
}


static void init_players()
{
   int i;

  for (i=0;i<MAX_PLAYERS;i++) {
	player[i].dy = 0;
	player[i].dx = (i*4) * (-1 * (i & 1)); /* (rand() % MAX_SPEED * 2) - MAX_SPEED; */
        player[i].dy = 0;
        player[i].ridernum = 70;
	player[i].birdtype = i;
        player[i].initframe = player[i].birdtype*7;
	player[i].framenum = player[i].initframe;
	player[i].wingnum = player[i].birdtype*7+5;
	player[i].flap = 0;
	player[i].dir = i & 1;

	player[i].plat = 0;
	player[i].platnum = 0;
	player[i].isgoal = 0;
	player[i].playeron = 1;
	player[i].dying = 0;
	player[i].wasonplat = 0;

/* FIX THIS IF SPRITES ARE EVER MULTI-SIZED */
	player[i].width = sprite_d[0]->w;
	player[i].height = sprite_d[0]->h;

     player[i].x = rand() % (REALWIDTH - 1);
     player[i].y = rand() % (GROUNDLEVEL - 1);

	player[i].lkeyon=0; player[i].rkeyon=0; player[i].ukeyon=0; player[i].skeyon=0;

	if (i > 1) player[i].computer = 1; else player[i].computer = 0;
  }
  player[0].lkey = SDLK_LEFT;
  player[0].rkey = SDLK_RIGHT;
  player[0].ukey = SDLK_UP;

  player[1].lkey = SDLK_a;
  player[1].rkey = SDLK_d;
  player[1].ukey = SDLK_w;

  player[2].lkey = SDLK_f;
  player[2].rkey = SDLK_h;
  player[2].ukey = SDLK_t;
 
  player[3].lkey = SDLK_j;
  player[3].rkey = SDLK_l;
  player[3].ukey = SDLK_i;
}

static void move_players()
{ 
/* FROM OLD CODE */

 int p;

 for (p=0;p<MAX_PLAYERS;p++) 
  {
   player[p].oldx = player[p].x;
   player[p].oldy = player[p].y;

   if (player[p].flap == 1) 
   {
      player[p].dy =  player[p].dy - accely[player[p].birdtype]; 
   } 
   
if ((player[p].playeron) || (player[p].isgoal)) 
   {

   player[p].y += player[p].dy; 

   if (player[p].y >= GROUNDLEVEL) player[p].y = GROUNDLEVEL; /* land on floor */

   if (player[p].y < ROOFLEVEL) {
          player[p].y = ROOFLEVEL+3;
          if (player[p].dy < 0) player[p].dy *= -1; /* bounce off ceiling */
     }

   if (((player[p].y == GROUNDLEVEL) || (player[p].plat)) && (player[p].dy > 0)) {
	 if (lava)  { player[p].playeron = 0; player[p].dying = 1; player[p].cod = FIRE; player[p].lifetime = 10; }
         else 
         if (player[p].dy < 5) player[p].dy=0; /* smack into floor */
         else if (player[p].playeron) player[p].dy=-(player[p].dy >> 2); /* bounce */
       }
    else if (!player[p].plat)
	{
        /* if (((!halfgrav) && (curpage)) || ((halfgrav) && (!(gameturn & 3)))) */
	     if (halfgrav && (phase & 1)) 
		{ player[p].dy+=gravity; }
	     else
		player[p].dy+=gravity;
	}
 /* if ((player[p].wasonplat) && (!player[p].plat))
     if (platform[player[p].wasonplat+1].effecttype == 4)
       player[p].dx += platform[player[p].wasonplat+1].dx; */ /* maintain relative speed when stepping off a moving platform */

    player[p].x += player[p].dx;

   if ((player[p].dx != 0) && ((player[p].y == GROUNDLEVEL) || (player[p].plat))) /* IF THE PLAYER IS MOVING ON THE GROUND/PLATFORM... */ 
      player[p].dx=player[p].dx * 3 / 4;
      /* player[p].framenum =  0; y+(7*player[p].birdtype); */
  
   if (player[p].plat) {
          /* player[p].x += platforms[player[p].plat-1].dx; move player @ speed of platform */
          if (player[p].dy > 0) player[p].dy = 0; /*
	  player[p].y = platforms[player[p].platnum].y-player[p].height;  */
        } 

   if (player[p].dy > 24) player[p].dy = 24; else
   if (player[p].dy < -1*maxspeedy[player[p].birdtype]) player[p].dy = -1*maxspeedy[player[p].birdtype];
   if ((player[p].dx) > maxspeedx[player[p].birdtype]) player[p].dx = maxspeedx[player[p].birdtype]; else
   if ((player[p].dx) < -1*maxspeedx[player[p].birdtype]) player[p].dx = -1*maxspeedx[player[p].birdtype];

/* Correct all values of X to fit in the range 0-(REALWIDTH-1) */
     if (player[p].x < 0) { player[p].x = REALWIDTH+player[p].x; }
     if (player[p].x >= REALWIDTH) { player[p].x = player[p].x-REALWIDTH; }

   }
   }
}

void drawcollision(bitmask *temp,bitmask *temp2)
{
  int i,j;

  printf("\n");
  for (j=0;j<max(temp->h,temp2->h);j++)
	{
	if (j < temp->h) {
	for (i=0;i<temp->w;i++)
	   if (bitmask_getbit(temp,i,j)) printf("*"); } else printf(" ");
	printf(" ");
        if (j < temp2->h) {
	for (i=0;i<temp2->w;i++)
	   if (bitmask_getbit(temp2,i,j)) printf("*"); } else printf(" ");
	printf("\n");
 	}
  printf("\n--\n");
} 

void drawcollision2(bitmask *temp)
{
  int i,j;

  for (j=0;j< temp->h;j++)
        {
        for (i=0;i<temp->w;i++)
           if (bitmask_getbit(temp,i,j)) printf("*"); else printf(" ");
        printf("\n");
        }
  printf("\n--\n");

}

void pre_scan()
{
    int p;

    for (p=0;p<MAX_PLAYERS;p++)
     {
      if (player[p].plat)
     {
        if (!((player[p].x > platforms[player[p].platnum].x+platforms[player[p].platnum].width) ||
            (player[p].x+player[p].width < platforms[player[p].platnum].x) ||
            (player[p].y != platforms[player[p].platnum].y-player[p].height)))
                player[p].plat=1;
         else player[p].plat=0;
     }
    }
} 

void platform_scan()
{
     int p,q,xoff,yoff,off=0;
     bitmask *temp;

     for (p=0;p<MAX_PLAYERS;p++) 
     {
     for (q=0;q<numplatforms;q++)
     {
       if (((player[p].y+player[p].height>platforms[q].y) && (player[p].y < platforms[q].y+platforms[q].height)) && 
        ((player[p].x+player[p].width>platforms[q].x) && (player[p].x < platforms[q].x+platforms[q].width))) 
        {
           xoff = platforms[q].x - player[p].x;
           yoff = platforms[q].y - player[p].y;

           temp = bitmask_create(player[p].width,player[p].height);
           if (player[p].dir) off = 100; else off = 0;
           bitmask_draw(temp,bitmasks[player[p].framenum+off],0,0);
	   bitmask_draw(temp,bitmasks[player[p].ridernum+off],0,0);
           bitmask_draw(temp,bitmasks[player[p].wingnum+off],0,0);

     /*     printf("Platform collision - p:%d (%d,%d) (%d %d)",p,player[p].x,player[p].y,xoff,yoff);  */
  

           if (bitmask_overlap(temp,platforms[q].bitmasks,xoff,yoff))
	   {	
                if (platforms[q].type == FIRE)
                        {
			  player[p].dying = 1;
			  if (player[p].playeron) player[p].lifetime = 10;
			  player[p].playeron = 0;
			  player[p].cod = FIRE;
                        }
                else
		if ((player[p].oldy+player[p].height <= platforms[q].y))
			/* top collision */
			{
			  if ((player[p].playeron)  && (player[p].dy > 5)) 
				player[p].dy = -player[p].dy / 4;
				else player[p].dy = 0;
                          player[p].y = platforms[q].y - player[p].height; 
                          player[p].plat++;
                          player[p].platnum = q;
			}
		else
		if ((player[p].oldy >= platforms[q].y+platforms[q].height)) /* bottom collision */
			{
			  player[p].y = platforms[q].y + platforms[q].height;
                          player[p].dy *= -1;
			}
		else
		if ((player[p].oldy < platforms[q].y+platforms[q].height) &&
			(player[p].oldy+player[p].height > platforms[q].y)) 
  /* side collision */
			{
			  if (player[p].dx > 0) player[p].x = platforms[q].x-player[p].width;
			  else if (player[p].dx < 0) player[p].x = platforms[q].x+platforms[q].width;
                          player[p].dx *= -1;
			}
		else
		printf("Unhandled collision..."); 

	   }
       }
    }
   }
}

void coll_detect()
{
  int p,q,off,xoff,yoff, vq;
  bitmask *temp,*temp2;

  for (p=0;p<MAX_PLAYERS;p++)
   for (q=0;q<MAX_PLAYERS;q++)
     if ((p != q) && (player[p].playeron) && (player[q].playeron))
     {	

                       if (player[p].x > REALWIDTH) player[p].x -= REALWIDTH;
                        if (player[p].x < 0) player[p].x += REALWIDTH;
                        if (player[q].x > REALWIDTH) player[q].x -= REALWIDTH;
                        if (player[q].x < 0) player[q].x += REALWIDTH;

	
       xoff = player[q].x-player[p].x;
       if ((player[q].x > WRAPWIDTH) && (player[p].x < player[q].width)) 
		{
		   if (debug) printf("%d %d %d %d\n",xoff,xoff-WRAPWIDTH,player[p].x,player[q].x);
		   xoff -= WRAPWIDTH;
		}
       /* CORRECT FOR WRAP-AROUND COLLISIONS */
  
       yoff = player[q].y-player[p].y;

       if (abs(xoff) <= min(player[p].width,player[q].width))
       if (abs(yoff) <= min(player[p].height,player[q].height))
	{
           temp = bitmask_create(player[p].width,player[p].height);
	   if (player[p].dir) off = 100; else off = 0;
	   bitmask_draw(temp,bitmasks[player[p].framenum+off],0,0);
	   bitmask_draw(temp,bitmasks[player[p].wingnum+off],0,0);

           temp2 = bitmask_create(player[q].width,player[q].height);
           if (player[q].dir) off = 100; else off = 0;
           bitmask_draw(temp2,bitmasks[player[q].framenum+off],0,0);
           bitmask_draw(temp2,bitmasks[player[q].wingnum+off],0,0);

	   if (bitmask_overlap(temp,temp2,xoff,yoff)) 
	   {
		if (yoff > 0) { player[q].playeron = 0; player[q].dying = 1; player[q].cod = LANCE; }
		else if (yoff < 0) { player[p].playeron = 0; player[q].dying = 1; player[q].cod = LANCE; }
		else if (yoff == 0) 
		  {
/*
   			massp = masses[player[p].birdtype];
			massq = masses[player[q].birdtype];
			vp = player[p].dx;
			
		        if (massp == massq) 
			{
			   player[p].dx = player[q].dx;
			   player[q].dx = vp;
			}
           		else 
			{
             		  vq = player[q].dx;
             		  player[p].dx = (int) ((massp*vp)+(2*(massq*vq))-
                                     (massq*vq))/(massp+massq);
             		  player[q].dx = player[p].dx + vp - vq;
		        }  */

			if (xoff < 0) player[q].x -= 16;
/* player[p].x - 16; */
			else player[q].x += 16;
/* player[p].x + 16; */  


/*
			if (xoff < 0) xoff *= -1;
			player[p].x -= xoff / 2;
			player[q].x += xoff / 2; */
 

			vq = player[q].dx;
                        player[q].dx = player[p].dx;
                        player[p].dx = vq;  

			if (player[p].x > REALWIDTH) player[p].x -= REALWIDTH;
			if (player[p].x < 0) player[p].x += REALWIDTH;
			if (player[q].x > REALWIDTH) player[q].x -= REALWIDTH;
			if (player[q].x < 0) player[q].x += REALWIDTH;
		  }
	   }

          bitmask_free(temp);
	  bitmask_free(temp2);	
	}
  
   }
}

static void make_platform_bitmasks()
{
 int i,j,spr_draw;

 for (i=0;i<numplatforms;i++)
 {
        platforms[i].bitmasks = bitmask_create(platforms[i].width,platforms[i].height);

        if (platforms[i].active)
        for (j=0;j<platforms[i].length;j++)
        {

        if (platforms[i].dir == VERTICAL) spr_draw = PLATVERT;
        else if (platforms[i].appearance == BLOCK) spr_draw = PLATMID;
        else if (platforms[i].length == 1) spr_draw=PLATFORM;
        else if (j == 0) spr_draw = PLATLEFT;
        else if (j == platforms[i].length-1) spr_draw = PLATRIGHT;
        else spr_draw = PLATMID;

        if (platforms[i].dir == VERTICAL) 
        bitmask_draw(platforms[i].bitmasks,bitmasks[spr_draw],0,j*bitmasks[spr_draw]->h);
        else
	bitmask_draw(platforms[i].bitmasks,bitmasks[spr_draw],j*bitmasks[spr_draw]->w,0);

	}
 }
}

static void draw_platforms()
{
  int i,j,off;
  int spr_draw;
   SDL_Rect src,dest;

   dest.w = sprite_d[PLATFORM]->w;
   dest.h = sprite_d[PLATFORM]->h;
   src.w = dest.w;
   src.h = src.h;
   src.y = 0;


/* DEBUG SHOW ALL SPRITES */
   if (debug) {
   for (off=0;off<8;off++)
        for (i=0;i<25;i++)
          {
            dest.x = i*32;
            dest.y = off*32;
            SDL_BlitSurface(sprite_d[i+(25*off)], NULL, screen, &dest);
          }
   }

   for (i=0;i<numplatforms;i++)
      if (platforms[i].active) 
        {

        dest.y = platforms[i].y;
	dest.x = platforms[i].x;

        for (j=0;j<platforms[i].length;j++)
        {

        if (platforms[i].dir == VERTICAL) spr_draw = PLATVERT;
        else if (platforms[i].appearance == BLOCK) spr_draw = PLATMID;
        else if (platforms[i].type == FIRE) spr_draw=FIRESPRITE+((phase+j) % 4);
        else if (platforms[i].length == 1) spr_draw=PLATFORM;
        else if (j == 0) spr_draw = PLATLEFT;
        else if (j == platforms[i].length-1) spr_draw = PLATRIGHT;
        else spr_draw = PLATMID;

   dest.w = sprite_d[spr_draw]->w;
   dest.h = sprite_d[spr_draw]->h;

/* if screen overlap, draw twice for wrap-around */

        if (platforms[i].x+platforms[i].width > REALWIDTH)
        {

          src.x = 0;
          src.w = REALWIDTH-player[i].x;

          SDL_BlitSurface(sprite_d[spr_draw], &src, screen, &dest);

          src.x = REALWIDTH-platforms[i].x;
          src.w = sprite_d[spr_draw]->w;
          dest.x = 0;

          SDL_BlitSurface(sprite_d[spr_draw], &src, screen, &dest);

          src.x = 0;
        }
        else
        {
           SDL_BlitSurface(sprite_d[spr_draw], NULL, screen, &dest);
        }

       if (platforms[i].dir == VERTICAL) dest.y += sprite_d[spr_draw]->h;
         else
       dest.x += sprite_d[spr_draw]->w;
     }
   }
}

static void dying_players()
{
  int p;
  SDL_Rect dest;

  for (p=0;p<MAX_PLAYERS;p++)
    if (player[p].dying)
      {
        dest.x = player[p].x;
	dest.y = player[p].y;
        if (player[p].cod == FIRE)
          SDL_BlitSurface(sprite_d[BURNSPRITE+(player[p].lifetime % 4)], NULL, screen, &dest);
        player[p].lifetime--;
        if (player[p].lifetime <= 0) player[p].dying = 0;
	player[p].x += player[p].dx;
	player[p].y += player[p].dy;
      }
}

static void draw_players()
{
   int i,off,f,r,w,quick;
   SDL_Rect src,dest;

   dest.w = player[0].width;
   dest.h = player[0].height;
   src.w = player[0].width;
   src.h = player[0].width;
   src.y = 0;

   for (i=0;i<MAX_PLAYERS;i++) 
	if (player[i].playeron) {

	dest.x = player[i].x;
	dest.y = player[i].y;
        off = player[i].dir * 100;

	f = player[i].framenum+off;
	r = player[i].ridernum+off;
	w = player[i].wingnum+off+player[i].flap;

/* if screen overlap, draw twice for wrap-around */

        if (player[i].x+player[i].width > REALWIDTH)
        {

/*	  src.x = 0;
	  src.w = REALWIDTH-player[i].x; 

	  SDL_BlitSurface(sprite_d[f], &src, screen, &dest);
          SDL_BlitSurface(sprite_d[r], &src, screen, &dest);
          SDL_BlitSurface(sprite_d[w], &src, screen, &dest);

	  src.x = REALWIDTH-player[i].x;
	  src.w = player[0].width;
	  dest.x = 0;

          SDL_BlitSurface(sprite_d[f], &src, screen, &dest);
          SDL_BlitSurface(sprite_d[r], &src, screen, &dest);
          SDL_BlitSurface(sprite_d[w], &src, screen, &dest);

          src.x = 0; */

          SDL_BlitSurface(sprite_d[f], NULL, screen, &dest);
          SDL_BlitSurface(sprite_d[r], NULL, screen, &dest);
          SDL_BlitSurface(sprite_d[w], NULL, screen, &dest);

	  quick = player[i].x-REALWIDTH;
          dest.x = quick;

          SDL_BlitSurface(sprite_d[f], NULL, screen, &dest);
	  dest.x = quick;
          SDL_BlitSurface(sprite_d[r], NULL, screen, &dest);
	  dest.x = quick;
          SDL_BlitSurface(sprite_d[w], NULL, screen, &dest);
 

        }
        else
	{
           SDL_BlitSurface(sprite_d[f], NULL, screen, &dest);
           SDL_BlitSurface(sprite_d[r], NULL, screen, &dest);
           SDL_BlitSurface(sprite_d[w], NULL, screen, &dest);
	}

        if (player[i].dx != 0) 
        {

            if (player[i].plat || player[i].y == GROUNDLEVEL)
            {
               player[i].framenum+=1;
               if (player[i].framenum > player[i].initframe+4) player[i].framenum=player[i].initframe;
	    }

            player[i].ridernum+=1;
            if (player[i].ridernum > 71) player[i].ridernum=70;
        }

   }

}

void post_draw()
{
  int i;

  for (i=0;i<MAX_PLAYERS;i++)
	player[i].flap = 0;

  phase++;
}

int make_bitmasks()
{
  int i=0,w,h;
  Uint8 index;

  for (i=0;i<MAX_SPRITES;i++)
	if (sprite_d[i] != NULL)
	  {

		bitmasks[i] = bitmask_create(sprite_d[i]->w,sprite_d[i]->h);

		SDL_LockSurface(sprite_d[i]);

	  	for (h=0;h<sprite_d[i]->h;h++)
		{
		for (w=0;w<sprite_d[i]->w;w++)
		  {
		        index=*(Uint8 *) (sprite_d[i]->pixels+h*sprite_d[i]->w+w);
			if (index != 0)
				bitmask_setbit(bitmasks[i],w,h);
		  }
		}

		SDL_UnlockSurface(sprite_d[i]);
		
	  }

   for (i=0;i<70;i++)
	if (bitmasks[i] != NULL)
	{
	  bitmask_draw(bitmasks[i],bitmasks[RIDER],0,0);
	  bitmask_draw(bitmasks[i+100],bitmasks[RIDER+100],0,0);
	}

   return 1;

}

int odd(int a)
{
  if (a & 1) return 1; else return 0;
}

int load_level(const char *name)
{
  FILE *stream;
  int x,z=0,pl_ind=0,plat_ind=0,done=0,inplat=0,backr,backg,backb;
  char *readem;

  bgon = 0;
  panon = 0;

  if ((stream = fopen(name,"r")) == NULL) printf("Level nonexistent\n");
  else
  {
    readem = malloc(255);

    printf("Reading level file %s\n",name);
    while ((!feof(stream)) && (!done))
    {
     fscanf(stream,"%s",readem);

     if (inplat)
     {
       if (strcmp(readem,"end") == 0) 
         {
           plat_ind++;
           inplat = 0;
	   printf("\n");
         }
       else
       if (strcmp(readem,"pos") == 0)
         {
           fscanf(stream," %d %d %d\n",&platforms[plat_ind].x,&platforms[plat_ind].y,&platforms[plat_ind].length);
           printf("Platform %d at %d,%d length %d",plat_ind,platforms[plat_ind].x,platforms[plat_ind].y,platforms[plat_ind].length);
         }
       else
	if (strcmp(readem,"fire") == 0)
	{
           platforms[plat_ind].type = FIRE;
	   printf(" (on fire) ");
        }	
       else
        if (strcmp(readem,"vertical") == 0)
        {
           platforms[plat_ind].dir = VERTICAL;
           printf(" (vertical) ");
        }
       else
       if (strcmp(readem,"block") == 0)
        {
           platforms[plat_ind].appearance = BLOCK;
           printf(" (block) ");
        }



     }
     else
     if (strcmp(readem,"bgcolor") == 0)
       {
         fscanf(stream," %d %d %d\n",&backr,&backg,&backb); 
         backcolor = SDL_MapRGB(screen->format,backr,backg,backb);
         printf("Background color is: %d %d %d\n",backr,backg,backb);
       }
     else
     if (strcmp(readem,"pancolor") == 0)
       {
         fscanf(stream," %d %d %d\n",&backr,&backg,&backb);
         pancolor = SDL_MapRGB(screen->format,backr,backg,backb);
         printf("Panel color is: %d %d %d\n",backr,backg,backb);
       }
     else
     if (strcmp(readem,"lava") == 0) 
       {
         lava=1;
         printf("Floor is lava.\n");
       }
     else
     if (strcmp(readem,"background") == 0)
       {
         fscanf(stream," %s\n",backfile);
         background = SDL_LoadBMP(backfile);
	 if (background == NULL)
           printf("Couldn't load background file %s ... defaulting to color %d\n",backfile,backcolor);
         else
         bgon = 1;
       }
     else
     if (strcmp(readem,"panel") == 0)
       {
         fscanf(stream," %s\n",backfile);
         panel = SDL_LoadBMP(backfile);
         if (panel == NULL)
           printf("Couldn't load panel file %s ... defaulting to color %d\n",backfile,pancolor);
         else
         panon = 1;
       }
     else
     if (strcmp(readem,"player") == 0)
       {
         fscanf(stream," %d %d\n",&player[pl_ind].x,&player[pl_ind].y); 
         printf("Player %d starting spot is %d,%d\n",pl_ind,player[pl_ind].x,player[pl_ind].y);
         pl_ind++;
       }
     if (strcmp(readem,"platform") == 0)
      {
        inplat = 1;
        platforms[plat_ind].dir = HORIZONTAL;
        platforms[plat_ind].active = 1;
      }
     else
     if ((strcmp(readem,"\n") == 0) || (strcmp(readem,"\a"))) ;
       /* do nothing */ 
     else
       printf("Unknown garbage in level file\n");
   
     z++;
     if (z > 60) done = 1;
    }

   numplatforms = plat_ind;

  free(readem);
  fclose(stream);
  return 1;
  }
  return 0;
}

int load_sprites()
{
    const char *filename;
    int p=0,i=0;

    filename=malloc(255);

    for (p=0;p<NUM_STEEDS-1;p++)
    {
    for (i=0;i<5;i++)
    {
       sprintf(filename,"images/%s%d.bmp",prefixes[p],i+1);
       sprite_d[i+p*7] = SDL_LoadBMP(filename);
 
       SDL_SetColorKey(sprite_d[i+p*7],
                    SDL_SRCCOLORKEY,
                    (Uint16) SDL_MapRGB(sprite_d[i+p*7]->format, 0, 0, 0));

       sprintf(filename,"images/%s%d-back.bmp",prefixes[p],i+1);
       sprite_d[i+100+p*7] = SDL_LoadBMP(filename);

       SDL_SetColorKey(sprite_d[i+100+p*7],
                    SDL_SRCCOLORKEY,
                    (Uint16) SDL_MapRGB(sprite_d[i+100+p*7]->format, 0, 0, 0));
    }

    for (i=0;i<2;i++)
    {
       sprintf(filename,"images/%s%s%d.bmp",prefixes[p],"W",i+1);
       sprite_d[i+5+p*7] = SDL_LoadBMP(filename);

       SDL_SetColorKey(sprite_d[i+5+p*7],
                    SDL_SRCCOLORKEY,
                    (Uint16) SDL_MapRGB(sprite_d[i+5+p*7]->format, 0, 0, 0));

       sprintf(filename,"images/%s%s%d-back.bmp",prefixes[p],"W",i+1);
       sprite_d[i+105+p*7] = SDL_LoadBMP(filename);
                  
       SDL_SetColorKey(sprite_d[i+105+p*7],
                    SDL_SRCCOLORKEY,
                    (Uint16) SDL_MapRGB(sprite_d[i+105+p*7]->format, 0, 0, 0));

    }
    }

    for (i=0;i<2;i++)
    {
       sprintf(filename,"images/RIDER%d.bmp",i+1);
       sprite_d[i+70] = SDL_LoadBMP(filename);

       SDL_SetColorKey(sprite_d[i+70],
                    SDL_SRCCOLORKEY,
                    (Uint16) SDL_MapRGB(sprite_d[i+70]->format, 0, 0, 0));

       sprintf(filename,"images/RIDER%d-back.bmp",i+1);
       sprite_d[i+170] = SDL_LoadBMP(filename);

       SDL_SetColorKey(sprite_d[i+170],
                    SDL_SRCCOLORKEY,
                    (Uint16) SDL_MapRGB(sprite_d[i+170]->format, 0, 0, 0));
    }

   for (i=0;i<FIREMAX-FIRESPRITE;i++)
    {
       sprintf(filename,"images/FIRE%d.bmp",i+1);
       sprite_d[i+FIRESPRITE] = SDL_LoadBMP(filename);

       SDL_SetColorKey(sprite_d[i+FIRESPRITE],
                    SDL_SRCCOLORKEY,
                    (Uint16) SDL_MapRGB(sprite_d[i+FIRESPRITE]->format, 0, 0, 0)); 
    }

   for (i=0;i<BURNMAX-BURNSPRITE;i++)
    {
       sprintf(filename,"images/BURN%d.bmp",i+1);
       sprite_d[i+BURNSPRITE] = SDL_LoadBMP(filename);

       SDL_SetColorKey(sprite_d[i+BURNSPRITE],
                    SDL_SRCCOLORKEY,
                    (Uint16) SDL_MapRGB(sprite_d[i+BURNSPRITE]->format, 0, 0, 0)
);
    }




    sprite_d[PLATLEFT] = SDL_LoadBMP(PLATLEFTFILENAME);
    SDL_SetColorKey(sprite_d[PLATLEFT], 
		    SDL_SRCCOLORKEY, (Uint16) SDL_MapRGB(sprite_d[PLATLEFT]->format, 0, 0, 0));
     if (debug) printf("Successfully loaded all sprites.\n");

   sprite_d[PLATRIGHT] = SDL_LoadBMP(PLATRIGHTFILENAME);
    SDL_SetColorKey(sprite_d[PLATRIGHT],
                    SDL_SRCCOLORKEY, (Uint16) SDL_MapRGB(sprite_d[PLATRIGHT]->format, 0, 0, 0));
     if (debug) printf("Successfully loaded all sprites.\n");

   sprite_d[PLATMID] = SDL_LoadBMP(PLATMIDFILENAME);
    SDL_SetColorKey(sprite_d[PLATMID],
                    SDL_SRCCOLORKEY, (Uint16) SDL_MapRGB(sprite_d[PLATMID]->format, 0, 0, 0));
     if (debug) printf("Successfully loaded all sprites.\n");

   sprite_d[PLATFORM] = SDL_LoadBMP(PLATFORMFILENAME);
    SDL_SetColorKey(sprite_d[PLATFORM],
                    SDL_SRCCOLORKEY, (Uint16) SDL_MapRGB(sprite_d[PLATFORM]->format, 0, 0, 0));

   sprite_d[PLATVERT] = SDL_LoadBMP(PLATVERTFILENAME);       
    SDL_SetColorKey(sprite_d[PLATVERT], SDL_SRCCOLORKEY, (Uint16) SDL_MapRGB(sprite_d[PLATVERT]->format, 0, 0, 0));
 

     if (debug) printf("Successfully loaded all sprites.\n");

    return 1;

}

void move_computers()
{
  int p;

  for (p=0;p<MAX_PLAYERS;p++)
   if (player[p].computer)
    {
      if ((phase % 5) & 2) { player[p].flap = 1; player[p].ukeyon = 1;}
      if (phase % 140 > 70) { player[p].lkeyon = 0; player[p].rkeyon = 1; }
      else
      if (phase % 140 < 70) { player[p].rkeyon = 0; player[p].lkeyon = 1; }  
    }
}

int handle_keys()
{
  SDL_Event event;
  SDL_keysym keysym;
  int p;
 
  while (SDL_PollEvent(&event)) {
          keysym = event.key.keysym;

	  if (keysym.sym == SDLK_q) { return 1; } 
	
	  if (keysym.sym == SDLK_1) for (p=0;p<MAX_PLAYERS;p++) { 
		player[p].birdtype=((player[p].birdtype+1) % 10);
		player[p].initframe = player[p].birdtype*7;
		player[p].framenum = player[p].initframe;
		player[p].wingnum = player[p].birdtype*7+5;
	}

  	  for (p=0;p<MAX_PLAYERS;p++) 
/* if (!players[p].computer) */
	  {
                if ((keysym.sym == player[p].ukey) && (event.key.type == SDL_KEYDOWN) && (!player[p].ukeyon))
                        { 
                           player[p].flap = 1; 
			   player[p].ukeyon = 1;
                        }
		else if ((keysym.sym == player[p].ukey) && (event.key.type == SDL_KEYUP)) player[p].ukeyon=0;

		else if (keysym.sym == player[p].lkey) 
		{
		  if (event.key.type == SDL_KEYDOWN) player[p].lkeyon = 1; else player[p].lkeyon = 0;
		}
		else if (keysym.sym == player[p].rkey)
		{
		  if (event.key.type == SDL_KEYDOWN) player[p].rkeyon = 1; else player[p].rkeyon = 0;
		}
		
		}
   } /* while */

   move_computers(); 
	
   for (p=0;p<MAX_PLAYERS;p++)
       { 
           if (player[p].lkeyon)
		{
	     	  if (player[p].dir) player[p].dx -= accelx[player[p].birdtype]; 
			else player[p].dir=1; 
		}
	   else if (player[p].rkeyon)
                {
                  if (!player[p].dir) player[p].dx += accelx[player[p].birdtype];
			else player[p].dir=0;
		}
        }

  return 0;
}

int main(int argc, char *argv[])
{
    SDL_Rect src, dest;
    int done=0,i,fullscreen;
    clock_t start, end;
    double elapsed;
    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = 50000000;

    backfile = malloc(255);

    if (argc > 2)
     {
/*        if ((strcmp(argv[1],"-h") == 0) || (strcmp(argv[1],"--help") == 0)) */
           printf("Usage is %s [level file w/relative path].\n(No other command line options, yet.)\nThe default level file is ./levels/level.lev\n",argv[0]);
      return 0;
    }
    else 
    if (argc == 2)
    {
      strcpy(backfile,argv[1]);
    } 
    else
      strcpy(backfile,"levels/level.lev");

    if (SDL_Init(SDL_INIT_VIDEO) != 0) 
    {
	printf("Couldnt INIT SDL\n");
	return 1;
    }

    atexit(SDL_Quit);

    fullscreen = 0; // SDL_FULLSCREEN ; 

    screen = SDL_SetVideoMode(REALWIDTH, REALHEIGHT, 32, SDL_DOUBLEBUF | SDL_HWSURFACE );
    if (screen == NULL) printf("Couldnt set the video mode.");

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

    load_sprites();

    make_bitmasks();
    if (debug) printf("Made bitmasks.\n");
 
    init_players();
    if (debug) printf("Players initialized.\n");

    if (!load_level(backfile)) 
     {
       if (!load_level("levels/level.lev")) 
         {
	   printf("Can't load default level file either\n");
           return 0;
         }
     } 

    init_platforms();

    make_platform_bitmasks();

    end = clock();

    while (done != 1) {

	start = end;

	src.x = 0;
        src.y = 0;

        if (bgon) 
        {
          src.w = background->w;
          src.h = background->h;

          dest = src; 
          SDL_BlitSurface(background, &src, screen, &dest);   
        }
 	  else 
	SDL_FillRect(screen, NULL, backcolor); 

        dying_players(); /* draw before Panel */

        dest.y = PANELPLACE;
        if (panon)
        {
          src.h = panel->h;
          SDL_BlitSurface(panel, &src, screen, &dest);
        }
          else
        {
          SDL_FillRect(screen, &dest, pancolor);
        }

        draw_platforms();

        done = handle_keys();

	move_players();

	pre_scan(); 

	platform_scan();

	coll_detect();

	draw_players();
 
	SDL_Flip(screen);

/*	clean_screen(); */

        post_draw();

	nanosleep(&tim, &tim2);
	end = clock();
       if (debug) printf("CPS: %f\n", (double) (end - start)); 
 
    }

    /* Free the memory that was allocated to the bitmap. */
    if (background != NULL) SDL_FreeSurface(background);

    for (i=0;i<MAX_SPRITES;i++)
    {
       if (sprite_d[i] != NULL) 
	{
	  SDL_FreeSurface(sprite_d[i]);
          bitmask_free(bitmasks[i]);
        }
    }  

    for (i=0;i<numplatforms;i++)
	if (platforms[i].bitmasks != NULL) bitmask_free(platforms[i].bitmasks);
  
    free(backfile);

    return 0;
}
