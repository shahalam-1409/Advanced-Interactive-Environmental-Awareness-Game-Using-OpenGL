#include <GL/gl.h>
#include <GL/glut.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// Window
const int WIN_W = 900;
const int WIN_H = 600;

//  Ortho world coords
const float WORLD_W = 500.0f;
const float WORLD_H = 700.0f;

//  Day/Night cycle
float dayTime    = 0.0f;
float daySpeed   = 0.0003f;

//  Stars
struct Star
{
    float x, y, size, twinkle;
};
Star stars[200];
float starPhase = 0;

//  Particles
struct Particle
{
    float x, y, vx, vy, life, maxLife, size;
    float r, g, b;
};
Particle smokes[120];
int smokeCount = 0;

//  Explosion Particle
struct ExpParticle
{
    float x, y, vx, vy, life, maxLife, size;
    float r, g, b;
};
ExpParticle expParticles[400];
int expCount = 0;
bool explosionActive = false;
int  explosionTimer  = 0;
float explosionShake = 0;

//  O2 Particles
struct O2Particle
{
    float x, y, vy, life, maxLife, alpha;
};
O2Particle o2Parts[80];
int o2Count = 0;
float o2SpawnTimer = 0;

//  Vehicles
float tx = -400, bx = -510;

//  Player
float playerX = 250, playerY = 90;
float walkAngle = 0;
float playerDir = 1;

//  Wind / Birds
float windAngle = 0;
float wing = 0;
float birdX = 0;

//  Clouds
float cloudShift1 = 0, cloudShift2 = -200;

//  Water
float wave = 0;

//  Rain / Lightning
bool  isRain = false;
float rainOffset = 0;
bool  lightning = false;
int   lightningTimer = 0;
float shakeX = 0, shakeY = 0;
int   shakeFrames = 0;

// Traffic Light
bool isGreen = true;
int  lightTimer = 0;

//  Game State
int   level      = 1;
int   timeLeft   = 30;
bool  gameOver   = false;
bool  gameWin    = false;
bool  timerStart = false;
int   score      = 0;

// ─── Level flags
int   treeCount  = 0;
bool  riverClean = false;
bool  smokeOff   = false;
bool  parkBuilt  = false;
float treeX[100], treeY[100];


float hudPulse = 0;


//  MATH HELPERS

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}
float clamp(float v, float lo, float hi)
{
    return v<lo?lo:v>hi?hi:v;
}


//  PRIMITIVE HELPERS

void circle(float rx, float ry, float cx, float cy)
{
    glBegin(GL_POLYGON);
    glVertex2f(cx, cy);
    for(int i=0; i<=360; i++)
    {
        float a=i*3.14159f/180.0f;
        glVertex2f(cx+rx*cosf(a), cy+ry*sinf(a));
    }
    glEnd();
}

void drawCircleMidpoint(int xc,int yc,int r)
{
    int x=0,y=r,p=1-r;
    glBegin(GL_POINTS);
    while(x<=y)
    {
        glVertex2i(xc+x,yc+y);
        glVertex2i(xc-x,yc+y);
        glVertex2i(xc+x,yc-y);
        glVertex2i(xc-x,yc-y);
        glVertex2i(xc+y,yc+x);
        glVertex2i(xc-y,yc+x);
        glVertex2i(xc+y,yc-x);
        glVertex2i(xc-y,yc-x);
        p = (p<0) ? p+2*x+3 : (y--,p+2*(x-y)+5);
        x++;
    }
    glEnd();
}

void drawLineDDA(float x1,float y1,float x2,float y2)
{
    float dx=x2-x1, dy=y2-y1;
    float steps=fabs(dx)>fabs(dy)?fabs(dx):fabs(dy);
    if(steps==0) return;
    float xi=dx/steps, yi=dy/steps, x=x1, y=y1;
    glBegin(GL_POINTS);
    for(int i=0; i<=steps; i++)
    {
        glVertex2f(x,y);
        x+=xi;
        y+=yi;
    }
    glEnd();
}

void drawLineBresenham(int x1,int y1,int x2,int y2)
{
    int dx=abs(x2-x1),dy=abs(y2-y1);
    int sx=(x1<x2)?1:-1, sy=(y1<y2)?1:-1, err=dx-dy;
    glBegin(GL_POINTS);
    while(true)
    {
        glVertex2i(x1,y1);
        if(x1==x2&&y1==y2) break;
        int e2=2*err;
        if(e2>-dy)
        {
            err-=dy;
            x1+=sx;
        }
        if(e2< dx)
        {
            err+=dx;
            y1+=sy;
        }
    }
    glEnd();
}

// CENTERED TEXT
void drawTextCentered(float cx, float y, const char* text, void* font = GLUT_BITMAP_HELVETICA_18)
{
    int w = 0;
    for(int i = 0; text[i]; i++)
        w += glutBitmapWidth(font, text[i]);
    glRasterPos2f(cx - w * 0.5f, y);
    for(int i = 0; text[i]; i++)
        glutBitmapCharacter(font, text[i]);
}

// ─── LARGE / BOLD
void drawTextBold(float cx, float y, const char* text, void* font = GLUT_BITMAP_HELVETICA_18)
{
    int w = 0;
    for(int i = 0; text[i]; i++)
        w += glutBitmapWidth(font, text[i]);
    float startX = cx - w * 0.5f;
    // Draw shadow
    float prevR, prevG, prevB;

    glColor3ub(0, 0, 0);
    glRasterPos2f(startX + 1, y - 1);
    for(int i = 0; text[i]; i++)
        glutBitmapCharacter(font, text[i]);

    glRasterPos2f(startX, y);
    for(int i = 0; text[i]; i++)
        glutBitmapCharacter(font, text[i]);

    glRasterPos2f(startX + 1, y);
    for(int i = 0; text[i]; i++)
        glutBitmapCharacter(font, text[i]);
}

void drawText(float x, float y, const char* text, void* font=GLUT_BITMAP_HELVETICA_18)
{
    glRasterPos2f(x,y);
    for(int i=0; text[i]; i++) glutBitmapCharacter(font, text[i]);
}


//  INIT STARS

void initStars()
{
    srand(42);
    for(int i=0; i<200; i++)
    {
        stars[i].x      = (float)(rand()%500);
        stars[i].y      = 200 + rand()%500;
        stars[i].size   = 0.5f + (rand()%3)*0.5f;
        stars[i].twinkle= (float)(rand()%628)/100.0f;
    }
}

void spawnSmoke(float cx, float baseY)
{
    if(smokeCount>=120||smokeOff) return;
    Particle& p = smokes[smokeCount++];
    p.x       = cx + (rand()%10-5);
    p.y       = baseY;
    p.vx      = (rand()%10-5)*0.05f;
    p.vy      = 0.4f + (rand()%4)*0.1f;
    p.maxLife = 80.0f + rand()%40;
    p.life    = p.maxLife;
    p.size    = 8.0f + rand()%8;
    p.r = p.g = p.b = 0.5f;
}


