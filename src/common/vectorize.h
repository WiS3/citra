// Copyright 2016 Citra Emulator Project / PPSSPP Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.
#include "common/common_types.h"

#pragma once
/*
 * Vectorize lib
 * This small utility will allow you to simplify repititive actions while
 * transforming/generating/consuming data in an 'ordered set' or array.
 * it achieves this under better performance when the next conditions are met:
 * - your morphism(transforming function) does not branch(no-ifs, no-switches, no-calls)
 * - very simple actions.
 * - morphisms are not monadic or dependant on state. (they are in theory pure functions).
 */

#ifdef _MSC_VER
#define VECTORIZE_NEXT __pragma("loop( ivdep )")
#elif __GNUC__
#define VECTORIZE_NEXT _Pragma("GCC ivdep")
#elif __clang__
#define VECTORIZE_NEXT _Pragma("clang loop vectorize(enable) interleave(enable)")
#else
#define VECTORIZE_NEXT
#endif

/*
 * Static compilers can't always detect if vectorization is possible,
 * if the programmer is 100% sure it's possible to vectorize a set
 * of actions, it can hint the compiler that it can vectorize a loop
 * unconditionaly.
 */
namespace Common {

// These macros are used to unroll/unfold the same action on tight loops
// should be used on actions that don't branch the pipeline.
// Static compilers can't detect unrollable loops easily. Normaly,
// they require some profiling data to unroll loops.
#define LOOP_UNROLL_1(CODE) CODE
#define LOOP_UNROLL_2(CODE)                                                                        \
    LOOP_UNROLL_1(CODE);                                                                           \
    LOOP_UNROLL_1(CODE)
#define LOOP_UNROLL_4(CODE)                                                                        \
    LOOP_UNROLL_2(CODE);                                                                           \
    LOOP_UNROLL_2(CODE)
#define LOOP_UNROLL_8(CODE)                                                                        \
    LOOP_UNROLL_4(CODE);                                                                           \
    LOOP_UNROLL_4(CODE)

// use this before a loop to tell the compiler, it's unconditionaly vectorizeable

// Endofunctor: https://www.quora.com/What-is-an-endofunctor
// map transforms each value in the set applying a function continuesly
// example:
//
// in C++
// inline void array_cosine(f32*& my_set) {
//     *myset = cos(my_set[0]);
//     my_set += 1;
// }
// .....
// map<f32,&array_cosine>(my_array);
//
template <class T, void morphism(T*&)>
void map(T*& set, u32 set_size) {
    u32 steps = set_size / 8; // 16 unfolds
    VECTORIZE_NEXT for (u32 i = 0; i != steps; i++) {
        LOOP_UNROLL_8(morphism(set));
    }
    // Now just do the rest
    steps = set_size - (steps * 8);
    u32 jump = (steps % 8);
    // This form of loop unfolding works for every set of data at the
    // expense of not marshelling/vectorizing but won't break the pipeline
    switch (jump) {
        do {
            jump = 8;
            morphism(set);
        case 7:
            morphism(set);
        case 6:
            morphism(set);
        case 5:
            morphism(set);
        case 4:
            morphism(set);
        case 3:
            morphism(set);
        case 2:
            morphism(set);
        case 1:
            morphism(set);
        case 0:
        default:
            steps -= jump;
        } while (steps != 0);
    }
}

// Anamorphism: https://en.wikipedia.org/wiki/Anamorphism
// unfold takes a finite ordered set (an array for instance)
// and generates a subset of values from the corresponding
// seed value in your set.
template <class T, void morphism(T*&, T*&)>
void unfold(T*& set, T*& generator, u32 set_size) {
    u32 steps = set_size / 8; // 16 unfolds
    VECTORIZE_NEXT for (u32 i = 0; i != steps; i++) {
        LOOP_UNROLL_8(morphism(set, generator));
    }
    // Now just do the rest
    steps = set_size - (steps * 8);
    u32 jump = (steps % 8);
    // This form of loop unfolding works for every set of data at the
    // expense of not marshelling/vectorizing but won't break the pipeline
    switch (jump) {
        do {
            jump = 8;
            morphism(set, generator);
        case 7:
            morphism(set, generator);
        case 6:
            morphism(set, generator);
        case 5:
            morphism(set, generator);
        case 4:
            morphism(set, generator);
        case 3:
            morphism(set, generator);
        case 2:
            morphism(set, generator);
        case 1:
            morphism(set, generator);
        case 0:
        default:
            steps -= jump;
        } while (steps != 0);
    }
}
// Catamorphism: https://wiki.haskell.org/Catamorphisms
// fold takes a finite ordered set (an array for instance)
// and collapses all values to a consumer.
// pseudocode example:
//  fold (*) [1,2,3] 1 = (3*(2*(1*1))) = 6
//
// in C++
// inline void array_product(u32*& my_set, u32& my_consumer) {
//     my_consumer *= *my_set;
//     my_set += 1;
// }
// .....
// u32 result = 1;
// fold<u32,&array_product>(my_array, result);
//
template <class T, void morphism(T*&, T&)>
void fold(T*& set, T& consumer, u32 set_size) {
    u32 steps = set_size / 8; // 16 unfolds
    VECTORIZE_NEXT for (u32 i = 0; i != steps; i++) {
        LOOP_UNROLL_8(morphism(set, consumer));
    }
    // Now just do the rest
    steps = set_size - (steps * 8);
    u32 jump = (steps % 8);
    // This form of loop unfolding works for every set of data at the
    // expense of not marshelling/vectorizing but won't break the pipeline
    switch (jump) {
        do {
            jump = 8;
            morphism(set, consumer);
        case 7:
            morphism(set, consumer);
        case 6:
            morphism(set, consumer);
        case 5:
            morphism(set, consumer);
        case 4:
            morphism(set, consumer);
        case 3:
            morphism(set, consumer);
        case 2:
            morphism(set, consumer);
        case 1:
            morphism(set, consumer);
        case 0:
        default:
            steps -= jump;
        } while (steps != 0);
    }
}

#undef LOOP_UNROLL_8
#undef LOOP_UNROLL_4
#undef LOOP_UNROLL_2
#undef LOOP_UNROLL_1

} // Common
