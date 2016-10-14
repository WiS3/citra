// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <vector>
#include "common/vector_math.h"
#include "core/memory.h"
#include "video_core/debug_utils/debug_utils.h"
#include "video_core/filtering/texture_filterer.h"
#include "video_core/filtering/xbrz/xbrz.h"

const bool Filtering::isScalingEnabled() {
    return Filtering::scale >= 2 && Filtering::scale <= 6;
}

void Filtering::filterTexture(Filtering::FilteringTypes type, Pica::DebugUtils::TextureInfo tex_info, unsigned int * fromBuffer, unsigned int * toBuffer) {
     switch (type) {
     case Filtering::FilteringTypes::XBR:
         if (tex_info.format == Pica::Regs::TextureFormat::RGB8) {
             xbrz::scale(Filtering::scale, fromBuffer, toBuffer, tex_info.width, tex_info.height, xbrz::ColorFormat::RGB);
         } else {
             xbrz::scale(Filtering::scale, fromBuffer, toBuffer, tex_info.width, tex_info.height, xbrz::ColorFormat::ARGB);
         }
         break;
     }
}

int Filtering::getScaledTextureSize(Pica::Regs::TextureFormat format, int width, int height) {
    if ((int) format < 14) { // Check that this is in range
        return (width * Filtering::scale) * (height * Filtering::scale);
    } else {
        return width * height;
    }
}
