//  main.cpp
//  playground
//
//  Created by Emil Ahlbäck on 16.07.20.
//  Copyright © 2020 Emil Ahlbäck. All rights reserved.

#include <utility>
#include <iostream>
#include <functional>
#include <type_traits>
#include <variant>
#include <array>
#include <vector>
#include <numeric>
#include <deque>
#include <algorithm>

#include "typedefs.hpp"
#include "directors.hpp"

constexpr size_t round_downwards(size_t number, size_t pow2 = 2) {
    return number - (pow2 - 1) & ~(pow2 - 1);
}

constexpr std::pair<size_t, size_t> split_value(size_t number)
{
    if (number % 2 == 0) return {number / 2, number / 2};
    else return {number / 2, number / 2 + number % 2};
}


const auto renderer = [](const val::Instruction& instr, const std::string script) -> const val::preproduction
{
    // Simulating vertical space needed to render a button
    constexpr auto y_needed_by_text = 1.0f;
    constexpr auto border_size = 1.0f; // per side
    
    // Note: The second values (instr.first.second, instr.first.second) are not
    //       relative to their preceding values, i.e. it the first value is 20
    //       and the director desires a width of 20, the value will be 40. The
    //       same goes for any offsets. Director gives absolute positions!
    const auto x_start = val::extract(instr.horizontal.low);
    const auto x_absolute_end = val::extract(instr.horizontal.high);
    const auto y_start = val::extract(instr.vertical.low);
    const auto x_relative_end = x_absolute_end - x_start;
    const auto x_end = val::resolve_direction(instr.horizontal.high, x_start + std::min(script.size(), static_cast<size_t>(x_relative_end)) + border_size * 2);
    
    const auto y_end = y_start + val::resolve_direction(instr.vertical.high, y_needed_by_text + (border_size * 2));
    const auto stage = val::Stage{x_start, x_end, y_start, y_end-1}; // -1 because array indices? [15] = row 16
    
    // TODO: Would be nice if this was {{instr, x}, {instr, y}}
    // i.e. that the constructors could handle it for us
    
    return {
        stage,
        // TODO: This should receive the final stage size as an argument, not capture it
        [script, stage](val::Screen& screen) {
            const size_t border_dim  = static_cast<size_t>(border_size);
            // const size_t text_height = static_cast<size_t>(y_needed_by_text);
            const size_t col_start   = static_cast<size_t>(stage.left);
            const size_t col_end     = static_cast<size_t>(stage.right);
            
            if (col_start >= col_end) throw std::runtime_error("These values make no sense.");
            
            const size_t final_width = col_end - col_start;
            const auto final_script  = script.substr(0, final_width - (border_dim * 2));
            const auto script_width  = final_script.length();
            const auto [left_padding, right_padding] = split_value(final_width - script_width);
            
            const auto frame_x_start = col_start;
            // const auto frame_x_end   = col_end;
            const auto frame_y_start = static_cast<size_t>(stage.top);
            const auto frame_y_end   = static_cast<size_t>(stage.bottom);
            const auto text_x_start  = frame_x_start + left_padding;
            const auto text_y_start  = frame_y_start + ((frame_y_end - frame_y_start) / 2);
            
            auto& buffer = screen.buffer;
            
            const size_t filler_col_count { final_width - border_dim*2 };
            size_t col_idx {frame_x_start};
            for (const auto c : "|" + std::string(filler_col_count, '-') + "|") buffer[frame_y_start][col_idx++] = c;
            
            // The fill-space between top border and text
            for (auto row_idx { frame_y_start + 1 }; row_idx < text_y_start; ++row_idx) {
                col_idx = frame_x_start;
                for (const auto c : "|" + std::string(filler_col_count, ' ') + "|") buffer[row_idx][col_idx++] = c;
            }
            
            // The text of the button plus its horizontal borders
            buffer[text_y_start][frame_x_start] = '|';
            col_idx = text_x_start;
            
            for (const auto c : final_script) buffer[text_y_start][col_idx++] = c;
            for (const auto c : std::string(right_padding - border_dim, ' ') + '|') buffer[text_y_start][col_idx++] = c;
                            
            
            // The fill-space between top border and text
            for (auto row_idx { text_y_start + 1 }; row_idx < frame_y_end; ++row_idx) {
                col_idx = frame_x_start;
                for (const auto c : "|" + std::string(filler_col_count, ' ') + "|") buffer[row_idx][col_idx++] = c;
            }
            // Final border
            col_idx = frame_x_start;
            for (const auto c : "|" + std::string(filler_col_count, '-') + "|") buffer[frame_y_end][col_idx++] = c;
        }
    };

};

using TPerformanceBuffer = std::vector<val::preproduction>;


constexpr auto act_scene = [](val::Set set, const std::string script) -> std::pair<val::Set, val::preproduction>
{
    auto [stage, stage_layout, crew] = set;
    auto [director, performer] = crew;
    const val::Instruction instruction = director.instruct(stage, stage_layout);
    const val::preproduction result = performer.perfom(instruction, script);

    return {
        val::Set {stage, director.adjust(stage, result.first, stage_layout), val::Crew{director, performer} },
        result
    };
};

const auto produce_scene =
[](val::Set set, const std::string& script) -> std::pair<val::Set, val::preproduction>
{
    return act_scene(set, script);
};

const auto produce_scenes =
[](val::Set initial_set, TPerformanceBuffer& into, const std::vector<std::string>& scripts)
{
    std::deque<val::Set> set_queue { initial_set };
    std::for_each(scripts.begin(), scripts.end(), [&set_queue, &into](const auto& script) {
        const auto set = set_queue.back();
        auto preprod = produce_scene(set, script);
        into.push_back(preprod.second);
        set_queue.push_back(preprod.first);
    });
};

void print_buffer(const TPerformanceBuffer& buffer, val::Screen screen)
{
    std::cout << std::string(screen.width + 2, '-');
    std::cout << "\n|";
    for (size_t idx { 0 }; idx < screen.width; ++idx) if (idx % 2 == 0) std::cout << idx % 10; else std::cout << ' ';
    std::cout << "|\n|" << std::string(screen.width, '-') << "|\n";
    
    for (const auto& perf : buffer) perf.second(screen);
    
    for (const auto& row : screen.buffer)
    {
        std::cout << '|';
        for (const auto& col : row) {
            std::cout << col;
        }
        std::cout << "|\n";
    }
    std::cout << '|' << std::string(screen.width, '-') << "|\n";
}

int main()
{
    std::vector<val::preproduction> preprod{};
    const std::vector<std::string> buttons{ "First", "Second button", "Third interaction"};
    
    val::Stage          stage { 0, 60, 0, 20 };
    val::StageLayout    stage_layout {};
    val::Actor          actor { renderer };
    
    stage_layout.horizontal_margin = 0.0f;
    stage_layout.vertical_margin = 0.0f;
    
    val::Set initial_set = { stage, stage_layout, val::Crew{dir::stack::magically, actor} };
    
    produce_scenes(initial_set, preprod, buttons);
    
    val::Screen screen = val::make_screen(stage.right, stage.bottom);
    print_buffer(preprod, screen);
    return 0;
}

