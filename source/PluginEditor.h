#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "UI/PixelScope.h"
#include "UI/RadioButtonComponent.h"
#include "UI/TextSlider.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              public juce::Timer
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    //==============================================================================
    void timerCallback() override;

private:
    AudioPluginAudioProcessor& processorRef;

    std::vector<float> scopeDataRaw;
    void updateScopeDataSize();

    //==============================================================================

    const int originalWidth = 715;
    const int originalHeight = 460;

    const float originalWidthF = static_cast<float>(originalWidth);
    const float originalHeightF = static_cast<float>(originalHeight);

    //==============================================================================

    struct LabelDetails
    {
        juce::String text;
        float posX;
        float posY;
        juce::Justification justification;

        std::weak_ptr<juce::Label> label_ptr;
        std::weak_ptr<juce::Label> shadow_ptr;
    };

    std::array<LabelDetails, 5> labelDetails
    {
        LabelDetails{"IN:", 144.0f, 82.0f, juce::Justification::centredLeft},
        LabelDetails{"OUT:", 478.0f, 82.0f, juce::Justification::centredLeft},
        LabelDetails{"FILTER", 144.0f, 295.0f, juce::Justification::centred},
        LabelDetails{"S.RATE", 311.0f, 295.0f, juce::Justification::centred},
        LabelDetails{"SPEAKER", 484.0f, 295.0f, juce::Justification::centred}
    };

    //==============================================================================

    std::unique_ptr<juce::Drawable> background;

    PixelScope scope;

    std::vector<std::shared_ptr<juce::Label>> labels;

    std::unique_ptr<TextSlider> inGainSlider, outGainSlider, sampleRateSlider;
    std::unique_ptr<juce::SliderParameterAttachment> inGainAttachment, outGainAttachment, sampleRateAttachment;

    std::unique_ptr<RadioButtonComponent> filterOnOff, speakerType;
    std::unique_ptr<RadioButtonAttachment> filterAttachment, speakerAttachment;

    std::unique_ptr<juce::DrawableButton> powerButton;
    std::unique_ptr<juce::ButtonParameterAttachment> powerAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
