// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <vector>
#include "common/vector_math.h"
#include "core/memory.h"
#include "core/settings.h"
#include "video_core/debug_utils/debug_utils.h"
#include "video_core/filtering/texture_filterer.h"
#include "video_core/filtering/xbrz/xbrz.h"

bool Filtering::isScalingEnabled() {
    int scaling = Filtering::getScaling();
    return Filtering::getScalingType() != Filtering::FilteringTypes::NONE && scaling >= 2 &&
           scaling <= 6;
}

int Filtering::getScaling() {
    int scaling = Settings::values.tex_filter_scaling;

    if (scaling < 1 || scaling > 6) {
        return 1;
    } else {
        return scaling;
    }
}

Filtering::FilteringTypes Filtering::getScalingType() {
    return (Filtering::FilteringTypes)Settings::values.tex_filter;
}

void Filtering::filterTexture(Pica::DebugUtils::TextureInfo tex_info, unsigned int* fromBuffer,
                              unsigned int* toBuffer) {
    // Discover filtering type
    Filtering::FilteringTypes type = getScalingType();

    switch (type) {
    case Filtering::FilteringTypes::XBRZ:
        if (tex_info.format == Pica::Regs::TextureFormat::RGB8) {
            xbrz::scale(Filtering::getScaling(), fromBuffer, toBuffer, tex_info.width,
                        tex_info.height, xbrz::ColorFormat::RGB);
        } else {
            xbrz::scale(Filtering::getScaling(), fromBuffer, toBuffer, tex_info.width,
                        tex_info.height, xbrz::ColorFormat::ARGB);
        }
        break;
    }
}

int Filtering::getScaledTextureSize(Pica::Regs::TextureFormat format, int width, int height) {
    if (Filtering::isScalingEnabled() && (int)format < 14) { // Check that this is in range
        int scaling = Filtering::getScaling();
        return (width * scaling) * (height * scaling);
    } else {
        return width * height;
    }
}
