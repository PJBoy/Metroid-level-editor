#pragma once

#include "../global.h"

#include <cairomm/cairomm.h>

namespace Util
{
    inline Cairo::RefPtr<Cairo::ImageSurface> makeImageSurface(int width, int height)
    try
    {
        return Cairo::ImageSurface::create(Cairo::Format::FORMAT_ARGB32, width, height);
    }
    LOG_RETHROW

    inline Cairo::RefPtr<Cairo::ImageSurface> flip(const Cairo::RefPtr<Cairo::ImageSurface>& p_surface, bool flip_x, bool flip_y)
    try
    {
        if (!flip_x && !flip_y)
            return p_surface;

        int
            width(p_surface->get_width()),
            height(p_surface->get_height());

        Cairo::Matrix m(Cairo::identity_matrix());
        m.translate(width / 2, height / 2);
        m.scale(flip_x ? -1 : 1, flip_y ? -1 : 1);
        m.translate(-width / 2, -height / 2);

        Cairo::RefPtr<Cairo::SurfacePattern> p_pattern(Cairo::SurfacePattern::create(p_surface));
        p_pattern->set_matrix(m);
        
        Cairo::RefPtr<Cairo::ImageSurface> p_target(makeImageSurface(width, height));
        Cairo::RefPtr<Cairo::Context> p_context(Cairo::Context::create(p_target));
        p_context->set_source(p_pattern);
        p_context->paint();

        return p_target;
    }
    LOG_RETHROW

    inline void bgr15ToRgba32(unsigned char* p_rgba, uint16_t bgr, bool transparent)
    {
        if (transparent)
            p_rgba[0] = p_rgba[1] = p_rgba[2] = p_rgba[3] = 0;
        else
        {
            p_rgba[0] = (bgr >> 10 & 0x1F) * 0xFF / 0x1F;
            p_rgba[1] = (bgr >> 5  & 0x1F) * 0xFF / 0x1F;
            p_rgba[2] = (bgr       & 0x1F) * 0xFF / 0x1F;
            p_rgba[3] = 0xFF;
        }
    }
};
