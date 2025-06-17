// TODO: FIX EVERYTHING

#define setPixelBlend(pixel, frameBufferClr) frameBufferClr = ((pixel >> 1) & 0x7BEF) + ((frameBufferClr >> 1) & 0x7BEF)

// Alpha blending
#define setPixelAlpha(pixel, frameBufferClr, alpha)                                                                                                  \
    int32 R = (fbufferBlend[(frameBufferClr & 0xF800) >> 11] + pixelBlend[(pixel & 0xF800) >> 11]) << 11;                                            \
    int32 G = (fbufferBlend[(frameBufferClr & 0x7E0) >> 6] + pixelBlend[(pixel & 0x7E0) >> 6]) << 6;                                                 \
    int32 B = fbufferBlend[frameBufferClr & 0x1F] + pixelBlend[pixel & 0x1F];                                                                        \
                                                                                                                                                     \
    frameBufferClr = R | G | B;

// Additive Blending
#define setPixelAdditive(pixel, frameBufferClr)                                                                                                      \
    int32 R = MIN((blendTablePtr[(pixel & 0xF800) >> 11] << 11) + (frameBufferClr & 0xF800), 0xF800);                                                \
    int32 G = MIN((blendTablePtr[(pixel & 0x7E0) >> 6] << 6) + (frameBufferClr & 0x7E0), 0x7E0);                                                     \
    int32 B = MIN(blendTablePtr[pixel & 0x1F] + (frameBufferClr & 0x1F), 0x1F);                                                                      \
                                                                                                                                                     \
    frameBufferClr = R | G | B;

// Subtractive Blending
#define setPixelSubtractive(pixel, frameBufferClr)                                                                                                   \
    int32 R = MAX((frameBufferClr & 0xF800) - (subBlendTable[(pixel & 0xF800) >> 11] << 11), 0);                                                     \
    int32 G = MAX((frameBufferClr & 0x7E0) - (subBlendTable[(pixel & 0x7E0) >> 6] << 6), 0);                                                         \
    int32 B = MAX((frameBufferClr & 0x1F) - subBlendTable[pixel & 0x1F], 0);                                                                         \
                                                                                                                                                     \
    frameBufferClr = R | G | B;

// Only draw if framebuffer clr IS maskColor
#define setPixelMasked(pixel, frameBufferClr)                                                                                                        \
    if (frameBufferClr == maskColor)                                                                                                                 \
        frameBufferClr = pixel;

// Only draw if framebuffer clr is NOT maskColor
#define setPixelUnmasked(pixel, frameBufferClr)                                                                                                      \
    if (frameBufferClr != maskColor)                                                                                                                 \
        frameBufferClr = pixel;


void SoftwareBackend::DrawLine(uint8 drawGroup, int32 x1, int32 y1, int32 x2, int32 y2,
                               uint32 color, int32 alpha, int32 inkEffect, 
                               bool32 screenRelative) {
    // drawGroup ignored here

    switch (inkEffect) {
        default: break;

        case INK_ALPHA:
            if (alpha > 0xFF)
                inkEffect = INK_NONE;
            else if (alpha <= 0)
                return;
            break;

        case INK_ADD:
        case INK_SUB:
            if (alpha > 0xFF)
                alpha = 0xFF;
            else if (alpha <= 0)
                return;
            break;

        case INK_TINT:
            if (!tintLookupTable)
                return;
            break;
    }

    int32 drawY1 = y1;
    int32 drawX1 = x1;
    int32 drawY2 = y2;
    int32 drawX2 = x2;

    if (!screenRelative) {
        drawX1 = FROM_FIXED(x1) - currentScreen->position.x;
        drawY1 = FROM_FIXED(y1) - currentScreen->position.y;
        drawX2 = FROM_FIXED(x2) - currentScreen->position.x;
        drawY2 = FROM_FIXED(y2) - currentScreen->position.y;
    }

    int32 flags1 = 0;
    if (drawX1 >= currentScreen->clipBound_X2)
        flags1 = 2;
    else if (drawX1 < currentScreen->clipBound_X1)
        flags1 = 1;

    if (drawY1 >= currentScreen->clipBound_Y2)
        flags1 |= 8;
    else if (drawY1 < currentScreen->clipBound_Y1)
        flags1 |= 4;

    int32 flags2 = 0;
    if (drawX2 >= currentScreen->clipBound_X2)
        flags2 = 2;
    else if (drawX2 < currentScreen->clipBound_X1)
        flags2 = 1;

    if (drawY2 >= currentScreen->clipBound_Y2)
        flags2 |= 8;
    else if (drawY2 < currentScreen->clipBound_Y1)
        flags2 |= 4;

    while (flags1 || flags2) {
        if (flags1 & flags2)
            return;

        int32 curFlags = flags2;
        if (flags1)
            curFlags = flags1;

        int32 x = 0;
        int32 y = 0;
        if (curFlags & 8) {
            int32 div = (drawY2 - drawY1);
            if (!div)
                div = 1;
            x = drawX1 + ((drawX2 - drawX1) * (((currentScreen->clipBound_Y2 - drawY1) << 8) / div) >> 8);
            y = currentScreen->clipBound_Y2;
        }
        else if (curFlags & 4) {
            int32 div = (drawY2 - drawY1);
            if (!div)
                div = 1;
            x = drawX1 + ((drawX2 - drawX1) * (((currentScreen->clipBound_Y1 - drawY1) << 8) / div) >> 8);
            y = currentScreen->clipBound_Y1;
        }
        else if (curFlags & 2) {
            int32 div = (drawX2 - drawX1);
            if (!div)
                div = 1;
            x = currentScreen->clipBound_X2;
            y = drawY1 + ((drawY2 - drawY1) * (((currentScreen->clipBound_X2 - drawX1) << 8) / div) >> 8);
        }
        else if (curFlags & 1) {
            int32 div = (drawX2 - drawX1);
            if (!div)
                div = 1;
            x = currentScreen->clipBound_X1;
            y = drawY1 + ((drawY2 - drawY1) * (((currentScreen->clipBound_X1 - drawX1) << 8) / div) >> 8);
        }

        if (curFlags == flags1) {
            drawX1 = x;
            drawY1 = y;
            flags1 = 0;
            if (x > currentScreen->clipBound_X2) {
                flags1 = 2;
            }
            else if (x < currentScreen->clipBound_X1) {
                flags1 = 1;
            }

            if (y < currentScreen->clipBound_Y1) {
                flags1 |= 4;
            }
            else if (y > currentScreen->clipBound_Y2) {
                flags1 |= 8;
            }
        }
        else {
            drawX2 = x;
            drawY2 = y;
            flags2 = 0;
            if (x > currentScreen->clipBound_X2) {
                flags2 = 2;
            }
            else if (x < currentScreen->clipBound_X1) {
                flags2 = 1;
            }

            if (y < currentScreen->clipBound_Y1) {
                flags2 |= 4;
            }
            else if (y > currentScreen->clipBound_Y2) {
                flags2 |= 8;
            }
        }
    }

    if (drawX1 > currentScreen->clipBound_X2)
        drawX1 = currentScreen->clipBound_X2;
    else if (drawX1 < currentScreen->clipBound_X1)
        drawX1 = currentScreen->clipBound_X1;

    if (drawY1 > currentScreen->clipBound_Y2)
        drawY1 = currentScreen->clipBound_Y2;
    else if (drawY1 < currentScreen->clipBound_Y1)
        drawY1 = currentScreen->clipBound_Y1;

    if (drawX2 > currentScreen->clipBound_X2)
        drawX2 = currentScreen->clipBound_X2;
    else if (drawX2 < currentScreen->clipBound_X1)
        drawX2 = currentScreen->clipBound_X1;

    if (drawY2 > currentScreen->clipBound_Y2)
        drawY2 = currentScreen->clipBound_Y2;
    else if (drawY2 < currentScreen->clipBound_Y1)
        drawY2 = currentScreen->clipBound_Y1;

    int32 sizeX = abs(drawX2 - drawX1);
    int32 sizeY = abs(drawY2 - drawY1);
    int32 max   = sizeY;
    int32 hSize = sizeX >> 2;
    if (sizeX <= sizeY)
        hSize = -sizeY >> 2;

    if (drawX2 < drawX1) {
        int32 v = drawX1;
        drawX1  = drawX2;
        drawX2  = v;

        v      = drawY1;
        drawY1 = drawY2;
        drawY2 = v;
    }

    uint16 color16      = rgb32To16_B[(color >> 0) & 0xFF] | rgb32To16_G[(color >> 8) & 0xFF] | rgb32To16_R[(color >> 16) & 0xFF];
    uint16 *frameBuffer = &currentScreen->frameBuffer[drawX1 + drawY1 * currentScreen->pitch];

    switch (inkEffect) {
        case INK_NONE:
            if (drawY1 > drawY2) {
                while (drawX1 < drawX2 || drawY1 >= drawY2) {
                    *frameBuffer = color16;

                    if (hSize > -sizeX) {
                        hSize -= max;
                        ++drawX1;
                        ++frameBuffer;
                    }

                    if (hSize < max) {
                        --drawY1;
                        hSize += sizeX;
                        frameBuffer -= currentScreen->pitch;
                    }
                }
            }
            else {
                while (true) {
                    *frameBuffer = color16;

                    if (drawX1 < drawX2 || drawY1 < drawY2) {
                        if (hSize > -sizeX) {
                            hSize -= max;
                            ++drawX1;
                            ++frameBuffer;
                        }

                        if (hSize < max) {
                            hSize += sizeX;
                            ++drawY1;
                            frameBuffer += currentScreen->pitch;
                        }
                    }
                    else {
                        break;
                    }
                }
            }
            break;

        case INK_BLEND:
            if (drawY1 > drawY2) {
                while (drawX1 < drawX2 || drawY1 >= drawY2) {
                    setPixelBlend(color16, *frameBuffer);

                    if (hSize > -sizeX) {
                        hSize -= max;
                        ++drawX1;
                        ++frameBuffer;
                    }

                    if (hSize < max) {
                        --drawY1;
                        hSize += sizeX;
                        frameBuffer -= currentScreen->pitch;
                    }
                }
            }
            else {
                while (true) {
                    setPixelBlend(color16, *frameBuffer);

                    if (drawX1 < drawX2 || drawY1 < drawY2) {
                        if (hSize > -sizeX) {
                            hSize -= max;
                            ++drawX1;
                            ++frameBuffer;
                        }

                        if (hSize < max) {
                            hSize += sizeX;
                            ++drawY1;
                            frameBuffer += currentScreen->pitch;
                        }
                    }
                    else {
                        break;
                    }
                }
            }
            break;

        case INK_ALPHA:
            if (drawY1 > drawY2) {
                uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - alpha)];
                uint16 *pixelBlend   = &blendLookupTable[0x20 * alpha];

                while (drawX1 < drawX2 || drawY1 >= drawY2) {
                    setPixelAlpha(color16, *frameBuffer, alpha);

                    if (hSize > -sizeX) {
                        hSize -= max;
                        ++drawX1;
                        ++frameBuffer;
                    }

                    if (hSize < max) {
                        --drawY1;
                        hSize += sizeX;
                        frameBuffer -= currentScreen->pitch;
                    }
                }
            }
            else {
                uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - alpha)];
                uint16 *pixelBlend   = &blendLookupTable[0x20 * alpha];

                while (true) {
                    setPixelAlpha(color16, *frameBuffer, alpha);

                    if (drawX1 < drawX2 || drawY1 < drawY2) {
                        if (hSize > -sizeX) {
                            hSize -= max;
                            ++drawX1;
                            ++frameBuffer;
                        }
                        if (hSize < max) {
                            hSize += sizeX;
                            ++drawY1;
                            frameBuffer += currentScreen->pitch;
                        }
                    }
                    else {
                        break;
                    }
                }
            }
            break;

        case INK_ADD:
            if (drawY1 > drawY2) {
                uint16 *blendTablePtr = &blendLookupTable[0x20 * alpha];

                while (drawX1 < drawX2 || drawY1 >= drawY2) {
                    setPixelAdditive(color16, *frameBuffer);

                    if (hSize > -sizeX) {
                        hSize -= max;
                        ++drawX1;
                        ++frameBuffer;
                    }

                    if (hSize < max) {
                        --drawY1;
                        hSize += sizeX;
                        frameBuffer -= currentScreen->pitch;
                    }
                }
            }
            else {
                uint16 *blendTablePtr = &blendLookupTable[0x20 * alpha];

                while (true) {
                    setPixelAdditive(color16, *frameBuffer);
                    if (drawX1 < drawX2 || drawY1 < drawY2) {
                        if (hSize > -sizeX) {
                            hSize -= max;
                            ++drawX1;
                            ++frameBuffer;
                        }

                        if (hSize < max) {
                            hSize += sizeX;
                            ++drawY1;
                            frameBuffer += currentScreen->pitch;
                        }
                    }
                    else {
                        break;
                    }
                }
            }
            break;

        case INK_SUB:
            if (drawY1 > drawY2) {
                uint16 *subBlendTable = &subtractLookupTable[0x20 * alpha];
                while (drawX1 < drawX2 || drawY1 >= drawY2) {
                    setPixelSubtractive(color16, *frameBuffer);

                    if (hSize > -sizeX) {
                        hSize -= max;
                        ++drawX1;
                        ++frameBuffer;
                    }

                    if (hSize < max) {
                        --drawY1;
                        hSize += sizeX;
                        frameBuffer -= currentScreen->pitch;
                    }
                }
            }
            else {
                uint16 *subBlendTable = &subtractLookupTable[0x20 * alpha];
                while (true) {
                    setPixelSubtractive(color16, *frameBuffer);

                    if (drawX1 < drawX2 || drawY1 < drawY2) {
                        if (hSize > -sizeX) {
                            hSize -= max;
                            ++drawX1;
                            ++frameBuffer;
                        }

                        if (hSize < max) {
                            hSize += sizeX;
                            ++drawY1;
                            frameBuffer += currentScreen->pitch;
                        }
                    }
                    else {
                        break;
                    }
                }
            }
            break;

        case INK_TINT:
            if (drawY1 > drawY2) {
                while (drawX1 < drawX2 || drawY1 >= drawY2) {
                    *frameBuffer = tintLookupTable[*frameBuffer];

                    if (hSize > -sizeX) {
                        hSize -= max;
                        ++drawX1;
                        ++frameBuffer;
                    }

                    if (hSize < max) {
                        --drawY1;
                        hSize += sizeX;
                        frameBuffer -= currentScreen->pitch;
                    }
                }
            }
            else {
                while (true) {
                    *frameBuffer = tintLookupTable[*frameBuffer];

                    if (drawX1 < drawX2 || drawY1 < drawY2) {
                        if (hSize > -sizeX) {
                            hSize -= max;
                            ++drawX1;
                            ++frameBuffer;
                        }

                        if (hSize < max) {
                            hSize += sizeX;
                            ++drawY1;
                            frameBuffer += currentScreen->pitch;
                        }
                    }
                    else {
                        break;
                    }
                }
            }
            break;

        case INK_MASKED:
            if (drawY1 > drawY2) {
                while (drawX1 < drawX2 || drawY1 >= drawY2) {
                    if (*frameBuffer == maskColor)
                        *frameBuffer = color16;

                    if (hSize > -sizeX) {
                        hSize -= max;
                        ++drawX1;
                        ++frameBuffer;
                    }

                    if (hSize < max) {
                        --drawY1;
                        hSize += sizeX;
                        frameBuffer -= currentScreen->pitch;
                    }
                }
            }
            else {
                while (true) {
                    if (*frameBuffer == maskColor)
                        *frameBuffer = color16;

                    if (drawX1 < drawX2 || drawY1 < drawY2) {
                        if (hSize > -sizeX) {
                            hSize -= max;
                            ++drawX1;
                            ++frameBuffer;
                        }

                        if (hSize < max) {
                            hSize += sizeX;
                            ++drawY1;
                            frameBuffer += currentScreen->pitch;
                        }
                    }
                    else {
                        break;
                    }
                }
            }
            break;

        case INK_UNMASKED:
            if (drawY1 > drawY2) {
                while (drawX1 < drawX2 || drawY1 >= drawY2) {
                    if (*frameBuffer != maskColor)
                        *frameBuffer = color16;

                    if (hSize > -sizeX) {
                        hSize -= max;
                        ++drawX1;
                        ++frameBuffer;
                    }

                    if (hSize < max) {
                        --drawY1;
                        hSize += sizeX;
                        frameBuffer -= currentScreen->pitch;
                    }
                }
            }
            else {
                while (true) {
                    if (*frameBuffer != maskColor)
                        *frameBuffer = color16;

                    if (drawX1 < drawX2 || drawY1 < drawY2) {
                        if (hSize > -sizeX) {
                            hSize -= max;
                            ++drawX1;
                            ++frameBuffer;
                        }

                        if (hSize < max) {
                            hSize += sizeX;
                            ++drawY1;
                            frameBuffer += currentScreen->pitch;
                        }
                    }
                    else {
                        break;
                    }
                }
            }
            break;
    }
}

