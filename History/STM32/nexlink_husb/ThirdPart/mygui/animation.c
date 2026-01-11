#include "animation.h"
#include "main.h"
#include "gpio.h"
#include "math.h"

#ifdef __cplusplus
extern "C"
{
#endif

uint16_t randomColor565()
{
  uint8_t r = rand() & 0x1F; // 5位红色
  uint8_t g = rand() & 0x3F; // 6位绿色
  uint8_t b = rand() & 0x1F; // 5位蓝色

  return (r << 11) | (g << 5) | b;
}

uint16_t attenuateColor(uint16_t color, float attenuation) {
    // Extracting red, green, and blue components from RGB565 color
    int red = (color >> 11) & 0x1F;
    int green = (color >> 5) & 0x3F;
    int blue = color & 0x1F;

    // Applying attenuation to each color component
    red = (int)(red * attenuation);
    green = (int)(green * attenuation);
    blue = (int)(blue * attenuation);

    // Combining attenuated color components into RGB565 color
    int attenuatedColor = (red << 11) | (green << 5) | blue;

    return attenuatedColor;
}

typedef struct
{
  uint8_t Mind;
  uint8_t Circle;
  uint8_t Snowflake;
  uint8_t Meteo;
  uint8_t Planet;
  uint8_t Triangle;
  uint8_t Starwar;
  uint8_t GCircle;
} ANI_TYPE;
ANI_TYPE ani_len = { .Mind = 5, .Circle = 1,
  .Snowflake = 5,
  .Meteo = 5,
  .Planet = 5,
  .Triangle = 2,
  .Starwar = 2,
  .GCircle = 2};

void Motion_Init(void)
{
  Motion_MindInit();
  Motion_CircleInit();
  Motion_SnowflakeInit();
  Motion_MovmeteorInit();
  Motion_PlanetInit();
  Motion_TriangleInit();
  Motion_StarWarInit();
  Motion_GCFireworksInit();
	Motion_FireworkInit();
}

typedef struct
{
  float x;
  float y;
  float dirx;
  float diry;
  float r;
  uint16_t color;
} MTMOVMIND;

MTMOVMIND mtmovmind[MINDMAX];

ANI_STATUS MovMind(uint8_t Index)
{
  if(mtmovmind[Index].x <= 3)
  {
    mtmovmind[Index].x = 4;
    return IDLE;
  }
  else if(mtmovmind[Index].x >= SCR_WIDTH - 2)
  {
    mtmovmind[Index].x = SCR_WIDTH - 3;
    return IDLE;
  }
  else if(mtmovmind[Index].y <= 0)
  {
    mtmovmind[Index].y = 1;
    return IDLE;
  }
  else if(mtmovmind[Index].y >= SCR_HEIGHT - 2)
  {
    mtmovmind[Index].y = SCR_HEIGHT - 3;
    return IDLE;
  }
  else
  {
    mtmovmind[Index].x += mtmovmind[Index].dirx;
    mtmovmind[Index].y += mtmovmind[Index].diry;
  }
  return BUSY;
}

uint16_t GetMindDistanceSquare(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  return ((x0 - x1) * (x0 - x1) + (y0 - y1) * (y0 - y1));
}

void Motion_MindInit(void)
{
  int i;
  for(i = 0; i < MINDMAX; i++)
  {
    mtmovmind[i].color = randomColor565();
    __ASM("NOP");
    mtmovmind[i].x = rand() % (SCR_WIDTH - 4) + 4;
    __ASM("NOP");
    mtmovmind[i].y = rand() % SCR_HEIGHT;

    mtmovmind[i].dirx = (rand() % 30 - 15) * 0.1f;
    mtmovmind[i].diry = (rand() % 30 - 15) * 0.1f;
    if(mtmovmind[i].dirx < 0.2f && mtmovmind[i].dirx > -0.2f)
      mtmovmind[i].dirx = 0.5f;
    if(mtmovmind[i].diry < 0.2f && mtmovmind[i].diry > -0.2f)
      mtmovmind[i].diry = 0.5f;
  }
}

void Motion_Mind(void)
{
  int i, j;
  for(i = 0; i < ani_len.Mind; i++)
  {
    if(MovMind(i) == IDLE)
    {
      mtmovmind[i].color = randomColor565();
      mtmovmind[i].dirx = (rand() % 30 - 15) * 0.1f;
      mtmovmind[i].diry = (rand() % 30 - 15) * 0.1f;
      if(mtmovmind[i].dirx < 0.2f && mtmovmind[i].dirx > -0.2f)
        mtmovmind[i].dirx = 0.5f;
      if(mtmovmind[i].diry < 0.2f && mtmovmind[i].diry > -0.2f)
        mtmovmind[i].diry = 0.5f;

			if(ani_len.Mind < MINDMAX)
			{
				ani_len.Mind ++;
			}
    }
  }

  for(i = 0; i < ani_len.Mind ; i++)
  {
    for(j = 0; j < ani_len.Mind ; j++)
    {
      if((mtmovmind[i].x - mtmovmind[j].x) * (mtmovmind[i].x - mtmovmind[j].x) + (mtmovmind[i].y - mtmovmind[j].y) * (mtmovmind[i].y - mtmovmind[j].y) < 900)
      {
        AniDrawLine(mtmovmind[j].x, mtmovmind[j].y, mtmovmind[i].x, mtmovmind[i].y, RGB565_GRAY);
      }
    }
  }

  for(i = 0; i < ani_len.Mind ; i++)
    AniDrawDisc(mtmovmind[i].x, mtmovmind[i].y, 2, mtmovmind[i].color, CIRCLE_DRAW_ALL);
}

typedef struct
{
  int x;
  int y;
  float r;
  uint16_t color;
  float crt;
} MTCIRCLE;

MTCIRCLE mtcircle[CIRCLEMAX];

ANI_STATUS FucCircle(uint8_t Index)
{
  if(mtcircle[Index].crt < (mtcircle[Index].x % 6 + 16))
  {
    if(mtcircle[Index].r - mtcircle[Index].crt < 0)
    {
      mtcircle[Index].crt = 0;
      return IDLE;
    }
    AniDrawDisc(mtcircle[Index].x, mtcircle[Index].y, mtcircle[Index].r - mtcircle[Index].crt, mtcircle[Index].color, CIRCLE_DRAW_ALL);
    AniDrawCircle(mtcircle[Index].x, mtcircle[Index].y, mtcircle[Index].r + mtcircle[Index].crt, RGB565_GRAY, CIRCLE_DRAW_ALL);
    mtcircle[Index].crt += 0.4f;
    return BUSY;
  }
  else
  {
    mtcircle[Index].crt = 0;
    return IDLE;
  }
}
void Motion_CircleInit(void)
{
  int i;
  for(i = 0; i < CIRCLEMAX; i++)
  {
    mtcircle[i].x = rand() % (SCR_WIDTH - 12) + 6;
    __ASM("NOP");
    mtcircle[i].y = rand() % SCR_HEIGHT;
    __ASM("NOP");
    mtcircle[i].r = rand() % 10 + 2;
    __ASM("NOP");
    mtcircle[i].color = randomColor565();
  }
}

void Motion_Circle(void)
{
  int i;
  for(i = 0; i < ani_len.Circle; i++)
  {
    if(FucCircle(i) == IDLE)
    {
      mtcircle[i].x = rand() % (SCR_WIDTH - 12) + 6;
      __ASM("NOP");
      mtcircle[i].y = rand() % SCR_HEIGHT;
      __ASM("NOP");
      mtcircle[i].r = rand() % 10 + 2;
      __ASM("NOP");
      mtcircle[i].color = randomColor565();

      if(ani_len.Circle < CIRCLEMAX)
      {
        ani_len.Circle++;
      }
    }
  }
}
typedef struct
{
  float x;
  float y;
  float dirx;
  uint16_t color;
  uint16_t type;
} MTSNOWFLAKE;

MTSNOWFLAKE mtsnowflake[STARMAX];

ANI_STATUS MovSnowflake(uint8_t Index)
{
  mtsnowflake[Index].x += mtsnowflake[Index].dirx; // 求取两点之间的差值
  mtsnowflake[Index].y++;
  if(mtsnowflake[Index].y > SCR_HEIGHT || mtsnowflake[Index].x > SCR_WIDTH || mtsnowflake[Index].y < 0 || mtsnowflake[Index].x < 0)
    return IDLE;
  return BUSY;
}

void Motion_SnowflakeInit(void)
{
  int i;
  for(i = 0; i < STARMAX; i++)
  {
    mtsnowflake[i].x = rand() % (SCR_WIDTH - 12) + 6;
    mtsnowflake[i].y = 1;
    mtsnowflake[i].dirx = mtsnowflake[i].x + 32;
    __ASM("NOP");
    mtsnowflake[i].type = rand() % 5;
    mtsnowflake[i].color = randomColor565();
    if(ani_len.Snowflake < STARMAX)
    {
      ani_len.Snowflake++;
    }
  }
  //	ani_len.Snowflake = 5;
}

void Motion_Snowflake(void)
{
  int i, j;
  //	AniDrawLine(MovSnowflake[0][0],mtsnowflake[i].y,mtsnowflake[i].dirx,95,15);
  for(i = 0; i < ani_len.Snowflake; i++)
  {
    for(j = 0; j < mtsnowflake[i].color % 5 + 1; j++)
    {
      if(MovSnowflake(i) == IDLE)
      {
        mtsnowflake[i].x = rand() % (SCR_WIDTH - 12) + 6;
        mtsnowflake[i].y = 1;
        mtsnowflake[i].dirx = 32.0f / SCR_HEIGHT;
        __ASM("NOP");
        mtsnowflake[i].type = rand() % 5;
        mtsnowflake[i].color = randomColor565();
        if(ani_len.Snowflake < STARMAX)
        {
          ani_len.Snowflake++;
        }
      }
    }
    switch(mtsnowflake[i].type)
    {
    case 0:
      AniDrawDisc(mtsnowflake[i].x, mtsnowflake[i].y, 1, mtsnowflake[i].color, CIRCLE_DRAW_ALL);
      break;
    case 1:
      AniDrawDisc(mtsnowflake[i].x, mtsnowflake[i].y, 1, mtsnowflake[i].color, CIRCLE_DRAW_ALL);
      AniDrawDot(mtsnowflake[i].x, mtsnowflake[i].y, 0);
      break;
    case 2:
      AniDrawDot(mtsnowflake[i].x, mtsnowflake[i].y, mtsnowflake[i].color);
      break;
    case 3:
      AniDrawDot(mtsnowflake[i].x, mtsnowflake[i].y, mtsnowflake[i].color);
      AniDrawDot(mtsnowflake[i].x + 1, mtsnowflake[i].y + 1, mtsnowflake[i].color);
      break;
    case 4:
      AniDrawDot(mtsnowflake[i].x, mtsnowflake[i].y, mtsnowflake[i].color);
      AniDrawDot(mtsnowflake[i].x, mtsnowflake[i].y + 1, mtsnowflake[i].color);
      AniDrawDot(mtsnowflake[i].x + 1, mtsnowflake[i].y, mtsnowflake[i].color);
      break;
    }
  }
}

typedef struct
{
  int start;
  uint16_t color;
  float spd;
  float x;
  float y;
  int len;
  int type;
} MTMOVMETEOR;

MTMOVMETEOR mtmovmeteor[METEORMAX];
#define MOVMETEORDEF 60.0f
float movmeteorstep = MOVMETEORDEF / SCR_HEIGHT;

ANI_STATUS Movmeteor(uint8_t Index)
{
  mtmovmeteor[Index].y++;
  mtmovmeteor[Index].x += movmeteorstep;
  if(mtmovmeteor[Index].y > SCR_HEIGHT)
    return IDLE;
  else
    return BUSY;
}

void Motion_MovmeteorInit(void)
{
  int i;
  for(i = 0; i < METEORMAX; i++)
  {
    mtmovmeteor[i].start = rand() % SCR_WIDTH;
    mtmovmeteor[i].x = mtmovmeteor[i].start;
    mtmovmeteor[i].y = 0;
    mtmovmeteor[i].spd = rand() % 5 + 1; // Speed
    __ASM("NOP");
    mtmovmeteor[i].type = rand() % 4; // Type
    __ASM("NOP");
    mtmovmeteor[i].len = rand() % 15;
    ; // LineColor
    __ASM("NOP");
    mtmovmeteor[i].color = randomColor565();
    __ASM("NOP");
    if(ani_len.Meteo < METEORMAX)
    {
      ani_len.Meteo++;
    }
  }
  //	ani_len.Meteo = 5;
}

void Motion_Movmeteor(void)
{
  int i, j;
  for(i = 0; i < ani_len.Meteo; i++)
  {
    for(j = 0; j < mtmovmeteor[i].spd; j++)
    {
      if(Movmeteor(i) == IDLE)
      {
        mtmovmeteor[i].start = rand() % SCR_WIDTH;
        mtmovmeteor[i].x = mtmovmeteor[i].start;
        mtmovmeteor[i].y = 0;
        mtmovmeteor[i].spd = rand() % 5 + 1; // Speed
        __ASM("NOP");
        mtmovmeteor[i].type = rand() % 4; // Type
        __ASM("NOP");
        mtmovmeteor[i].len = rand() % 15;
        ; // LineColor
        __ASM("NOP");
        mtmovmeteor[i].color = randomColor565();
        __ASM("NOP");
        if(ani_len.Meteo < METEORMAX)
        {
          ani_len.Meteo++;
        }
      }
    }
    if(mtmovmeteor[i].len > 1)
    {
      mtmovmeteor[i].len--;
      AniDrawLine(mtmovmeteor[i].start, 0, mtmovmeteor[i].x, mtmovmeteor[i].y, RGB565_GRAY);
    }
    else if((mtmovmeteor[i].y - (mtmovmeteor[i].spd * 3)) > 0)
      AniDrawLine((mtmovmeteor[i].y - (mtmovmeteor[i].spd * 3)) * movmeteorstep + mtmovmeteor[i].start, mtmovmeteor[i].y - (mtmovmeteor[i].spd * 3), mtmovmeteor[i].x, mtmovmeteor[i].y, RGB565_GRAY);
    switch(mtmovmeteor[i].type)
    {
    case 0:
      AniDrawDot(mtmovmeteor[i].x, mtmovmeteor[i].y, mtmovmeteor[i].color);
      break;
    case 1:
      AniDrawDot(mtmovmeteor[i].x + 1, mtmovmeteor[i].y, mtmovmeteor[i].color);
      AniDrawDot(mtmovmeteor[i].x, mtmovmeteor[i].y + 1, mtmovmeteor[i].color);
      break;
    case 2:
      AniDrawDisc(mtmovmeteor[i].x, mtmovmeteor[i].y, 1, mtmovmeteor[i].color, CIRCLE_DRAW_ALL);
      break;
    case 3:
      AniDrawCircle(mtmovmeteor[i].x, mtmovmeteor[i].y, 1, mtmovmeteor[i].color, CIRCLE_DRAW_ALL);
      AniDrawDot(mtmovmeteor[i].x, mtmovmeteor[i].y, 0);
      break;
    }
  }
}

typedef struct
{
  int r;		 //行星大小
  uint16_t color;	 //行星颜色
  float spd;	 //行星转速
  float orb;	 //行星运行轨道
  float angle; //行星运行角度
  float x;	 //行星X
  float y;	 //行星Y
} MTPLANET;

MTPLANET mtplanet[PLANETMAX];

ANI_STATUS Planet(uint8_t Index)
{
  mtplanet[Index].angle += mtplanet[Index].spd;
  if(mtplanet[Index].angle >= 360)
  {
    mtplanet[Index].angle = 0;
    mtplanet[Index].orb++;
    if(mtplanet[Index].orb >= 64)
      return IDLE;
  }
  return BUSY;
}

void Motion_PlanetInit(void)
{

  int i;
  for(i = 0; i < PLANETMAX; i++)
  {
    mtplanet[i].r = rand() % 4 + 1;
    __ASM("NOP");
    mtplanet[i].color = randomColor565();
    __ASM("NOP");
    mtplanet[i].spd = (rand() % 20 + 10) / 5;
    __ASM("NOP");
    mtplanet[i].orb = rand() % 60 + 4;
    __ASM("NOP");
    mtplanet[i].angle = 0;

    mtplanet[i].x = 0;
    mtplanet[i].y = 0;

    if(ani_len.Planet < PLANETMAX)
    {
      ani_len.Planet++;
    }
  }
  //	ani_len.Planet = 5;
}

void Motion_Planet(void)
{

  int i, j;
  for(i = 0; i < ani_len.Planet; i++)
  {
    if(Planet(i) == IDLE)
    {
      mtplanet[i].r = rand() % 4 + 1; //行星大小
      __ASM("NOP");
      mtplanet[i].color = randomColor565(); //行星颜色
      __ASM("NOP");
      mtplanet[i].spd = (rand() % 20 + 10) / 5; //行星转速
      __ASM("NOP");
      mtplanet[i].orb = rand() % 60 + 4; //行星运行轨道
      __ASM("NOP");
      mtplanet[i].angle = 0; //行星运行角度

      mtplanet[i].x = 0; //行星X
      mtplanet[i].y = 0; //行星Y

      if(ani_len.Planet < PLANETMAX)
      {
        ani_len.Planet++;
      }
    }
    mtplanet[i].x = SCR_WIDTH/2 + (mtplanet[i].orb * cos(mtplanet[i].angle * PI / 180));
    mtplanet[i].y = SCR_HEIGHT/2 + (mtplanet[i].orb * sin(mtplanet[i].angle * PI / 180));

    //		AniDrawCircle(SCR_WIDTH/2,SCR_HEIGHT/2,mtplanet[i].orb,3);
  }

  for(i = 0; i < ani_len.Planet; i++)
  {
    for(j = 0; j < ani_len.Planet; j++)
    {
      //			Temp = GetMindDistanceSquare(mtmovmind[i].x,mtmovmind[i].y,mtmovmind[j].x,mtmovmind[j].y);
      if((mtplanet[i].x - mtplanet[j].x) * (mtplanet[i].x - mtplanet[j].x) + (mtplanet[i].y - mtplanet[j].y) * (mtplanet[i].y - mtplanet[j].y) < 1600)
      {
        if(mtplanet[i].r == 1 || mtplanet[j].r == 1)
          ;
        else
          AniDrawLine(mtplanet[j].x, mtplanet[j].y, mtplanet[i].x, mtplanet[i].y, RGB565_GRAY);
      }
    }
  }

  //	for(i=0;i<ani_len.Planet-1;i++)
  //	{
  //		AniDrawLine(SCR_WIDTH/2+(mtplanet[i].orb*cos(mtplanet[i].angle*PI/180)),SCR_HEIGHT/2+(mtplanet[i].orb*sin(mtplanet[i].angle*PI/180)),SCR_WIDTH/2+(MovPlanet[3][i+1]*cos(MovPlanet[4][i+1]*PI/180)),SCR_HEIGHT/2+(MovPlanet[3][i+1]*sin(MovPlanet[4][i+1]*PI/180)),RGB565_GRAY);
  //	}
  //	AniDrawLine(SCR_WIDTH/2+(MovPlanet[3][0]*cos(MovPlanet[4][0]*PI/180)),SCR_HEIGHT/2+(MovPlanet[3][0]*sin(MovPlanet[4][0]*PI/180)),SCR_WIDTH/2+(MovPlanet[3][ani_len.Planet-1]*cos(MovPlanet[4][ani_len.Planet-1]*PI/180)),SCR_HEIGHT/2+(MovPlanet[3][ani_len.Planet-1]*sin(MovPlanet[4][ani_len.Planet-1]*PI/180)),RGB565_GRAY);

  for(i = 0; i < ani_len.Planet; i++)
  {
    AniDrawDisc(mtplanet[i].x, mtplanet[i].y, mtplanet[i].r, mtplanet[i].color, CIRCLE_DRAW_ALL);
  }
}

typedef struct
{
  int r;		 //外切圆半径
  uint16_t color;	 //三角形颜色
  float spd;	 //三角形转速
  float angle; //三角形角度
  float x;	 //行星X
  float y;	 //行星Y
  float dirx;
  float diry;
  float delt1;
  float delt2;
} MTTRIANGLE;

MTTRIANGLE mttriangle[TRIANGLEMAX];

ANI_STATUS Triangle(uint8_t Index)
{
  mttriangle[Index].angle += mttriangle[Index].spd;

  mttriangle[Index].x += mttriangle[Index].dirx;
  mttriangle[Index].y += mttriangle[Index].diry;

  if(mttriangle[Index].x - mttriangle[Index].r > SCR_WIDTH || mttriangle[Index].y - mttriangle[Index].r > SCR_HEIGHT || mttriangle[Index].x + mttriangle[Index].r < 0 || mttriangle[Index].y + mttriangle[Index].r < 0)
  {
    return IDLE;
  }
  return BUSY;
}

void Motion_TriangleInit(void)
{

  int i;
  for(i = 0; i < FULLTRIANGLEMAX; i++)
  {
    mttriangle[i].r = rand() % 10 + 5;
    __ASM("NOP");
    mttriangle[i].color = randomColor565();
    __ASM("NOP");
    mttriangle[i].spd = (rand() % 20 + 10) / 15;
    __ASM("NOP");
    mttriangle[i].angle = rand() % 60; //运行角度30 150 270

    mttriangle[i].x = rand() % SCR_WIDTH;
    mttriangle[i].y = rand() % SCR_HEIGHT;

    mttriangle[i].dirx = rand() % 20 * 0.1f - 1.0f;
    if(mttriangle[i].dirx < 0.2f && mttriangle[i].dirx > -0.2f)
      mttriangle[i].dirx = 0.5f;
    mttriangle[i].diry = rand() % 20 * 0.1f - 1.0f;
    if(mttriangle[i].diry < 0.2f && mttriangle[i].diry > -0.2f)
      mttriangle[i].diry = 0.5f;
    mttriangle[i].delt1 = rand() % 60 + 90;
    mttriangle[i].delt2 = rand() % 60 + 210;

    if(ani_len.Triangle < TRIANGLEMAX)
    {
      ani_len.Triangle++;
    }
  }
  //	ani_len.Triangle = 2;
}

void Motion_Triangle(void)
{

  int i;
  for(i = 0; i < ani_len.Triangle; i++)
  {
    if(Triangle(i) == IDLE)
    {
      mttriangle[i].r = rand() % 10 + 5;
      __ASM("NOP");
      mttriangle[i].color = randomColor565();
      __ASM("NOP");
      mttriangle[i].spd = (rand() % 20 + 10) / 15;
      __ASM("NOP");
      mttriangle[i].angle = rand() % 60; //运行角度30 150 270

      mttriangle[i].x = rand() % SCR_WIDTH;
      mttriangle[i].y = rand() % SCR_HEIGHT;

      mttriangle[i].dirx = rand() % 20 * 0.1f - 1.0f;
      if(mttriangle[i].dirx < 0.2f && mttriangle[i].dirx > -0.2f)
        mttriangle[i].dirx = 0.5f;
      mttriangle[i].diry = rand() % 20 * 0.1f - 1.0f;
      if(mttriangle[i].diry < 0.2f && mttriangle[i].diry > -0.2f)
        mttriangle[i].diry = 0.5f;
      mttriangle[i].delt1 = rand() % 60 + 90;
      mttriangle[i].delt2 = rand() % 60 + 210;

      if(ani_len.Triangle < TRIANGLEMAX)
      {
        ani_len.Triangle++;
      }
      else if(ani_len.Triangle > TRIANGLEMAX)
        ani_len.Triangle = TRIANGLEMAX;
    }
  }

  for(i = 0; i < ani_len.Triangle; i++)
  {
    AniTriangle(mttriangle[i].x + (mttriangle[i].r * cos(mttriangle[i].angle * PI / 180)), mttriangle[i].y + (mttriangle[i].r * sin(mttriangle[i].angle * PI / 180)), mttriangle[i].x + (mttriangle[i].r * cos((mttriangle[i].angle + mttriangle[i].delt1) * PI / 180)), mttriangle[i].y + (mttriangle[i].r * sin((mttriangle[i].angle + mttriangle[i].delt1) * PI / 180)), mttriangle[i].x + (mttriangle[i].r * cos((mttriangle[i].angle + mttriangle[i].delt2) * PI / 180)), mttriangle[i].y + (mttriangle[i].r * sin((mttriangle[i].angle + mttriangle[i].delt2) * PI / 180)), mttriangle[i].color);
    if(mttriangle[i].r > 5)
      AniTriangle(mttriangle[i].x + ((mttriangle[i].r - 5) * cos(mttriangle[i].angle * PI / 180)), mttriangle[i].y + ((mttriangle[i].r - 5) * sin(mttriangle[i].angle * PI / 180)), mttriangle[i].x + ((mttriangle[i].r - 5) * cos((mttriangle[i].angle + mttriangle[i].delt1) * PI / 180)), mttriangle[i].y + ((mttriangle[i].r - 5) * sin((mttriangle[i].angle + mttriangle[i].delt1) * PI / 180)), mttriangle[i].x + ((mttriangle[i].r - 5) * cos((mttriangle[i].angle + mttriangle[i].delt2) * PI / 180)), mttriangle[i].y + ((mttriangle[i].r - 5) * sin((mttriangle[i].angle + mttriangle[i].delt2) * PI / 180)), RGB565_GRAY);
  }
}

void Motion_TriangleF(void)
{

  int i;
  for(i = 0; i < ani_len.Triangle; i++)
  {
    if(Triangle(i) == IDLE)
    {
      mttriangle[i].r = rand() % 10 + 5;
      __ASM("NOP");
      mttriangle[i].color = randomColor565();
      __ASM("NOP");
      mttriangle[i].spd = (rand() % 20 + 10) / 15;
      __ASM("NOP");
      mttriangle[i].angle = rand() % 60; //运行角度30 150 270

      mttriangle[i].x = rand() % SCR_WIDTH;
      mttriangle[i].y = rand() % SCR_HEIGHT;

      mttriangle[i].dirx = rand() % 20 * 0.1f - 1.0f;
      if(mttriangle[i].dirx < 0.2f && mttriangle[i].dirx > -0.2f)
        mttriangle[i].dirx = 0.5f;
      mttriangle[i].diry = rand() % 20 * 0.1f - 1.0f;
      if(mttriangle[i].diry < 0.2f && mttriangle[i].diry > -0.2f)
        mttriangle[i].diry = 0.5f;
      mttriangle[i].delt1 = rand() % 60 + 90;
      mttriangle[i].delt2 = rand() % 60 + 210;

      if(ani_len.Triangle < FULLTRIANGLEMAX)
      {
        ani_len.Triangle++;
      }
    }
  }

  for(i = 0; i < ani_len.Triangle; i++)
  {
    AniFillTriangle(mttriangle[i].x + ((mttriangle[i].r) * cos(mttriangle[i].angle * PI / 180)), mttriangle[i].y + ((mttriangle[i].r) * sin(mttriangle[i].angle * PI / 180)), mttriangle[i].x + ((mttriangle[i].r) * cos((mttriangle[i].angle + mttriangle[i].delt1) * PI / 180)), mttriangle[i].y + ((mttriangle[i].r) * sin((mttriangle[i].angle + mttriangle[i].delt1) * PI / 180)), mttriangle[i].x + ((mttriangle[i].r) * cos((mttriangle[i].angle + mttriangle[i].delt2) * PI / 180)), mttriangle[i].y + ((mttriangle[i].r) * sin((mttriangle[i].angle + mttriangle[i].delt2) * PI / 180)), mttriangle[i].color);

    AniTriangle(mttriangle[i].x + ((mttriangle[i].r + 1) * cos(mttriangle[i].angle * PI / 180)), mttriangle[i].y + ((mttriangle[i].r + 1) * sin(mttriangle[i].angle * PI / 180)), mttriangle[i].x + ((mttriangle[i].r + 1) * cos((mttriangle[i].angle + mttriangle[i].delt1) * PI / 180)), mttriangle[i].y + ((mttriangle[i].r + 1) * sin((mttriangle[i].angle + mttriangle[i].delt1) * PI / 180)), mttriangle[i].x + ((mttriangle[i].r + 1) * cos((mttriangle[i].angle + mttriangle[i].delt2) * PI / 180)), mttriangle[i].y + ((mttriangle[i].r + 1) * sin((mttriangle[i].angle + mttriangle[i].delt2) * PI / 180)), RGB565_GRAY);
    //		if(mttriangle[i].r>5)
  }
}

typedef struct
{
  int m_start_x, m_start_y;
  float m_x, m_y, m_x_factor, m_y_factor, m_size_factor, m_size;
  uint16_t m_color; //颜色
} MTSTARWAR;

MTSTARWAR mtstarwar[STARWARMAX];

ANI_STATUS MovStarWar(uint8_t Index)
{
  mtstarwar[Index].m_x_factor -= 6;
  mtstarwar[Index].m_y_factor -= 6;
  mtstarwar[Index].m_size += mtstarwar[Index].m_size / 30;
  if(mtstarwar[Index].m_x_factor < 1 || mtstarwar[Index].m_y_factor < 1)
  {
    return IDLE;
  }
  if(mtstarwar[Index].m_start_x > (SCR_WIDTH / 2) && mtstarwar[Index].m_start_y > (SCR_HEIGHT / 2))
  {
    mtstarwar[Index].m_x = (SCR_WIDTH / 2) + (SCR_WIDTH * (mtstarwar[Index].m_start_x - (SCR_WIDTH / 2)) / mtstarwar[Index].m_x_factor);
    mtstarwar[Index].m_y = (SCR_HEIGHT / 2) + (SCR_HEIGHT * (mtstarwar[Index].m_start_y - (SCR_HEIGHT / 2)) / mtstarwar[Index].m_y_factor);
  }
  else if(mtstarwar[Index].m_start_x <= (SCR_WIDTH / 2) && mtstarwar[Index].m_start_y > (SCR_HEIGHT / 2))
  {
    mtstarwar[Index].m_x = (SCR_WIDTH / 2) - (SCR_WIDTH * ((SCR_WIDTH / 2) - mtstarwar[Index].m_start_x) / mtstarwar[Index].m_x_factor);
    mtstarwar[Index].m_y = (SCR_HEIGHT / 2) + (SCR_HEIGHT * (mtstarwar[Index].m_start_y - (SCR_HEIGHT / 2)) / mtstarwar[Index].m_y_factor);
  }
  else if(mtstarwar[Index].m_start_x > (SCR_WIDTH / 2) && mtstarwar[Index].m_start_y <= (SCR_HEIGHT / 2))
  {
    mtstarwar[Index].m_x = (SCR_WIDTH / 2) + (SCR_WIDTH * (mtstarwar[Index].m_start_x - (SCR_WIDTH / 2)) / mtstarwar[Index].m_x_factor);
    mtstarwar[Index].m_y = (SCR_HEIGHT / 2) - (SCR_HEIGHT * ((SCR_HEIGHT / 2) - mtstarwar[Index].m_start_y) / mtstarwar[Index].m_y_factor);
  }
  else if(mtstarwar[Index].m_start_x <= (SCR_WIDTH / 2) && mtstarwar[Index].m_start_y <= (SCR_HEIGHT / 2))
  {
    mtstarwar[Index].m_x = (SCR_WIDTH / 2) - (SCR_WIDTH * ((SCR_WIDTH / 2) - mtstarwar[Index].m_start_x) / mtstarwar[Index].m_x_factor);
    mtstarwar[Index].m_y = (SCR_HEIGHT / 2) - (SCR_HEIGHT * ((SCR_HEIGHT / 2) - mtstarwar[Index].m_start_y) / mtstarwar[Index].m_y_factor);
  }

  if(mtstarwar[Index].m_x < 0 || (mtstarwar[Index].m_x + mtstarwar[Index].m_size - 1) >= SCR_WIDTH ||
      mtstarwar[Index].m_y < 0 || (mtstarwar[Index].m_y + mtstarwar[Index].m_size - 1) >= SCR_HEIGHT)
  {
    return IDLE;
  }
  return BUSY;
}

void Motion_StarWarInit(void)
{
  int i;
  for(i = 0; i < STARWARMAX; i++)
  {
    mtstarwar[i].m_x = mtstarwar[i].m_start_x = rand() % SCR_WIDTH;
    mtstarwar[i].m_y = mtstarwar[i].m_start_y = rand() % SCR_HEIGHT;
    mtstarwar[i].m_size = FACTOR_SIZE;
    mtstarwar[i].m_x_factor = SCR_WIDTH;
    mtstarwar[i].m_y_factor = SCR_HEIGHT;
    mtstarwar[i].m_size_factor = 1;
    mtstarwar[i].m_color = randomColor565();
  }
  //	ani_len.Starwar = 5;
}

void Motion_StarWar(void)
{
  int i;
  //	AniDrawLine(MovSnowflake[0][0],mtstarwar[i].y,mtstarwar[i].dirx,95,15);
  for(i = 0; i < ani_len.Starwar; i++)
  {
    if(MovStarWar(i) == IDLE)
    {
      mtstarwar[i].m_x = mtstarwar[i].m_start_x = rand() % SCR_WIDTH;
      mtstarwar[i].m_y = mtstarwar[i].m_start_y = rand() % SCR_HEIGHT;
      mtstarwar[i].m_size = FACTOR_SIZE;
      mtstarwar[i].m_x_factor = SCR_WIDTH;
      mtstarwar[i].m_y_factor = SCR_HEIGHT;
      mtstarwar[i].m_size_factor = 1;
      mtstarwar[i].m_color = randomColor565();
      if(ani_len.Starwar < STARWARMAX)
      {
        ani_len.Starwar++;
      }
    }
  }
  for(i = 0; i < ani_len.Starwar; i++)
  {
    if(FACTOR_SHAPE == 0)
    {
      AniDrawBox(mtstarwar[i].m_x, mtstarwar[i].m_y, mtstarwar[i].m_x + mtstarwar[i].m_size - 1, mtstarwar[i].m_y + mtstarwar[i].m_size - 1, mtstarwar[i].m_color); // draw star
    }
    else if(FACTOR_SHAPE == 1)
    {
      AniDrawDisc(mtstarwar[i].m_x, mtstarwar[i].m_y, mtstarwar[i].m_size, mtstarwar[i].m_color, CIRCLE_DRAW_ALL);
    }
  }
}

typedef struct
{
  float x;			// x坐标
  float y;			// y坐标
  float dirx;			// x运动差
  float diry;			// y运动差
  float size;			//大小
  float r;			//半径
  float a;			//角度
  float maxh;			//高度
  float radius;		//公转半径
  float speed;		//速率
  bool state;			//动画状态
  double Stime;		//计数器
  uint16_t linecolor; //线条颜色
  uint16_t color;		//点位颜色
} MTGCIRCLE;

MTGCIRCLE GCircle[GCIRCLEMAX];
void RefresGCircle(int i)
{
  GCircle[i].state = true;
  GCircle[i].speed = 0.01;

  GCircle[i].Stime = 0;

  GCircle[i].r = 3 + rand() % 3;

  GCircle[i].x = GCircle[i].r + rand() % SCR_WIDTH;

  GCircle[i].y = SCR_HEIGHT + GCircle[i].r;
  GCircle[i].dirx = 0;
  GCircle[i].diry = 0;
  GCircle[i].maxh = (SCR_HEIGHT / 2) + rand() % 60;
  GCircle[i].color = randomColor565();
}

void Motion_GCFireworksInit()
{
  for(int i = 0; i < GCIRCLEMAX; i++)
  {
    RefresGCircle(i);
  }
}

/*烟花动画*/
void Motion_GCFireworks(void)
{
  for(int i = 0; i < ani_len.Circle; i++)
  {
    if(GCircle[i].state == true && SCR_HEIGHT - GCircle[i].y < GCircle[i].maxh / 2)
    {

      GCircle[i].Stime++;

      GCircle[i].diry = GCircle[i].Stime * GCircle[i].Stime * GCircle[i].speed;

      GCircle[i].y -= GCircle[i].diry;

      int s = 1;

      for(double a = 0; a < GCircle[i].diry; a++)
      {
        AniDrawDisc(GCircle[i].x, GCircle[i].y + a * 5, GCircle[i].r - s, RGB565_GRAY, CIRCLE_DRAW_ALL); //半空
        s++;
      }
    }
    else if(GCircle[i].state == true && SCR_HEIGHT - GCircle[i].y > GCircle[i].maxh / 2)
    {
      GCircle[i].Stime--;

      GCircle[i].diry = GCircle[i].Stime * GCircle[i].Stime * GCircle[i].speed;

      GCircle[i].y -= GCircle[i].diry;

      int s = 1;

      for(double a = 0; a < GCircle[i].diry - 1; a++)
      {
        AniDrawDisc(GCircle[i].x, GCircle[i].y + a, GCircle[i].r - s, GCircle[i].color, CIRCLE_DRAW_ALL);
        s++;
      }
    }
    if(GCircle[i].state == true && SCR_HEIGHT - GCircle[i].y >= GCircle[i].maxh)
    {
      GCircle[i].Stime = 0;
      GCircle[i].state = false;
    }
    else if(GCircle[i].state == false)
    {
      if(GCircle[i].dirx + GCircle[i].r > SCR_HEIGHT || GCircle[i].diry + GCircle[i].r > SCR_WIDTH)
      {
        RefresGCircle(i);
        if(ani_len.Circle < GCIRCLEMAX)
        {
          ani_len.Circle++;
        }
        return;
      }
      GCircle[i].Stime += 12;
      GCircle[i].dirx = GCircle[i].x + cos(PI / 2 + PI / 4) * GCircle[i].Stime;
      GCircle[i].diry = GCircle[i].y + sin(PI / 2 + PI / 4) * GCircle[i].Stime;
    }
  }
}

#define FIREWORKFLASHL 10
#define FIREWORKMAX 1

#define FIREWORKSINGLE 20
#define FIREWORKTUOYING 10
uint8_t FireworkNum = 1;
typedef struct
{
	float spd[FIREWORKSINGLE];				   // 减速
	float xspd[FIREWORKSINGLE];			   // 减速
	float yspd[FIREWORKSINGLE];			   // 匀速
	float x[FIREWORKSINGLE][FIREWORKTUOYING]; // X
	float y[FIREWORKSINGLE][FIREWORKTUOYING]; // Y
	float yup[FIREWORKTUOYING];
	int states; // 0,上升；2,爆炸下落
	int runtimes;
	int upruntimes;
	uint16_t color[FIREWORKSINGLE]; //

} MTFIREWORK;

typedef struct
{
	int ange;						// 0-360
	int r[FIREWORKSINGLE];		//
	uint16_t color[FIREWORKSINGLE]; //

} MTFIREWORKC;

MTFIREWORK mtfirework[FIREWORKMAX];
MTFIREWORKC mtfireworkc[FIREWORKMAX];
ANI_STATUS Firework(uint8_t Index)
{
	if (mtfirework[Index].states == 2)
	{
		for (int i = 0; i < FIREWORKSINGLE; i++)
		{
			for (int j = FIREWORKTUOYING - 2; j >= 0; j--)
			{
				mtfirework[Index].x[i][j + 1] = mtfirework[Index].x[i][j];
				mtfirework[Index].y[i][j + 1] = mtfirework[Index].y[i][j];
			}
			mtfirework[Index].x[i][0] += mtfirework[Index].xspd[i];
			mtfirework[Index].y[i][0] += mtfirework[Index].yspd[i];
			mtfirework[Index].yspd[i] = mtfirework[Index].yspd[i] + 0.025f * mtfirework[Index].runtimes;
		}

		if (mtfirework[Index].runtimes++ > 80)
		{
			return IDLE;
		}
	}
	else
	{
		for (int j = FIREWORKTUOYING - 2; j >= 0; j--)
		{
			mtfirework[Index].yup[j + 1] = mtfirework[Index].yup[j];
		}
		mtfirework[Index].yup[0] -= (mtfirework[Index].yup[0] - mtfirework[Index].y[0][0]) * 0.2f + 0.1f;

		if (mtfirework[Index].yup[0] < mtfirework[Index].y[0][0])
			mtfirework[Index].states = 2;

		if (mtfirework[Index].upruntimes++ > 100)
		{
			return IDLE;
		}
	}
	return BUSY;
}

void Motion_FireworkInit(void)
{
	for (int p = 0; p < FIREWORKMAX; p++)
	{
		int x = rand() % (SCR_WIDTH / 2) + SCR_WIDTH / 4;
		int y = rand() % (SCR_HEIGHT / 2) + SCR_HEIGHT / 6;
		for (int i = 0; i < FIREWORKSINGLE; i++)
		{
			for (int j = 1; j < FIREWORKTUOYING; j++)
			{
				mtfirework[p].x[i][j] = 0;
				mtfirework[p].y[i][j] = 0;
			}
			mtfirework[p].x[i][0] = x;
			mtfirework[p].y[i][0] = y;
			mtfirework[p].yup[i] = SCR_HEIGHT;
			mtfireworkc[p].r[i] = rand() % 3 + 1;
			mtfireworkc[p].color[i] = randomColor565();
			mtfireworkc[p].ange = rand() % 360;
			mtfirework[p].spd[i] = (rand() % 1200) * 0.005 + 0.04;

			mtfirework[p].xspd[i] = cos(mtfireworkc[p].ange) * mtfirework[p].spd[i];
			mtfirework[p].yspd[i] = sin(mtfireworkc[p].ange) * mtfirework[p].spd[i];
		}
		mtfirework[p].states = 0;
		mtfirework[p].runtimes = 0;
		mtfirework[p].upruntimes = 0;
	}
}

void Motion_Firework(void)
{
	for (int p = 0; p < FireworkNum; p++)
	{
		if (Firework(p) == IDLE)
		{
			int x = rand() % (SCR_WIDTH / 2) + SCR_WIDTH / 4;
			int y = rand() % (SCR_HEIGHT / 2) + SCR_HEIGHT / 6;
			for (int i = 0; i < FIREWORKSINGLE; i++)
			{
				for (int j = 1; j < FIREWORKTUOYING; j++)
				{
					mtfirework[p].x[i][j] = 0;
					mtfirework[p].y[i][j] = 0;
				}
				mtfirework[p].x[i][0] = x;
				mtfirework[p].y[i][0] = y;
				mtfirework[p].yup[i] = SCR_HEIGHT;
				mtfireworkc[p].r[i] = rand() % 3 + 1;
				mtfireworkc[p].color[i] = randomColor565();
				mtfireworkc[p].ange = rand() % 360;
				mtfirework[p].spd[i] = (rand() % 1200) * 0.005 + 0.04;

				mtfirework[p].xspd[i] = cos(mtfireworkc[p].ange) * mtfirework[p].spd[i];
				mtfirework[p].yspd[i] = sin(mtfireworkc[p].ange) * mtfirework[p].spd[i];
			}
			mtfirework[p].states = 0;
			mtfirework[p].runtimes = 0;
			mtfirework[p].upruntimes = 0;
			if (FireworkNum < FIREWORKMAX)
			{
				FireworkNum++;
			}
		}
		if (mtfirework[p].states == 2)
		{
			for (int i = 0; i < FIREWORKSINGLE; i++)
			{
				for (int j = FIREWORKTUOYING - 1; j > 0; j--)
				{
					if(rand()%FIREWORKFLASHL)
						AniDrawDisc(mtfirework[p].x[i][j], mtfirework[p].y[i][j], mtfireworkc[p].r[i] - (j >> 4), attenuateColor(mtfireworkc[p].color[i], (((float)(FIREWORKTUOYING-1-j))/FIREWORKTUOYING)*0.6f+0.4f), CIRCLE_DRAW_ALL);
				}
			}
		}
		else
		{
			for (int j = FIREWORKTUOYING - 1; j > 0; j--)
			{
				if(rand()%FIREWORKFLASHL)
					AniDrawDisc(mtfirework[p].x[0][0], mtfirework[p].yup[j], 3 - j / 20, RGB565_GRAY, CIRCLE_DRAW_ALL);
			}
		}
	}
}


#ifdef __cplusplus
}
#endif
