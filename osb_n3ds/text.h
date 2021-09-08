#ifndef TEXT_DEF
#define TEXT_DEF

enum {
	FONT0 = 0,
	NUM_FONTS,
};

void Text_Init(void);
void Text_Draw(int font, int x, int y, int hSpacing, int vSpacing, int maxWidth, const char *text);
void Text_Close(void);
void Text_Clear(void);

#endif