void SoftwareBackend::DrawRectangle(uint8 drawGroup, int32 x, int32 y, int32 width,
                                    int32 height, uint32 color, int32 alpha,
                                    int32 inkEffect, bool32 screenRelative) {
    switch (inkEffect) {
        default: break;
        case INK_ALPHA:
            if (alpha > 0xFF)
                inkEffect = INK_NONE;
            else if (alpha <= 0)
                return;
            break;

        case INK_ADD:
        case INK_SUB:
            if (alpha > 0xFF)
                alpha = 0xFF;
            else if (alpha <= 0)
                return;
            break;

        case INK_TINT:
            if (!tintLookupTable)
                return;
            break;
    }

    if (!screenRelative) {
        x      = FROM_FIXED(x) - currentScreen->position.x;
        y      = FROM_FIXED(y) - currentScreen->position.y;
        width  = FROM_FIXED(width);
        height = FROM_FIXED(height);
    }

    if (width + x > currentScreen->clipBound_X2)
        width = currentScreen->clipBound_X2 - x;

    if (x < currentScreen->clipBound_X1) {
        width += x - currentScreen->clipBound_X1;
        x = currentScreen->clipBound_X1;
    }

    if (height + y > currentScreen->clipBound_Y2)
        height = currentScreen->clipBound_Y2 - y;

    if (y < currentScreen->clipBound_Y1) {
        height += y - currentScreen->clipBound_Y1;
        y = currentScreen->clipBound_Y1;
    }

    if (width <= 0 || height <= 0)
        return;

    int32 pitch         = currentScreen->pitch - width;
    validDraw           = true;
    uint16 *frameBuffer = &currentScreen->frameBuffer[x + (y * currentScreen->pitch)];
    uint16 color16      = rgb32To16_B[(color >> 0) & 0xFF] | rgb32To16_G[(color >> 8) & 0xFF] | rgb32To16_R[(color >> 16) & 0xFF];

    switch (inkEffect) {
        case INK_NONE: {
            int32 h = height;
            while (h--) {
                int32 w = width;
                while (w--) {
                    *frameBuffer = color16;
                    ++frameBuffer;
                }

                frameBuffer += pitch;
            }
            break;
        }

        case INK_BLEND: {
            int32 h = height;
            while (h--) {
                int32 w = width;
                while (w--) {
                    setPixelBlend(color16, *frameBuffer);
                    ++frameBuffer;
                }
                frameBuffer += pitch;
            }
            break;
        }

        case INK_ALPHA: {
            uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - alpha)];
            uint16 *pixelBlend   = &blendLookupTable[0x20 * alpha];

            int32 h = height;
            while (h--) {
                int32 w = width;
                while (w--) {
                    setPixelAlpha(color16, *frameBuffer, alpha);
                    ++frameBuffer;
                }
                frameBuffer += pitch;
            }
            break;
        }

        case INK_ADD: {
            uint16 *blendTablePtr = &blendLookupTable[0x20 * alpha];
            int32 h               = height;
            while (h--) {
                int32 w = width;
                while (w--) {
                    setPixelAdditive(color16, *frameBuffer);
                    ++frameBuffer;
                }
                frameBuffer += pitch;
            }
            break;
        }

        case INK_SUB: {
            uint16 *subBlendTable = &subtractLookupTable[0x20 * alpha];
            int32 h               = height;
            while (h--) {
                int32 w = width;
                while (w--) {
                    setPixelSubtractive(color16, *frameBuffer);
                    ++frameBuffer;
                }
                frameBuffer += pitch;
            }
            break;
        }

        case INK_TINT: {
            int32 h = height;
            while (h--) {
                int32 w = width;
                while (w--) {
                    *frameBuffer = tintLookupTable[*frameBuffer];
                    ++frameBuffer;
                }
                frameBuffer += pitch;
            }
            break;
        }

        case INK_MASKED: {
            int32 h = height;
            while (h--) {
                int32 w = width;
                while (w--) {
                    if (*frameBuffer == maskColor)
                        *frameBuffer = color16;
                    ++frameBuffer;
                }
                frameBuffer += pitch;
            }
            break;
        }

        case INK_UNMASKED: {
            int32 h = height;
            while (h--) {
                int32 w = width;
                while (w--) {
                    if (*frameBuffer != maskColor)
                        *frameBuffer = color16;
                    ++frameBuffer;
                }
                frameBuffer += pitch;
            }
            break;
        }
    }

}

void SoftwareBackend::DrawCircle(uint8 drawGroup, int32 x, int32 y, int32 radius,
                                 uint32 color, int32 alpha, int32 inkEffect,
                                 bool32 screenRelative) {
    if (radius > 0) {
        switch (inkEffect) {
            default: break;
            case INK_ALPHA:
                if (alpha > 0xFF)
                    inkEffect = INK_NONE;
                else if (alpha <= 0)
                    return;
                break;

            case INK_ADD:
            case INK_SUB:
                if (alpha > 0xFF)
                    alpha = 0xFF;
                else if (alpha <= 0)
                    return;
                break;

            case INK_TINT:
                if (!tintLookupTable)
                    return;
                break;
        }

        if (!screenRelative) {
            x = FROM_FIXED(x) - currentScreen->position.x;
            y = FROM_FIXED(y) - currentScreen->position.y;
        }

        int32 yRadiusBottom = y + radius;
        int32 bottom        = yRadiusBottom + 1;
        int32 yRadiusTop    = y - radius;
        int32 top           = yRadiusTop;

        if (top < currentScreen->clipBound_Y1)
            top = currentScreen->clipBound_Y1;
        else if (top > currentScreen->clipBound_Y2)
            top = currentScreen->clipBound_Y2;

        if (bottom < currentScreen->clipBound_Y1)
            bottom = currentScreen->clipBound_Y1;
        else if (bottom > currentScreen->clipBound_Y2)
            bottom = currentScreen->clipBound_Y2;

        if (top != bottom) {
            for (int32 i = top; i < bottom; ++i) {
                scanEdgeBuffer[i].start = 0x7FFF;
                scanEdgeBuffer[i].end   = -1;
            }

            int32 r                  = 3 - 2 * radius;
            int32 xRad               = x - radius;
            int32 curY               = y;
            int32 curX               = x;
            int32 dist               = x - y;

            for (int32 i = 0; i <= radius; ++i) {
                int32 scanX = i + curX;

                if (yRadiusBottom >= top && yRadiusBottom <= bottom && scanX > scanEdgeBuffer[yRadiusBottom].end)
                    scanEdgeBuffer[yRadiusBottom].end = scanX;

                if (yRadiusTop >= top && yRadiusTop <= bottom && scanX > scanEdgeBuffer[yRadiusTop].end)
                    scanEdgeBuffer[yRadiusTop].end = scanX;

                int32 scanY = i + y;
                if (scanY >= top && scanY <= bottom) {
                    ScanEdge *edge = &scanEdgeBuffer[scanY];
                    if (yRadiusBottom + dist > edge->end)
                        edge->end = yRadiusBottom + dist;
                }

                if (curY >= top && curY <= bottom && yRadiusBottom + dist > scanEdgeBuffer[curY].end)
                    scanEdgeBuffer[curY].end = yRadiusBottom + dist;

                if (yRadiusBottom >= top && yRadiusBottom <= bottom && x < scanEdgeBuffer[yRadiusBottom].start)
                    scanEdgeBuffer[yRadiusBottom].start = x;

                if (yRadiusTop >= top && yRadiusTop <= bottom && x < scanEdgeBuffer[yRadiusTop].start)
                    scanEdgeBuffer[yRadiusTop].start = x;

                if (scanY >= top && scanY <= bottom) {
                    ScanEdge *edge = &scanEdgeBuffer[scanY];
                    if (xRad < edge->start)
                        edge->start = xRad;
                }

                if (curY >= top && curY <= bottom && xRad < scanEdgeBuffer[curY].start)
                    scanEdgeBuffer[curY].start = xRad;

                if (r >= 0) {
                    --yRadiusBottom;
                    ++yRadiusTop;
                    r += 4 * (i - radius) + 10;
                    --radius;
                    ++xRad;
                }
                else {
                    r += 4 * i + 6;
                }
                --curY;
                --x;
            }

            // validDraw              = true;
            uint16 *frameBuffer = &currentScreen->frameBuffer[top * currentScreen->pitch];
            uint16 color16      = rgb32To16_B[(color >> 0) & 0xFF] | rgb32To16_G[(color >> 8) & 0xFF] | rgb32To16_R[(color >> 16) & 0xFF];

            switch (inkEffect) {
                default: break;
                case INK_NONE:
                    if (top <= bottom) {
                        ScanEdge *edge = &scanEdgeBuffer[top];
                        int32 sizeY    = bottom - top;

                        for (int32 y = 0; y < sizeY; ++y) {
                            if (edge->start < currentScreen->clipBound_X1)
                                edge->start = currentScreen->clipBound_X1;
                            if (edge->start > currentScreen->clipBound_X2)
                                edge->start = currentScreen->clipBound_X2;

                            if (edge->end < currentScreen->clipBound_X1)
                                edge->end = currentScreen->clipBound_X1;
                            if (edge->end > currentScreen->clipBound_X2)
                                edge->end = currentScreen->clipBound_X2;

                            int32 count = edge->end - edge->start;
                            for (int32 x = 0; x < count; ++x) {
                                frameBuffer[edge->start + x] = color16;
                            }
                            ++edge;
                            frameBuffer += currentScreen->pitch;
                        }
                    }
                    break;

                case INK_BLEND:
                    if (top <= bottom) {
                        ScanEdge *edge = &scanEdgeBuffer[top];
                        int32 sizeY    = bottom - top;

                        for (int32 y = 0; y < sizeY; ++y) {
                            if (edge->start < currentScreen->clipBound_X1)
                                edge->start = currentScreen->clipBound_X1;
                            if (edge->start > currentScreen->clipBound_X2)
                                edge->start = currentScreen->clipBound_X2;

                            if (edge->end < currentScreen->clipBound_X1)
                                edge->end = currentScreen->clipBound_X1;
                            if (edge->end > currentScreen->clipBound_X2)
                                edge->end = currentScreen->clipBound_X2;

                            int32 count = edge->end - edge->start;
                            for (int32 x = 0; x < count; ++x) {
                                setPixelBlend(color16, frameBuffer[edge->start + x]);
                            }

                            ++edge;
                            frameBuffer += currentScreen->pitch;
                        }
                    }
                    break;

                case INK_ALPHA:
                    if (top <= bottom) {
                        uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - alpha)];
                        uint16 *pixelBlend   = &blendLookupTable[0x20 * alpha];

                        ScanEdge *edge = &scanEdgeBuffer[top];
                        int32 sizeY    = bottom - top;

                        for (int32 y = 0; y < sizeY; ++y) {
                            if (edge->start < currentScreen->clipBound_X1)
                                edge->start = currentScreen->clipBound_X1;
                            if (edge->start > currentScreen->clipBound_X2)
                                edge->start = currentScreen->clipBound_X2;

                            if (edge->end < currentScreen->clipBound_X1)
                                edge->end = currentScreen->clipBound_X1;
                            if (edge->end > currentScreen->clipBound_X2)
                                edge->end = currentScreen->clipBound_X2;

                            int32 count = edge->end - edge->start;
                            for (int32 x = 0; x < count; ++x) {
                                setPixelAlpha(color16, frameBuffer[edge->start + x], alpha);
                            }
                            ++edge;
                            frameBuffer += currentScreen->pitch;
                        }
                    }
                    break;

                case INK_ADD: {
                    uint16 *blendTablePtr = &blendLookupTable[0x20 * alpha];
                    if (top <= bottom) {
                        ScanEdge *edge = &scanEdgeBuffer[top];
                        int32 sizeY    = bottom - top;

                        for (int32 y = 0; y < sizeY; ++y) {
                            if (edge->start < currentScreen->clipBound_X1)
                                edge->start = currentScreen->clipBound_X1;
                            if (edge->start > currentScreen->clipBound_X2)
                                edge->start = currentScreen->clipBound_X2;

                            if (edge->end < currentScreen->clipBound_X1)
                                edge->end = currentScreen->clipBound_X1;
                            if (edge->end > currentScreen->clipBound_X2)
                                edge->end = currentScreen->clipBound_X2;

                            int32 count = edge->end - edge->start;
                            for (int32 x = 0; x < count; ++x) {
                                setPixelAdditive(color16, frameBuffer[edge->start + x]);
                            }
                            ++edge;
                            frameBuffer += currentScreen->pitch;
                        }
                    }
                    break;
                }

                case INK_SUB: {
                    uint16 *subBlendTable = &subtractLookupTable[0x20 * alpha];
                    if (top <= bottom) {
                        ScanEdge *edge = &scanEdgeBuffer[top];
                        int32 sizeY    = bottom - top;

                        for (int32 y = 0; y < sizeY; ++y) {
                            if (edge->start < currentScreen->clipBound_X1)
                                edge->start = currentScreen->clipBound_X1;
                            if (edge->start > currentScreen->clipBound_X2)
                                edge->start = currentScreen->clipBound_X2;

                            if (edge->end < currentScreen->clipBound_X1)
                                edge->end = currentScreen->clipBound_X1;
                            if (edge->end > currentScreen->clipBound_X2)
                                edge->end = currentScreen->clipBound_X2;

                            int32 count = edge->end - edge->start;
                            for (int32 x = 0; x < count; ++x) {
                                setPixelSubtractive(color16, frameBuffer[edge->start + x]);
                            }
                            ++edge;
                            frameBuffer += currentScreen->pitch;
                        }
                    }
                    break;
                }

                case INK_TINT:
                    if (top <= bottom) {
                        ScanEdge *edge = &scanEdgeBuffer[top];
                        int32 sizeY    = bottom - top;

                        for (int32 y = 0; y < sizeY; ++y) {
                            if (edge->start < currentScreen->clipBound_X1)
                                edge->start = currentScreen->clipBound_X1;
                            if (edge->start > currentScreen->clipBound_X2)
                                edge->start = currentScreen->clipBound_X2;

                            if (edge->end < currentScreen->clipBound_X1)
                                edge->end = currentScreen->clipBound_X1;
                            if (edge->end > currentScreen->clipBound_X2)
                                edge->end = currentScreen->clipBound_X2;

                            int32 count = edge->end - edge->start;
                            for (int32 x = 0; x < count; ++x) {
                                frameBuffer[edge->start + x] = tintLookupTable[frameBuffer[edge->start + x]];
                            }
                            ++edge;
                            frameBuffer += currentScreen->pitch;
                        }
                    }
                    break;

                case INK_MASKED:
                    if (top <= bottom) {
                        ScanEdge *edge = &scanEdgeBuffer[top];
                        int32 sizeY    = bottom - top;

                        for (int32 y = 0; y < sizeY; ++y) {
                            if (edge->start < currentScreen->clipBound_X1)
                                edge->start = currentScreen->clipBound_X1;
                            if (edge->start > currentScreen->clipBound_X2)
                                edge->start = currentScreen->clipBound_X2;

                            if (edge->end < currentScreen->clipBound_X1)
                                edge->end = currentScreen->clipBound_X1;
                            if (edge->end > currentScreen->clipBound_X2)
                                edge->end = currentScreen->clipBound_X2;

                            int32 count = edge->end - edge->start;
                            for (int32 x = 0; x < count; ++x) {
                                if (frameBuffer[edge->start + x] == maskColor)
                                    frameBuffer[edge->start + x] = color16;
                            }
                            ++edge;
                            frameBuffer += currentScreen->pitch;
                        }
                    }
                    break;

                case INK_UNMASKED:
                    if (top <= bottom) {
                        ScanEdge *edge = &scanEdgeBuffer[top];
                        int32 sizeY    = bottom - top;

                        for (int32 y = 0; y < sizeY; ++y) {
                            if (edge->start < currentScreen->clipBound_X1)
                                edge->start = currentScreen->clipBound_X1;
                            if (edge->start > currentScreen->clipBound_X2)
                                edge->start = currentScreen->clipBound_X2;

                            if (edge->end < currentScreen->clipBound_X1)
                                edge->end = currentScreen->clipBound_X1;
                            if (edge->end > currentScreen->clipBound_X2)
                                edge->end = currentScreen->clipBound_X2;

                            int32 count = edge->end - edge->start;
                            for (int32 x = 0; x < count; ++x) {
                                if (frameBuffer[edge->start + x] != maskColor)
                                    frameBuffer[edge->start + x] = color16;
                            }
                            ++edge;
                            frameBuffer += currentScreen->pitch;
                        }
                    }
                    break;
            }
        }
    }

}

