#include "appleIIhelpers.h"


void aii_frame_to_rgb(const aii_frame_t *frame, uint8_t *rgb_frame)
{
    for(unsigned i = 0; i < aii_width * aii_height / aii_pixels_per_clock; i++)
    {
        uint8_t pixels = frame->bitmap[i];
        for(int i = 0; i < 7; i++)
        {
            if (pixels & 1)
            {
                *rgb_frame++ = 10;
                *rgb_frame++ = 220;
                *rgb_frame++ = 10;
            }
            else
            {
                *rgb_frame++ = 10;
                *rgb_frame++ = 15;
                *rgb_frame++ = 10;
            }
            pixels >>= 1;
        }
    }
}
