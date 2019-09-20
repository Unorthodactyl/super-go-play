#ifndef __DMGPAL_H__
#define __DMGPAL_H__

#include "defs.h"

struct palette 
{
    un32 hash[20];
    int bg[4];
    int obj0[4];
    int obj1[4];
};

extern int dmg_pal[][4];
extern byte dmg_color;
extern byte dmg_cur_pal;
extern byte dmg_dedicated_pal;

void dmg_pal_cycle();
void dmg_pal_set();
void dmg_pal_update(int palette);

#endif