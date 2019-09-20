#include "dmgpal.h"
#include "mem.h"
#include "lcd.h"
#include "stdio.h"
#include "defs.h"

#define GB_DEFAULT_PALETTE { 0xd5f3ef, 0x7ab6a3, 0x3b6137, 0x161c04 }
#if 0
// Testing/Debug palette
 int def_pal[4][4] = {{0xffffff, 0x808080, 0x404040, 0x000000},
					  {0xff0000, 0x800000, 0x400000, 0x000000},
					  {0x00ff00, 0x008000, 0x004000, 0x000000},
					  {0x0000ff, 0x000080, 0x000040, 0x000000} };
#else
int dmg_pal[4][4] = {GB_DEFAULT_PALETTE,
	 				 GB_DEFAULT_PALETTE,
					 GB_DEFAULT_PALETTE,
					 GB_DEFAULT_PALETTE };
#endif

// keep in mind these are actually BGR values, not RGB
// TODO: make these palette names actually mean something (they are quite random at the moment)
#define GB_GREEN_PALETTE { 0xffffff, 0x00ff52, 0x0042ff, 0x000000 }
#define GB_ORANGE_PALETTE { 0xffffff, 0x00ffff, 0x0000ff, 0x000000 }
#define GB_LIGHT_BROWN_PALETTE { 0xffffff, 0x63adff, 0x003184, 0x000000 }
#define GB_INVERTED_MONOCHROME_PALETTE { 0x000000, 0x848400, 0x00deff, 0xffffff }
#define GB_MONOCHROME_PALETTE { 0xffffff, 0xa5a5a5, 0x525252, 0x000000 }
#define GB_PASTEL_MIX_PALETTE { 0xa5ffff, 0x9494ff, 0xff9494, 0x000000 }
#define GB_DARK_BROWN_PALETTE { 0xc5e6ff, 0x849cce, 0x296b84, 0x08315a }
#define GB_DARK_GREEN_BG_PALETTE { 0xffffff, 0x31ff7b, 0xc56300, 0x000000 }
#define GB_BROWN_OBJ_PALETTE { 0xffffff, 0x8484ff, 0x3a3a94, 0x000000 }
#define GB_DARK_BLUE_PALETTE { 0xffffff, 0xde8c8c, 0x8c5252, 0x000000 }
#define GB_BLUE_PALETTE { 0xffffff, 0xffa563, 0xff0000, 0x000000 }
#define GB_GREEN_OBJ_PALETTE { 0xffffff, 0x31ff7b, 0x008400, 0x000000 }
#define GB_YELLOW_PALETTE { 0xffffff, 0x00ffff, 0x004a7b, 0x000000 }
#define GB_BALLOONKID_PALETTE { 0xffffff, 0x009cff, 0x0000ff, 0x000000 }
#define GB_ALLEYWAY_PALETTE { 0xff9ca5, 0x00ffff, 0x006300, 0x000000 }
#define GB_CAMERA_PALETTE { 0xffffff, 0x00ceff, 0x00639c, 0x000000 }
#define GB_RADARMISSION_BG_PALETTE { 0xffffff, 0x84adad, 0x7b7342, 0x000000 }
#define GB_RADARMISSION_OBJ_PALETTE { 0xffffff, 0x0073ff, 0x004294, 0x000000 }
#define GB_PINOCCHIO_PALETTE { 0x42c5ff, 0x00d6ff, 0x003a94, 0x00004a }
#define GB_GALLERY_PALETTE { 0xffffff, 0x00ff7b, 0x0073b5, 0x000000 }
#define GB_KPINBALL_PALETTE { 0x5263ff, 0x0000d6, 0x000063, 0x000000 }
#define GB_MARIOLAND_BG_PALETTE { 0xffb5b5, 0x94ffff, 0x425aad, 0x000000 }
#define GB_MARIOLAND_OBJ_PALETTE { 0x000000, 0xffffff, 0x8484ff, 0x3a3a94 }
#define GB_TETRISATTACK_PALETTE { 0xfffffff, 0xffbd5a, 0x0000ff, 0xff0000 }
#define GB_DKLAND_PALETTE { 0x9cffff, 0xffb594, 0x739463, 0x3a3a00 }
#define GB_SOCCER_BG_PALETTE { 0x00ff6b, 0xffffff, 0x4a52ff, 0x000000 }
#define GB_SOCCER_OBJ_PALETTE { 0xffffff, 0xffffff, 0xffa563, 0xff0000 }
#define GB_BASEBALL_PALETTE { 0x00de52, 0x0084ff, 0x003184, 0x000000 }
#define GB_KIRBY_PALETTE { 0xff0000, 0xffffff, 0x7bffff, 0xff8400 }
#define GB_MARIOLAND2_PALETTE { 0xceffff, 0xefef63, 0x31849c, 0x5a5a5a }
#define GB_WAVERACE_PALETTE { 0xffffff, 0x7bffff, 0xff8400, 0x0000ff }
#define GB_ZELDA_PALETTE { 0xffffff, 0x00ff00, 0x008431, 0x004a00 }
#define GB_METROID_PALETTE { 0x00ffff, 0x0000ff, 0x000063, 0x000000 }

