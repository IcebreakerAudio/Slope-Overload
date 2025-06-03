#pragma once
#include <juce_dsp/juce_dsp.h>
#include <numbers>
#include "OnePoleEQFilter.h"

template <typename SampleType>
class DeltaModulation
{
public:
    enum struct System
    {
        PAL,
        NTSC
    };

    DeltaModulation();

    //==============================================================================
    /** Sets the Sample Rate index (values 0-15 accepted)*/
    void setSampleRate (int sampleRateIndex);
    
    /** Sets the system to use (PAL or NTSC)*/
    void setSystem (System systemToUse);

    /** Sets whether filtering should be applied before and after re-sampling to reduce aliasing*/
    void setAntiAliasing (bool shouldUseAntiAliasing);

    //==============================================================================
    /** Returns the number of available sample rates to be used with setSampleRate()*/
    int getNumSampleRates() const { return 16; }

    /** Returns the latency produced by the module. Call this after prepare(). Latency may be 0 at higher sample rates.*/
    int getLatencyInSamples() const { return juce::roundToInt(overSampler.getLatencyInSamples()); }

    //==============================================================================
    /** Initialises the processor. */
    void prepare (const juce::dsp::ProcessSpec& spec);

    /** Resets the internal state variables of the processor. */
    void reset();

    //==============================================================================
    /** Processes the input and output samples supplied in the processing context. */
    template <typename ProcessContext>
    void process (const ProcessContext& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        auto& outputBlock      = context.getOutputBlock();
        const auto numChannels = outputBlock.getNumChannels();

        jassert (inputBlock.getNumChannels() == numChannels);
        jassert (inputBlock.getNumSamples() == outputBlock.getNumSamples());

        outputBlock.copyFrom(inputBlock);
        if (context.isBypassed) {
            return;
        }

        if(antiAliasing)
        {
            for(auto& f : aaFilters) {
                f.process(context);
            }
        }

        auto numSamples = outputBlock.getNumSamples();
        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto* outputSamples = outputBlock.getChannelPointer (channel);

            for (size_t i = 0; i < numSamples; ++i) {
                outputSamples[i] = dcPreFilter.processSample ((int) channel, outputSamples[i]);
                outputSamples[i] += highBoost.processSample(outputSamples[i], (int) channel);
            }
        }

        auto osBlock = overSampler.processSamplesUp(outputBlock);
        numSamples = osBlock.getNumSamples();

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto* outputSamples = osBlock.getChannelPointer (channel);

            for (size_t i = 0; i < numSamples; ++i) {
                outputSamples[i] = processSample ((int) channel, outputSamples[i]);
            }
        }

        overSampler.processSamplesDown(outputBlock);

        numSamples = outputBlock.getNumSamples();
        if(antiAliasing)
        {
            for (size_t channel = 0; channel < numChannels; ++channel)
            {
                auto* outputSamples = outputBlock.getChannelPointer (channel);

                for (size_t i = 0; i < numSamples; ++i) {
                    outputSamples[i] = postFilter.processSample ((int) channel, outputSamples[i]);
                }
            }
        }

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto* outputSamples = outputBlock.getChannelPointer (channel);

            for (size_t i = 0; i < numSamples; ++i) {
                outputSamples[i] = dcPostFilter.processSample ((int) channel, outputSamples[i]); }
        }

        for(auto& f : aaFilters) {
            f.snapToZero();
        }
        postFilter.snapToZero();
        dcPreFilter.snapToZero();
        dcPostFilter.snapToZero();
    }

private:

    void update();

    SampleType processSample (int channel, SampleType inputValue);

    static constexpr double targetSampleRate = 133000.0;
    static constexpr SampleType numBits = 7.0;
    const SampleType bitDepth = static_cast<SampleType>(std::pow(2.0, 7.0) - 1.0);
    const SampleType bitFactor = bitDepth * static_cast<SampleType>(0.5);

    static constexpr std::array<double, 16> srLookupPAL
    {
        4177.4,
        4696.63,
        5261.41,
        5579.22,
        6023.94,
        7044.94,
        7917.18,
        8397.01,
        9446.63,
        11233.8,
        12595.5,
        14089.9,
        16965.4,
        21315.5,
        25191.0,
        33252.1
    };

    static constexpr std::array<double, 16> srLookupNTSC
    {
        4181.71,
        4709.93,
        5264.04,
        5593.04,
        6257.95,
        7046.35,
        7919.35,
        8363.42,
        9419.86,
        11186.1,
        12604.0,
        13982.6,
        16884.6,
        21306.8,
        24858.0,
        33143.9
    };

    bool antiAliasing = true;
    int srIndex = 15;
    System system = System::PAL;
    double externalSampleRate = 48000.0, internalSampleRate = 33252.1;
    double clockInc = 1.0;

    int channels = 1;
    static constexpr int numFilters = 4;

    const SampleType threshold = static_cast<SampleType>(1.0) / bitFactor;
    const SampleType gateRatio = static_cast<SampleType>(50.0);

    OnePoleEQFilter<SampleType> highBoost { OnePoleEQFilterMode::HighPass };
    std::vector<juce::dsp::StateVariableTPTFilter<SampleType>> aaFilters;
    juce::dsp::StateVariableTPTFilter<SampleType> postFilter;
    juce::dsp::Oversampling<SampleType> overSampler;
    juce::dsp::BallisticsFilter<SampleType> envelopeFilter, RMSFilter;
    juce::dsp::FirstOrderTPTFilter<SampleType> dcPreFilter, dcPostFilter;

    std::vector<SampleType> z1;
    std::vector<SampleType> output;
    std::vector<double> clockPhase;

};
