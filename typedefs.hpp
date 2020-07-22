//
//  typedefs.hpp
//  playground
//
//  Created by Emil Ahlbäck on 16.07.20.
//  Copyright © 2020 Emil Ahlbäck. All rights reserved.
//

#pragma once

// Overloaded lambdas for a pattern match-based resolving of layouting issues
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

/*
  The following code uses theater analogies for describing a layout system.
  Some definitions are useful up front:
  - Stage: The total space available for expression (rendering :)
  - Actor: An element to be displayed on the stage
  - Expression: An actor's idea of how it should look on the stage

  - Director: Interprets the stage and suggests (or enforces) expression upon it
    - the director can be Lenient or Strict
    - the director considers what else is on stage when it interprets it
  - Producer: Resolves "disputes" between the actor and director and produces the final performance
    - the producer overrides the director if needed

  The words actor/button are synonymous in the next definitions
  - Lenient
    - Offers lenient boundaries on an axis, with room for "freedom of expression"
  - Strict
    - Enforces strict boundaries on an axis

  Axis: A lower treshold value + upper treshold value
    - eg. X: [Lenient, Strict]
      - Interpreted as:
        - The minimum X value is minium value of the Canvas
        - The element must end on the Strict value, with no room for expression

  Let's make an example where we want to create a "horizontal button group",
  where each button can be as wide as its wants, but must "stack against"
  the button before it. (i.e. the next button must start where the previous
  button ends, with possibly some margin but let's not worry about margins
  here.) Additionally, we want each button to have the same exact height
  to ensure that they look good together.


  Given a stage of [200, 40] pixels width:
    - X-axis direction: [Strict, Lenient] // enforces lower(left) bound
    - Y-axis direction: [Strict, Strict]  // enforces lower and upper bounds
  
  Let's render two buttons:
  1. Needs [40, 20] pixels to render itself:
    - Director's input for X-axis: [Strict(0), Lenient(200)]
    - The strict instructions on the Y-axis from the direction means the button
      won't actually try to render itself with 20 pixels on the Y-axis, therefore
      it is immediately changed to 40
    - The producer resolves it to [40, 40] which it reports to the director

  1.1 The director records the [40, 40] value, and notes the 40 pixel occupied space on the X-axis.
    - Making the total amount of space on the X-axis = 200-40 = 160

  2. Needs [120, 20] pixels to render itself:
    - Director's input for X-axis: [Strict(40), Lenient(160)]
    - 20 again resolves to 40 from the strict direction
    - The producer resolves it to [120, 40]

  2.1 The director records the [120, 40] value
    - Making the total amoutn of space on the X-axis = 160-120 = 40
  
*/

namespace val
{

/*
 * The following two algebraic types represent types of instructional
 * values from, say, a director. A def (Strict) value cannot be overruled.
 * A lnt (Lenient) value represents room for expression.
 *
 * Examples:
 * [Strict(0),  Strict(20)]     - Must start at 0, must end at 20 (for a total of 20 units)
 * [Strict(0),  Lenient(20)]    - Must start at 0, may end at 20, but not past 20
 * [Lenient(0), Strict(20)]     - May start at 0, but not before 0, and must end at 20
 * [Leneint(0), Lenient(20)]    - Anywhere within the range of 0-20
 */
struct def {
    const float value;
    constexpr operator float() const { return value; }
}; // strict value
struct lnt {
    const float value;
    constexpr operator float() const { return value; }
}; // lenient value



/*
 * Represents axis bounds for a stage, including some helpers methods
 * for different crew members to best figure out how to put things on stage,
 * such as the "magic stacker director" who looks at the aspect ratio of the
 * stage and desides whether to stack horizontally or vertically based on that
 * information.
 */
struct Stage {
    enum class Aspect
    {
        horizontal,
        vertical
    };
    
    const float left;
    const float right;
    const float top;
    const float bottom;
    
    constexpr Aspect aspect() const {
        if (right - left > bottom - top) return Aspect::horizontal;
        return Aspect::vertical;
    }
};

using Direction     = std::variant<def, lnt>;

// Upper and lower bounds for direction, interpret as either left->right or top->bottom,
// depending on their usage.
struct AxisDirection
{
    const Direction low;
    const Direction high;
};

// TODO: Expand on this
using performance   = Stage;

// What actors receive from directors
struct Instruction
{
    AxisDirection     horizontal;
    AxisDirection     vertical;
};

// TODO: Expand on this
struct Screen
{
    const size_t width;
    const size_t height;
    
    std::vector<std::vector<char>> buffer {};
};

Screen make_screen(size_t width, size_t height)
{
    Screen screen { width, height };
    screen.buffer.resize(height);
    for (auto& row : screen.buffer) {
        row.resize(width);
        std::fill(row.begin(), row.end(), ' ');
    }
    return screen;
}


using preproduction = std::pair<val::Stage, std::function<void(Screen&)>>;


struct StageLayout
{
    float x_offset {0};
    float y_offset {0};
    float horizontal_margin {0};
    float vertical_margin {0};
    float x_size {0};
    float y_size {0};
};


using instruct_fn = std::function<Instruction(const Stage&, const StageLayout&)>;
using adjust_fn = std::function<StageLayout(const Stage&, const performance&, const StageLayout&)>;

struct Director
{
    const instruct_fn   instruct;
    const adjust_fn     adjust;
};


using perform_fn = std::function<const preproduction(const Instruction&, const std::string)>;

struct Actor
{
    perform_fn perfom;
};


struct Crew
{
    const Director          director;
    const Actor             actor;
};


struct Set
{
    const Stage             stage;
    const StageLayout       stage_layout;
    const Crew              crew;
};

constexpr auto extract = [](const Direction& value) -> const float {
    const auto extractor = [](const auto& v) -> const float {
        return v.value;
    };
    
    return std::visit(extractor, value);
};

// Resolves either an axis instruction with an incoming value, or
// (at the bottom) individual directional values with an incoming value.
constexpr auto resolve_axis = [](const AxisDirection& instruction, const float incoming_value) -> const float {
    const auto resolver = overloaded {
        [&]
        (const def& lower, const def& upper) -> const float {
            if (incoming_value < lower.value) return lower.value;
            return upper.value;
        },
        
        [&]
        (const def& lower, const lnt& upper) -> const float {
            if (incoming_value < lower.value) return lower.value;
            
            return std::min(upper.value, incoming_value);
        },
        
        [&]
        (const lnt& lower, const lnt& upper) -> const float {
            if (incoming_value < lower.value) return lower.value;
            return std::min(upper.value, incoming_value);
        },
        
        [&]
        (const lnt& lower, const def& upper) -> const float {
            return upper.value;
        },
    };
    
    return std::visit(resolver, instruction.low, instruction.high);
};

constexpr auto resolve_direction = [](const Direction& dir, const float incoming_value) -> const float {
    return std::visit(overloaded {
        [&incoming_value]
        (const lnt& direction) -> const float {
            return std::min(direction.value, incoming_value);
        },
        []
        (const def& direction) -> const float {
            return direction.value;
        }
    }, dir);
};

} // namespace val
