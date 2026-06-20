#include "keyboard.h"

/* ============================================
   KEYBOARD LAYOUT TABLES

   Each layout maps PS/2 Set 1 scancodes (0-127)
   to ASCII characters, for both unshifted and
   shifted (Shift held) states.

   Physical key POSITIONS are fixed by the
   hardware scancode (e.g. 0x10 is always the key
   to the right of Tab on a physical UK/US board).
   What changes between layouts is which LETTER
   that physical position produces.

   Note: this is "ASCII-only" — true Swedish/German/
   French layouts use accented letters (å, ö, ü, é)
   that don't exist in standard VGA text mode's basic
   ASCII range. We map their common substitutions
   and keep accented keys as their closest ASCII or
   leave blank, since our VGA driver only prints
   standard 8-bit codepage 437 characters.
   ============================================ */

/* ---- UK QWERTY (existing default) ---- */
static const char uk_unshift[128] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,'\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','#',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const char uk_shift[128] = {
    0,0,'!','"','#','$','%','^','&','*','(',')','_','+',0,'\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',':','@','~',0,'|',
    'Z','X','C','V','B','N','M','<','>','?',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/* ---- US QWERTY (mostly same as UK, different punctuation keys) ---- */
static const char us_unshift[128] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,'\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
    'z','x','c','v','b','n','m',',','.','/',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const char us_shift[128] = {
    0,0,'!','@','#','$','%','^','&','*','(',')','_','+',0,'\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',':','"','~',0,'|',
    'Z','X','C','V','B','N','M','<','>','?',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/* ---- Dvorak (remaps letters to the Dvorak Simplified layout) ---- */
static const char dvorak_unshift[128] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','[',']',0,'\t',
    '\'',',','.','p','y','f','g','c','r','l','/','=','\n',0,
    'a','o','e','u','i','d','h','t','n','s','-','\\',0,';',
    ';','q','j','k','x','b','m','w','v','z',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const char dvorak_shift[128] = {
    0,0,'!','@','#','$','%','^','&','*','(',')','{','}',0,'\t',
    '"','<','>','P','Y','F','G','C','R','L','?','+','\n',0,
    'A','O','E','U','I','D','H','T','N','S','_','|',0,':',
    ':','Q','J','K','X','B','M','W','V','Z',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/* ---- Colemak (keeps most punctuation, remaps letters) ---- */
static const char colemak_unshift[128] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,'\t',
    'q','w','f','p','g','j','l','u','y',';','[',']','\n',0,
    'a','r','s','t','d','h','n','e','i','o','\'','#',0,'\\',
    'z','x','c','v','b','k','m',',','.','/',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const char colemak_shift[128] = {
    0,0,'!','"','#','$','%','^','&','*','(',')','_','+',0,'\t',
    'Q','W','F','P','G','J','L','U','Y',':','{','}','\n',0,
    'A','R','S','T','D','H','N','E','I','O','@','~',0,'|',
    'Z','X','C','V','B','K','M','<','>','?',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/* ---- Swedish QWERTY (ASCII-approximated: å->[, ä->', ö->;) ---- */
static const char swedish_unshift[128] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','+','\'',0,'\t',
    'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','#',0,'\\',
    'z','x','c','v','b','n','m',',','.','-',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const char swedish_shift[128] = {
    0,0,'!','"','#','$','%','&','/','(',')','=','?','`',0,'\t',
    'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',':','*','~',0,'|',
    'Z','X','C','V','B','N','M',';',':','_',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/* ---- German QWERTZ (ASCII-approximated: y/z swapped, ä->', ö->;, ü->[) ---- */
static const char german_unshift[128] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,'\t',
    'q','w','e','r','t','z','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','#',0,'\\',
    'y','x','c','v','b','n','m',',','.','-',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const char german_shift[128] = {
    0,0,'!','"','#','$','%','&','/','(',')','=','_','+',0,'\t',
    'Q','W','E','R','T','Z','U','I','O','P','{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',':','@','~',0,'|',
    'Y','X','C','V','B','N','M',';',':','_',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/* ---- French AZERTY (ASCII-approximated) ---- */
static const char french_unshift[128] = {
    0,0,'&','e','"','\'','(','-','e','_','c','a',')','=',0,'\t',
    'a','z','e','r','t','y','u','i','o','p','^','$','\n',0,
    'q','s','d','f','g','h','j','k','l','m','%','*',0,'\\',
    'w','x','c','v','b','n',',',';',':','!',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const char french_shift[128] = {
    0,0,'1','2','3','4','5','6','7','8','9','0','`','+',0,'\t',
    'A','Z','E','R','T','Y','U','I','O','P','{','}','\n',0,
    'Q','S','D','F','G','H','J','K','L','M','#','~',0,'|',
    'W','X','C','V','B','N','?','.','/','_',0,'*',0,' ',
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/* ---- Layout table: index by keyboard_layout_t ---- */
typedef struct {
    const char* unshift;
    const char* shift;
    const char* name;
} layout_entry_t;

static const layout_entry_t layouts[LAYOUT_COUNT] = {
    { uk_unshift,      uk_shift,      "UK QWERTY" },
    { us_unshift,      us_shift,      "US QWERTY" },
    { dvorak_unshift,  dvorak_shift,  "Dvorak" },
    { colemak_unshift, colemak_shift, "Colemak" },
    { swedish_unshift, swedish_shift, "Swedish" },
    { german_unshift,  german_shift,  "German (QWERTZ)" },
    { french_unshift,  french_shift,  "French (AZERTY)" },
};

const char* keyboard_layout_table_name(int layout) {
    if (layout < 0 || layout >= LAYOUT_COUNT) return "Unknown";
    return layouts[layout].name;
}

char keyboard_layout_lookup(int layout, uint8_t scancode, int shifted) {
    if (layout < 0 || layout >= LAYOUT_COUNT) layout = LAYOUT_UK_QWERTY;
    if (scancode >= 128) return 0;
    return shifted ? layouts[layout].shift[scancode] : layouts[layout].unshift[scancode];
}