void SoftwareBackend::DrawCircleOutline(uint8 drawGroup, int32 x, int32 y,
                                        int32 innerRadius, int32 outerRadius, uint32 color,
                                        int32 alpha, int32 inkEffect, 
                                        bool32 screenRelative) {
  switch (inkEffect) {
    default: break;
    case INK_ALPHA:
        if (alpha > 0xFF)
            inkEffect = INK_NONE;
        else if (alpha <= 0)
            return;
        break;

    case INK_ADD:
    case INK_SUB:
        if (alpha > 0xFF)
            alpha = 0xFF;
        else if (alpha <= 0)
            return;
        break;

    case INK_TINT:
        if (!tintLookupTable)
            return;
        break;
    }

    if (!screenRelative) {
        x = FROM_FIXED(x) - currentScreen->position.x;
        y = FROM_FIXED(y) - currentScreen->position.y;
    }

    if (outerRadius > 0 && innerRadius < outerRadius) {
        int32 top    = y - outerRadius;
        int32 left   = x - outerRadius;
        int32 right  = x + outerRadius;
        int32 bottom = y + outerRadius;

        if (left < currentScreen->clipBound_X1)
            left = currentScreen->clipBound_X1;
        if (left > currentScreen->clipBound_X2)
            left = currentScreen->clipBound_X2;

        if (right < currentScreen->clipBound_X1)
            right = currentScreen->clipBound_X1;
        if (right > currentScreen->clipBound_X2)
            right = currentScreen->clipBound_X2;

        if (top < currentScreen->clipBound_Y1)
            top = currentScreen->clipBound_Y1;
        if (top > currentScreen->clipBound_Y1)
            top = currentScreen->clipBound_Y1;

        if (bottom < currentScreen->clipBound_Y2)
            bottom = currentScreen->clipBound_Y2;
        if (bottom > currentScreen->clipBound_Y2)
            bottom = currentScreen->clipBound_Y2;

        if (left != right && top != bottom) {
            int32 ir2           = innerRadius * innerRadius;
            int32 or2           = outerRadius * outerRadius;
            validDraw           = true;
            uint16 *frameBuffer = &currentScreen->frameBuffer[left + top * currentScreen->pitch];
            uint16 color16      = rgb32To16_B[(color >> 0) & 0xFF] | rgb32To16_G[(color >> 8) & 0xFF] | rgb32To16_R[(color >> 16) & 0xFF];
            int32 pitch         = (left + currentScreen->pitch - right);

            switch (inkEffect) {
                default: break;
                case INK_NONE:
                    if (top < bottom) {
                        int32 distY1 = top - y;
                        int32 distY2 = bottom - top;
                        do {
                            int32 y2 = distY1 * distY1;
                            if (left < right) {
                                int32 distX1 = left - x;
                                int32 distX2 = right - left;
                                do {
                                    int32 r2 = y2 + distX1 * distX1;
                                    if (r2 >= ir2 && r2 < or2)
                                        *frameBuffer = color16;
                                    ++frameBuffer;
                                    ++distX1;
                                    --distX2;
                                } while (distX2);
                            }
                            frameBuffer += pitch;
                            --distY2;
                            ++distY1;
                        } while (distY2);
                    }
                    break;

                case INK_BLEND:
                    if (top < bottom) {
                        int32 distY1 = top - y;
                        int32 distY2 = bottom - top;
                        do {
                            int32 y2 = distY1 * distY1;
                            if (left < right) {
                                int32 distX1 = left - x;
                                int32 distX2 = right - left;
                                do {
                                    int32 r2 = y2 + distX1 * distX1;
                                    if (r2 >= ir2 && r2 < or2)
                                        setPixelBlend(color16, *frameBuffer);
                                    ++frameBuffer;
                                    ++distX1;
                                    --distX2;
                                } while (distX2);
                            }
                            frameBuffer += pitch;
                            --distY2;
                            ++distY1;
                        } while (distY2);
                    }
                    break;

                case INK_ALPHA:
                    if (top < bottom) {
                        uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - alpha)];
                        uint16 *pixelBlend   = &blendLookupTable[0x20 * alpha];

                        int32 distY1 = top - y;
                        int32 distY2 = bottom - top;
                        do {
                            int32 y2 = distY1 * distY1;
                            if (left < right) {
                                int32 distX1 = left - x;
                                int32 distX2 = right - left;
                                do {
                                    int32 r2 = y2 + distX1 * distX1;
                                    if (r2 >= ir2 && r2 < or2) {
                                        setPixelAlpha(color16, *frameBuffer, alpha);
                                    }
                                    ++frameBuffer;
                                    ++distX1;
                                    --distX2;
                                } while (distX2);
                            }
                            frameBuffer += pitch;
                            --distY2;
                            ++distY1;
                        } while (distY2);
                    }
                    break;

                case INK_ADD: {
                    uint16 *blendTablePtr = &blendLookupTable[0x20 * alpha];
                    if (top < bottom) {
                        int32 distY1 = top - y;
                        int32 distY2 = bottom - top;
                        do {
                            int32 y2 = distY1 * distY1;
                            if (left < right) {
                                int32 distX1 = left - x;
                                int32 distX2 = right - left;
                                do {
                                    int32 r2 = y2 + distX1 * distX1;
                                    if (r2 >= ir2 && r2 < or2) {
                                        setPixelAdditive(color16, *frameBuffer);
                                    }
                                    ++frameBuffer;
                                    ++distX1;
                                    --distX2;
                                } while (distX2);
                            }
                            frameBuffer += pitch;
                            --distY2;
                            ++distY1;
                        } while (distY2);
                    }
                    break;
                }

                case INK_SUB: {
                    uint16 *subBlendTable = &subtractLookupTable[0x20 * alpha];
                    if (top < bottom) {
                        int32 distY1 = top - y;
                        int32 distY2 = bottom - top;
                        do {
                            int32 y2 = distY1 * distY1;
                            if (left < right) {
                                int32 distX1 = left - x;
                                int32 distX2 = right - left;
                                do {
                                    int32 r2 = y2 + distX1 * distX1;
                                    if (r2 >= ir2 && r2 < or2) {
                                        setPixelSubtractive(color16, *frameBuffer);
                                    }
                                    ++frameBuffer;
                                    ++distX1;
                                    --distX2;
                                } while (distX2);
                            }
                            frameBuffer += pitch;
                            --distY2;
                            ++distY1;
                        } while (distY2);
                    }
                    break;
                }

                case INK_TINT:
                    if (top < bottom) {
                        int32 distY1 = top - y;
                        int32 distY2 = bottom - top;
                        do {
                            int32 y2 = distY1 * distY1;
                            if (left < right) {
                                int32 distX1 = left - x;
                                int32 distX2 = right - left;
                                do {
                                    int32 r2 = y2 + distX1 * distX1;
                                    if (r2 >= ir2 && r2 < or2)
                                        *frameBuffer = tintLookupTable[*frameBuffer];
                                    ++frameBuffer;
                                    ++distX1;
                                    --distX2;
                                } while (distX2);
                            }
                            frameBuffer += pitch;
                            --distY2;
                            ++distY1;
                        } while (distY2);
                    }
                    break;

                case INK_MASKED:
                    if (top < bottom) {
                        int32 distY1 = top - y;
                        int32 distY2 = bottom - top;
                        do {
                            int32 y2 = distY1 * distY1;
                            if (left < right) {
                                int32 distX1 = left - x;
                                int32 distX2 = right - left;
                                do {
                                    int32 r2 = y2 + distX1 * distX1;
                                    if (r2 >= ir2 && r2 < or2 && *frameBuffer == maskColor)
                                        *frameBuffer = color16;
                                    ++frameBuffer;
                                    ++distX1;
                                    --distX2;
                                } while (distX2);
                            }
                            frameBuffer += pitch;
                            --distY2;
                            ++distY1;
                        } while (distY2);
                    }
                    break;

                case INK_UNMASKED:
                    if (top < bottom) {
                        int32 distY1 = top - y;
                        int32 distY2 = bottom - top;
                        do {
                            int32 y2 = distY1 * distY1;
                            if (left < right) {
                                int32 distX1 = left - x;
                                int32 distX2 = right - left;
                                do {
                                    int32 r2 = y2 + distX1 * distX1;
                                    if (r2 >= ir2 && r2 < or2 && *frameBuffer != maskColor)
                                        *frameBuffer = color16;
                                    ++frameBuffer;
                                    ++distX1;
                                    --distX2;
                                } while (distX2);
                            }
                            frameBuffer += pitch;
                            --distY2;
                            ++distY1;
                        } while (distY2);
                    }
                    break;
            }
        }
    }

}

