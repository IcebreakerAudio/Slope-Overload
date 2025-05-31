#include "DeltaModulation.h"

template <typename SampleType>
DeltaModulation<SampleType>::DeltaModulation()
{
    RMSFilter.setLevelCalculationType (juce::dsp::BallisticsFilterLevelCalculationType::RMS);
    RMSFilter.setAttackTime(static_cast<SampleType>(0.0));
    RMSFilter.setReleaseTime(static_cast<SampleType>(50.0));
    
    envelopeFilter.setAttackTime(static_cast<SampleType>(0.0));
    envelopeFilter.setReleaseTime(static_cast<SampleType>(10.0));

    dcPreFilter.setType(juce::dsp::FirstOrderTPTFilterType::highpass);
    dcPostFilter.setType(juce::dsp::FirstOrderTPTFilterType::highpass);

    dcPreFilter.setCutoffFrequency(static_cast<SampleType>(20.0));
    dcPostFilter.setCutoffFrequency(static_cast<SampleType>(20.0));

    highBoost.setFrequency(static_cast<SampleType>(1000.0));
    highBoost.setGainDB(static_cast<SampleType>(6.0));
}

template <typename SampleType>
void DeltaModulation<SampleType>::prepare (const juce::dsp::ProcessSpec& spec)
{
    jassert (spec.sampleRate > 0);
    jassert (spec.numChannels > 0);

    channels = spec.numChannels;

    z1.resize(channels);
    output.resize(channels);
    clockPhase.resize(channels);

    highBoost.setSampleRate(spec.sampleRate);
    highBoost.setNumChannels(channels);

    aaFilters.clear();
    aaFilters.resize(numFilters);

    for(auto& f : aaFilters) {
        f.prepare(spec);
        f.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        f.setResonance(static_cast<SampleType>(1.0) / std::numbers::sqrt2_v<SampleType>);
    }

    postFilter.prepare(spec);
    postFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    postFilter.setResonance(static_cast<SampleType>(1.0) / std::numbers::sqrt2_v<SampleType>);

    dcPreFilter.prepare(spec);
    dcPostFilter.prepare(spec);

    overSampler.numChannels = channels;
    overSampler.setUsingIntegerLatency(true);
    overSampler.clearOversamplingStages();

    externalSampleRate = spec.sampleRate;
    if(externalSampleRate >= targetSampleRate)
    {
        overSampler.addDummyOversamplingStage();
    }
    else
    {
        int n = 0;
        while(externalSampleRate < targetSampleRate)
        {
            externalSampleRate *= 2.0;
            if(n == 0) {
                overSampler.addOversamplingStage(juce::dsp::Oversampling<SampleType>::FilterType::filterHalfBandFIREquiripple, 0.05f, -90.0f, 0.06f, -75.0f);
            }
            else {
                overSampler.addOversamplingStage(juce::dsp::Oversampling<SampleType>::FilterType::filterHalfBandPolyphaseIIR, 0.1f, -70.0f, 0.12f, -60.0f);
            }
            ++n;
        }
    }
    overSampler.initProcessing(spec.maximumBlockSize);
    auto osSpec = spec;
    osSpec.sampleRate = externalSampleRate;
    RMSFilter.prepare(osSpec);
    envelopeFilter.prepare(osSpec);

    update();
    reset();
}

template <typename SampleType>
void DeltaModulation<SampleType>::reset()
{
    std::fill(z1.begin(), z1.end(), static_cast<SampleType>(63.0));
    std::fill(output.begin(), output.end(), static_cast<SampleType>(0.0));
    std::fill(clockPhase.begin(), clockPhase.end(), static_cast<SampleType>(1.0));

    for(auto& f : aaFilters) {
        f.reset();
    }
    postFilter.reset();

    RMSFilter.reset();
    envelopeFilter.reset();
}

template <typename SampleType>
void DeltaModulation<SampleType>::update()
{
    clockInc = internalSampleRate / externalSampleRate;
    for(auto& f : aaFilters) {
        f.setCutoffFrequency(static_cast<SampleType>(internalSampleRate * 0.5));
    }
    postFilter.setCutoffFrequency(static_cast<SampleType>(internalSampleRate * 0.5));
}

template <typename SampleType>
SampleType DeltaModulation<SampleType>::processSample (int channel, SampleType inputValue)
{
    jassert(channel < channels);

    auto env = RMSFilter.processSample (channel, inputValue);
    env = envelopeFilter.processSample (channel, env);

    if(clockPhase[channel] >= 1.0)
    {
        clockPhase[channel] -= 1.0;

        auto x = inputValue * bitFactor;
        x += bitFactor;
        juce::jlimit(static_cast<SampleType>(0.0), bitDepth, x);

        x = round(x) > z1[channel] ? static_cast<SampleType>(1.0) : static_cast<SampleType>(-1.0);
        x += z1[channel];

        z1[channel] = x;
        x = x / bitFactor;
        x -= static_cast<SampleType>(1.0);
        output[channel] = x;
    }
    clockPhase[channel] += clockInc;

    auto gain = (env > threshold) ? static_cast<SampleType> (1.0)
                                  : std::pow (env * bitFactor, gateRatio);

    return output[channel] * gain;
}

template <typename SampleType>
void DeltaModulation<SampleType>::setSampleRate (int sampleRateIndex)
{
    jassert(juce::isPositiveAndNotGreaterThan(sampleRateIndex, 16));
    srIndex = juce::jlimit(0, 15, sampleRateIndex);

    if(system == System::PAL) {
        internalSampleRate = srLookupPAL[srIndex];
    }
    else {
        internalSampleRate = srLookupNTSC[srIndex];
    }

    update();
}
    
template <typename SampleType>
void DeltaModulation<SampleType>::setSystem (System systemToUse)
{
    if(systemToUse != system)
    {
        system = systemToUse;
        setSampleRate(srIndex);
    }
}

template <typename SampleType>
void DeltaModulation<SampleType>::setAntiAliasing (bool shouldUseAntiAliasing)
{
    if(antiAliasing != shouldUseAntiAliasing)
    {
        antiAliasing = shouldUseAntiAliasing;
        
        for(auto& f : aaFilters) {
            f.reset();
        }
        postFilter.reset();
    }
}

//==============================================================================
template class DeltaModulation<float>;
template class DeltaModulation<double>;
