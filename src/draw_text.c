#include "game_main.h"

static image_t *font_sheet = NULL;
static uint8_t charIndex[256];

void init_font() {
  if (!font_sheet) {
    const char *chars =
        " *+,-./0123!456789:;<=\">?@ABCDEFG#HIJKLMNOPQ$RSTUVWXYZ%&_'()";
    for (int i = 0; i < strlen(chars); i++) {
      char c = chars[i];
      uint8_t index = c;
      charIndex[index] = i;
    }
    font_sheet = image("assets/fonts/font.qoi");
  }
}

void draw_text(char *text, int x, int y, int align) {

  vec2_t pos = vec2(x, y);
  for (int i = 0; i < strlen(text); i++) {
    char c = text[i];
    if (c == '\n') {
      pos.x = x;
      pos.y += 16;
      continue;
    }
    if (c >= 'a' && c <= 'z') {
      c += 'A' - 'a';
    }
    uint8_t index = charIndex[c];
    image_draw_tile(font_sheet, index, vec2i(16, 16), pos);
    pos.x += 14;
  }
}