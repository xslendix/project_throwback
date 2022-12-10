#pragma once

#include "common.hpp"

void ImageDrawFloodEx(Image *dst, Vector2 pos, Color color, Color to_replace);
void ImageDrawFlood(Image *dst, Vector2 pos, Color color);
void ImageDrawLineEx(Image *dst, Vector2 pos, Vector2 pos2, int thickness, Color color);

void DrawTextBoxed(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint);
void DrawTextBoxedSelectable(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint, int selectStart, int selectLength, Color selectTint, Color selectBackTint);