const struct palette predefs[] = {
    // manual selection palettes (some are used in specific games)
    {
        { -1 },
        GB_DEFAULT_PALETTE, GB_DEFAULT_PALETTE, GB_DEFAULT_PALETTE
    },
    {
        { -1 },
        GB_GREEN_PALETTE, GB_GREEN_PALETTE, GB_GREEN_PALETTE
    },
    { /* TETRIS, POKEMON YELLOW, POKEMON YELAPSD, POKEMON YELAPSS, POKEMON YELAPSF, POKEMON YELAPSI */
        { 0x4f1952, 0x2b969d, 0x9aa5c2, 0x9ab4c2, 0x9aa7c2, 0x9aaac2, -1 },
        GB_ORANGE_PALETTE, GB_ORANGE_PALETTE, GB_ORANGE_PALETTE
    },
    { /* F1RACE, GAME&WATCH, STAR STACKER, MARIO'S PICROSS, NIGEL MANSELL, PICROSS2, YAKUMAN  */
        { 0x13d73e, 0x1307fe, 0xf50f05, 0x444494, 0x70f710, 0x1ca192, 0x112671, -1 },
        GB_LIGHT_BROWN_PALETTE, GB_LIGHT_BROWN_PALETTE, GB_LIGHT_BROWN_PALETTE
    },
    { /* GALAGA&GALAXIAN, SOLARSTRIKER, SPACE INVADERS */
        { 0xf4b3e3, 0x5d7e40, 0x671e8f, -1 },
        GB_INVERTED_MONOCHROME_PALETTE, GB_INVERTED_MONOCHROME_PALETTE, GB_INVERTED_MONOCHROME_PALETTE
    },
    { /* X */
        { 0x58, -1 },
        GB_MONOCHROME_PALETTE, GB_MONOCHROME_PALETTE, GB_MONOCHROME_PALETTE
    },
    {
        { -1 },
        GB_PASTEL_MIX_PALETTE, GB_PASTEL_MIX_PALETTE, GB_PASTEL_MIX_PALETTE
    },
    {
        { -1 },
        GB_DARK_BROWN_PALETTE, GB_LIGHT_BROWN_PALETTE, GB_LIGHT_BROWN_PALETTE
    },
    { /* MILLI/CENTI/PEDE, KEN GRIFFEY JR, TETRIS PLUS, WARIO BLAST */
        { 0x79fcc3, 0x2ce649, 0x457e27, 0x8fe1e3, -1 },
        GB_DARK_GREEN_BG_PALETTE, GB_BROWN_OBJ_PALETTE, GB_BROWN_OBJ_PALETTE
    },
    { /* KILLERINSTINCT95 */
        { 0x722b48, -1 },
        GB_DARK_BLUE_PALETTE, GB_BROWN_OBJ_PALETTE, GB_LIGHT_BROWN_PALETTE
    },
    {
        { -1 },
        GB_BROWN_OBJ_PALETTE, GB_GREEN_OBJ_PALETTE, GB_BLUE_PALETTE
    },
    {
        { -1 },
        GB_BLUE_PALETTE, GB_BROWN_OBJ_PALETTE, GB_GREEN_OBJ_PALETTE
    },
    {
        { -1 },
        GB_YELLOW_PALETTE, GB_BLUE_PALETTE, GB_GREEN_OBJ_PALETTE
    },
    // game specific palettes
    { /* BALLOON KID, TETRIS BLAST */
        { 0xeedf4f, 0x69fe23, -1 },
        GB_BALLOONKID_PALETTE, GB_BALLOONKID_PALETTE, GB_BALLOONKID_PALETTE
    },
    { /* ALLEY WAY */
        { 0x72fb6e, -1 },
        GB_ALLEYWAY_PALETTE, GB_ALLEYWAY_PALETTE, GB_ALLEYWAY_PALETTE
    },
    { /* POCKETCAMERA, GAMEBOYCAMERA */
        { 0x2ce251, 0x25f59e, -1 },
        GB_CAMERA_PALETTE, GB_CAMERA_PALETTE, GB_CAMERA_PALETTE
    },
    { /* RADARMISSION */
        { 0x1e104c, -1 },
        GB_RADARMISSION_BG_PALETTE, GB_RADARMISSION_OBJ_PALETTE, GB_RADARMISSION_BG_PALETTE
    },
    { /* POKEMON BLUE */
        { 0x1ecadf, -1 },
        GB_BLUE_PALETTE, GB_BROWN_OBJ_PALETTE, GB_BLUE_PALETTE
    },
    { /* KAERUNOTAMENI */
        { 0x243a79, -1 },
        GB_DARK_BLUE_PALETTE, GB_BROWN_OBJ_PALETTE, GB_DARK_BLUE_PALETTE
    },
    { /* GAMEBOYCAMERA G, POKEMON RED */
        { 0x25f59e, 0xee17b7, -1 },
        GB_BROWN_OBJ_PALETTE, GB_GREEN_OBJ_PALETTE, GB_BROWN_OBJ_PALETTE
    },
    { /* JAMES  BOND  007, POKEMON GREEN */
        { 0xbf7728, 0xbaeeba, -1 },
        GB_DARK_GREEN_BG_PALETTE, GB_BROWN_OBJ_PALETTE, GB_DARK_GREEN_BG_PALETTE
    },
    { /* DR.MARIO, Dr.MARIO */
        { 0xdf97f4, 0xdf9914, -1 },
        GB_BLUE_PALETTE, GB_BLUE_PALETTE, GB_BROWN_OBJ_PALETTE
    },
    { /* PINNOCHIO */
        { 0x8a6e2, -1 },
        GB_DARK_BLUE_PALETTE, GB_DARK_BLUE_PALETTE, GB_PINOCCHIO_PALETTE
    },
    { /* MOGURANYA */
        { 0x6198f, -1 },
        GB_RADARMISSION_BG_PALETTE, GB_RADARMISSION_OBJ_PALETTE, GB_RADARMISSION_OBJ_PALETTE
    },
    { /* G&W GALLERY, GAMEBOY GALLERY, GAMEBOY GALLERY2 */ 
        { 0x20daca, 0x501d4, 0x1d441, -1 },
        GB_GALLERY_PALETTE, GB_BROWN_OBJ_PALETTE, GB_BROWN_OBJ_PALETTE
    },
    { /* MARIO & YOSHI, YOSSY NO TAMAGO */
        { 0x603bba, 0xb5374 -1 },
        GB_GREEN_PALETTE, GB_BROWN_OBJ_PALETTE, GB_BROWN_OBJ_PALETTE
    },
    { /* DONKEY KONG */
        { 0x2288f6, -1 },
        GB_BALLOONKID_PALETTE, GB_BROWN_OBJ_PALETTE, GB_BROWN_OBJ_PALETTE
    },
    {  /* KIRBY'S PINBALL */
        { 0xdaf457, -1 },
        GB_ALLEYWAY_PALETTE, GB_KPINBALL_PALETTE, GB_KPINBALL_PALETTE
    },
    { /* SUPER MARIOLAND */
        { 0x823029, -1 },
        GB_MARIOLAND_BG_PALETTE, GB_MARIOLAND_OBJ_PALETTE, GB_MARIOLAND_OBJ_PALETTE
    },
    { /* POKEBOM */
        { 0x3033ef, -1 },
        GB_DARK_BLUE_PALETTE, GB_PINOCCHIO_PALETTE, GB_PINOCCHIO_PALETTE
    },
    { /* KID ICARUS */
        { 0xc18c28, -1 },
        GB_DARK_BLUE_PALETTE, GB_BROWN_OBJ_PALETTE, GB_BROWN_OBJ_PALETTE
    },
    { /* ASTEROIDS/MISCMD, GOLF, WORLD CUP, DMG FOOTBALL, TOY STORY */
        { 0x9e5a91, 0x4f4d1b, 0x39eb9e, 0x85e871, 0xa7167f, -1 },
        GB_GREEN_OBJ_PALETTE, GB_BROWN_OBJ_PALETTE, GB_BROWN_OBJ_PALETTE
    },
    { /* THE CHESSMASTER, DYNABLASTER, KINGOFTHEZOO */
        { 0xbdee06, 0x8a0c, 0xc9acef, -1 },
        GB_LIGHT_BROWN_PALETTE, GB_BLUE_PALETTE, GB_BLUE_PALETTE
    },
    { /* BT2RAGNAROKWORLD */
        { 0x6e739, -1 },
        GB_LIGHT_BROWN_PALETTE, GB_GREEN_OBJ_PALETTE, GB_GREEN_OBJ_PALETTE
    },
    { /* TETRIS ATTACK, YOSSY NO PANEPON */
        { 0xe92bbf, 0x5c7757, -1 },
        GB_GREEN_PALETTE, GB_GREEN_PALETTE, GB_TETRISATTACK_PALETTE
    },
    { /* YOSHI'S COOKIE, YOSSY NO COOKIE */
        { 0x5c0525, 0xe27f6d, -1 },
        GB_BALLOONKID_PALETTE, GB_BALLOONKID_PALETTE, GB_TETRISATTACK_PALETTE
    },
    { /* QIX, TETRIS2 */
        { 0x514958, 0x19531f, 0x6a0a17, -1 },
        GB_ORANGE_PALETTE, GB_ORANGE_PALETTE, GB_TETRISATTACK_PALETTE
    },
    {  /* GBWARS, SUPERMARIOLAND3 */
        { 0x17195b, 0x35ba65, -1 },
        GB_RADARMISSION_BG_PALETTE, GB_RADARMISSION_OBJ_PALETTE, GB_TETRISATTACK_PALETTE
    },
    { /* DONKEYKONGLAND95, SUPERDONKEYKONG */
        { 0x5b2f3a, 0xc6b7e2, -1 },
        GB_DKLAND_PALETTE, GB_PINOCCHIO_PALETTE, GB_BROWN_OBJ_PALETTE 
    },
    {  /* SOCCER, TENNIS, TOPRANKTENNIS, TOPRANKINGTENNIS */
        { 0x3d331e, 0x4b1940, 0xf46d3b, 0xdad5bc, -1 },
        GB_SOCCER_BG_PALETTE, GB_SOCCER_OBJ_PALETTE, GB_LIGHT_BROWN_PALETTE
    },
    { /* BASEBALL */
        { 0x3d705e, -1 },
        GB_BASEBALL_PALETTE, GB_SOCCER_OBJ_PALETTE, GB_BROWN_OBJ_PALETTE
    },
    { /* HOSHINOKA-BI, KIRBY2, KIRBY BLOCKBALL, KIRBY DREAM LAND */
        { 0x4615ac, 0x24352b, 0x317691, 0xa6d41c, -1 },
        GB_ALLEYWAY_PALETTE, GB_KPINBALL_PALETTE, GB_KIRBY_PALETTE
    },
    { /* MARIOLAND2 */
        { 0x8815ae, -1 },
        GB_MARIOLAND2_PALETTE, GB_RADARMISSION_OBJ_PALETTE, GB_BLUE_PALETTE
    },
    { /* WAVERACE */
        { 0x472487, -1 },
        GB_BLUE_PALETTE, GB_BROWN_OBJ_PALETTE, GB_WAVERACE_PALETTE
    },
    { /* DONKEYKONGLAND, DONKEYKONGLAND 2, DONKEYKONGLAND 3 */
        { 0xa75759, 0x5b1637, 0x5b1638, -1 },
        GB_DARK_BLUE_PALETTE, GB_PINOCCHIO_PALETTE, GB_TETRISATTACK_PALETTE
    },
    { /* MAGNETIC SOCCER, MYSTIC QUEST, OTHELLO, VEGAS STAKES  */
        { 0x8386fa, 0xc54690, 0x4927e8, 0xaa7cd7, -1 },
        GB_GREEN_OBJ_PALETTE, GB_BROWN_OBJ_PALETTE, GB_BLUE_PALETTE
    },
    { /* LOLO2, DEFENDER/JOUST, BA.TOSHINDEN, NETTOU TOSHINDEN, NETTOU KOF 95, MEGAMAN, MEGA MAN 2, MEGAMAN3, ROCKMAN WORLD, ROCMAN WORLD2, ROCKMANWORLD3, STREET FIGHTER 2, SUPER RC PRO-AM */
        { 0x4d341f, 0xc88b50, 0x10cb1e, 0x61de87, 0x1c22d2, 0x1d19c9, 0x1e9166, 0x19c98a, 0xc6d0eb, 0xeda4f0, 0x65b0e6, 0xa1e6ed, 0xb03c79, -1 },
        GB_LIGHT_BROWN_PALETTE, GB_BLUE_PALETTE, GB_GREEN_OBJ_PALETTE
    },
    { /* ZELDA*/
        { 0x4d5310, -1 },
        GB_BROWN_OBJ_PALETTE, GB_ZELDA_PALETTE, GB_BLUE_PALETTE
    },
    { /* BOY AND BLOB GB1, BOY AND BLOB GB2 */
        { 0x561095, 0x561096, -1 },
        GB_LIGHT_BROWN_PALETTE, GB_GREEN_OBJ_PALETTE, GB_BLUE_PALETTE
    },
    { /* METROID2 */
        { 0x48f28f, -1 },
        GB_BLUE_PALETTE, GB_METROID_PALETTE, GB_GREEN_OBJ_PALETTE
    },
    { /* WARIOLAND2 */
        { 0x8816bc, -1 },
        GB_RADARMISSION_BG_PALETTE, GB_LIGHT_BROWN_PALETTE, GB_BLUE_PALETTE
    },
    { /* PAC-IN-TIME */
        { 0x98e3b6, -1 },
        GB_DARK_GREEN_BG_PALETTE, GB_BROWN_OBJ_PALETTE, GB_BLUE_PALETTE
    },
    {{0}}
};

