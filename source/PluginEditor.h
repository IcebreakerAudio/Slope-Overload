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
    void resized() override;

    //==============================================================================
    void timerCallback() override;

private:
    AudioPluginAudioProcessor& processorRef;

    std::vector<float> scopeDataRaw;
    void updateScopeDataSize();

    //==============================================================================

    static constexpr int originalWidth = 715;
    static constexpr int originalHeight = 460;

    static constexpr float originalWidthF = static_cast<float>(originalWidth);
    static constexpr float originalHeightF = static_cast<float>(originalHeight);

    //==============================================================================

    struct LabelWithDetails
    {
        juce::String text;
        float posX;
        float posY;
        juce::Justification justification;

        std::unique_ptr<juce::Label> shadow;
        std::unique_ptr<juce::Label> label;
    };

    std::array<LabelWithDetails, 5> labelDetails
    {
        LabelWithDetails{"IN:", 144.0f, 82.0f, juce::Justification::centredLeft},
        LabelWithDetails{"OUT:", 478.0f, 82.0f, juce::Justification::centredLeft},
        LabelWithDetails{"FILTER", 144.0f, 295.0f, juce::Justification::centred},
        LabelWithDetails{"S.RATE", 311.0f, 295.0f, juce::Justification::centred},
        LabelWithDetails{"SPEAKER", 484.0f, 295.0f, juce::Justification::centred}
    };

    //==============================================================================

    std::unique_ptr<juce::Drawable> background;

    PixelScope scope;

    std::unique_ptr<TextSlider> inGainSlider, outGainSlider, sampleRateSlider;
    std::unique_ptr<juce::SliderParameterAttachment> inGainAttachment, outGainAttachment, sampleRateAttachment;

    std::unique_ptr<RadioButtonComponent> filterOnOff, speakerType;
    std::unique_ptr<RadioButtonAttachment> filterAttachment, speakerAttachment;

    std::unique_ptr<juce::DrawableButton> powerButton;
    std::unique_ptr<juce::ButtonParameterAttachment> powerAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