//  EXPLOSION SYSTEM

void triggerExplosion()
{
    if(explosionActive) return;
    explosionActive = true;
    explosionTimer  = 0;
    expCount = 0;


    float origins[][2] =
    {
        {60,280},{140,280},{240,330},{420,350},  // buildings
        {100,97},{300,97},{450,97},              // ground
        {250,97},{350,97},{150,97}
    };
    for(int o=0; o<10; o++)
    {
        for(int i=0; i<40&&expCount<400; i++)
        {
            ExpParticle& p = expParticles[expCount++];
            p.x = origins[o][0] + (rand()%40-20);
            p.y = origins[o][1] + (rand()%30-10);
            float angle = (rand()%360)*3.14159f/180.0f;
            float speed = 0.5f + (rand()%30)*0.3f;
            p.vx = cosf(angle)*speed;
            p.vy = sinf(angle)*speed;
            p.maxLife = 60.0f + rand()%80;
            p.life    = p.maxLife;
            p.size    = 4.0f + rand()%12;

            int col = rand()%3;
            if(col==0)
            {
                p.r=1.0f;
                p.g=0.1f+rand()%3*0.1f;
                p.b=0.0f;
            }
            else if(col==1)
            {
                p.r=1.0f;
                p.g=0.5f+rand()%3*0.1f;
                p.b=0.0f;
            }
            else
            {
                p.r=1.0f;
                p.g=0.9f;
                p.b=0.2f;
            }
        }
    }
    explosionShake = 30.0f;
}

void updateExplosion()
{
    if(!explosionActive) return;

    explosionTimer++;
    if(explosionTimer < 120 && explosionTimer % 8 == 0 && expCount < 380)
    {
        float cx = 80.0f + rand()%340;
        float cy = 180.0f + rand()%180;
        float speed = 1.0f + (rand()%30)*0.5f;  // stronger blast
        for(int i=0; i<15&&expCount<800; i++)
        {
            ExpParticle& p = expParticles[expCount++];
            p.x = cx;
            p.y = cy;
            float angle = (rand()%360)*3.14159f/180.0f;
            float speed = 0.3f + rand()%20*0.2f;
            p.vx = cosf(angle)*speed;
            p.vy = sinf(angle)*speed;
            p.maxLife = 40.0f+rand()%50;
            p.life=p.maxLife;
            p.size = 3.0f+rand()%10;
            p.r=1.0f;
            p.g=0.3f+rand()%6*0.1f;
            p.b=0.0f;
        }
    }

    if(explosionShake > 0.1f) explosionShake *= 0.88f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for(int i=0; i<expCount;)
    {
        ExpParticle& p = expParticles[i];
        p.x  += p.vx;
        p.y  += p.vy;
        p.vy -= 0.08f; // gravity
        p.vx *= 0.98f;
        p.life--;
        if(p.life<=0)
        {
            expParticles[i]=expParticles[--expCount];
            continue;
        }
        float t   = p.life / p.maxLife;
        float a   = t * 0.85f;

        float r   = p.r;
        float g   = p.g * t;
        float b   = p.b;
        glColor4f(r, g, b, a);
        circle(p.size*t, p.size*t, p.x, p.y);

        if(t > 0.5f)
        {
            glColor4f(1.0f, 1.0f, 0.8f, a*0.4f);
            circle(p.size*t*0.4f, p.size*t*0.4f, p.x, p.y);
        }
        i++;
    }
    glDisable(GL_BLEND);


    if(explosionTimer > 30)
    {
        float smokeAlpha = clamp((explosionTimer-30)*0.004f, 0, 0.6f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.05f,0.05f,0.05f, smokeAlpha);
        glBegin(GL_QUADS);
        glVertex2f(0,0);
        glVertex2f(500,0);
        glVertex2f(500,700);
        glVertex2f(0,700);
        glEnd();
        glDisable(GL_BLEND);
    }
}


//  O2 OXYGEN SYSTEM

void spawnO2()
{
    if(o2Count >= 80) return;
    O2Particle& p = o2Parts[o2Count++];

    p.x      = 50.0f + rand()%400;
    p.y      = 180.0f; // ground level
    p.vy     = 0.3f + (rand()%5)*0.1f;
    p.maxLife= 200.0f + rand()%100;
    p.life   = p.maxLife;
    p.alpha  = 0.0f;
}

void updateO2()
{
    if(!gameWin) return;

    o2SpawnTimer++;
    if(o2SpawnTimer > 12)
    {
        spawnO2();
        o2SpawnTimer = 0;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for(int i=0; i<o2Count;)
    {
        O2Particle& p = o2Parts[i];
        p.y    += p.vy;
        p.x    += sinf(p.life * 0.05f) * 0.4f;
        p.life--;

        float t = p.life / p.maxLife;
        if(t > 0.8f)      p.alpha = (1.0f-t)*5.0f;
        else if(t < 0.2f) p.alpha = t*5.0f;
        else               p.alpha = 1.0f;

        if(p.life<=0 || p.y>700)
        {
            o2Parts[i]=o2Parts[--o2Count];
            continue;
        }

        float a = p.alpha * 0.85f;


        glColor4f(0.3f, 0.9f, 0.4f, a * 0.35f);
        circle(12, 12, p.x, p.y);
        glColor4f(0.5f, 1.0f, 0.6f, a * 0.6f);
        circle(10, 10, p.x, p.y);

        glColor4f(1.0f, 1.0f, 1.0f, a * 0.5f);
        circle(3, 3, p.x+3, p.y+3);


        glColor4f(0.0f, 0.4f, 0.0f, a);

        drawTextCentered(p.x, p.y - 4, "O2", GLUT_BITMAP_HELVETICA_12);

        i++;
    }
    glDisable(GL_BLEND);
}


//  SKY

void drawSky()
{
    float d = clamp(sinf(dayTime * 3.14159f * 2.0f)*0.5f+0.5f, 0, 1);
    float tr = lerp(0.02f, 0.11f, d);
    float tg = lerp(0.02f, 0.56f, d);
    float tb = lerp(0.15f, 1.00f, d);
    float hr = lerp(0.4f,  1.0f, d);
    float hg = lerp(0.2f,  0.9f, d);
    float hb = lerp(0.05f, 0.5f, d);
    float gt = 1.0f - clamp(fabsf(sinf(dayTime*3.14159f*2))*3.0f, 0, 1);
    hr = lerp(hr, 1.0f, gt*0.5f);
    hg = lerp(hg, 0.5f, gt*0.3f);
    hb = lerp(hb, 0.1f, gt*0.4f);

    glBegin(GL_QUADS);
    glColor3f(tr,tg,tb);
    glVertex2f(0,700);
    glVertex2f(500,700);
    glColor3f(hr,hg,hb);
    glVertex2f(500,180);
    glVertex2f(0,180);
    glEnd();

    if(isRain)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.15f, 0.15f, 0.25f, 0.55f);
        glBegin(GL_QUADS);
        glVertex2f(0,180);
        glVertex2f(500,180);
        glVertex2f(500,700);
        glVertex2f(0,700);
        glEnd();
        glDisable(GL_BLEND);
    }

    if(gameOver)
    {
        glColor3ub(80,20,20); // red dark sky
    }
}


