

#define HEALTH_NAME_X 0
#define HEALTH_NAME_Y 10


#define MANA_NAME_X 0
#define MANA_NAME_Y 20

#define HEALTH_BAR_X 10
#define HEALTH_BAR_Y 10

#define MANA_BAR_X 10
#define MANA_BAR_Y 20

#define BAR_WIDTH 32
#define BAR_HEIGHT 2

void draw_health(int x, int y);

void draw_mana(int x, int y);

void draw_health_bar(int totalHealth);

void draw_mana_bar(int totalMana);

#define upArrow 0x18
#define downArrow 0x19
#define rightArrow 0x1A
#define leftArrow 0x1B

void draw_combo(int x, int y, Spell spell);

void draw_spell_name(int x, int y, Spell spell);

#define COMBO_OFFSET 5
void draw_spell(int x, int y, Spell spell);