void SoftwareBackend::DrawFace(uint8 drawGroup, Vector2 *vertices, int32 vertCount,
                               int32 r, int32 g, int32 b, int32 alpha, 
                               int32 inkEffect) {
    switch (inkEffect) {
        default: break;
        case INK_ALPHA:
            if (alpha > 0xFF)
                inkEffect = INK_NONE;
            else if (alpha <= 0)
                return;
            break;

        case INK_ADD:
        case INK_SUB:
            if (alpha > 0xFF)
                alpha = 0xFF;
            else if (alpha <= 0)
                return;
            break;

        case INK_TINT:
            if (!tintLookupTable)
                return;
            break;
    }

    int32 top    = 0x7FFFFFFF;
    int32 bottom = -0x10000;
    for (int32 v = 0; v < vertCount; ++v) {
        if (vertices[v].y < top)
            top = vertices[v].y;
        if (vertices[v].y > bottom)
            bottom = vertices[v].y;
    }

    int32 topScreen    = FROM_FIXED(top);
    int32 bottomScreen = FROM_FIXED(bottom);

    if (topScreen < currentScreen->clipBound_Y1)
        topScreen = currentScreen->clipBound_Y1;
    if (topScreen > currentScreen->clipBound_Y2)
        topScreen = currentScreen->clipBound_Y2;

    if (bottomScreen < currentScreen->clipBound_Y1)
        bottomScreen = currentScreen->clipBound_Y1;
    if (bottomScreen > currentScreen->clipBound_Y2)
        bottomScreen = currentScreen->clipBound_Y2;

    if (topScreen != bottomScreen) {
        ScanEdge *edge = &scanEdgeBuffer[topScreen];
        for (int32 s = topScreen; s <= bottomScreen; ++s) {
            edge->start = 0x7FFF;
            edge->end   = -1;
            ++edge;
        }

        for (int32 v = 0; v < vertCount - 1; ++v) {
            ProcessScanEdge(vertices[v + 0].x, vertices[v + 0].y, vertices[v + 1].x, vertices[v + 1].y);
        }
        ProcessScanEdge(vertices[0].x, vertices[0].y, vertices[vertCount - 1].x, vertices[vertCount - 1].y);

        uint16 *frameBuffer = &currentScreen->frameBuffer[topScreen * currentScreen->pitch];
        uint16 color16      = rgb32To16_B[b] | rgb32To16_G[g] | rgb32To16_R[r];

        edge = &scanEdgeBuffer[topScreen];
        switch (inkEffect) {
            default: break;

            case INK_NONE:
                for (int32 s = topScreen; s <= bottomScreen; ++s) {
                    if (edge->start < currentScreen->clipBound_X1)
                        edge->start = currentScreen->clipBound_X1;
                    if (edge->start > currentScreen->clipBound_X2)
                        edge->start = currentScreen->clipBound_X2;

                    if (edge->end < currentScreen->clipBound_X1)
                        edge->end = currentScreen->clipBound_X1;
                    if (edge->end > currentScreen->clipBound_X2)
                        edge->end = currentScreen->clipBound_X2;

                    int32 count = edge->end - edge->start;
                    for (int32 x = 0; x < count; ++x) {
                        frameBuffer[edge->start + x] = color16;
                    }
                    ++edge;
                    frameBuffer += currentScreen->pitch;
                }
                break;

            case INK_BLEND:
                for (int32 s = topScreen; s <= bottomScreen; ++s) {
                    if (edge->start < currentScreen->clipBound_X1)
                        edge->start = currentScreen->clipBound_X1;
                    if (edge->start > currentScreen->clipBound_X2)
                        edge->start = currentScreen->clipBound_X2;

                    if (edge->end < currentScreen->clipBound_X1)
                        edge->end = currentScreen->clipBound_X1;
                    if (edge->end > currentScreen->clipBound_X2)
                        edge->end = currentScreen->clipBound_X2;

                    int32 count = edge->end - edge->start;
                    for (int32 x = 0; x < count; ++x) {
                        setPixelBlend(color16, frameBuffer[edge->start + x]);
                    }
                    ++edge;
                    frameBuffer += currentScreen->pitch;
                }
                break;

            case INK_ALPHA: {
                uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - alpha)];
                uint16 *pixelBlend   = &blendLookupTable[0x20 * alpha];

                for (int32 s = topScreen; s <= bottomScreen; ++s) {
                    if (edge->start < currentScreen->clipBound_X1)
                        edge->start = currentScreen->clipBound_X1;
                    if (edge->start > currentScreen->clipBound_X2)
                        edge->start = currentScreen->clipBound_X2;

                    if (edge->end < currentScreen->clipBound_X1)
                        edge->end = currentScreen->clipBound_X1;
                    if (edge->end > currentScreen->clipBound_X2)
                        edge->end = currentScreen->clipBound_X2;

                    int32 count = edge->end - edge->start;
                    for (int32 x = 0; x < count; ++x) {
                        setPixelAlpha(color16, frameBuffer[edge->start + x], alpha);
                    }
                    ++edge;
                    frameBuffer += currentScreen->pitch;
                }
                break;
            }

            case INK_ADD: {
                uint16 *blendTablePtr = &blendLookupTable[0x20 * alpha];

                for (int32 s = topScreen; s <= bottomScreen; ++s) {
                    if (edge->start < currentScreen->clipBound_X1)
                        edge->start = currentScreen->clipBound_X1;
                    if (edge->start > currentScreen->clipBound_X2)
                        edge->start = currentScreen->clipBound_X2;

                    if (edge->end < currentScreen->clipBound_X1)
                        edge->end = currentScreen->clipBound_X1;
                    if (edge->end > currentScreen->clipBound_X2)
                        edge->end = currentScreen->clipBound_X2;

                    int32 count = edge->end - edge->start;
                    for (int32 x = 0; x < count; ++x) {
                        setPixelAdditive(color16, frameBuffer[edge->start + x]);
                    }

                    ++edge;
                    frameBuffer += currentScreen->pitch;
                }
                break;
            }

            case INK_SUB: {
                uint16 *subBlendTable = &subtractLookupTable[0x20 * alpha];
                for (int32 s = topScreen; s <= bottomScreen; ++s) {
                    if (edge->start < currentScreen->clipBound_X1)
                        edge->start = currentScreen->clipBound_X1;
                    if (edge->start > currentScreen->clipBound_X2)
                        edge->start = currentScreen->clipBound_X2;

                    if (edge->end < currentScreen->clipBound_X1)
                        edge->end = currentScreen->clipBound_X1;
                    if (edge->end > currentScreen->clipBound_X2)
                        edge->end = currentScreen->clipBound_X2;

                    int32 count = edge->end - edge->start;
                    for (int32 x = 0; x < count; ++x) {
                        setPixelSubtractive(color16, frameBuffer[edge->start + x]);
                    }

                    ++edge;
                    frameBuffer += currentScreen->pitch;
                }
                break;
            }

            case INK_TINT:
                for (int32 s = topScreen; s <= bottomScreen; ++s) {
                    if (edge->start < currentScreen->clipBound_X1)
                        edge->start = currentScreen->clipBound_X1;
                    if (edge->start > currentScreen->clipBound_X2)
                        edge->start = currentScreen->clipBound_X2;

                    if (edge->end < currentScreen->clipBound_X1)
                        edge->end = currentScreen->clipBound_X1;
                    if (edge->end > currentScreen->clipBound_X2)
                        edge->end = currentScreen->clipBound_X2;

                    int32 count = edge->end - edge->start;
                    for (int32 x = 0; x < count; ++x) {
                        frameBuffer[edge->start + x] = tintLookupTable[frameBuffer[edge->start + x]];
                    }

                    ++edge;
                    frameBuffer += currentScreen->pitch;
                }
                break;

            case INK_MASKED:
                for (int32 s = topScreen; s <= bottomScreen; ++s) {
                    if (edge->start < currentScreen->clipBound_X1)
                        edge->start = currentScreen->clipBound_X1;
                    if (edge->start > currentScreen->clipBound_X2)
                        edge->start = currentScreen->clipBound_X2;

                    if (edge->end < currentScreen->clipBound_X1)
                        edge->end = currentScreen->clipBound_X1;
                    if (edge->end > currentScreen->clipBound_X2)
                        edge->end = currentScreen->clipBound_X2;

                    int32 count = edge->end - edge->start;
                    for (int32 x = 0; x < count; ++x) {
                        if (frameBuffer[edge->start + x] == maskColor)
                            frameBuffer[edge->start + x] = color16;
                    }

                    ++edge;
                    frameBuffer += currentScreen->pitch;
                }
                break;

            case INK_UNMASKED:
                for (int32 s = topScreen; s <= bottomScreen; ++s) {
                    if (edge->start < currentScreen->clipBound_X1)
                        edge->start = currentScreen->clipBound_X1;
                    if (edge->start > currentScreen->clipBound_X2)
                        edge->start = currentScreen->clipBound_X2;

                    if (edge->end < currentScreen->clipBound_X1)
                        edge->end = currentScreen->clipBound_X1;
                    if (edge->end > currentScreen->clipBound_X2)
                        edge->end = currentScreen->clipBound_X2;

                    int32 count = edge->end - edge->start;
                    for (int32 x = 0; x < count; ++x) {
                        if (frameBuffer[edge->start + x] != maskColor)
                            frameBuffer[edge->start + x] = color16;
                    }

                    ++edge;
                    frameBuffer += currentScreen->pitch;
                }
                break;
        }
    }

}

