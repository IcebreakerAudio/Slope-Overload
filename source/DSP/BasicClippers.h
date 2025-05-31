#pragma once

#include <cmath>
#include <numbers>

namespace BasicClippers
{
    template <typename Type>
    constexpr Type inv6 = static_cast<Type>(1.0 / 6.0);

    template<typename Type>
    [[maybe_unused]] inline Type hardClip(Type input, Type threshold)
    {
        if(input < -threshold) {
            return -threshold;
        }
        else if(input > threshold) {
            return threshold;
        }
        else {
            return input;
        }
    }

    template<typename Type>
    [[maybe_unused]] inline Type saturate(Type input)
    {
        return input *= static_cast<Type>(1.0) / (abs(input) + static_cast<Type>(1.0));
    }

    template<typename Type>
    [[maybe_unused]] inline Type softClip(Type input)
    {
        input = hardClip(input, std::numbers::sqrt2_v<Type>);
        return input - (inv6<Type> * input * input * input);
    }

    template<typename Type>
    [[maybe_unused]] inline Type cubicSoftClip(Type input, Type factor)
    {
        if (factor < 2.0) factor = 2.0;

        auto sign = (input < 0.0) ? -1.0 : 1.0;
        input = abs(input);
        if (input >= 1.0) {
            input = (factor - 1.0) / factor;
        }
        else {
            input = input - (pow(input, factor) / factor);
        }

        return input * sign;
    }

    template<typename Type>
    [[maybe_unused]] inline Type polySoftClip(Type input)
    {
        if(input > 1.875) {
            return 1.0;
        }
        else if (input < -1.875) {
            return -1.0;
        }
        else
        {
            auto a = pow(input, 3.0) * -0.18963;
            auto b = pow(input, 5.0) * 0.0161817;
            return a + b + input;
        }
    }
}