enum pal{BG,WIN,OBJ0,OBJ1};

byte dmg_cur_pal = 0;
byte dmg_dedicated_pal = 0;
byte dmg_color = 1;

void dmg_pal_set_layer(enum pal layer, const int* pal) 
{
    int i;
    for (i = 0; i < 4; i++) 
    {
        dmg_pal[layer][i] = pal[i];
    }
}

void dmg_pal_update(int palette) {
    dmg_pal_set_layer(BG, predefs[palette].bg);
    dmg_pal_set_layer(WIN, predefs[palette].bg);
    dmg_pal_set_layer(OBJ0, predefs[palette].obj0);
    dmg_pal_set_layer(OBJ1, predefs[palette].obj1);
    pal_dirty();
}

un32 calc_name_hash() 
{
    un32 d = 256;
    un32 h = 0;
    un32 p = 16777213; /* last 24-bit prime number */
    int i;

    for (i = 0; rom.name[i]; i++) 
    {
        h = (d*h + (un32)rom.name[i])%p;
    }
    return h;
}

void dmg_pal_cycle() 
{
#if 0
    // leave testing/debug palette intact
#else
    dmg_cur_pal = (dmg_cur_pal+1) % 14;
    printf("dmgpal: cur palette: %d\n", dmg_cur_pal);

    if (dmg_cur_pal == 13) 
    {
        if (dmg_dedicated_pal)
        {
            dmg_pal_set();
            return;
        }
        else
        {
            dmg_cur_pal = (dmg_cur_pal+1) % 14;
        }
    }

    dmg_pal_update(dmg_cur_pal);
#endif
}

void dmg_pal_set() 
{
#if 0
    // leave testing/debug palette intact
#else
    int i, j;
    un32 hash = calc_name_hash();
    printf("dmgpal: running palette scan, looking for %x...", hash);

    for (i = 0; *((int*)(predefs + i)); i++) 
    {
        printf("%d..", i);
        for (j = 0; predefs[i].hash[j] != -1; j++) 
        {
            if (hash == predefs[i].hash[j]) 
            {
                printf("hash matched\n");
                dmg_pal_update(i);
                dmg_cur_pal = 13;
                dmg_dedicated_pal = 1;
                return;
            }
        }
    }
    printf("not found\n");
#endif
}