//  STARS

void drawStars()
{
    float d = clamp(sinf(dayTime*3.14159f*2.0f)*0.5f+0.5f, 0, 1);
    float alpha = clamp(1.0f - d*2.5f, 0, 1);
    if(alpha < 0.01f) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    starPhase += 0.02f;
    for(int i=0; i<200; i++)
    {
        float tw = 0.6f + 0.4f*sinf(starPhase + stars[i].twinkle);
        glColor4f(1,1,1, alpha*tw);
        glPointSize(stars[i].size);
        glBegin(GL_POINTS);
        glVertex2f(stars[i].x, stars[i].y);
        glEnd();
    }
    glPointSize(1);
    glDisable(GL_BLEND);
}


//  SUN + MOON

void drawSun()
{
    float d = clamp(sinf(dayTime*3.14159f*2)*0.5f+0.5f, 0, 1);
    float cy = 400 + d*250;
    float cx = 440;

    if(d > 0.1f){

        // 🌟 SUN RAYS
        glColor3ub(255,215,0);
        for(int i=0;i<360;i+=20){
            float a=i*3.14159f/180;
            glBegin(GL_LINES);
            glVertex2f(cx+30*cosf(a), cy+30*sinf(a));
            glVertex2f(cx+50*cosf(a), cy+50*sinf(a));
            glEnd();
        }


        glColor3ub(255,255,0);
        circle(26,28,cx,cy);

        glColor3ub(255,200,0);
        drawCircleMidpoint((int)cx,(int)cy,26);


        if(isRain){
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


            glColor4f(0.0f, 0.0f, 0.0f, 0.8f);


            circle(28,30,cx,cy);

            glDisable(GL_BLEND);
        }
    }


    float moonAlpha = clamp(1.0f - d*2.5f, 0, 1);
    if(moonAlpha > 0.02f){
        float mcy = 620 - (1-d)*250;

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(0.95f,0.95f,0.85f,moonAlpha);
        circle(22,22, 440, mcy);

        glColor4f(0.04f,0.04f,0.14f,moonAlpha*0.9f);
        circle(20,22, 450, mcy);

        glDisable(GL_BLEND);
    }
}



//  MOUNTAINS

void drawMountains()
{
    glColor3ub(60,80,120);
    glBegin(GL_POLYGON);
    glVertex2f(0,180);
    glVertex2f(0,300);
    glVertex2f(60,420);
    glVertex2f(130,310);
    glVertex2f(190,380);
    glVertex2f(260,290);
    glVertex2f(340,410);
    glVertex2f(420,300);
    glVertex2f(500,350);
    glVertex2f(500,180);
    glEnd();
    glColor3ub(40,55,85);
    glBegin(GL_POLYGON);
    glVertex2f(0,180);
    glVertex2f(0,260);
    glVertex2f(80,340);
    glVertex2f(150,270);
    glVertex2f(230,360);
    glVertex2f(310,270);
    glVertex2f(390,340);
    glVertex2f(500,270);
    glVertex2f(500,180);
    glEnd();
    glColor3ub(220,230,240);
    auto peak=[](float px,float py,float w,float h)
    {
        glBegin(GL_TRIANGLES);
        glVertex2f(px-w,py);
        glVertex2f(px+w,py);
        glVertex2f(px,py+h);
        glEnd();
    };
    peak(60,420,20,25);
    peak(260,290,18,22);
    peak(420,300,15,20);
}


//  CLOUDS

void cloud(float cx, float cy, float scale, float alpha)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1,1,1,alpha);
    circle(scale*22, scale*14, cx,      cy);
    circle(scale*16, scale*11, cx-scale*28, cy-scale*3);
    circle(scale*16, scale*11, cx+scale*28, cy-scale*3);
    circle(scale*12, scale*9,  cx-scale*46, cy-scale*8);
    circle(scale*12, scale*9,  cx+scale*46, cy-scale*8);
    glDisable(GL_BLEND);
}

void drawClouds()
{
    cloudShift1 += 0.18f;
    cloudShift2 += 0.10f;
    if(cloudShift1 > 700) cloudShift1=-700;
    if(cloudShift2 > 700) cloudShift2=-700;
    cloud(90  + cloudShift2, 620, 1.0f, 0.55f);
    cloud(270 + cloudShift2, 590, 0.9f, 0.50f);
    cloud(430 + cloudShift2, 640, 0.8f, 0.45f);
    cloud(150 + cloudShift1, 600, 1.1f, 0.90f);
    cloud(370 + cloudShift1, 575, 0.95f,0.85f);
}


//  RAIN + LIGHTNING

void drawRain()
{
    if(!isRain) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.6f,0.6f,1.0f,0.55f);
    glLineWidth(1.0f);
    for(int i=0; i<200; i++)
    {
        float x=(float)(rand()%500);
        float y=(float)(rand()%700) - rainOffset;
        glBegin(GL_LINES);
        glVertex2f(x,y);
        glVertex2f(x+4,y-18);
        glEnd();
    }
    rainOffset+=2;
    if(rainOffset>700) rainOffset=0;
    glDisable(GL_BLEND);
}

void drawLightning()
{
    if(!isRain) return;
    lightningTimer++;
    if(lightningTimer>150)
    {
        lightning=true;
        lightningTimer=0;
    }
    if(lightning)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(1,1,1,0.4f);
        glBegin(GL_QUADS);
        glVertex2f(0,0);
        glVertex2f(500,0);
        glVertex2f(500,700);
        glVertex2f(0,700);
        glEnd();
        glDisable(GL_BLEND);
        glColor3ub(255,255,120);
        glLineWidth(2.0f);
        float bx2=100+rand()%300, by=700;
        glBegin(GL_LINE_STRIP);
        while(by>200)
        {
            glVertex2f(bx2,by);
            bx2+=(rand()%30-15);
            by-=20+rand()%20;
        }
        glEnd();
        glLineWidth(1.0f);
        shakeFrames=6;
        lightning=false;
    }
}


//  GROUND + ROAD

void drawGround()
{
    glColor3ub(34,139,34);
    glBegin(GL_QUADS);
    glVertex2f(0,0);
    glVertex2f(500,0);
    glVertex2f(500,180);
    glVertex2f(0,180);
    glEnd();
    glColor3ub(60,160,60);
    glBegin(GL_QUADS);
    glVertex2f(0,155);
    glVertex2f(500,155);
    glVertex2f(500,180);
    glVertex2f(0,180);
    glEnd();
}