void SoftwareBackend::DrawBlendedFace(uint8 drawGroup, Vector2 *vertices, uint32 *colors,
                                      int32 vertCount, int32 alpha, int32 inkEffect) {
    switch (inkEffect) {
        default: break;
        case INK_ALPHA:
            if (alpha > 0xFF)
                inkEffect = INK_NONE;
            else if (alpha <= 0)
                return;
            break;

        case INK_ADD:
        case INK_SUB:
            if (alpha > 0xFF)
                alpha = 0xFF;
            else if (alpha <= 0)
                return;
            break;

        case INK_TINT:
            if (!tintLookupTable)
                return;
            break;
    }

    int32 top    = 0x7FFFFFFF;
    int32 bottom = -0x10000;
    for (int32 v = 0; v < vertCount; ++v) {
        if (vertices[v].y < top)
            top = vertices[v].y;
        if (vertices[v].y > bottom)
            bottom = vertices[v].y;
    }

    int32 topScreen    = FROM_FIXED(top);
    int32 bottomScreen = FROM_FIXED(bottom);

    if (topScreen < currentScreen->clipBound_Y1)
        topScreen = currentScreen->clipBound_Y1;
    if (topScreen > currentScreen->clipBound_Y2)
        topScreen = currentScreen->clipBound_Y2;

    if (bottomScreen < currentScreen->clipBound_Y1)
        bottomScreen = currentScreen->clipBound_Y1;
    if (bottomScreen > currentScreen->clipBound_Y2)
        bottomScreen = currentScreen->clipBound_Y2;

    if (topScreen != bottomScreen) {
        ScanEdge *edge = &scanEdgeBuffer[topScreen];
        for (int32 s = topScreen; s <= bottomScreen; ++s) {
            edge->start = 0x7FFF;
            edge->end   = -1;
            ++edge;
        }

        for (int32 v = 0; v < vertCount - 1; ++v) {
            ProcessScanEdgeClr(colors[v + 0], colors[v + 1], vertices[v + 0].x, vertices[v + 0].y, vertices[v + 1].x, vertices[v + 1].y);
        }
        ProcessScanEdgeClr(colors[vertCount - 1], colors[0], vertices[vertCount - 1].x, vertices[vertCount - 1].y, vertices[0].x, vertices[0].y);

        uint16 *frameBuffer = &currentScreen->frameBuffer[topScreen * currentScreen->pitch];

        edge = &scanEdgeBuffer[topScreen];
        switch (inkEffect) {
            default: break;
            case INK_NONE:
                for (int32 s = topScreen; s <= bottomScreen; ++s) {
                    int32 count  = edge->end - edge->start;
                    int32 deltaR = 0;
                    int32 deltaG = 0;
                    int32 deltaB = 0;
                    if (count > 0) {
                        deltaR = (edge->endR - edge->startR) / count;
                        deltaG = (edge->endG - edge->startG) / count;
                        deltaB = (edge->endB - edge->startB) / count;
                    }
                    int32 startR = edge->startR;
                    int32 startG = edge->startG;
                    int32 startB = edge->startB;

                    if (edge->start > currentScreen->clipBound_X2) {
                        edge->start = currentScreen->clipBound_X2;
                    }
                    else if (edge->start < currentScreen->clipBound_X1) {
                        int32 dist = (currentScreen->clipBound_X1 - edge->start);
                        startR += deltaR * dist;
                        startG += deltaG * dist;
                        startB += deltaB * dist;
                        count -= dist;
                        edge->start = currentScreen->clipBound_X1;
                    }

                    if (edge->end < currentScreen->clipBound_X1) {
                        edge->end = currentScreen->clipBound_X1;
                        count     = currentScreen->clipBound_X1 - edge->start;
                    }

                    if (edge->end > currentScreen->clipBound_X2) {
                        edge->end = currentScreen->clipBound_X2;
                        count     = currentScreen->clipBound_X2 - edge->start;
                    }

                    for (int32 x = 0; x < count; ++x) {
                        frameBuffer[edge->start + x] = (startB >> 19) + ((startG >> 13) & 0x7E0) + ((startR >> 8) & 0xF800);

                        startR += deltaR;
                        startG += deltaG;
                        startB += deltaB;
                    }
                    ++edge;
                    frameBuffer += currentScreen->pitch;
                }
                break;

            case INK_BLEND:
                for (int32 s = topScreen; s <= bottomScreen; ++s) {
                    int32 start  = edge->start;
                    int32 count  = edge->end - edge->start;
                    int32 deltaR = 0;
                    int32 deltaG = 0;
                    int32 deltaB = 0;
                    if (count > 0) {
                        deltaR = (edge->endR - edge->startR) / count;
                        deltaG = (edge->endG - edge->startG) / count;
                        deltaB = (edge->endB - edge->startB) / count;
                    }
                    int32 startR = edge->startR;
                    int32 startG = edge->startG;
                    int32 startB = edge->startB;

                    if (start > currentScreen->clipBound_X2) {
                        edge->start = currentScreen->clipBound_X2;
                    }

                    if (start < currentScreen->clipBound_X1) {
                        startR += deltaR * (currentScreen->clipBound_X1 - edge->start);
                        startG += deltaG * (currentScreen->clipBound_X1 - edge->start);
                        startB += deltaB * (currentScreen->clipBound_X1 - edge->start);
                        count -= (currentScreen->clipBound_X1 - edge->start);
                        edge->start = currentScreen->clipBound_X1;
                    }

                    if (edge->end < currentScreen->clipBound_X1) {
                        edge->end = currentScreen->clipBound_X1;
                    }

                    if (edge->end > currentScreen->clipBound_X2) {
                        edge->end = currentScreen->clipBound_X2;
                        count     = currentScreen->clipBound_X2 - edge->start;
                    }

                    for (int32 x = 0; x < count; ++x) {
                        uint16 color = (startB >> 19) + ((startG >> 13) & 0x7E0) + ((startR >> 8) & 0xF800);
                        setPixelBlend(color, frameBuffer[edge->start + x]);

                        startR += deltaR;
                        startG += deltaG;
                        startB += deltaB;
                    }

                    ++edge;
                    frameBuffer += currentScreen->pitch;
                }
                break;

            case INK_ALPHA: {
                uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - alpha)];
                uint16 *pixelBlend   = &blendLookupTable[0x20 * alpha];

                for (int32 s = topScreen; s <= bottomScreen; ++s) {
                    int32 start  = edge->start;
                    int32 count  = edge->end - edge->start;
                    int32 deltaR = 0;
                    int32 deltaG = 0;
                    int32 deltaB = 0;
                    if (count > 0) {
                        deltaR = (edge->endR - edge->startR) / count;
                        deltaG = (edge->endG - edge->startG) / count;
                        deltaB = (edge->endB - edge->startB) / count;
                    }
                    int32 startR = edge->startR;
                    int32 startG = edge->startG;
                    int32 startB = edge->startB;

                    if (start > currentScreen->clipBound_X2) {
                        edge->start = currentScreen->clipBound_X2;
                    }

                    if (start < currentScreen->clipBound_X1) {
                        startR += deltaR * (currentScreen->clipBound_X1 - edge->start);
                        startG += deltaG * (currentScreen->clipBound_X1 - edge->start);
                        startB += deltaB * (currentScreen->clipBound_X1 - edge->start);
                        count -= (currentScreen->clipBound_X1 - edge->start);
                        edge->start = currentScreen->clipBound_X1;
                    }

                    if (edge->end < currentScreen->clipBound_X1) {
                        edge->end = currentScreen->clipBound_X1;
                    }

                    if (edge->end > currentScreen->clipBound_X2) {
                        edge->end = currentScreen->clipBound_X2;
                        count     = currentScreen->clipBound_X2 - edge->start;
                    }

                    for (int32 x = 0; x < count; ++x) {
                        uint16 color = (startB >> 19) + ((startG >> 13) & 0x7E0) + ((startR >> 8) & 0xF800);
                        setPixelAlpha(color, frameBuffer[edge->start + x], alpha);

                        startR += deltaR;
                        startG += deltaG;
                        startB += deltaB;
                    }
                    ++edge;
                    frameBuffer += currentScreen->pitch;
                }
                break;
            }

            case INK_ADD: {
                uint16 *blendTablePtr = &blendLookupTable[0x20 * alpha];

                for (int32 s = topScreen; s <= bottomScreen; ++s) {
                    int32 start  = edge->start;
                    int32 count  = edge->end - edge->start;
                    int32 deltaR = 0;
                    int32 deltaG = 0;
                    int32 deltaB = 0;
                    if (count > 0) {
                        deltaR = (edge->endR - edge->startR) / count;
                        deltaG = (edge->endG - edge->startG) / count;
                        deltaB = (edge->endB - edge->startB) / count;
                    }
                    int32 startR = edge->startR;
                    int32 startG = edge->startG;
                    int32 startB = edge->startB;

                    if (start > currentScreen->clipBound_X2) {
                        edge->start = currentScreen->clipBound_X2;
                    }

                    if (start < currentScreen->clipBound_X1) {
                        startR += deltaR * (currentScreen->clipBound_X1 - edge->start);
                        startG += deltaG * (currentScreen->clipBound_X1 - edge->start);
                        startB += deltaB * (currentScreen->clipBound_X1 - edge->start);
                        count -= (currentScreen->clipBound_X1 - edge->start);
                        edge->start = currentScreen->clipBound_X1;
                    }

                    if (edge->end < currentScreen->clipBound_X1) {
                        edge->end = currentScreen->clipBound_X1;
                    }

                    if (edge->end > currentScreen->clipBound_X2) {
                        edge->end = currentScreen->clipBound_X2;
                        count     = currentScreen->clipBound_X2 - edge->start;
                    }

                    for (int32 x = 0; x < count; ++x) {
                        uint16 color = (startB >> 19) + ((startG >> 13) & 0x7E0) + ((startR >> 8) & 0xF800);
                        setPixelAdditive(color, frameBuffer[edge->start + x]);

                        startR += deltaR;
                        startG += deltaG;
                        startB += deltaB;
                    }

                    ++edge;
                    frameBuffer += currentScreen->pitch;
                }
                break;
            }

            case INK_SUB: {
                uint16 *subBlendTable = &subtractLookupTable[0x20 * alpha];

                for (int32 s = topScreen; s <= bottomScreen; ++s) {
                    int32 start  = edge->start;
                    int32 count  = edge->end - edge->start;
                    int32 deltaR = 0;
                    int32 deltaG = 0;
                    int32 deltaB = 0;
                    if (count > 0) {
                        deltaR = (edge->endR - edge->startR) / count;
                        deltaG = (edge->endG - edge->startG) / count;
                        deltaB = (edge->endB - edge->startB) / count;
                    }
                    int32 startR = edge->startR;
                    int32 startG = edge->startG;
                    int32 startB = edge->startB;

                    if (start > currentScreen->clipBound_X2) {
                        edge->start = currentScreen->clipBound_X2;
                    }

                    if (start < currentScreen->clipBound_X1) {
                        startR += deltaR * (currentScreen->clipBound_X1 - edge->start);
                        startG += deltaG * (currentScreen->clipBound_X1 - edge->start);
                        startB += deltaB * (currentScreen->clipBound_X1 - edge->start);
                        count -= (currentScreen->clipBound_X1 - edge->start);
                        edge->start = currentScreen->clipBound_X1;
                    }

                    if (edge->end < currentScreen->clipBound_X1) {
                        edge->end = currentScreen->clipBound_X1;
                    }

                    if (edge->end > currentScreen->clipBound_X2) {
                        edge->end = currentScreen->clipBound_X2;
                        count     = currentScreen->clipBound_X2 - edge->start;
                    }

                    for (int32 x = 0; x < count; ++x) {
                        uint16 color = (startB >> 19) + ((startG >> 13) & 0x7E0) + ((startR >> 8) & 0xF800);
                        setPixelSubtractive(color, frameBuffer[edge->start + x]);

                        startR += deltaR;
                        startG += deltaG;
                        startB += deltaB;
                    }

                    ++edge;
                    frameBuffer += currentScreen->pitch;
                }
                break;
            }

            case INK_TINT:
                for (int32 s = topScreen; s <= bottomScreen; ++s) {
                    int32 start  = edge->start;
                    int32 count  = edge->end - edge->start;

#if RETRO_USE_ORIGINAL_CODE
                    // Unused, ifdef'd out to help out ports for weaker hardware
                    int32 deltaR = 0;
                    int32 deltaG = 0;
                    int32 deltaB = 0;
                    if (count > 0) {
                        deltaR = (edge->endR - edge->startR) / count;
                        deltaG = (edge->endG - edge->startG) / count;
                        deltaB = (edge->endB - edge->startB) / count;
                    }
                    int32 startR = edge->startR;
                    int32 startG = edge->startG;
                    int32 startB = edge->startB;
#endif

                    if (start > currentScreen->clipBound_X2) {
                        edge->start = currentScreen->clipBound_X2;
                    }

                    if (start < currentScreen->clipBound_X1) {
#if RETRO_USE_ORIGINAL_CODE
                        // Unused, ifdef'd out to help out ports for weaker hardware
                        startR += deltaR * (currentScreen->clipBound_X1 - edge->start);
                        startG += deltaG * (currentScreen->clipBound_X1 - edge->start);
                        startB += deltaB * (currentScreen->clipBound_X1 - edge->start);
#endif

                        count -= (currentScreen->clipBound_X1 - edge->start);
                        edge->start = currentScreen->clipBound_X1;
                    }

                    if (edge->end < currentScreen->clipBound_X1) {
                        edge->end = currentScreen->clipBound_X1;
                    }

                    if (edge->end > currentScreen->clipBound_X2) {
                        edge->end = currentScreen->clipBound_X2;
                        count     = currentScreen->clipBound_X2 - edge->start;
                    }

                    for (int32 x = 0; x < count; ++x) {
                        frameBuffer[edge->start + x] = tintLookupTable[frameBuffer[edge->start + x]];

#if RETRO_USE_ORIGINAL_CODE
                        // Unused, ifdef'd out to help out ports for weaker hardware
                        startR += deltaR;
                        startG += deltaG;
                        startB += deltaB;
#endif
                    }

                    ++edge;
                    frameBuffer += currentScreen->pitch;
                }
                break;

            case INK_MASKED:
                for (int32 s = topScreen; s <= bottomScreen; ++s) {
                    int32 start  = edge->start;
                    int32 count  = edge->end - edge->start;
                    int32 deltaR = 0;
                    int32 deltaG = 0;
                    int32 deltaB = 0;
                    if (count > 0) {
                        deltaR = (edge->endR - edge->startR) / count;
                        deltaG = (edge->endG - edge->startG) / count;
                        deltaB = (edge->endB - edge->startB) / count;
                    }
                    int32 startR = edge->startR;
                    int32 startG = edge->startG;
                    int32 startB = edge->startB;

                    if (start > currentScreen->clipBound_X2) {
                        edge->start = currentScreen->clipBound_X2;
                    }

                    if (start < currentScreen->clipBound_X1) {
                        startR += deltaR * (currentScreen->clipBound_X1 - edge->start);
                        startG += deltaG * (currentScreen->clipBound_X1 - edge->start);
                        startB += deltaB * (currentScreen->clipBound_X1 - edge->start);
                        count -= (currentScreen->clipBound_X1 - edge->start);
                        edge->start = currentScreen->clipBound_X1;
                    }

                    if (edge->end < currentScreen->clipBound_X1 || edge->end > currentScreen->clipBound_X2) {
                        edge->end = currentScreen->clipBound_X2;
                        count     = currentScreen->clipBound_X2 - edge->start;
                    }

                    for (int32 x = 0; x < count; ++x) {
                        if (frameBuffer[edge->start + x] == maskColor)
                            frameBuffer[edge->start + x] = (startB >> 19) + ((startG >> 13) & 0x7E0) + ((startR >> 8) & 0xF800);

                        startR += deltaR;
                        startG += deltaG;
                        startB += deltaB;
                    }

                    ++edge;
                    frameBuffer += currentScreen->pitch;
                }
                break;

            case INK_UNMASKED:
                for (int32 s = topScreen; s <= bottomScreen; ++s) {
                    int32 start  = edge->start;
                    int32 count  = edge->end - edge->start;
                    int32 deltaR = 0;
                    int32 deltaG = 0;
                    int32 deltaB = 0;
                    if (count > 0) {
                        deltaR = (edge->endR - edge->startR) / count;
                        deltaG = (edge->endG - edge->startG) / count;
                        deltaB = (edge->endB - edge->startB) / count;
                    }
                    int32 startR = edge->startR;
                    int32 startG = edge->startG;
                    int32 startB = edge->startB;

                    if (start > currentScreen->clipBound_X2) {
                        edge->start = currentScreen->clipBound_X2;
                    }

                    if (start < currentScreen->clipBound_X1) {
                        startR += deltaR * (currentScreen->clipBound_X1 - edge->start);
                        startG += deltaG * (currentScreen->clipBound_X1 - edge->start);
                        startB += deltaB * (currentScreen->clipBound_X1 - edge->start);
                        count -= (currentScreen->clipBound_X1 - edge->start);
                        edge->start = currentScreen->clipBound_X1;
                    }

                    if (edge->end < currentScreen->clipBound_X1) {
                        edge->end = currentScreen->clipBound_X1;
                    }

                    if (edge->end > currentScreen->clipBound_X2) {
                        edge->end = currentScreen->clipBound_X2;
                        count     = currentScreen->clipBound_X2 - edge->start;
                    }

                    for (int32 x = 0; x < count; ++x) {
                        if (frameBuffer[edge->start + x] != maskColor)
                            frameBuffer[edge->start + x] = (startB >> 19) + ((startG >> 13) & 0x7E0) + ((startR >> 8) & 0xF800);

                        startR += deltaR;
                        startG += deltaG;
                        startB += deltaB;
                    }

                    ++edge;
                    frameBuffer += currentScreen->pitch;
                }
                break;
        }
    }

}

