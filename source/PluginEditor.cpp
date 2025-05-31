#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <BinaryData.h>

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // set default font
    auto& lnf = getLookAndFeel();
    lnf.setDefaultSansSerifTypeface(juce::Typeface::createSystemTypefaceFor(BinaryData::VT323Regular_ttf, BinaryData::VT323Regular_ttfSize));

    // set colours
    const auto shadowColour = juce::Colours::black.withAlpha(0.125f);
    const auto mainColour = juce::Colour(0xFF2A2E0D);

    lnf.setColour(juce::Label::ColourIds::textColourId, mainColour);

    lnf.setColour(juce::TextButton::ColourIds::buttonColourId, shadowColour);
    lnf.setColour(juce::TextButton::ColourIds::textColourOnId, mainColour);

    lnf.setColour(juce::Slider::ColourIds::backgroundColourId, shadowColour);
    lnf.setColour(juce::Slider::ColourIds::textBoxTextColourId, mainColour);
    
    lnf.setColour(juce::DrawableButton::ColourIds::backgroundColourId, juce::Colours::transparentWhite);
    lnf.setColour(juce::DrawableButton::ColourIds::backgroundOnColourId, juce::Colours::transparentWhite);

    // load background svg
    background = juce::Drawable::createFromImageData(BinaryData::Background_svg, BinaryData::Background_svgSize);

    // create labels
    auto labelFont = juce::Font(juce::FontOptions().withPointHeight(32.0f));
    for(auto& details : labelDetails)
    {
        auto s = labels.emplace_back(std::make_shared<juce::Label>(details.text, details.text));
        addAndMakeVisible(s.get());
        s->setBounds(details.posX + 2, details.posY + 2, 139, 34);
        s->setJustificationType(details.justification);
        s->setFont(labelFont);
        s->setColour(juce::Label::ColourIds::textColourId, juce::Colours::black.withAlpha(0.25f));
        details.shadow_ptr = s;

        auto& l = labels.emplace_back(std::make_shared<juce::Label>(details.text, details.text));
        addAndMakeVisible(l.get());
        l->setBounds(details.posX, details.posY, 139, 34);
        l->setJustificationType(details.justification);
        l->setFont(labelFont);
        details.label_ptr = l;
    }

    // In and Out Gain
    inGainSlider = std::make_unique<TextSlider>();
    inGainSlider->setBounds(144 + 60, 82, 139 - 60, 34);
    inGainSlider->setTextValueSuffix("dB");
    addAndMakeVisible(inGainSlider.get());

    outGainSlider = std::make_unique<TextSlider>();
    outGainSlider->setBounds(478 + 60, 82, 139 - 60, 34);
    outGainSlider->setTextValueSuffix("dB");
    addAndMakeVisible(outGainSlider.get());

    inGainAttachment = std::make_unique<juce::SliderParameterAttachment>(*processorRef.apvts.getParameter("inGain"), *inGainSlider.get());
    outGainAttachment = std::make_unique<juce::SliderParameterAttachment>(*processorRef.apvts.getParameter("outGain"), *outGainSlider.get());

    //Sample Rate
    sampleRateSlider = std::make_unique<TextSlider>();
    sampleRateSlider->setUseDigitalReadout(true);
    sampleRateSlider->setBounds(351, 336, 53, 38);
    addAndMakeVisible(sampleRateSlider.get());

    sampleRateAttachment = std::make_unique<juce::SliderParameterAttachment>(*processorRef.apvts.getParameter("sRate"), *sampleRateSlider.get());

    // Filter On/Off
    filterOnOff = std::make_unique<RadioButtonComponent>(juce::StringArray("OFF", "ON"));
    filterOnOff->addDivider("/");
    filterOnOff->setBounds(137, 339, 152, 34);
    addAndMakeVisible(filterOnOff.get());

    filterAttachment = std::make_unique<RadioButtonAttachment>(*processorRef.apvts.getParameter("aaFilt"), *filterOnOff.get());

    // Speaker Select
    speakerType = std::make_unique<RadioButtonComponent>(juce::StringArray("A", "B", "C"));
    speakerType->setBounds(478, 339 - 4, 152, 34 + 4);
    addAndMakeVisible(speakerType.get());

    speakerAttachment = std::make_unique<RadioButtonAttachment>(*processorRef.apvts.getParameter("speaker"), *speakerType.get());

    // Power Button
    auto offImage = juce::Drawable::createFromImageData(BinaryData::PowerButton_Off_svg, BinaryData::PowerButton_Off_svgSize);
    auto onImage = juce::Drawable::createFromImageData(BinaryData::PowerButton_On_svg, BinaryData::PowerButton_On_svgSize);
    powerButton = std::make_unique<juce::DrawableButton>("Power", juce::DrawableButton::ButtonStyle::ImageFitted);
    powerButton->setImages(offImage.get(), nullptr, nullptr, nullptr, onImage.get(), nullptr, nullptr, nullptr);
    powerButton->setClickingTogglesState(true);
    powerButton->setBounds(40 - 3, 183 - 3, 29 + 6, 29 + 6);
    addAndMakeVisible(powerButton.get());

    powerAttachment = std::make_unique<juce::ButtonParameterAttachment>(*processorRef.apvts.getParameter("active"), *powerButton.get());

    // scope
    updateScopeDataSize();
    addAndMakeVisible(scope);
    scope.setBounds(138, 127 + 4, 480, 155);

    //
    setSize (715, 460);
    startTimerHz(12);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================

void AudioPluginAudioProcessorEditor::updateScopeDataSize()
{
    auto scopeSize = processorRef.getScopeSize();
    if(scopeSize <= 0) {
        return;
    }

    scopeDataRaw.resize(scopeSize);
    std::fill(scopeDataRaw.begin(), scopeDataRaw.end(), 0.0f);
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    if(scopeDataRaw.empty())
    {
        updateScopeDataSize();
    }
    else
    {
        auto numSamples = processorRef.getScopeNumSamplesToRead();
        processorRef.readScopeData(scopeDataRaw.data(), (int)scopeDataRaw.size());

        const auto scopeSize = scope.getDataSize();
        const auto samplesPerPixel = numSamples / scopeSize;
        int smpCount = 0;
        int scopeIdx = 0;
        float max = 0.0f;
        float min = 0.0f;

        for(int i = 0; i < numSamples; ++i)
        {
            smpCount++;
            
            if(scopeDataRaw[i] > max) {
                max = scopeDataRaw[i];
            }
            else if(scopeDataRaw[i] < min) {
                min = scopeDataRaw[i];
            }

            if(smpCount >= samplesPerPixel)
            {
                scope.setDataAt(scopeIdx, min, max);
                min = max = 0.0f;
                smpCount = 0;
                scopeIdx++;
            }
            if(scopeIdx >= scopeSize) {
                i = numSamples; 
            }
        }

        scope.repaint();
    }
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    background->drawWithin(g, getLocalBounds().toFloat(), juce::RectanglePlacement::fillDestination, 1.0f);
}

void AudioPluginAudioProcessorEditor::resized()
{
}