void road()
{
    glColor3ub(80,80,80);
    glBegin(GL_QUADS);
    glVertex2f(0,0);
    glVertex2f(500,0);
    glVertex2f(500,52);
    glVertex2f(0,52);
    glEnd();
    glColor3ub(200,200,200);
    glBegin(GL_QUADS);
    glVertex2f(0,50);
    glVertex2f(500,50);
    glVertex2f(500,55);
    glVertex2f(0,55);
    glEnd();
    glColor3ub(255,255,255);
    glLineWidth(2.0f);
    for(int x=0; x<500; x+=50)
    {
        drawLineDDA((float)x,25,(float)x+35,25);
    }
    glLineWidth(1.0f);
}


//  RIVER

void river()
{
    if(riverClean) glColor3ub(30,144,255);
    else           glColor3ub(100,60,20);
    glBegin(GL_QUADS);
    glVertex2f(0,55);
    glVertex2f(500,55);
    glVertex2f(500,140);
    glVertex2f(0,140);
    glEnd();
    wave+=1.5f;
    if(riverClean)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        for(int i=0; i<500; i+=30)
        {
            float wy=100+6*sinf((i+wave)*0.04f);
            glColor4f(0.7f,0.9f,1.0f,0.45f);
            circle(14,4,(float)i,wy);
        }
        glDisable(GL_BLEND);
    }
    else
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.3f,0.2f,0.0f,0.4f);
        for(int i=0; i<500; i+=60)
            circle(10,6,(float)i+20*sinf(wave*0.01f+i),95);
        glDisable(GL_BLEND);
    }
    // Centered "River" label
    glColor3ub(255,255,255);
    drawTextCentered(250, 118, "River", GLUT_BITMAP_HELVETICA_12);
}


//  BRIDGE

void bridge()
{
    float cX=230, w=36;
    glColor3ub(50,50,50);
    glBegin(GL_POLYGON);
    for(float y=50; y<=170; y+=1)
    {
        float x=cX+22*sinf((y-50)*3.14159f/120);
        glVertex2f(x,y);
    }
    for(float y=170; y>=50; y-=1)
    {
        float x=cX+w+22*sinf((y-50)*3.14159f/120);
        glVertex2f(x,y);
    }
    glEnd();
    glColor3ub(255,215,0);
    glLineWidth(2.0f);
    glBegin(GL_LINE_STRIP);
    for(float y=50; y<=170; y++)
        glVertex2f(cX+22*sinf((y-50)*3.14159f/120),y);
    glEnd();
    glBegin(GL_LINE_STRIP);
    for(float y=50; y<=170; y++)
        glVertex2f(cX+w+22*sinf((y-50)*3.14159f/120),y);
    glEnd();
    glColor3ub(180,180,180);
    for(float y=55; y<=165; y+=10)
    {
        float x1=cX+22*sinf((y-50)*3.14159f/120);
        float x2=cX+w+22*sinf((y-50)*3.14159f/120);
        drawLineBresenham((int)x1-6,(int)y,(int)x1-6,(int)y+8);
        drawLineBresenham((int)x2+6,(int)y,(int)x2+6,(int)y+8);
    }
    glLineWidth(1.0f);
}


//  BUILDINGS

void house(float x)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0,0,0.15f);
    glBegin(GL_QUADS);
    glVertex2f(x+5,175);
    glVertex2f(x+87,175);
    glVertex2f(x+85,180);
    glVertex2f(x+5,180);
    glEnd();
    glDisable(GL_BLEND);
    glColor3ub(255,182,193);
    glBegin(GL_QUADS);
    glVertex2f(x,180);
    glVertex2f(x+80,180);
    glVertex2f(x+80,260);
    glVertex2f(x,260);
    glEnd();
    glColor3ub(200,140,150);
    for(int row=0; row<4; row++)
    {
        float y=190+row*18;
        glBegin(GL_LINES);
        glVertex2f(x,y);
        glVertex2f(x+80,y);
        glEnd();
        for(int col=0; col<4; col++)
        {
            float bx2=x+col*20+(row%2)*10;
            glBegin(GL_LINES);
            glVertex2f(bx2,y);
            glVertex2f(bx2,y+18);
            glEnd();
        }
    }
    glColor3ub(165,42,42);
    glBegin(GL_TRIANGLES);
    glVertex2f(x-10,260);
    glVertex2f(x+90,260);
    glVertex2f(x+40,315);
    glEnd();
    glColor3ub(130,25,25);
    glBegin(GL_LINES);
    glVertex2f(x-10,260);
    glVertex2f(x+40,315);
    glVertex2f(x+90,260);
    glVertex2f(x+40,315);
    glEnd();
    glColor3ub(100,50,10);
    glBegin(GL_QUADS);
    glVertex2f(x+30,180);
    glVertex2f(x+50,180);
    glVertex2f(x+50,220);
    glVertex2f(x+30,220);
    glEnd();
    glColor3ub(255,215,0);
    circle(2,2,x+48,200);
    glColor3ub(173,216,230);
    glBegin(GL_QUADS);
    glVertex2f(x+10,220);
    glVertex2f(x+25,220);
    glVertex2f(x+25,240);
    glVertex2f(x+10,240);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(x+55,220);
    glVertex2f(x+70,220);
    glVertex2f(x+70,240);
    glVertex2f(x+55,240);
    glEnd();
    glColor3ub(100,150,180);
    glBegin(GL_LINES);
    glVertex2f(x+17,220);
    glVertex2f(x+17,240);
    glVertex2f(x+10,230);
    glVertex2f(x+25,230);
    glVertex2f(x+62,220);
    glVertex2f(x+62,240);
    glVertex2f(x+55,230);
    glVertex2f(x+70,230);
    glEnd();

    if(gameOver)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // dark burn layer
        glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
        glBegin(GL_QUADS);
        glVertex2f(x,180);
        glVertex2f(x+80,180);
        glVertex2f(x+80,260);
        glVertex2f(x,260);
        glEnd();

        glDisable(GL_BLEND);

        // fire
        glColor3ub(255,80,0);
        circle(10,15,x+40,240);
    }
}

void smallHouse()
{
    float x=120, y=150;
    glColor3ub(255,200,150);
    glBegin(GL_QUADS);
    glVertex2f(x,y);
    glVertex2f(x+40,y);
    glVertex2f(x+40,y+30);
    glVertex2f(x,y+30);
    glEnd();
    glColor3ub(150,50,50);
    glBegin(GL_TRIANGLES);
    glVertex2f(x-5,y+30);
    glVertex2f(x+45,y+30);
    glVertex2f(x+20,y+55);
    glEnd();
    glColor3ub(100,50,10);
    glBegin(GL_QUADS);
    glVertex2f(x+15,y);
    glVertex2f(x+25,y);
    glVertex2f(x+25,y+20);
    glVertex2f(x+15,y+20);
    glEnd();
    // Centered "Park" label in green, bold
    glColor3ub(0, 0, 0);
    drawTextBold(140, 162, "Park", GLUT_BITMAP_HELVETICA_18);
}

