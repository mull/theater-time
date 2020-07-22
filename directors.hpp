//
//  directors.hpp
//  playground
//
//  Created by Emil Ahlbäck on 22.07.20.
//  Copyright © 2020 Emil Ahlbäck. All rights reserved.
//

#pragma once

#include "typedefs.hpp"

namespace dir
{
constexpr val::Direction Lenient(const float& val) { return val::lnt{val}; }
constexpr val::Direction Strict(const float& val) { return val::def{val}; }

namespace stack
{
val::instruct_fn horizontal_next = [](const val::Stage& stage, const val::StageLayout& layout) -> val::Instruction {
    return {
        { Strict(layout.x_offset + layout.horizontal_margin), Lenient(stage.right) },
        { Strict(stage.top), Lenient(stage.bottom) }
    };
};

val::adjust_fn horizontal_adjust = [](const val::Stage&, const val::performance& perf, const val::StageLayout& layout) -> val::StageLayout {
    auto next = layout;
    next.x_offset += (perf.right - perf.left) + 1 +layout.horizontal_margin;
    next.x_size += perf.left + perf.right;
    
    return next;
};

val::instruct_fn vertical_next = [](const val::Stage& stage, const val::StageLayout& layout) -> val::Instruction {
    return {
        { Strict(stage.left), Lenient(stage.right) },
        { Strict(layout.y_offset + layout.vertical_margin), Lenient(stage.bottom) },
    };
};

val::adjust_fn vertical_adjust = [](const val::Stage&, const val::performance& perf, const val::StageLayout& layout) -> val::StageLayout {
    auto next = layout;
    next.y_offset += (perf.bottom - perf.top) + 1 + layout.vertical_margin;
    next.y_size += perf.top + perf.bottom + 1;
    
    return next;
};


val::instruct_fn magically_next = [](const val::Stage& stage, const val::StageLayout& layout) -> val::Instruction {
    switch (stage.aspect())
    {
        case val::Stage::Aspect::horizontal:
            return horizontal_next(stage, layout);
        case val::Stage::Aspect::vertical:
            return vertical_next(stage, layout);
    };
};

val::adjust_fn magically_adjust = [](const val::Stage& stage, const val::performance& perf, const val::StageLayout& layout) -> val::StageLayout {
    switch (stage.aspect())
    {
        case val::Stage::Aspect::horizontal:
            return horizontal_adjust(stage, perf, layout);
        case val::Stage::Aspect::vertical:
            return vertical_adjust(stage, perf, layout);
    };
};

static const val::Director horizontally {horizontal_next, horizontal_adjust};
static const val::Director vertically   {vertical_next,   vertical_adjust};
static const val::Director magically    {magically_next,  magically_adjust};
}

} // namespace dir