void SoftwareBackend::DrawSpriteFlipped(uint8 drawGroup, int32 x, int32 y, 
                       int32 width, int32 height,
                       int32 sprX, int32 sprY, int32 direction, int32 inkEffect,
                       int32 alpha, int32 sheetID) {
    switch (inkEffect) {
        default: break;
        case INK_ALPHA:
            if (alpha > 0xFF)
                inkEffect = INK_NONE;
            else if (alpha <= 0)
                return;
            break;

        case INK_ADD:
        case INK_SUB:
            if (alpha > 0xFF)
                alpha = 0xFF;
            else if (alpha <= 0)
                return;
            break;

        case INK_TINT:
            if (!tintLookupTable)
                return;
            break;
    }
    int32 widthFlip  = width;
    int32 heightFlip = height;

    if (width + x > currentScreen->clipBound_X2)
        width = currentScreen->clipBound_X2 - x;

    if (x < currentScreen->clipBound_X1) {
        int32 val = x - currentScreen->clipBound_X1;
        sprX -= val;
        width += val;
        widthFlip += 2 * val;
        x = currentScreen->clipBound_X1;
    }

    if (height + y > currentScreen->clipBound_Y2)
        height = currentScreen->clipBound_Y2 - y;

    if (y < currentScreen->clipBound_Y1) {
        int32 val = y - currentScreen->clipBound_Y1;
        sprY -= val;
        height += val;
        heightFlip += 2 * val;
        y = currentScreen->clipBound_Y1;
    }

    if (width <= 0 || height <= 0)
        return;

    GFXSurface *surface = &gfxSurface[sheetID];
    validDraw           = true;
    int32 pitch         = currentScreen->pitch - width;
    int32 gfxPitch      = 0;
    uint8 *lineBuffer   = NULL;
    uint8 *pixels       = NULL;
    uint16 *frameBuffer = NULL;

    switch (direction) {
        default: break;

        case FLIP_NONE:
            gfxPitch    = surface->width - width;
            lineBuffer  = &gfxLineBuffer[y];
            pixels      = &surface->pixels[sprX + surface->width * sprY];
            frameBuffer = &currentScreen->frameBuffer[x + currentScreen->pitch * y];
            switch (inkEffect) {
                case INK_NONE:
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels += gfxPitch;
                    }
                    break;

                case INK_BLEND:
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0)
                                setPixelBlend(activePalette[*pixels], *frameBuffer);
                            ++pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels += gfxPitch;
                    }
                    break;

                case INK_ALPHA: {
                    uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - alpha)];
                    uint16 *pixelBlend   = &blendLookupTable[0x20 * alpha];

                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0) {
                                uint16 color = activePalette[*pixels];
                                setPixelAlpha(color, *frameBuffer, alpha);
                            }
                            ++pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels += gfxPitch;
                    }
                    break;
                }

                case INK_ADD: {
                    uint16 *blendTablePtr = &blendLookupTable[0x20 * alpha];
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0) {
                                uint16 color = activePalette[*pixels];
                                setPixelAdditive(color, *frameBuffer);
                            }
                            ++pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels += gfxPitch;
                    }
                    break;
                }

                case INK_SUB: {
                    uint16 *subBlendTable = &subtractLookupTable[0x20 * alpha];
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0) {
                                uint16 color = activePalette[*pixels];
                                setPixelSubtractive(color, *frameBuffer);
                            }
                            ++pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels += gfxPitch;
                    }
                    break;
                }

                case INK_TINT:
                    while (height--) {
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0)
                                *frameBuffer = tintLookupTable[*frameBuffer];
                            ++pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels += gfxPitch;
                    }
                    break;

                case INK_MASKED:
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0 && *frameBuffer == maskColor)
                                *frameBuffer = activePalette[*pixels];
                            ++pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels += gfxPitch;
                    }
                    break;

                case INK_UNMASKED:
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0 && *frameBuffer != maskColor)
                                *frameBuffer = activePalette[*pixels];
                            ++pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels += gfxPitch;
                    }
                    break;
            }
            break;

        case FLIP_X:
            gfxPitch    = width + surface->width;
            lineBuffer  = &gfxLineBuffer[y];
            pixels      = &surface->pixels[widthFlip - 1 + sprX + surface->width * sprY];
            frameBuffer = &currentScreen->frameBuffer[x + currentScreen->pitch * y];
            switch (inkEffect) {
                case INK_NONE:
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            --pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels += gfxPitch;
                    }
                    break;

                case INK_BLEND:
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0)
                                setPixelBlend(activePalette[*pixels], *frameBuffer);
                            --pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels += gfxPitch;
                    }
                    break;

                case INK_ALPHA: {
                    uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - alpha)];
                    uint16 *pixelBlend   = &blendLookupTable[0x20 * alpha];

                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0) {
                                uint16 color = activePalette[*pixels];
                                setPixelAlpha(color, *frameBuffer, alpha);
                            }
                            --pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels += gfxPitch;
                    }
                    break;
                }

                case INK_ADD: {
                    uint16 *blendTablePtr = &blendLookupTable[0x20 * alpha];
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0) {
                                uint16 color = activePalette[*pixels];
                                setPixelAdditive(color, *frameBuffer);
                            }
                            --pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels += gfxPitch;
                    }
                    break;
                }

                case INK_SUB: {
                    uint16 *subBlendTable = &subtractLookupTable[0x20 * alpha];
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0) {
                                uint16 color = activePalette[*pixels];
                                setPixelSubtractive(color, *frameBuffer);
                            }
                            --pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels += gfxPitch;
                    }
                    break;
                }

                case INK_TINT:
                    while (height--) {
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0)
                                *frameBuffer = tintLookupTable[*frameBuffer];
                            --pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels += gfxPitch;
                    }
                    break;

                case INK_MASKED:
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0 && *frameBuffer == maskColor)
                                *frameBuffer = activePalette[*pixels];
                            --pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels += gfxPitch;
                    }
                    break;

                case INK_UNMASKED:
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0 && *frameBuffer != maskColor)
                                *frameBuffer = activePalette[*pixels];
                            --pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels += gfxPitch;
                    }
                    break;
            }
            break;

        case FLIP_Y:
            gfxPitch    = width + surface->width;
            lineBuffer  = &gfxLineBuffer[y];
            pixels      = &surface->pixels[sprX + surface->width * (sprY + heightFlip - 1)];
            frameBuffer = &currentScreen->frameBuffer[x + currentScreen->pitch * y];
            switch (inkEffect) {
                case INK_NONE:
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            ++pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels -= gfxPitch;
                    }
                    break;

                case INK_BLEND:
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0)
                                setPixelBlend(activePalette[*pixels], *frameBuffer);
                            ++pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels -= gfxPitch;
                    }
                    break;

                case INK_ALPHA: {
                    uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - alpha)];
                    uint16 *pixelBlend   = &blendLookupTable[0x20 * alpha];

                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0) {
                                uint16 color = activePalette[*pixels];
                                setPixelAlpha(color, *frameBuffer, alpha);
                            }
                            ++pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels -= gfxPitch;
                    }
                    break;
                }

                case INK_ADD: {
                    uint16 *blendTablePtr = &blendLookupTable[0x20 * alpha];
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0) {
                                uint16 color = activePalette[*pixels];
                                setPixelAdditive(color, *frameBuffer);
                            }
                            ++pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels -= gfxPitch;
                    }
                    break;
                }

                case INK_SUB: {
                    uint16 *subBlendTable = &subtractLookupTable[0x20 * alpha];
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0) {
                                uint16 color = activePalette[*pixels];
                                setPixelSubtractive(color, *frameBuffer);
                            }
                            ++pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels -= gfxPitch;
                    }
                    break;
                }

                case INK_TINT:
                    while (height--) {
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0)
                                *frameBuffer = tintLookupTable[*frameBuffer];
                            ++pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels -= gfxPitch;
                    }
                    break;

                case INK_MASKED:
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0 && *frameBuffer == maskColor)
                                *frameBuffer = activePalette[*pixels];
                            ++pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels -= gfxPitch;
                    }
                    break;

                case INK_UNMASKED:
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0 && *frameBuffer != maskColor)
                                *frameBuffer = activePalette[*pixels];
                            ++pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels -= gfxPitch;
                    }
                    break;
            }
            break;

        case FLIP_XY:
            gfxPitch    = surface->width - width;
            lineBuffer  = &gfxLineBuffer[y];
            pixels      = &surface->pixels[widthFlip - 1 + sprX + surface->width * (sprY + heightFlip - 1)];
            frameBuffer = &currentScreen->frameBuffer[x + currentScreen->pitch * y];
            switch (inkEffect) {
                case INK_NONE:
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0)
                                *frameBuffer = activePalette[*pixels];
                            --pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels -= gfxPitch;
                    }
                    break;

                case INK_BLEND:
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0)
                                setPixelBlend(activePalette[*pixels], *frameBuffer);
                            --pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels -= gfxPitch;
                    }
                    break;

                case INK_ALPHA: {
                    uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - alpha)];
                    uint16 *pixelBlend   = &blendLookupTable[0x20 * alpha];

                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0) {
                                uint16 color = activePalette[*pixels];
                                setPixelAlpha(color, *frameBuffer, alpha);
                            }
                            --pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels -= gfxPitch;
                    }
                    break;
                }

                case INK_ADD: {
                    uint16 *blendTablePtr = &blendLookupTable[0x20 * alpha];
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0) {
                                uint16 color = activePalette[*pixels];
                                setPixelAdditive(color, *frameBuffer);
                            }
                            --pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels -= gfxPitch;
                    }
                    break;
                }

                case INK_SUB: {
                    uint16 *subBlendTable = &subtractLookupTable[0x20 * alpha];
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0) {
                                uint16 color = activePalette[*pixels];
                                setPixelSubtractive(color, *frameBuffer);
                            }
                            --pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels -= gfxPitch;
                    }
                    break;
                }

                case INK_TINT:
                    while (height--) {
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0)
                                *frameBuffer = tintLookupTable[*frameBuffer];
                            --pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels -= gfxPitch;
                    }
                    break;

                case INK_MASKED:
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0 && *frameBuffer == maskColor)
                                *frameBuffer = activePalette[*pixels];
                            --pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels -= gfxPitch;
                    }
                    break;

                case INK_UNMASKED:
                    while (height--) {
                        uint16 *activePalette = fullPalette[*lineBuffer];
                        lineBuffer++;
                        int32 w = width;
                        while (w--) {
                            if (*pixels > 0 && *frameBuffer != maskColor)
                                *frameBuffer = activePalette[*pixels];
                            --pixels;
                            ++frameBuffer;
                        }
                        frameBuffer += pitch;
                        pixels -= gfxPitch;
                    }
                    break;
            }
            break;
    }
}