void school()
{
    glColor3ub(255,255,153);
    for(int fl=0; fl<3; fl++)
    {
        glBegin(GL_QUADS);
        glVertex2f(180,180+fl*50);
        glVertex2f(300,180+fl*50);
        glVertex2f(300,230+fl*50);
        glVertex2f(180,230+fl*50);
        glEnd();
        glColor3ub(200,200,80);
        glBegin(GL_LINES);
        glVertex2f(180,230+fl*50);
        glVertex2f(300,230+fl*50);
        glEnd();
        glColor3ub(255,255,153);
    }
    glColor3ub(220,0,0);
    glBegin(GL_TRIANGLES);
    glVertex2f(170,330);
    glVertex2f(310,330);
    glVertex2f(240,375);
    glEnd();
    glColor3ub(80,80,80);
    glBegin(GL_LINES);
    glVertex2f(240,375);
    glVertex2f(240,410);
    glEnd();
    glColor3ub(255,0,0);
    glBegin(GL_TRIANGLES);
    glVertex2f(240,405);
    glVertex2f(255,402);
    glVertex2f(240,398);
    glEnd();
    glColor3ub(0,0,0);
    for(int i=185; i<300; i+=10)
    {
        glBegin(GL_LINES);
        glVertex2f((float)i,225);
        glVertex2f((float)i,215);
        glVertex2f((float)i,275);
        glVertex2f((float)i,265);
        glVertex2f((float)i,325);
        glVertex2f((float)i,315);
        glEnd();
    }
    glColor3ub(139,69,19);
    glBegin(GL_QUADS);
    glVertex2f(230,180);
    glVertex2f(250,180);
    glVertex2f(250,215);
    glVertex2f(230,215);
    glEnd();
    glColor3ub(173,216,230);
    for(int y=195; y<=295; y+=50)
        for(int wx=190; wx<=270; wx+=40)
        {
            glBegin(GL_QUADS);
            glVertex2f((float)wx,(float)y);
            glVertex2f((float)wx+20,(float)y);
            glVertex2f((float)wx+20,(float)y+20);
            glVertex2f((float)wx,(float)y+20);
            glEnd();
        }


    glColor3f(0,0,0);
    drawText(220,340,"Daffodil University",(void*)GLUT_BITMAP_HELVETICA_12);

    if(gameOver)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(0.0f,0.0f,0.0f,0.4f);
        glBegin(GL_QUADS);
        glVertex2f(180,180);
        glVertex2f(300,180);
        glVertex2f(300,330);
        glVertex2f(180,330);
        glEnd();

        glDisable(GL_BLEND);

        // fire
        glColor3ub(255,100,0);
        circle(12,18,240,300);
    }

}

void factory()
{
    int sx=40;

    // 🔥 1. CHIMNEY
    glColor3ub(130,50,20);
    float chimneys[]= {360,400,440};
    float chTop[]   = {380,400,390};

    for(int i=0; i<3; i++)
    {
        glBegin(GL_QUADS);
        glVertex2f(chimneys[i]+sx,     180);
        glVertex2f(chimneys[i]+sx+20,  180);
        glVertex2f(chimneys[i]+sx+20,  chTop[i]);
        glVertex2f(chimneys[i]+sx,     chTop[i]);
        glEnd();
    }

    // 🔥 2. BUILDING
    glColor3ub(170,170,175);
    int heights[]= {60,60,50};
    int tops[]   = {180,240,300};
    int widths[] = {160,120,80};
    int lefts[]  = {320,340,360};

    for(int i=0; i<3; i++)
    {
        glBegin(GL_QUADS);
        glVertex2f(lefts[i]+sx, tops[i]);
        glVertex2f(lefts[i]+sx+widths[i], tops[i]);
        glVertex2f(lefts[i]+sx+widths[i], tops[i]+heights[i]);
        glVertex2f(lefts[i]+sx, tops[i]+heights[i]);
        glEnd();

        glColor3ub(150,150,155);
        glBegin(GL_LINES);
        glVertex2f(lefts[i]+sx, tops[i]+heights[i]);
        glVertex2f(lefts[i]+sx+widths[i], tops[i]+heights[i]);
        glEnd();

        glColor3ub(170,170,175);
    }

    // 🔥 3. WINDOWS
    glColor3ub(100,180,255);
    for(int x=330; x<=450; x+=40)
    {
        glBegin(GL_QUADS);
        glVertex2f(x+sx,200);
        glVertex2f(x+sx+25,200);
        glVertex2f(x+sx+25,230);
        glVertex2f(x+sx,230);
        glEnd();
    }
    for(int x=350; x<=430; x+=40)
    {
        glBegin(GL_QUADS);
        glVertex2f(x+sx,260);
        glVertex2f(x+sx+25,260);
        glVertex2f(x+sx+25,290);
        glVertex2f(x+sx,290);
        glEnd();
    }

    // 🔥 4. LABEL
   glColor3f(0,0,0);
    drawText(430,330,"Factory",(void*)GLUT_BITMAP_HELVETICA_12);

    // 💥 5. DESTROY EFFECT (unchanged)
    if(gameOver)
    {
        glColor3ub(255,50,0);
        circle(12,18,400,420);
        circle(10,15,440,430);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(0.0f,0.0f,0.0f,0.5f);
        glBegin(GL_QUADS);
        glVertex2f(320,180);
        glVertex2f(480,180);
        glVertex2f(480,350);
        glVertex2f(320,350);
        glEnd();

        glDisable(GL_BLEND);

        glColor3ub(255,50,0);
        circle(15,20,400,350);
        circle(12,18,450,370);
    }
}


//  SMOKE PARTICLES

