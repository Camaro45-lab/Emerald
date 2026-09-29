// Pure, allocation-free 64x64 GBA 4bpp tile transformation. Also compiled by
// the host-side coverage test, so tests exercise the exact ROM implementation.
static unsigned int PixelOffset(unsigned int x, unsigned int y)
{
    return ((y / 8) * 8 + x / 8) * 32 + (y % 8) * 4 + (x % 8) / 2;
}

static unsigned int ReadPixel(const unsigned char *src, unsigned int x, unsigned int y)
{
    return (src[PixelOffset(x, y)] >> ((x & 1) * 4)) & 15;
}

void BuildThreePosePixels(const unsigned char *src, unsigned char *dst, unsigned int pose)
{
    unsigned int x, y, i, color;
    int left = 64, right = -1, top = 64, bottom = -1;
    int width, height, dstWidth, dstHeight, dstLeft, dstTop, sx, sy;

    for (i = 0; i < 2048; i++)
        dst[i] = 0;
    for (y = 0; y < 64; y++)
        for (x = 0; x < 64; x++)
            if (ReadPixel(src, x, y))
            {
                if ((int)x < left) left = x;
                if ((int)x > right) right = x;
                if ((int)y < top) top = y;
                if ((int)y > bottom) bottom = y;
            }
    if (right < left)
        return;

    width = right - left + 1;
    height = bottom - top + 1;
    // Crouch, then a narrower upright recoil. Keep feet anchored and use
    // nearest-neighbour sampling: no new palette entries, blur, or clipping.
    dstWidth = pose == 1 ? width : width - (width / 10 + 1);
    dstHeight = pose == 1 ? height - (height / 8 + 1) : height;
    if (dstWidth < 1) dstWidth = 1;
    if (dstHeight < 1) dstHeight = 1;
    dstLeft = left + (width - dstWidth) / 2;
    dstTop = bottom - dstHeight + 1;
    for (y = 0; y < (unsigned int)dstHeight; y++)
        for (x = 0; x < (unsigned int)dstWidth; x++)
        {
            sx = left + x * width / dstWidth;
            sy = top + y * height / dstHeight;
            color = ReadPixel(src, sx, sy);
            i = PixelOffset(dstLeft + x, dstTop + y);
            dst[i] |= color << (((dstLeft + x) & 1) * 4);
        }
}