void SoftwareBackend::DrawSpriteRotozoom(uint8 drawGroup, int32 x, int32 y, int32 pivotX,
                        int32 pivotY, int32 width, int32 height, int32 sprX, int32 sprY,
                        int32 scaleX, int32 scaleY, int32 direction, int16 rotation,
                        int32 inkEffect, int32 alpha, int32 sheetID) {
    switch (inkEffect) {
        default: break;
        case INK_ALPHA:
            if (alpha > 0xFF)
                inkEffect = INK_NONE;
            else if (alpha <= 0)
                return;
            break;

        case INK_ADD:
        case INK_SUB:
            if (alpha > 0xFF)
                alpha = 0xFF;
            else if (alpha <= 0)
                return;
            break;

        case INK_TINT:
            if (!tintLookupTable)
                return;
            break;
    }

    int32 angle = 0x200 - (rotation & 0x1FF);
    if (!(rotation & 0x1FF))
        angle = rotation & 0x1FF;

    int32 sine        = sin512LookupTable[angle];
    int32 cosine      = cos512LookupTable[angle];
    int32 fullScaleXS = scaleX * sine >> 9;
    int32 fullScaleXC = scaleX * cosine >> 9;
    int32 fullScaleYS = scaleY * sine >> 9;
    int32 fullScaleYC = scaleY * cosine >> 9;

    int32 posX[4];
    int32 posY[4];
    int32 sprXPos = TO_FIXED(sprX - pivotX);
    int32 sprYPos = TO_FIXED(sprY - pivotY);

    int32 xMax     = 0;
    int32 scaledX1 = 0;
    int32 scaledX2 = 0;
    int32 scaledY1 = 0;
    int32 scaledY2 = 0;
    switch (direction) {
        default:
        case FLIP_NONE: {
            scaledX1 = fullScaleXS * (pivotX - 2);
            scaledX2 = fullScaleXC * (pivotX - 2);
            scaledY1 = fullScaleYS * (pivotY - 2);
            scaledY2 = fullScaleYC * (pivotY - 2);
            xMax     = pivotX + 2 + width;
            posX[0]  = x + ((scaledX2 + scaledY1) >> 9);
            posY[0]  = y + ((fullScaleYC * (pivotY - 2) - scaledX1) >> 9);
            break;
        }

        case FLIP_X: {
            scaledX1 = fullScaleXS * (2 - pivotX);
            scaledX2 = fullScaleXC * (2 - pivotX);
            scaledY1 = fullScaleYS * (pivotY - 2);
            scaledY2 = fullScaleYC * (pivotY - 2);
            xMax     = -2 - pivotX - width;
            posX[0]  = x + ((scaledX2 + scaledY1) >> 9);
            posY[0]  = y + ((fullScaleYC * (pivotY - 2) - scaledX1) >> 9);
            break;
        }

        case FLIP_Y:
        case FLIP_XY: break;
    }

    int32 scaledXMaxS = fullScaleXS * xMax;
    int32 scaledXMaxC = fullScaleXC * xMax;
    int32 scaledYMaxC = fullScaleYC * (pivotY + 2 + height);
    int32 scaledYMaxS = fullScaleYS * (pivotY + 2 + height);
    posX[1]           = x + ((scaledXMaxC + scaledY1) >> 9);
    posY[1]           = y + ((scaledY2 - scaledXMaxS) >> 9);
    posX[2]           = x + ((scaledYMaxS + scaledX2) >> 9);
    posY[2]           = y + ((scaledYMaxC - scaledX1) >> 9);
    posX[3]           = x + ((scaledXMaxC + scaledYMaxS) >> 9);
    posY[3]           = y + ((scaledYMaxC - scaledXMaxS) >> 9);

    int32 left = currentScreen->pitch;
    for (int32 i = 0; i < 4; ++i) {
        if (posX[i] < left)
            left = posX[i];
    }
    if (left < currentScreen->clipBound_X1)
        left = currentScreen->clipBound_X1;

    int32 right = 0;
    for (int32 i = 0; i < 4; ++i) {
        if (posX[i] > right)
            right = posX[i];
    }
    if (right > currentScreen->clipBound_X2)
        right = currentScreen->clipBound_X2;

    int32 top = currentScreen->size.y;
    for (int32 i = 0; i < 4; ++i) {
        if (posY[i] < top)
            top = posY[i];
    }
    if (top < currentScreen->clipBound_Y1)
        top = currentScreen->clipBound_Y1;

    int32 bottom = 0;
    for (int32 i = 0; i < 4; ++i) {
        if (posY[i] > bottom)
            bottom = posY[i];
    }
    if (bottom > currentScreen->clipBound_Y2)
        bottom = currentScreen->clipBound_Y2;

    int32 xSize = right - left;
    int32 ySize = bottom - top;
    if (xSize >= 1 && ySize >= 1) {
        GFXSurface *surface = &gfxSurface[sheetID];

        int32 fullX         = TO_FIXED(sprX + width);
        int32 fullY         = TO_FIXED(sprY + height);
        validDraw           = true;
        int32 fullScaleX    = (int32)((512.0 / (float)scaleX) * 512.0);
        int32 fullScaleY    = (int32)((512.0 / (float)scaleY) * 512.0);
        int32 deltaXLen     = fullScaleX * sine >> 2;
        int32 deltaX        = fullScaleX * cosine >> 2;
        int32 pitch         = currentScreen->pitch - xSize;
        int32 deltaYLen     = fullScaleY * cosine >> 2;
        int32 deltaY        = fullScaleY * sine >> 2;
        int32 lineSize      = surface->lineSize;
        uint8 *lineBuffer   = &gfxLineBuffer[top];
        int32 xLen          = left - x;
        int32 yLen          = top - y;
        uint8 *pixels       = surface->pixels;
        uint16 *frameBuffer = &currentScreen->frameBuffer[left + (top * currentScreen->pitch)];
        int32 fullSprX      = TO_FIXED(sprX) - 1;
        int32 fullSprY      = TO_FIXED(sprY) - 1;

        int32 drawX = 0, drawY = 0;
        if (direction == FLIP_X) {
            drawX     = sprXPos + deltaXLen * yLen - deltaX * xLen - (fullScaleX >> 1);
            drawY     = sprYPos + deltaYLen * yLen + deltaY * xLen;
            deltaX    = -deltaX;
            deltaXLen = -deltaXLen;
        }
        else if (!direction) {
            drawX = sprXPos + deltaX * xLen - deltaXLen * yLen;
            drawY = sprYPos + deltaYLen * yLen + deltaY * xLen;
        }

        switch (inkEffect) {
            case INK_NONE:
                for (int32 y = 0; y < ySize; ++y) {
                    uint16 *activePalette = fullPalette[*lineBuffer++];
                    int32 drawXPos        = drawX;
                    int32 drawYPos        = drawY;
                    for (int32 x = 0; x < xSize; ++x) {
                        if (drawXPos >= fullSprX && drawXPos < fullX && drawYPos >= fullSprY && drawYPos < fullY) {
                            uint8 index = pixels[(FROM_FIXED(drawYPos) << lineSize) + FROM_FIXED(drawXPos)];
                            if (index)
                                *frameBuffer = activePalette[index];
                        }

                        ++frameBuffer;
                        drawXPos += deltaX;
                        drawYPos += deltaY;
                    }

                    drawX -= deltaXLen;
                    drawY += deltaYLen;
                    frameBuffer += pitch;
                }
                break;

            case INK_BLEND:
                for (int32 y = 0; y < ySize; ++y) {
                    uint16 *activePalette = fullPalette[*lineBuffer++];
                    int32 drawXPos        = drawX;
                    int32 drawYPos        = drawY;
                    for (int32 x = 0; x < xSize; ++x) {
                        if (drawXPos >= fullSprX && drawXPos < fullX && drawYPos >= fullSprY && drawYPos < fullY) {
                            uint8 index = pixels[(FROM_FIXED(drawYPos) << lineSize) + FROM_FIXED(drawXPos)];
                            if (index)
                                setPixelBlend(activePalette[index], *frameBuffer);
                        }

                        ++frameBuffer;
                        drawXPos += deltaX;
                        drawYPos += deltaY;
                    }

                    drawX -= deltaXLen;
                    drawY += deltaYLen;
                    frameBuffer += pitch;
                }
                break;

            case INK_ALPHA: {
                uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - alpha)];
                uint16 *pixelBlend   = &blendLookupTable[0x20 * alpha];

                for (int32 y = 0; y < ySize; ++y) {
                    uint16 *activePalette = fullPalette[*lineBuffer++];
                    int32 drawXPos        = drawX;
                    int32 drawYPos        = drawY;
                    for (int32 x = 0; x < xSize; ++x) {
                        if (drawXPos >= fullSprX && drawXPos < fullX && drawYPos >= fullSprY && drawYPos < fullY) {
                            uint8 index = pixels[(FROM_FIXED(drawYPos) << lineSize) + FROM_FIXED(drawXPos)];
                            if (index) {
                                setPixelAlpha(activePalette[index], *frameBuffer, alpha);
                            }
                        }

                        ++frameBuffer;
                        drawXPos += deltaX;
                        drawYPos += deltaY;
                    }

                    drawX -= deltaXLen;
                    drawY += deltaYLen;
                    frameBuffer += pitch;
                }
                break;
            }

            case INK_ADD: {
                uint16 *blendTablePtr = &blendLookupTable[0x20 * alpha];
                for (int32 y = 0; y < ySize; ++y) {
                    uint16 *activePalette = fullPalette[*lineBuffer++];
                    int32 drawXPos        = drawX;
                    int32 drawYPos        = drawY;
                    for (int32 x = 0; x < xSize; ++x) {
                        if (drawXPos >= fullSprX && drawXPos < fullX && drawYPos >= fullSprY && drawYPos < fullY) {
                            uint8 index = pixels[(FROM_FIXED(drawYPos) << lineSize) + FROM_FIXED(drawXPos)];
                            if (index) {
                                setPixelAdditive(activePalette[index], *frameBuffer);
                            }
                        }

                        ++frameBuffer;
                        drawXPos += deltaX;
                        drawYPos += deltaY;
                    }

                    drawX -= deltaXLen;
                    drawY += deltaYLen;
                    frameBuffer += pitch;
                }
                break;
            }

            case INK_SUB: {
                uint16 *subBlendTable = &subtractLookupTable[0x20 * alpha];
                for (int32 y = 0; y < ySize; ++y) {
                    uint16 *activePalette = fullPalette[*lineBuffer++];
                    int32 drawXPos        = drawX;
                    int32 drawYPos        = drawY;
                    for (int32 x = 0; x < xSize; ++x) {
                        if (drawXPos >= fullSprX && drawXPos < fullX && drawYPos >= fullSprY && drawYPos < fullY) {
                            uint8 index = pixels[(FROM_FIXED(drawYPos) << lineSize) + FROM_FIXED(drawXPos)];
                            if (index) {
                                setPixelSubtractive(activePalette[index], *frameBuffer);
                            }
                        }

                        ++frameBuffer;
                        drawXPos += deltaX;
                        drawYPos += deltaY;
                    }

                    drawX -= deltaXLen;
                    drawY += deltaYLen;
                    frameBuffer += pitch;
                }
                break;
            }

            case INK_TINT:
                for (int32 y = 0; y < ySize; ++y) {
                    int32 drawXPos = drawX;
                    int32 drawYPos = drawY;
                    for (int32 x = 0; x < xSize; ++x) {
                        if (drawXPos >= fullSprX && drawXPos < fullX && drawYPos >= fullSprY && drawYPos < fullY) {
                            uint8 index = pixels[(FROM_FIXED(drawYPos) << lineSize) + FROM_FIXED(drawXPos)];
                            if (index)
                                *frameBuffer = tintLookupTable[*frameBuffer];
                        }

                        ++frameBuffer;
                        drawXPos += deltaX;
                        drawYPos += deltaY;
                    }

                    drawX -= deltaXLen;
                    drawY += deltaYLen;
                    frameBuffer += pitch;
                }
                break;

            case INK_MASKED:
                for (int32 y = 0; y < ySize; ++y) {
                    uint16 *activePalette = fullPalette[*lineBuffer++];
                    int32 drawXPos        = drawX;
                    int32 drawYPos        = drawY;
                    for (int32 x = 0; x < xSize; ++x) {
                        if (drawXPos >= fullSprX && drawXPos < fullX && drawYPos >= fullSprY && drawYPos < fullY) {
                            uint8 index = pixels[(FROM_FIXED(drawYPos) << lineSize) + FROM_FIXED(drawXPos)];
                            if (index && *frameBuffer == maskColor)
                                *frameBuffer = activePalette[index];
                        }

                        ++frameBuffer;
                        drawXPos += deltaX;
                        drawYPos += deltaY;
                    }

                    drawX -= deltaXLen;
                    drawY += deltaYLen;
                    frameBuffer += pitch;
                }
                break;

            case INK_UNMASKED:
                for (int32 y = 0; y < ySize; ++y) {
                    uint16 *activePalette = fullPalette[*lineBuffer++];
                    int32 drawXPos        = drawX;
                    int32 drawYPos        = drawY;
                    for (int32 x = 0; x < xSize; ++x) {
                        if (drawXPos >= fullSprX && drawXPos < fullX && drawYPos >= fullSprY && drawYPos < fullY) {
                            uint8 index = pixels[(FROM_FIXED(drawYPos) << lineSize) + FROM_FIXED(drawXPos)];
                            if (index && *frameBuffer != maskColor)
                                *frameBuffer = activePalette[index];
                        }

                        ++frameBuffer;
                        drawXPos += deltaX;
                        drawYPos += deltaY;
                    }

                    drawX -= deltaXLen;
                    drawY += deltaYLen;
                    frameBuffer += pitch;
                }
                break;
        }
    }
}

void SoftwareBackend::DrawDeformedSprite(uint8 drawGroup, uint16 sheetID, int32 inkEffect,
                        int32 alpha) {
    switch (inkEffect) {
        default: break;
        case INK_ALPHA:
            if (alpha > 0xFF)
                inkEffect = INK_NONE;
            else if (alpha <= 0)
                return;
            break;

        case INK_ADD:
        case INK_SUB:
            if (alpha > 0xFF)
                alpha = 0xFF;
            else if (alpha <= 0)
                return;
            break;

        case INK_TINT:
            if (!tintLookupTable)
                return;
            break;
    }

    validDraw              = true;
    GFXSurface *surface    = &gfxSurface[sheetID];
    uint8 *pixels          = surface->pixels;
    int32 clipY1           = currentScreen->clipBound_Y1;
    ScanlineInfo *scanline = &scanlines[clipY1];
    uint16 *frameBuffer    = &currentScreen->frameBuffer[clipY1 * currentScreen->pitch];
    uint8 *lineBuffer      = &gfxLineBuffer[clipY1];
    int32 width            = surface->width - 1;
    int32 height           = surface->height - 1;
    int32 lineSize         = surface->lineSize;

    switch (inkEffect) {
        case INK_NONE:
            for (; clipY1 < currentScreen->clipBound_Y2; ++clipY1) {
                uint16 *activePalette = fullPalette[*lineBuffer++];
                int32 lx              = scanline->position.x;
                int32 ly              = scanline->position.y;
                int32 dx              = scanline->deform.x;
                int32 dy              = scanline->deform.y;
                for (int32 i = 0; i < currentScreen->pitch; ++i) {
                    uint8 palIndex = pixels[((FROM_FIXED(ly) & height) << lineSize) + (FROM_FIXED(lx) & width)];
                    if (palIndex)
                        *frameBuffer = activePalette[palIndex];

                    lx += dx;
                    ly += dy;
                    ++frameBuffer;
                }
                ++scanline;
            }
            break;

        case INK_BLEND:
            for (; clipY1 < currentScreen->clipBound_Y2; ++clipY1) {
                uint16 *activePalette = fullPalette[*lineBuffer++];
                int32 lx              = scanline->position.x;
                int32 ly              = scanline->position.y;
                int32 dx              = scanline->deform.x;
                int32 dy              = scanline->deform.y;
                for (int32 i = 0; i < currentScreen->pitch; ++i) {
                    uint8 palIndex = pixels[((FROM_FIXED(ly) & height) << lineSize) + (FROM_FIXED(lx) & width)];
                    if (palIndex)
                        setPixelBlend(activePalette[palIndex], *frameBuffer);

                    lx += dx;
                    ly += dy;
                    ++frameBuffer;
                }
                ++scanline;
            }
            break;

        case INK_ALPHA: {
            uint16 *fbufferBlend = &blendLookupTable[0x20 * (0xFF - alpha)];
            uint16 *pixelBlend   = &blendLookupTable[0x20 * alpha];

            for (; clipY1 < currentScreen->clipBound_Y2; ++clipY1) {
                uint16 *activePalette = fullPalette[*lineBuffer++];
                int32 lx              = scanline->position.x;
                int32 ly              = scanline->position.y;
                int32 dx              = scanline->deform.x;
                int32 dy              = scanline->deform.y;
                for (int32 i = 0; i < currentScreen->pitch; ++i) {
                    uint8 palIndex = pixels[((FROM_FIXED(ly) & height) << lineSize) + (FROM_FIXED(lx) & width)];
                    if (palIndex) {
                        setPixelAlpha(activePalette[palIndex], *frameBuffer, alpha);
                    }

                    lx += dx;
                    ly += dy;
                    ++frameBuffer;
                }
                ++scanline;
            }
            break;
        }

        case INK_ADD: {
            uint16 *blendTablePtr = &blendLookupTable[0x20 * alpha];

            for (; clipY1 < currentScreen->clipBound_Y2; ++clipY1) {
                uint16 *activePalette = fullPalette[*lineBuffer++];
                int32 lx              = scanline->position.x;
                int32 ly              = scanline->position.y;
                int32 dx              = scanline->deform.x;
                int32 dy              = scanline->deform.y;
                for (int32 i = 0; i < currentScreen->pitch; ++i) {
                    uint8 palIndex = pixels[((FROM_FIXED(ly) & height) << lineSize) + (FROM_FIXED(lx) & width)];
                    if (palIndex) {
                        setPixelAdditive(activePalette[palIndex], *frameBuffer);
                    }

                    lx += dx;
                    ly += dy;
                    ++frameBuffer;
                }
                ++scanline;
            }
            break;
        }

        case INK_SUB: {
            uint16 *subBlendTable = &subtractLookupTable[0x20 * alpha];

            for (; clipY1 < currentScreen->clipBound_Y2; ++clipY1) {
                uint16 *activePalette = fullPalette[*lineBuffer++];
                int32 lx              = scanline->position.x;
                int32 ly              = scanline->position.y;
                int32 dx              = scanline->deform.x;
                int32 dy              = scanline->deform.y;
                for (int32 i = 0; i < currentScreen->pitch; ++i) {
                    uint8 palIndex = pixels[((FROM_FIXED(ly) & height) << lineSize) + (FROM_FIXED(lx) & width)];
                    if (palIndex) {
                        setPixelSubtractive(activePalette[palIndex], *frameBuffer);
                    }
                    lx += dx;
                    ly += dy;
                    ++frameBuffer;
                }
                ++scanline;
            }
            break;
        }

        case INK_TINT:
            for (; clipY1 < currentScreen->clipBound_Y2; ++clipY1) {
                int32 lx = scanline->position.x;
                int32 ly = scanline->position.y;
                int32 dx = scanline->deform.x;
                int32 dy = scanline->deform.y;
                for (int32 i = 0; i < currentScreen->pitch; ++i) {
                    uint8 palIndex = pixels[((FROM_FIXED(ly) & height) << lineSize) + (FROM_FIXED(lx) & width)];
                    if (palIndex)
                        *frameBuffer = tintLookupTable[*frameBuffer];
                    lx += dx;
                    ly += dy;
                    ++frameBuffer;
                }
                ++scanline;
            }
            break;

        case INK_MASKED:
            for (; clipY1 < currentScreen->clipBound_Y2; ++clipY1) {
                uint16 *activePalette = fullPalette[*lineBuffer++];
                int32 lx              = scanline->position.x;
                int32 ly              = scanline->position.y;
                int32 dx              = scanline->deform.x;
                int32 dy              = scanline->deform.y;
                for (int32 i = 0; i < currentScreen->pitch; ++i) {
                    uint8 palIndex = pixels[((FROM_FIXED(ly) & height) << lineSize) + (FROM_FIXED(lx) & width)];
                    if (palIndex && *frameBuffer == maskColor)
                        *frameBuffer = activePalette[palIndex];
                    lx += dx;
                    ly += dy;
                    ++frameBuffer;
                }
                ++scanline;
            }
            break;

        case INK_UNMASKED:
            for (; clipY1 < currentScreen->clipBound_Y2; ++clipY1) {
                uint16 *activePalette = fullPalette[*lineBuffer++];
                int32 lx              = scanline->position.x;
                int32 ly              = scanline->position.y;
                int32 dx              = scanline->deform.x;
                int32 dy              = scanline->deform.y;
                for (int32 i = 0; i < currentScreen->pitch; ++i) {
                    uint8 palIndex = pixels[((FROM_FIXED(ly) & height) << lineSize) + (FROM_FIXED(lx) & width)];
                    if (palIndex && *frameBuffer != maskColor)
                        *frameBuffer = activePalette[palIndex];
                    lx += dx;
                    ly += dy;
                    ++frameBuffer;
                }
                ++scanline;
            }
            break;
    }

}

