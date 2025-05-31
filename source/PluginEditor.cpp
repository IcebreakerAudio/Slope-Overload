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
    for(auto& details : labelDetails)
    {
        auto s = labels.emplace_back(std::make_shared<juce::Label>(details.text, details.text));
        addAndMakeVisible(s.get());
        s->setJustificationType(details.justification);
        s->setColour(juce::Label::ColourIds::textColourId, juce::Colours::black.withAlpha(0.125f));
        details.shadow_ptr = s;

        auto& l = labels.emplace_back(std::make_shared<juce::Label>(details.text, details.text));
        addAndMakeVisible(l.get());
        l->setJustificationType(details.justification);
        details.label_ptr = l;
    }

    // In and Out Gain
    inGainSlider = std::make_unique<TextSlider>();
    inGainSlider->setTextValueSuffix("dB");
    addAndMakeVisible(inGainSlider.get());

    outGainSlider = std::make_unique<TextSlider>();
    outGainSlider->setTextValueSuffix("dB");
    addAndMakeVisible(outGainSlider.get());

    inGainAttachment = std::make_unique<juce::SliderParameterAttachment>(*processorRef.apvts.getParameter("inGain"), *inGainSlider.get());
    outGainAttachment = std::make_unique<juce::SliderParameterAttachment>(*processorRef.apvts.getParameter("outGain"), *outGainSlider.get());

    //Sample Rate
    sampleRateSlider = std::make_unique<TextSlider>();
    sampleRateSlider->setUseDigitalReadout(true);
    addAndMakeVisible(sampleRateSlider.get());

    sampleRateAttachment = std::make_unique<juce::SliderParameterAttachment>(*processorRef.apvts.getParameter("sRate"), *sampleRateSlider.get());

    // Filter On/Off
    filterOnOff = std::make_unique<RadioButtonComponent>(juce::StringArray("OFF", "ON"));
    filterOnOff->addDivider("/");
    addAndMakeVisible(filterOnOff.get());

    filterAttachment = std::make_unique<RadioButtonAttachment>(*processorRef.apvts.getParameter("aaFilt"), *filterOnOff.get());

    // Speaker Select
    speakerType = std::make_unique<RadioButtonComponent>(juce::StringArray("A", "B", "C"));
    addAndMakeVisible(speakerType.get());

    speakerAttachment = std::make_unique<RadioButtonAttachment>(*processorRef.apvts.getParameter("speaker"), *speakerType.get());

    // Power Button
    auto offImage = juce::Drawable::createFromImageData(BinaryData::PowerButton_Off_svg, BinaryData::PowerButton_Off_svgSize);
    auto onImage = juce::Drawable::createFromImageData(BinaryData::PowerButton_On_svg, BinaryData::PowerButton_On_svgSize);
    powerButton = std::make_unique<juce::DrawableButton>("Power", juce::DrawableButton::ButtonStyle::ImageFitted);
    powerButton->setImages(offImage.get(), nullptr, nullptr, nullptr, onImage.get(), nullptr, nullptr, nullptr);
    powerButton->setClickingTogglesState(true);
    addAndMakeVisible(powerButton.get());

    powerAttachment = std::make_unique<juce::ButtonParameterAttachment>(*processorRef.apvts.getParameter("active"), *powerButton.get());

    // scope
    updateScopeDataSize();
    addAndMakeVisible(scope);

    // resizing
    setResizable(true, true);
    auto sizeRatio = processorRef.getInterfaceSizeRatio();
    auto width = originalWidthF * sizeRatio;
    auto height = originalHeightF * sizeRatio;
    const auto ratio = static_cast<double>(originalWidthF / originalHeightF);
    getConstrainer()->setFixedAspectRatio(ratio);
    setResizeLimits(originalWidth / 2, originalHeight / 2, originalWidth * 2, originalHeight * 2);

    setSize(juce::roundToInt(width), juce::roundToInt(height));

    // timer
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
    auto bounds = getLocalBounds().toFloat();
    auto sizeRatio = bounds.getWidth() / originalWidthF;

    processorRef.setInterfaceSizeRatio(sizeRatio);

    const auto shadowDistance = 2.0f * sizeRatio;

    const auto labelFont = juce::Font(juce::FontOptions().withPointHeight(28.0f * sizeRatio));
    const auto labelHeight = juce::roundToInt(34.0f * sizeRatio);
    const auto labelWidth = juce::roundToInt(139.0f * sizeRatio);

    for(auto& details : labelDetails)
    {
        auto s = details.shadow_ptr.lock();
        if(s)
        {
            s->setBounds(juce::roundToInt((details.posX * sizeRatio) + shadowDistance),
                         juce::roundToInt((details.posY * sizeRatio) + shadowDistance),
                         labelWidth, labelHeight);
            s->setFont(labelFont);
        }

        auto l = details.label_ptr.lock();
        if(l)
        {
            l->setBounds(juce::roundToInt(details.posX * sizeRatio),
                         juce::roundToInt(details.posY * sizeRatio),
                         labelWidth, labelHeight);
            l->setFont(labelFont);
        }
    }

    powerButton->setBounds(juce::roundToInt(36.0f * sizeRatio),
                           juce::roundToInt(180.0f * sizeRatio),
                           juce::roundToInt(35.0f * sizeRatio),
                           juce::roundToInt(35.0f * sizeRatio));

    inGainSlider->setFontHeight(28.0f * sizeRatio);
    outGainSlider->setFontHeight(28.0f * sizeRatio);
    inGainSlider->setShadowOffset(shadowDistance);
    outGainSlider->setShadowOffset(shadowDistance);
    inGainSlider->setBounds(juce::roundToInt(190.0f * sizeRatio),
                            juce::roundToInt(82.0f * sizeRatio),
                            juce::roundToInt(90.0f * sizeRatio),
                            juce::roundToInt(34.0f * sizeRatio));
    outGainSlider->setBounds(juce::roundToInt(524.0f * sizeRatio),
                            juce::roundToInt(82.0f * sizeRatio),
                            juce::roundToInt(90.0f * sizeRatio),
                            juce::roundToInt(34.0f * sizeRatio));

    sampleRateSlider->setShadowOffset(shadowDistance);
    sampleRateSlider->setFontHeight(32.0f * sizeRatio);
    sampleRateSlider->setBounds(juce::roundToInt(351.0f * sizeRatio),
                                juce::roundToInt(336.0f * sizeRatio),
                                juce::roundToInt(53.0f * sizeRatio),
                                juce::roundToInt(38.0f * sizeRatio));

    filterOnOff->setShadowDistance(shadowDistance);
    speakerType->setShadowDistance(shadowDistance);
    filterOnOff->setBounds(juce::roundToInt(137.0f * sizeRatio),
                           juce::roundToInt(339.0f * sizeRatio),
                           juce::roundToInt(152.0f * sizeRatio),
                           juce::roundToInt(34.0f * sizeRatio));
    speakerType->setBounds(juce::roundToInt(475.0f * sizeRatio),
                           juce::roundToInt(335.0f * sizeRatio),
                           juce::roundToInt(155.0f * sizeRatio),
                           juce::roundToInt(38.0f * sizeRatio));

    scope.setSizeRatio(sizeRatio);
    scope.setBounds(juce::roundToInt(140.0f * sizeRatio),
                    juce::roundToInt(131.0f * sizeRatio),
                    juce::roundToInt(480.0f * sizeRatio),
                    juce::roundToInt(155.0f * sizeRatio));
}