void updateSmokes()
{
    if(!smokeOff)
    {
        static int spawnTimer=0;
        spawnTimer++;
        if(spawnTimer>8)
        {
            spawnSmoke(410,380);
            spawnSmoke(450,400);
            spawnSmoke(490,390);
            spawnTimer=0;
        }
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    for(int i=0; i<smokeCount;)
    {
        Particle& p=smokes[i];
        p.x+=p.vx;
        p.y+=p.vy;
        p.life--;
        if(p.life<=0)
        {
            smokes[i]=smokes[--smokeCount];
            continue;
        }
        float t=p.life/p.maxLife;
        float a=t*0.45f;
        float g=lerp(0.3f,0.7f,1-t);
        glColor4f(g,g,g,a);
        circle(p.size*(2-t), p.size*(2-t)*1.2f, p.x, p.y);
        i++;
    }
    glDisable(GL_BLEND);
}


//  WINDMILL

void windmill()
{
    glColor3ub(139,69,19);
    glBegin(GL_QUADS);
    glVertex2f(320,180);
    glVertex2f(330,180);
    glVertex2f(330,400);
    glVertex2f(320,400);
    glEnd();
    glPushMatrix();
    glTranslatef(325,400,0);
    glRotatef(windAngle,0,0,1);
    glColor3ub(240,240,240);
    for(int i=0; i<4; i++)
    {
        glRotatef(90,0,0,1);
        glBegin(GL_TRIANGLES);
        glVertex2f(0,0);
        glVertex2f(55,12);
        glVertex2f(55,-12);
        glEnd();
    }
    glPopMatrix();
    windAngle+=1.5f;
}


//  TREES

void treeAt(float x, float y, float scale)
{
    glColor3ub(101,67,33);
    glBegin(GL_QUADS);
    glVertex2f(x-4*scale, y);
    glVertex2f(x+4*scale, y);
    glVertex2f(x+4*scale, y+20*scale);
    glVertex2f(x-4*scale, y+20*scale);
    glEnd();
    float colors[][3]= {{0,100,0},{0,130,0},{50,160,50}};
    float sizes[]= {0.7f,1.0f,0.6f};
    float offs[] = {0,   1.0f,  1.7f};
    for(int i=0; i<3; i++)
    {
        glColor3ub((GLubyte)colors[i][0],(GLubyte)colors[i][1],(GLubyte)colors[i][2]);
        circle(22*scale*sizes[i], 28*scale*sizes[i],
               x, y+20*scale+18*scale*offs[i]);
    }
}

void tree()
{
    treeAt(60,180,1.6f);
    treeAt(42,180,1.0f);
}

void little_tree()
{
    float xs[]= {65,0,125,190,300,365,425,490};
    for(int i=0; i<8; i++) treeAt(xs[i]+5,60,0.8f);
}

void drawPlantedTrees()
{
    for(int i=0; i<treeCount; i++)
    {
        treeAt(treeX[i], treeY[i], 0.6f);
    }
}

void plantTree()
{
    if(treeCount<100)
    {
        treeX[treeCount]=playerX;
        treeY[treeCount]=playerY;
        treeCount++;
        score+=10;
    }
}


//  FLOWERS

void flowerAt(float cx, float cy, float hue)
{
    for(int i=0; i<5; i++)
    {
        float a=i*72.0f*3.14159f/180.0f;
        float px=cx+7*cosf(a), py=cy+7*sinf(a);
        if(hue<0.5f) glColor3ub(255,80,80);
        else glColor3ub(255,180,0);
        circle(3,4,px,py);
    }
    glColor3ub(255,220,0);
    circle(3,3,cx,cy);
}

void flower()
{
    flowerAt(455,210,0);
    flowerAt(440,195,1);
    flowerAt(470,192,0);
    flowerAt(30,200,1);
    flowerAt(12,210,0);
}


//  BUSHES

void Bushes()
{
    glColor3ub(0,110,0);
    circle(22,32,20,180);
    circle(22,32,40,210);
    circle(22,32,60,180);
    glColor3ub(50,160,50);
    circle(22,32,430,180);
    circle(22,32,450,210);
    circle(22,32,470,180);
    glColor3ub(0,110,0);
    circle(22,32,490,180);
}


//  TRAFFIC LIGHT

void trafficLight()
{
    float x=175, y=200;
    glColor3ub(0,0,0);
    glBegin(GL_QUADS);
    glVertex2f(x,50);
    glVertex2f(x+5,50);
    glVertex2f(x+5,y);
    glVertex2f(x,y);
    glEnd();
    glColor3ub(40,40,40);
    glBegin(GL_QUADS);
    glVertex2f(x-15,y);
    glVertex2f(x+20,y);
    glVertex2f(x+20,y+55);
    glVertex2f(x-15,y+55);
    glEnd();
    if(!isGreen) glColor3ub(255,0,0);
    else glColor3ub(60,0,0);
    circle(6,8,x+2,y+38);
    if(isGreen) glColor3ub(0,255,0);
    else glColor3ub(0,60,0);
    circle(6,8,x+2,y+16);
    if(isGreen)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0,1,0,0.15f);
        circle(14,16,x+2,y+16);
        glDisable(GL_BLEND);
    }
}

void roadLight()
{
    glColor3ub(30,30,30);
    glBegin(GL_QUADS);
    glVertex2f(80,50);
    glVertex2f(85,50);
    glVertex2f(85,180);
    glVertex2f(80,180);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(65,150);
    glVertex2f(100,150);
    glVertex2f(100,156);
    glVertex2f(65,156);
    glEnd();
    glColor3ub(255,255,200);
    circle(11,16,83,180);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(1,1,0.8f,0.12f);
    circle(22,30,83,180);
    glDisable(GL_BLEND);
}


//  CAR

void car()
{
    glPushMatrix();
    glTranslatef(tx,0,0);
    glColor3ub(220,40,40);
    glBegin(GL_QUADS);
    glVertex2f(410,40);
    glVertex2f(490,40);
    glVertex2f(490,70);
    glVertex2f(410,70);
    glEnd();
    glColor3ub(200,30,30);
    glBegin(GL_POLYGON);
    glVertex2f(425,70);
    glVertex2f(470,70);
    glVertex2f(462,98);
    glVertex2f(433,98);
    glEnd();
    glColor3ub(200,230,255);
    glBegin(GL_QUADS);
    glVertex2f(427,72);
    glVertex2f(444,72);
    glVertex2f(442,90);
    glVertex2f(428,90);
    glEnd();
    glBegin(GL_QUADS);
    glVertex2f(448,72);
    glVertex2f(465,72);
    glVertex2f(460,90);
    glVertex2f(448,90);
    glEnd();
    glColor3ub(255,255,200);
    circle(4,4,488,60);
    glColor3ub(20,20,20);
    circle(11,14,432,40);
    circle(11,14,468,40);
    glColor3ub(200,200,200);
    circle(6,9,432,40);
    circle(6,9,468,40);
    glColor3ub(180,180,180);
    circle(3,3,432,40);
    circle(3,3,468,40);
    glPopMatrix();
    if(isGreen) tx+=0.35f;
    if(tx>0) tx=-500;
    glutPostRedisplay();

}


//  TRUCK

void truck()
{
    glPushMatrix();
    glTranslatef(bx,0,0);
    glColor3ub(70,180,130);
    glBegin(GL_QUADS);
    glVertex2f(450,40);
    glVertex2f(510,40);
    glVertex2f(510,105);
    glVertex2f(450,105);
    glEnd();
    glColor3ub(50,140,100);
    glBegin(GL_LINES);
    glVertex2f(450,72);
    glVertex2f(510,72);
    glEnd();
    glColor3ub(200,30,60);
    glBegin(GL_POLYGON);
    glVertex2f(510,40);
    glVertex2f(538,40);
    glVertex2f(538,68);
    glVertex2f(525,88);
    glVertex2f(510,88);
    glEnd();
    glColor3ub(200,230,255);
    glBegin(GL_POLYGON);
    glVertex2f(514,70);
    glVertex2f(528,70);
    glVertex2f(524,84);
    glVertex2f(514,84);
    glEnd();
    glColor3ub(20,20,20);
    circle(11,14,462,40);
    circle(11,14,498,40);
    circle(11,14,526,40);
    glColor3ub(200,200,200);
    circle(6,9,462,40);
    circle(6,9,498,40);
    circle(6,9,526,40);
    glPopMatrix();
    if(isGreen) bx+=0.28f;
    if(bx>0) bx=-540;
    glutPostRedisplay();
}


