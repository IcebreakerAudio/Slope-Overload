#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <BinaryData.h>
#include "DSP/DeltaModulation.h"
#include "Utilities/ParameterListener.h"
#include "Utilities/FiFo.h"

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override { juce::ignoreUnused (index); }
    void changeProgramName (int index, const juce::String& newName) override { juce::ignoreUnused (index, newName); }
    const juce::String getProgramName (int index) override
    {
        juce::ignoreUnused (index);
        return {};
    }

    //==============================================================================
    void readScopeData(float* data, int maxNumItems);

    int getScopeNumSamplesToRead() { return scopeData.getSizeToRead(); }
    int getScopeSize() { return scopeData.size(); }

    float getInterfaceSizeRatio() { return sizeRatio; }
    void setInterfaceSizeRatio(float newRatio) { sizeRatio = newRatio; }

private:

    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();

    ParameterListener mainControlListener, dpcmControlListener, speakerListener;

    void updateMainParameters();
    void updateDPCMParameters();
    void updateSpeakerParameters();
    void updateAllParameters();

    float loadRawParameterValue(juce::StringRef parameterID) const
    {
        return apvts.getRawParameterValue(parameterID)->load();
    }

    //==============================================================================

    static constexpr double smoothingTime = 15.0 * 0.0001;
    juce::LinearSmoothedValue<float> smInGain {1.0f}, smOutGain {1.0f};
    bool effectActive = true, speakerActive = false;
    bool prepared = false;
    int speakerChoice = -1;

    DeltaModulation<float> dpcm;
    std::unique_ptr<juce::dsp::DryWetMixer<float>> mixer;
    juce::dsp::Convolution speaker;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> bypassDelay;

    //==============================================================================

    float sizeRatio = 1.0f;
    const double scopeSize = 0.5;
    Fifo<float> scopeData;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
