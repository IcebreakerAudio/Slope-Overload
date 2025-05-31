
#include "OnePoleEQFilter.h"

template<typename Type>
OnePoleEQFilter<Type>::OnePoleEQFilter(Mode filterMode)
{
    mode = filterMode;
}

template<typename Type>
void OnePoleEQFilter<Type>::setSampleRate(double newSampleRate)
{
    sampleRate = newSampleRate;
    iFs = 1.0 / sampleRate;

    prepared = true;

    update();
    reset();
}

template<typename Type>
void OnePoleEQFilter<Type>::setNumChannels(int channelsToUse)
{
    y1.resize(channelsToUse);

    reset();
}

template<typename Type>
void OnePoleEQFilter<Type>::reset()
{
    std::fill(y1.begin(), y1.end(), static_cast<Type>(0.0));
}

template<typename Type>
void OnePoleEQFilter<Type>::setMode(Mode newMode)
{
    mode = newMode;
}

template<typename Type>
void OnePoleEQFilter<Type>::setFrequency(double newFreq)
{
    frequency = newFreq;
    update();
}

template<typename Type>
void OnePoleEQFilter<Type>::setGainDB(double newGain)
{
    decibelChange = newGain;
    update();
}

template<typename Type>
Type OnePoleEQFilter<Type>::processSample(Type input, int channel)
{
    if(!prepared)
        return input;
    
    auto y = y1[channel];
    auto x = (input - (y * a1)) * invA0;

    y1[channel] = x;

    if(mode == Mode::LowPass)
    {
        x = (x + y) * w;
        x *= boost;
    }
    else
    {
        x = x - y;
        x *= boost;
    }

    return x;
}

template<typename Type>
void OnePoleEQFilter<Type>::update()
{
    if(!prepared)
        return;
    
    auto x = pow(1.059253692626953, decibelChange);

    if(mode == Mode::LowPass) {
        adjustedFreq = std::min(frequency / x, sampleRate * 0.5);
    }
    else {
        adjustedFreq = std::min(frequency * x, sampleRate * 0.5);
    }

    boost = static_cast<Type>((x * x) - 1.0);

    w = static_cast<Type>(std::tan(adjustedFreq * std::numbers::pi_v<double> * iFs));

    auto one = static_cast<Type>(1.0);

    invA0 = one / (one + w);
    a1 = w - one;
}

//==============================================================================
template class OnePoleEQFilter<float>;
template class OnePoleEQFilter<double>;