//  PLAYER

void player()
{
    glPushMatrix();
    glTranslatef(playerX, playerY, 0);
    if(playerDir < 0) glScalef(-1,1,1);
    float sw = sinf(walkAngle * 0.08f) * 18.0f;
    float aw = sinf(walkAngle * 0.08f) * 22.0f;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0,0,0.18f);
    circle(10,4,0,-2);
    glDisable(GL_BLEND);
    glColor3ub(30,30,120);
    glLineWidth(3.5f);
    glPushMatrix();
    glTranslatef(-5,0,0);
    glRotatef(sw,1,0,0);
    glBegin(GL_LINES);
    glVertex2f(0,0);
    glVertex2f(-2,-22);
    glEnd();
    glPopMatrix();
    glPushMatrix();
    glTranslatef(5,0,0);
    glRotatef(-sw,1,0,0);
    glBegin(GL_LINES);
    glVertex2f(0,0);
    glVertex2f(2,-22);
    glEnd();
    glPopMatrix();
    glColor3ub(30,100,220);
    glBegin(GL_POLYGON);
    glVertex2f(-9,0);
    glVertex2f(9,0);
    glVertex2f(8,28);
    glVertex2f(-8,28);
    glEnd();
    glColor3ub(255,200,160);
    glLineWidth(2.5f);
    glPushMatrix();
    glTranslatef(-9,22,0);
    glRotatef(-aw,1,0,0);
    glBegin(GL_LINES);
    glVertex2f(0,0);
    glVertex2f(-2,-16);
    glEnd();
    glPopMatrix();
    glPushMatrix();
    glTranslatef(9,22,0);
    glRotatef(aw,1,0,0);
    glBegin(GL_LINES);
    glVertex2f(0,0);
    glVertex2f(2,-16);
    glEnd();
    glPopMatrix();
    glLineWidth(1.0f);
    glColor3ub(255,210,175);
    circle(8,9,0,38);
    glColor3ub(80,40,10);
    circle(8,5,0,44);
    glColor3ub(0,0,0);
    circle(1.5f,1.5f,4,40);
    glBegin(GL_LINE_STRIP);
    for(int i=-3; i<=3; i++)
        glVertex2f((float)i*1.2f, 36.0f+0.3f*(i*i));
    glEnd();
    glPopMatrix();
}


//  BIRD

void birdAt(float ox, float oy)
{
    glPushMatrix();
    glTranslatef(birdX+ox, oy, 0);
    float by=650;
    glColor3ub(0,180,0);
    circle(11,7,120,by);
    glColor3ub(220,60,60);
    circle(6,6,130,by+3);
    glColor3ub(255,190,0);
    glBegin(GL_TRIANGLES);
    glVertex2f(136,by+3);
    glVertex2f(147,by+5);
    glVertex2f(136,by+8);
    glEnd();
    glColor3ub(10,10,10);
    circle(1.5f,1.5f,133,by+5);
    glColor3ub(0,130,220);
    float wa=wing*0.7f;
    glBegin(GL_TRIANGLES);
    glVertex2f(120,by);
    glVertex2f(100,by+wa);
    glVertex2f(114,by-1);
    glVertex2f(120,by);
    glVertex2f(140,by+wa);
    glVertex2f(126,by-1);
    glEnd();
    glColor3ub(0,100,0);
    glBegin(GL_TRIANGLES);
    glVertex2f(110,by);
    glVertex2f(95,by-3);
    glVertex2f(95,by+3);
    glEnd();
    glPopMatrix();
}


//  HUD

void drawHUD()
{
    hudPulse += 0.05f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0,0,0,0.55f);
    glBegin(GL_QUADS);
    glVertex2f(0,660);
    glVertex2f(500,660);
    glVertex2f(500,700);
    glVertex2f(0,700);
    glEnd();
    glDisable(GL_BLEND);

    // Level badge
    glColor3ub(255,215,0);
    circle(14,14, 20, 681);
    glColor3f(0,0,0);
    char lvlCh[4];
    sprintf(lvlCh,"%d",level);
    drawText(level<10?16:12,677,lvlCh,(void*)GLUT_BITMAP_HELVETICA_18);

    // Timer bar
    float frac = (float)timeLeft/45.0f;
    glColor3ub(50,50,50);
    glBegin(GL_QUADS);
    glVertex2f(40,674);
    glVertex2f(160,674);
    glVertex2f(160,688);
    glVertex2f(40,688);
    glEnd();
    GLubyte tr2=(GLubyte)(lerp(200,0,frac));
    GLubyte tg2=(GLubyte)(lerp(0,200,frac));
    glColor3ub(tr2,tg2,0);
    glBegin(GL_QUADS);
    glVertex2f(40,674);
    glVertex2f(40+120*frac,674);
    glVertex2f(40+120*frac,688);
    glVertex2f(40,688);
    glEnd();
    char timeBuf[32];
    sprintf(timeBuf,"  %ds",timeLeft);
    glColor3ub(255,255,255);
    drawText(40,690,timeBuf,(void*)GLUT_BITMAP_HELVETICA_12);

    // Score
    char scBuf[32];
    sprintf(scBuf,"Score: %d",score);
    glColor3ub(255,215,0);
    drawText(170,676,scBuf,(void*)GLUT_BITMAP_HELVETICA_18);

    // Instruction
    const char* instr[]=
    {
        "LEVEL 1: Press T to Plant Trees (need 3)",
        "LEVEL 2: Press C near River to Clean it",
        "LEVEL 3: Press F near Factory to Stop Smoke",
        "LEVEL 4: Press B on left to Build Park",
        "LEVEL 5: Plant 1 more Tree (Press T)"
    };
    if(level>=1 && level<=5)
    {
        float pulse=0.8f+0.2f*sinf(hudPulse);
        glColor3ub((GLubyte)(200*pulse),255,(GLubyte)(200*pulse));
        drawTextCentered(250, 665, instr[level-1], GLUT_BITMAP_HELVETICA_12);
    }

    // Start prompt  (centered)
    if(!timerStart)
    {
        float p=0.7f+0.3f*sinf(hudPulse*2);
        glColor3ub(255,255,(GLubyte)(100*p));
        drawTextCentered(250, 420, "< PRESS ARROW KEY TO START >");
    }

    // ── WIN screen
    if(gameWin)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0,0.5f,0,0.75f);
        glBegin(GL_QUADS);
        glVertex2f(40,360);
        glVertex2f(460,360);
        glVertex2f(460,450);
        glVertex2f(40,450);
        glEnd();
        glDisable(GL_BLEND);

        // Pulsing golden text, centered
        float wp = 0.85f + 0.15f*sinf(hudPulse*3);
        glColor3ub((GLubyte)(255*wp), (GLubyte)(220*wp), 0);
        drawTextBold(250, 430, "CONGRATULATIONS! CITY SAVED!", GLUT_BITMAP_HELVETICA_18);
        glColor3ub(200, 255, 200);
        drawTextCentered(250, 408, "Trees are producing fresh Oxygen for the city!", GLUT_BITMAP_HELVETICA_12);
        char sc2[32];
        sprintf(sc2,"Final Score: %d", score);
        glColor3ub(255,255,255);
        drawTextCentered(250, 390, sc2, GLUT_BITMAP_HELVETICA_12);
    }

    // ── GAME OVER screen
    if(gameOver)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0.6f,0,0,0.80f);
        glBegin(GL_QUADS);
        glVertex2f(40,300);
        glVertex2f(460,300);
        glVertex2f(460,390);
        glVertex2f(40,390);
        glEnd();
        glDisable(GL_BLEND);
        float op = 0.8f+0.2f*sinf(hudPulse*4);
        glColor3ub((GLubyte)(255*op), (GLubyte)(60*op), (GLubyte)(60*op));
        drawTextBold(250, 370, "TIME'S UP — CITY DESTROYED!", GLUT_BITMAP_HELVETICA_18);
        glColor3ub(220,180,180);
        drawTextCentered(250, 345, "The city has been lost to pollution.", GLUT_BITMAP_HELVETICA_12);
        glColor3ub(200,200,200);
        drawTextCentered(250, 325, "Press R to toggle Rain  |  Restart?", GLUT_BITMAP_HELVETICA_12);
    }

    // Rain indicator
    if(isRain)
    {
        glColor3ub(170,200,255);
        drawText(455,690,"RAIN",(void*)GLUT_BITMAP_HELVETICA_12);
    }
}


