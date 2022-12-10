#include "raylib_ex.hpp"

#include <queue>

bool CompareColors(Color a, Color b) {
  return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

struct Pos {
  i32 x, y, x1, x2, dy;
};

int validCoord(int x, int y, int n, int m) {

  if (x < 0 || y < 0) {
    return 0;
  }
  if (x >= n || y >= m) {
    return 0;
  }
  return 1;
}

void ImageDrawFlood(Image *dst, Vector2 pos, Color color) {
#define Inside(x, y)                                                           \
  (CheckCollisionPointRec({x, y},                                              \
                          {0, 0, (f64)dst->width - 1, (f64)dst->height - 1}))

  i32 x = pos.x;
  i32 y = pos.y;
  i32 n = dst->width - 1;
  i32 m = dst->height - 1;

  i32 vis[dst->width][dst->height];
  memset(vis, 0, sizeof(vis));
  queue<Vector2> obj;

  obj.push({(f64)x, (f64)y});

  vis[x][y] = 1;

  while (obj.empty() != 1) {
    let coord = obj.front();
    int x = coord.x;
    int y = coord.y;
    Color preColor = GetImageColor(*dst, x, y);
    // int preColor = data[x][y];

    ImageDrawPixel(dst, x, y, color);
    obj.pop();
    if (validCoord(x + 1, y, n, m) && vis[x + 1][y] == 0 &&
        CompareColors(GetImageColor(*dst, x + 1, y), preColor)) {
      obj.push({(f64)x + 1, (f64)y});
      vis[x + 1][y] = 1;
    }

    if (validCoord(x - 1, y, n, m) && vis[x - 1][y] == 0 &&
        CompareColors(GetImageColor(*dst, x - 1, y), preColor)) {
      obj.push({(f64)x - 1, (f64)y});
      vis[x - 1][y] = 1;
    }

    if (validCoord(x, y + 1, n, m) && vis[x][y + 1] == 0 &&
        CompareColors(GetImageColor(*dst, x, y + 1), preColor)) {
      obj.push({(f64)x, (f64)y + 1});
      vis[x][y + 1] = 1;
    }

    if (validCoord(x, y - 1, n, m) && vis[x][y - 1] == 0 &&
        CompareColors(GetImageColor(*dst, x, y - 1), preColor)) {
      obj.push({(f64)x, (f64)y - 1});
      vis[x][y - 1] = 1;
    }
  }
}

void ImageDrawFloodEx(Image *dst, Vector2 pos, Color color, Color to_replace) {
#define check_validity(p)                                                      \
  (CheckCollisionPointRec(                                                     \
       p, {0, 0, (f64)dst->width - 1, (f64)dst->height - 1}) &&                \
   CompareColors(GetImageColor(*dst, p.x, p.y), to_replace))

  std::queue<Vector2> q;
  q.push(pos);

  while (!q.empty()) {
    Vector2 current_pos = q.back();
    q.pop();

    // Color current = GetImageColor(*dst, current_pos.x, current_pos.y);
    // if (CompareColors(current, to_replace) == false)
    // continue;

    DrawPixel(current_pos.x, current_pos.y, color);
    TraceLog(LOG_INFO, "%.2f %.2f", current_pos.x, current_pos.y);
    if (check_validity(Vector2Add(current_pos, {1, 0})))
      q.push(Vector2Add(current_pos, {1, 0}));
    if (check_validity(Vector2Add(current_pos, {-1, 0})))
      q.push(Vector2Add(current_pos, {-1, 0}));
    if (check_validity(Vector2Add(current_pos, {0, 1})))
      q.push(Vector2Add(current_pos, {0, 1}));
    if (check_validity(Vector2Add(current_pos, {0, -1})))
      q.push(Vector2Add(current_pos, {0, -1}));
  }
}

/*void ImageDrawFloodEx(Image *dst, Vector2 pos, Color color, Color to_replace)
{ if (CheckCollisionPointRec( pos, {0, 0, (f64)dst->width - 1, (f64)dst->height
- 1}) == false) return;

  Color current = GetImageColor(*dst, pos.x, pos.y);
  if (CompareColors(current, to_replace) == false)
    return;

  ImageDrawPixel(dst, pos.x, pos.y, color);
  ImageDrawFloodEx(dst, Vector2Add(pos, {1, 0}), color, to_replace);
  ImageDrawFloodEx(dst, Vector2Add(pos, {-1, 0}), color, to_replace);
  ImageDrawFloodEx(dst, Vector2Add(pos, {0, 1}), color, to_replace);
  ImageDrawFloodEx(dst, Vector2Add(pos, {0, -1}), color, to_replace);
}
*/

// void ImageDrawFlood(Image *dst, Vector2 pos, Color color) {
//   if (!CheckCollisionPointRec(
//           pos, {0, 0, (f64)dst->width - 1, (f64)dst->height - 1}))
//     return;
//
//   Color current = GetImageColor(*dst, pos.x, pos.y);
//   if (ColorToInt(current) == ColorToInt(BLACK))
//     return;
//
//   ImageDrawPixel(dst, pos.x, pos.y, color);
//   ImageDrawFlood(dst, Vector2Add(pos, {1, 0}), color);
//   ImageDrawFlood(dst, Vector2Add(pos, {-1, 0}), color);
//   ImageDrawFlood(dst, Vector2Add(pos, {0, 1}), color);
//   ImageDrawFlood(dst, Vector2Add(pos, {0, -1}), color);
// }

void ImageDrawLineEx(Image *dst, Vector2 pos, Vector2 pos2, int thickness,
                     Color color) {
  float x0 = pos.x, x1 = pos2.x;
  float y0 = pos.y, y1 = pos2.y;
  int wd = thickness;

  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx - dy, e2, x2, y2;
  float ed = dx + dy == 0 ? 1 : sqrt((float)dx * dx + (float)dy * dy);

  for (wd = (wd + 1) / 2;;) {
    ImageDrawPixel(dst, x0, y0, color);
    e2 = err;
    x2 = x0;
    if (2 * e2 >= -dx) {
      for (e2 += dy, y2 = y0; e2 < ed * wd && (y1 != y2 || dx > dy); e2 += dx)
        ImageDrawPixel(dst, x0, y2 += sy, color);
      if (x0 == x1)
        break;
      e2 = err;
      err -= dy;
      x0 += sx;
    }
    if (2 * e2 <= dy) {
      for (e2 = dx - e2; e2 < ed * wd && (x1 != x2 || dx < dy); e2 += dy)
        ImageDrawPixel(dst, x2 += sx, y0, color);
      if (y0 == y1)
        break;
      err += dx;
      y0 += sy;
    }
  }
}

void DrawTextBoxed(Font font, const char *text, Rectangle rec, float fontSize,
                   float spacing, bool wordWrap, Color tint) {
  DrawTextBoxedSelectable(font, text, rec, fontSize, spacing, wordWrap, tint, 0,
                          0, WHITE, WHITE);
}

let const SUB_Y = -10;

// Draw text using font inside rectangle limits with support for text selection
void DrawTextBoxedSelectable(Font font, const char *text, Rectangle rec,
                             float fontSize, float spacing, bool wordWrap,
                             Color tint, int selectStart, int selectLength,
                             Color selectTint, Color selectBackTint) {
  int length = TextLength(
      text); // Total length in bytes of the text, scanned by codepoints in loop

  float textOffsetY = 0;    // Offset between lines (on line break '\n')
  float textOffsetX = 0.0f; // Offset X to next character to draw

  float scaleFactor =
      fontSize / (float)font.baseSize; // Character rectangle scaling factor

  // Word/character wrapping mechanism variables
  enum { MEASURE_STATE = 0, DRAW_STATE = 1 };
  int state = wordWrap ? MEASURE_STATE : DRAW_STATE;

  int startLine = -1; // Index where to begin drawing (where a line begins)
  int endLine = -1;   // Index where to stop drawing (where a line ends)
  int lastk = -1;     // Holds last value of the character position

  for (int i = 0, k = 0; i < length; i++, k++) {
    // Get next codepoint from byte string and glyph index in font
    int codepointByteCount = 0;
    int codepoint = GetCodepoint(&text[i], &codepointByteCount);
    int index = GetGlyphIndex(font, codepoint);

    // NOTE: Normally we exit the decoding sequence as soon as a bad byte is
    // found (and return 0x3f) but we need to draw all of the bad bytes using
    // the '?' symbol moving one byte
    if (codepoint == 0x3f)
      codepointByteCount = 1;
    i += (codepointByteCount - 1);

    float glyphWidth = 0;
    if (codepoint != '\n') {
      glyphWidth = (font.glyphs[index].advanceX == 0)
                       ? font.recs[index].width * scaleFactor
                       : font.glyphs[index].advanceX * scaleFactor;

      if (i + 1 < length)
        glyphWidth = glyphWidth + spacing;
    }

    // NOTE: When wordWrap is ON we first measure how much of the text we can
    // draw before going outside of the rec container We store this info in
    // startLine and endLine, then we change states, draw the text between those
    // two variables and change states again and again recursively until the end
    // of the text (or until we get outside of the container). When wordWrap is
    // OFF we don't need the measure state so we go to the drawing state
    // immediately and begin drawing on the next line before we can get outside
    // the container.
    if (state == MEASURE_STATE) {
      // TODO: There are multiple types of spaces in UNICODE, maybe it's a good
      // idea to add support for more Ref: http://jkorpela.fi/chars/spaces.html
      if ((codepoint == ' ') || (codepoint == '\t') || (codepoint == '\n'))
        endLine = i;

      if ((textOffsetX + glyphWidth) > rec.width) {
        endLine = (endLine < 1) ? i : endLine;
        if (i == endLine)
          endLine -= codepointByteCount;
        if ((startLine + codepointByteCount) == endLine)
          endLine = (i - codepointByteCount);

        state = !state;
      } else if ((i + 1) == length) {
        endLine = i;
        state = !state;
      } else if (codepoint == '\n')
        state = !state;

      if (state == DRAW_STATE) {
        textOffsetX = 0;
        i = startLine;
        glyphWidth = 0;

        // Save character position when we switch states
        int tmp = lastk;
        lastk = k - 1;
        k = tmp;
      }
    } else {
      if (codepoint == '\n') {
        if (!wordWrap) {
          textOffsetY +=
              (int)(font.baseSize + font.baseSize / 2 + SUB_Y) * scaleFactor;
          textOffsetX = 0;
        }
      } else {
        if (!wordWrap && ((textOffsetX + glyphWidth) > rec.width)) {
          textOffsetY +=
              (int)(font.baseSize + font.baseSize / 2 + SUB_Y) * scaleFactor;
          textOffsetX = 0;
        }

        // When text overflows rectangle height limit, just stop drawing
        if ((textOffsetY + font.baseSize * scaleFactor) > rec.height)
          break;

        // Draw selection background
        bool isGlyphSelected = false;
        if ((selectStart >= 0) && (k >= selectStart) &&
            (k < (selectStart + selectLength))) {
          DrawRectangleRec({rec.x + textOffsetX - 1, rec.y + textOffsetY,
                            glyphWidth, (float)font.baseSize * scaleFactor},
                           selectBackTint);
          isGlyphSelected = true;
        }

        // Draw current character glyph
        if ((codepoint != ' ') && (codepoint != '\t')) {
          DrawTextCodepoint(font, codepoint,
                            {rec.x + textOffsetX, rec.y + textOffsetY},
                            fontSize, isGlyphSelected ? selectTint : tint);
        }
      }

      if (wordWrap && (i == endLine)) {
        textOffsetY +=
            (int)(font.baseSize + font.baseSize / 2 + SUB_Y) * scaleFactor;
        textOffsetX = 0;
        startLine = endLine;
        endLine = -1;
        glyphWidth = 0;
        selectStart += lastk - k;
        k = lastk;

        state = !state;
      }
    }

    if ((textOffsetX != 0) || (codepoint != ' '))
      textOffsetX += glyphWidth; // avoid leading spaces
  }
}
