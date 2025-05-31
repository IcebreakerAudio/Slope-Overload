#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DSP/BasicClippers.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
       .withInput("Input", juce::AudioChannelSet::stereo(), true)
       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
       apvts(*this, nullptr, "PARAMETERS", createParameters())
{
    apvts.addParameterListener("active", &mainControlListener);
    apvts.addParameterListener("inGain", &mainControlListener);
    apvts.addParameterListener("outGain", &mainControlListener);

    apvts.addParameterListener("sRate", &dpcmControlListener);
    apvts.addParameterListener("aaFilt", &dpcmControlListener);

    apvts.addParameterListener("speaker", &speakerListener);
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterBool>(
                juce::ParameterID{ "active", 1 },
                "On/Off",
                true));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{ "inGain", 1 },
                "Input",
                juce::NormalisableRange<float>(-60.0f, 12.0f, 0.01f, 1.5f),
                0.0f,
                juce::AudioParameterFloatAttributes().withLabel("dB")
                ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{ "outGain", 1 },
                "Output",
                juce::NormalisableRange<float>(-60.0f, 12.0f, 0.01f, 1.5f),
                0.0f,
                juce::AudioParameterFloatAttributes().withLabel("dB")
                ));

    layout.add(std::make_unique<juce::AudioParameterInt>(
                juce::ParameterID{ "sRate", 1 },
                "Sample Rate",
                0, 15, 7));

    layout.add(std::make_unique<juce::AudioParameterBool>(
                juce::ParameterID{ "aaFilt", 1 },
                "Pre-Filter",
                true));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
                juce::ParameterID{ "speaker", 1 },
                "Speaker",
                juce::StringArray{"A", "B", "C"},
                0));

    return layout;
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto numChannels = getTotalNumInputChannels();

    if (numChannels == 0) {
        return;
    }

    auto spec = juce::dsp::ProcessSpec{sampleRate, juce::uint32(samplesPerBlock), juce::uint32(numChannels)};

    smInGain.reset(sampleRate, smoothingTime);
    smOutGain.reset(sampleRate, smoothingTime);

    dpcm.prepare(spec);
    speaker.prepare(spec);

    auto latency = dpcm.getLatencyInSamples();
    latency += speaker.getLatency();
    setLatencySamples(latency);

    bypassDelay.prepare(spec);
    bypassDelay.setMaximumDelayInSamples(latency + 1);
    bypassDelay.setDelay(static_cast<float>(latency));

    mixer.reset(nullptr);
    mixer = std::make_unique<juce::dsp::DryWetMixer<float>>(latency + 1);
    mixer->prepare(spec);
    mixer->setWetLatency(static_cast<float>(latency));

    scopeData.setSize(juce::roundToInt(sampleRate * scopeSize));

    prepared = true;

    updateAllParameters();
}

void AudioPluginAudioProcessor::releaseResources()
{
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    //support for mono->mono, stereo->stereo, and mono->stereo
    return !layouts.getMainInputChannelSet().isDisabled()
        && layouts.getMainInputChannels() < 3
        && layouts.getMainOutputChannels() < 3
        && layouts.getMainInputChannels() <= layouts.getMainOutputChannels();
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    if(!prepared) {
        return;
    }

    if(mainControlListener.checkForChanges()) {
        updateMainParameters();
    }

    if(dpcmControlListener.checkForChanges()) {
        updateDPCMParameters();
    }

    if(speakerListener.checkForChanges()) {
        updateSpeakerParameters();
    }

    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    mixer->pushDrySamples(buffer);

    smInGain.applyGain(buffer, numSamples);
    auto block = juce::dsp::AudioBlock<float>(buffer).getSubsetChannelBlock(0, totalNumInputChannels);
    auto context = juce::dsp::ProcessContextReplacing<float>(block);

    dpcm.process(context);

    if(speakerActive)
    {
        speaker.process(context);
        smOutGain.applyGain(buffer, numSamples);
        for(int c = 0; c < totalNumInputChannels; ++c)
        {
            auto data = block.getChannelPointer(c);
            for(int s = 0; s < numSamples; ++s)
            {
                data[s] = BasicClippers::softClip(data[s]);
            }
        }
    }
    else
    {
        smOutGain.applyGain(buffer, numSamples);
    }

    mixer->mixWetSamples(block);

    if(effectActive) {
        scopeData.addToFifo(buffer, totalNumInputChannels);
    }
    else {
        scopeData.zeroFifo(numSamples);
    }
}

void AudioPluginAudioProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto block = juce::dsp::AudioBlock<float>(buffer).getSubsetChannelBlock(0, totalNumInputChannels);
    auto context = juce::dsp::ProcessContextReplacing<float>(block);
    bypassDelay.process(context);
    
    scopeData.zeroFifo(buffer.getNumSamples());
}

//==============================================================================

void AudioPluginAudioProcessor::updateMainParameters()
{
    effectActive = loadRawParameterValue("active") > 0.5f;
    mixer->setWetMixProportion(effectActive ? 1.0f : 0.0f); // using mixer for bypass to avoid clicks

    smInGain.setTargetValue(juce::Decibels::decibelsToGain(loadRawParameterValue("inGain")));
    smOutGain.setTargetValue(juce::Decibels::decibelsToGain(loadRawParameterValue("outGain")));
}

void AudioPluginAudioProcessor::updateDPCMParameters()
{
    dpcm.setAntiAliasing(loadRawParameterValue("aaFilt") > 0.5f);
    dpcm.setSampleRate(juce::roundToInt(loadRawParameterValue("sRate")));
}

void AudioPluginAudioProcessor::updateSpeakerParameters()
{
    auto newSpeakerChoice = juce::roundToInt(loadRawParameterValue("speaker"));

    if(speakerChoice == newSpeakerChoice) {
        return;
    }

    speakerChoice = newSpeakerChoice;
    speakerActive = speakerChoice > 0;

    if(!speakerActive) {
        return;
    }

    speaker.reset();
    
    if(speakerChoice == 1) {
        speaker.loadImpulseResponse(BinaryData::HS200_SM58_Close_wav, size_t(BinaryData::HS200_SM58_Close_wavSize),
                                    juce::dsp::Convolution::Stereo::no,
                                    juce::dsp::Convolution::Trim::no,
                                    0,
                                    juce::dsp::Convolution::Normalise::no);
    }
    else {
        speaker.loadImpulseResponse(BinaryData::VL1_SM58_Edge_wav, size_t(BinaryData::VL1_SM58_Edge_wavSize),
                                    juce::dsp::Convolution::Stereo::no,
                                    juce::dsp::Convolution::Trim::no,
                                    0,
                                    juce::dsp::Convolution::Normalise::no);
    }
}

void AudioPluginAudioProcessor::updateAllParameters()
{
    updateMainParameters();
    updateDPCMParameters();
    updateSpeakerParameters();
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Store
    auto copyState = apvts.copyState();
    auto xml = copyState.createXml();

    copyXmlToBinary(*xml.get(), destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore
    auto xml = getXmlFromBinary(data, sizeInBytes);
    auto copyState = juce::ValueTree::fromXml(*xml.get());

    apvts.replaceState(copyState);
}

//==============================================================================
void AudioPluginAudioProcessor::readScopeData(float* data, int maxNumItems)
{
    auto size = juce::jmin(scopeData.getSizeToRead(), maxNumItems);
    scopeData.readFromFifo(data, size);
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
