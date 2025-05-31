#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <BinaryData.h>

class TextSlider : public juce::Slider
{
public:

    TextSlider();
    void paint (juce::Graphics& g) override;

    void setUseDigitalReadout(bool shouldBeDigital);
    void setNumDecimalPlaces(int numPlaces);

    void setFontHeight(float newHeight);

private:

    bool digitalReadout = false;
    int numDecimals = 1;

    float fontHeight = 32.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TextSlider)

};