void SoftwareBackend::DrawTile(uint8 drawGroup, uint16 *tiles, int32 countX,
                        int32 countY, Vector2 *position, Vector2 *offset,
                        bool32 screenRelative) {
    if (tiles) {
        if (!position)
            position = &sceneInfo.entity->position;

        int32 x = FROM_FIXED(position->x);
        int32 y = FROM_FIXED(position->y);
        if (!screenRelative) {
            x -= currentScreen->position.x;
            y -= currentScreen->position.y;
        }

        int32 pivotX = 0;
        int32 pivotY = 0;
        switch (sceneInfo.entity->drawFX) {
            case FX_NONE:
            case FX_FLIP:
                if (offset) {
                    pivotX = x - (offset->x >> 17);
                    pivotY = y - (offset->y >> 17);
                }
                else {
                    pivotX = x - (countX * (TILE_SIZE >> 1));
                    pivotY = y - (countY * (TILE_SIZE >> 1));
                }

                for (int32 ty = 0; ty < countY; ++ty) {
                    for (int32 tx = 0; tx < countX; ++tx) {
                        uint16 tile = tiles[tx + (ty * countX)];
                        if (tile < 0xFFFF) {
                            this->DrawSpriteFlipped(drawGroup, (tx * TILE_SIZE) + pivotX, (ty * TILE_SIZE) + pivotY, TILE_SIZE, TILE_SIZE, 0,
                                              TILE_SIZE * (tile & 0xFFF), FLIP_NONE, sceneInfo.entity->inkEffect, sceneInfo.entity->alpha, 0);
                        }
                    }
                }
                break;

            case FX_ROTATE: // Flip
            case FX_ROTATE | FX_FLIP:
                if (offset) {
                    pivotX = -(offset->x >> 17);
                    pivotY = -(offset->y >> 17);
                }
                else {
                    pivotX = -(countX * (TILE_SIZE >> 1));
                    pivotY = -(countY * (TILE_SIZE >> 1));
                }

                for (int32 ty = 0; ty < countY; ++ty) {
                    for (int32 tx = 0; tx < countX; ++tx) {
                        uint16 tile = tiles[tx + (ty * countX)];
                        if (tile < 0xFFFF) {
                            switch ((tile >> 10) & 3) {
                                case FLIP_NONE:
                                    this->DrawSpriteRotozoom(drawGroup, x + (tx * TILE_SIZE), y + (ty * TILE_SIZE), pivotX, pivotY, TILE_SIZE, TILE_SIZE, 0,
                                                       TILE_SIZE * (tile & 0x3FF), 0x200, 0x200, FLIP_NONE, sceneInfo.entity->rotation,
                                                       sceneInfo.entity->inkEffect, sceneInfo.entity->alpha, 0);
                                    break;

                                case FLIP_X:
                                    this->DrawSpriteRotozoom(drawGroup, x + (tx * TILE_SIZE), y + (ty * TILE_SIZE), pivotX, pivotY, TILE_SIZE, TILE_SIZE, 0,
                                                       TILE_SIZE * (tile & 0x3FF), 0x200, 0x200, FLIP_X, sceneInfo.entity->rotation,
                                                       sceneInfo.entity->inkEffect, sceneInfo.entity->alpha, 0);
                                    break;

                                case FLIP_Y:
                                    this->DrawSpriteRotozoom(drawGroup, x + (tx * TILE_SIZE), y + (ty * TILE_SIZE), pivotX, pivotY, TILE_SIZE, TILE_SIZE, 0,
                                                       TILE_SIZE * (tile & 0x3FF), 0x200, 0x200, FLIP_X, sceneInfo.entity->rotation + 0x100,
                                                       sceneInfo.entity->inkEffect, sceneInfo.entity->alpha, 0);
                                    break;

                                case FLIP_XY:
                                    this->DrawSpriteRotozoom(0, x + (tx * TILE_SIZE), y + (ty * TILE_SIZE), pivotX, pivotY, TILE_SIZE, TILE_SIZE, 0,
                                                       TILE_SIZE * (tile & 0x3FF), 0x200, 0x200, FLIP_NONE, sceneInfo.entity->rotation + 0x100,
                                                       sceneInfo.entity->inkEffect, sceneInfo.entity->alpha, 0);
                                    break;
                            }
                        }
                    }
                }
                break;

            case FX_SCALE: // Scale
            case FX_SCALE | FX_FLIP:
                if (offset) {
                    pivotX = -(offset->x >> 17);
                    pivotY = -(offset->y >> 17);
                }
                else {
                    pivotX = -(countX * (TILE_SIZE >> 1));
                    pivotY = -(countY * (TILE_SIZE >> 1));
                }

                for (int32 ty = 0; ty < countY; ++ty) {
                    for (int32 tx = 0; tx < countX; ++tx) {
                        uint16 tile = tiles[tx + (ty * countX)];
                        if (tile < 0xFFFF) {
                            switch ((tile >> 10) & 3) {
                                case FLIP_NONE:
                                    this->DrawSpriteRotozoom(0, x + (tx * TILE_SIZE), y + (ty * TILE_SIZE), pivotX, pivotY, TILE_SIZE, TILE_SIZE, 0,
                                                       TILE_SIZE * (tile & 0x3FF), sceneInfo.entity->scale.x, sceneInfo.entity->scale.y, FLIP_NONE,
                                                       0x000, sceneInfo.entity->inkEffect, sceneInfo.entity->alpha, 0);
                                    break;

                                case FLIP_X:
                                    this->DrawSpriteRotozoom(0, x + (tx * TILE_SIZE), y + (ty * TILE_SIZE), pivotX, pivotY, TILE_SIZE, TILE_SIZE, 0,
                                                       TILE_SIZE * (tile & 0x3FF), sceneInfo.entity->scale.x, sceneInfo.entity->scale.y, FLIP_X,
                                                       0x000, sceneInfo.entity->inkEffect, sceneInfo.entity->alpha, 0);
                                    break;

                                case FLIP_Y:
                                    this->DrawSpriteRotozoom(0, x + (tx * TILE_SIZE), y + (ty * TILE_SIZE), pivotX, pivotY, TILE_SIZE, TILE_SIZE, 0,
                                                       TILE_SIZE * (tile & 0x3FF), sceneInfo.entity->scale.x, sceneInfo.entity->scale.y, FLIP_X,
                                                       0x100, sceneInfo.entity->inkEffect, sceneInfo.entity->alpha, 0);
                                    break;

                                case FLIP_XY:
                                    this->DrawSpriteRotozoom(0, x + (tx * TILE_SIZE), y + (ty * TILE_SIZE), pivotX, pivotY, TILE_SIZE, TILE_SIZE, 0,
                                                       TILE_SIZE * (tile & 0x3FF), sceneInfo.entity->scale.x, sceneInfo.entity->scale.y, FLIP_NONE,
                                                       0x100, sceneInfo.entity->inkEffect, sceneInfo.entity->alpha, 0);
                                    break;
                            }
                        }
                    }
                }
                break;

            case FX_SCALE | FX_ROTATE: // Flip + Scale + Rotation
            case FX_SCALE | FX_ROTATE | FX_FLIP:
                if (offset) {
                    pivotX = -(offset->x >> 17);
                    pivotY = -(offset->y >> 17);
                }
                else {
                    pivotX = -(countX * (TILE_SIZE >> 1));
                    pivotY = -(countY * (TILE_SIZE >> 1));
                }

                for (int32 ty = 0; ty < countY; ++ty) {
                    for (int32 tx = 0; tx < countX; ++tx) {
                        uint16 tile = tiles[tx + (ty * countX)];
                        if (tile < 0xFFFF) {
                            switch ((tile >> 10) & 3) {
                                case FLIP_NONE:
                                    this->DrawSpriteRotozoom(0, x + (tx * TILE_SIZE), y + (ty * TILE_SIZE), pivotX, pivotY, TILE_SIZE, TILE_SIZE, 0,
                                                       TILE_SIZE * (tile & 0x3FF), sceneInfo.entity->scale.x, sceneInfo.entity->scale.y, FLIP_NONE,
                                                       sceneInfo.entity->rotation, sceneInfo.entity->inkEffect, sceneInfo.entity->alpha, 0);
                                    break;

                                case FLIP_X:
                                    this->DrawSpriteRotozoom(0, x + (tx * TILE_SIZE), y + (ty * TILE_SIZE), pivotX, pivotY, TILE_SIZE, TILE_SIZE, 0,
                                                       TILE_SIZE * (tile & 0x3FF), sceneInfo.entity->scale.x, sceneInfo.entity->scale.y, FLIP_X,
                                                       sceneInfo.entity->rotation, sceneInfo.entity->inkEffect, sceneInfo.entity->alpha, 0);
                                    break;

                                case FLIP_Y:
                                    this->DrawSpriteRotozoom(0, x + (tx * TILE_SIZE), y + (ty * TILE_SIZE), pivotX, pivotY, TILE_SIZE, TILE_SIZE, 0,
                                                       TILE_SIZE * (tile & 0x3FF), sceneInfo.entity->scale.x, sceneInfo.entity->scale.y, FLIP_X,
                                                       sceneInfo.entity->rotation + 0x100, sceneInfo.entity->inkEffect, sceneInfo.entity->alpha, 0);
                                    break;

                                case FLIP_XY:
                                    this->DrawSpriteRotozoom(0, x + (tx * TILE_SIZE), y + (ty * TILE_SIZE), pivotX, pivotY, TILE_SIZE, TILE_SIZE, 0,
                                                       TILE_SIZE * (tile & 0x3FF), sceneInfo.entity->scale.x, sceneInfo.entity->scale.y, FLIP_NONE,
                                                       sceneInfo.entity->rotation + 0x100, sceneInfo.entity->inkEffect, sceneInfo.entity->alpha, 0);
                                    break;
                            }
                        }
                    }
                }
                break;
        }
    }
}

void SoftwareBackend::DrawAniTile(uint8 drawGroup, uint16 sheetID, uint16 tileIndex,
                        uint16 srcX, uint16 srcY, uint16 width, uint16 height) {
    if (sheetID < SURFACE_COUNT && tileIndex < TILE_COUNT) {
        GFXSurface *surface = &gfxSurface[sheetID];

        // FLIP_NONE
        uint8 *tilePixels = &tilesetPixels[tileIndex << 8];
        int32 cnt         = 0;
        for (int32 fy = 0; fy < height; fy += TILE_SIZE) {
            uint8 *pixels = &surface->pixels[((fy + srcY) << surface->lineSize) + srcX];
            cnt += ((width - 1) / TILE_SIZE) + 1;
            for (int32 fx = 0; fx < width; fx += TILE_SIZE) {
                uint8 *pixelsPtr = &pixels[fx];
                for (int32 ty = 0; ty < TILE_SIZE; ++ty) {
                    for (int32 tx = 0; tx < TILE_SIZE; ++tx) *tilePixels++ = *pixelsPtr++;

                    pixelsPtr += surface->width - TILE_SIZE;
                }
            }
        }

        // FLIP_X
        uint8 *srcTilePixels = &tilesetPixels[tileIndex << 8];
        if (cnt * TILE_SIZE > 0) {
            tilePixels = &tilesetPixels[(tileIndex << 8) + (FLIP_X * TILESET_SIZE) + (TILE_SIZE - 1)];

            for (int32 i = 0; i < cnt * TILE_SIZE; ++i) {
                for (int32 p = 0; p < TILE_SIZE; ++p) *tilePixels-- = *srcTilePixels++;

                tilePixels += (TILE_SIZE * 2);
            }
        }

        // FLIP_Y
        srcTilePixels = &tilesetPixels[tileIndex << 8];
        if (cnt * TILE_SIZE > 0) {
            int32 index = tileIndex;
            for (int32 i = 0; i < cnt; ++i) {
                tilePixels = &tilesetPixels[(index << 8) + (FLIP_Y * TILESET_SIZE) + (TILE_DATASIZE - TILE_SIZE)];
                for (int32 y = 0; y < TILE_SIZE; ++y) {
                    for (int32 x = 0; x < TILE_SIZE; ++x) *tilePixels++ = *srcTilePixels++;

                    tilePixels -= (TILE_SIZE * 2);
                }
                ++index;
            }
        }

        // FLIP_XY
        srcTilePixels = &tilesetPixels[(tileIndex << 8) + (FLIP_Y * TILESET_SIZE)];
        if (cnt * TILE_SIZE > 0) {
            tilePixels = &tilesetPixels[(tileIndex << 8) + (FLIP_XY * TILESET_SIZE) + (TILE_SIZE - 1)];

            for (int32 i = 0; i < cnt * TILE_SIZE; ++i) {
                for (int32 p = 0; p < TILE_SIZE; ++p) *tilePixels-- = *srcTilePixels++;

                tilePixels += (TILE_SIZE * 2);
            }
        }
    }
}

bool32 SoftwareBackend::LoadSpritesheetData(void* bufPtr, int id) {
  return false;
}