//  TIMER CALLBACK

void timerCallback(int)
{
    if(timerStart && !gameOver && !gameWin)
    {
        timeLeft--;
        if(timeLeft<=0)
        {
            gameOver=true;
            triggerExplosion();   // ← TRIGGER BLAST!
        }
    }
    lightTimer++;
    if(lightTimer>=3)
    {
        isGreen=!isGreen;
        lightTimer=0;
    }
    glutPostRedisplay();
    glutTimerFunc(1000, timerCallback, 0);
}


//  INIT

void init()
{
    glClearColor(0,0,0,0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WORLD_W, 0, WORLD_H);
    initStars();
}


//  DISPLAY

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Screen shake (lightning + explosion)
    float totalShake = explosionShake;
    if(shakeFrames>0)
    {
        totalShake += 3.0f;
        shakeFrames--;
    }
    if(totalShake > 0.1f)
    {
        shakeX=(float)(rand()%((int)(totalShake*2)+1)) - totalShake;
        shakeY=(float)(rand()%((int)(totalShake*2)+1)) - totalShake;
    }
    else
    {
        shakeX=shakeY=0;
    }

    glPushMatrix();
    glTranslatef(shakeX, shakeY, 0);

    dayTime += daySpeed;
    if(dayTime>1) dayTime=0;

    if(gameOver)
    {
        for(int i=0; i<10; i++)
        {
            spawnSmoke(rand()%500, 200 + rand()%200);
        }
    }

    if(gameOver)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glColor4f(1.0f, 0.2f, 0.0f, 0.3f);
        glBegin(GL_QUADS);
        glVertex2f(0,0);
        glVertex2f(500,0);
        glVertex2f(500,700);
        glVertex2f(0,700);
        glEnd();

        glDisable(GL_BLEND);
    }

    drawSky();
    drawStars();
    drawMountains();
    drawSun();
    drawClouds();

    drawGround();

    windmill();
    house(20);
    house(100);
    school();
    factory();
    updateSmokes();

    river();
    bridge();
    road();

    little_tree();
    tree();
    drawPlantedTrees();
    Bushes();
    flower();

    trafficLight();
    roadLight();
    truck();
    car();

    player();

    if(parkBuilt) smallHouse();

    birdAt(0,0);
    birdAt(-80,20);
    birdAt(-160,-10);

    static bool birdPlayed=false;
    if(birdX>10&&birdX<300)
    {
        if(!birdPlayed)
        {
            PlaySound(TEXT("bird.wav"),NULL,SND_ASYNC);
            birdPlayed=true;
        }
    }
    if(birdX>350) birdPlayed=false;

    drawRain();
    drawLightning();

    // ── NEW EFFECTS
    updateExplosion();
    updateO2();

    drawHUD();

    glPopMatrix();

    birdX+=0.55f;
    if(birdX>500) birdX=-150;
    wing+=0.7f;
    if(wing>12) wing=-12;

    glutSwapBuffers();

    if(gameOver)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // red/orange glow (like your image)
        glColor4f(1.0f, 0.2f, 0.0f, 0.25f);
        glBegin(GL_QUADS);
        glVertex2f(0,0);
        glVertex2f(500,0);
        glVertex2f(500,700);
        glVertex2f(0,700);
        glEnd();

        glDisable(GL_BLEND);
    }
}


//  KEYBOARD

void key(unsigned char k, int, int)
{
    if(k=='r'||k=='R') isRain=!isRain;

    if(level==1&&(k=='t'||k=='T'))
    {
        if(playerX>50&&playerX<450&&playerY>80&&playerY<180) plantTree();
        if(treeCount>=3)
        {
            level=2;    // 45s per level
            score+=50;

        }
    }
    else if(level==2&&(k=='c'||k=='C'))
    {
        if(playerY>55&&playerY<140)
        {
            riverClean=true;
            level=3;
            score+=50;

        }
    }
    else if(level==3&&(k=='f'||k=='F'))
    {
        if(playerX>300)
        {
            smokeOff=true;
            level=4;
            score+=50;

        }
    }
    else if(level==4&&(k=='b'||k=='B'))
    {
        if(playerX<150)
        {
            parkBuilt=true;
            level=5;
            score+=50;

        }
    }
    else if(level==5&&(k=='t'||k=='T'))
    {
        if(playerX>50&&playerX<450&&playerY>80&&playerY<180) plantTree();
        if(treeCount>=4)
        {
            gameWin=true;
            score+=200;
        }
    }
    glutPostRedisplay();
}

void specialKeys(int k, int, int)
{
    timerStart=true;
    if(k==GLUT_KEY_LEFT  && playerX>10)
    {
        playerX-=6;
        playerDir=-1;
    }
    if(k==GLUT_KEY_RIGHT && playerX<490)
    {
        playerX+=6;
        playerDir= 1;
    }
    if(k==GLUT_KEY_UP    && playerY<170)
    {
        playerY+=6;
    }
    if(k==GLUT_KEY_DOWN  && playerY>60)
    {
        playerY-=6;
    }
    walkAngle+=10;
    glutPostRedisplay();
}

//  MAIN

int main(int argc, char** argv)
{
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(100,80);
    glutCreateWindow("Environment Saver Game");
    init();
    glutDisplayFunc(display);
    glutSpecialFunc(specialKeys);
    glutKeyboardFunc(key);
    glutIdleFunc(display);
    glutTimerFunc(1000, timerCallback, 0);
    glutMainLoop();
    return 0;
}
