#pragma once

#include <cmath>
#include <numbers>
#include <vector>

enum struct OnePoleEQFilterMode
{
    LowPass,
    HighPass
};

template<typename Type>
class OnePoleEQFilter
{
public:

    using Mode = OnePoleEQFilterMode;

    OnePoleEQFilter(Mode filterMode);

    void setSampleRate(double newSampleRate);
    void setNumChannels(int channelsToUse);

    void setMode(Mode newMode);
    void setFrequency(double newFreq);
    void setGainDB(double newGain);

    Type processSample(Type input, int channel = 0);

    void reset();

private:

    void update();

    bool prepared = false;
    double sampleRate = 1.0, iFs = 1.0;

    double frequency = 500.0, decibelChange = 0.0, adjustedFreq = 0.0;
    Type boost = 0.0, w = 0.0;
    Type invA0 = 0.0, a1 = 0.0;

    std::vector<Type> y1 { 1 };

    Mode mode;
};
