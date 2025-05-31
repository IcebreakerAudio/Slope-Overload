#include "TextSlider.h"

TextSlider::TextSlider()
{
    setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, true, 1, 1);
}

void TextSlider::paint (juce::Graphics& g)
{
    auto val = getValue();
    auto units = getTextValueSuffix();
    auto bounds = getLocalBounds().toFloat();

    juce::String bgText;
    auto text = juce::String(val, digitalReadout ? 0 : numDecimals, false);

    if(digitalReadout)
    {
        bgText = "88";

        auto font = juce::Font(juce::FontOptions(juce::Typeface::createSystemTypefaceFor(BinaryData::DigitalNumbersRegular_ttf, BinaryData::DigitalNumbersRegular_ttfSize)));
        g.setFont(font.withPointHeight(fontHeight));
    }
    else
    {
        text << units;
        if(val >= 0)
        {
            if(text.startsWith("-")) {
                text = text.trimCharactersAtStart("-");
            }
            text = "+" + text;
        }
        bgText = text;

        g.setFont(juce::Font(juce::FontOptions().withPointHeight(fontHeight)));
    }

    g.setColour(getLookAndFeel().findColour(juce::Slider::ColourIds::backgroundColourId));
    g.drawText(bgText, bounds.withX(offset).withY(offset), juce::Justification::centredRight, false);

    g.setColour(getLookAndFeel().findColour(juce::Slider::ColourIds::textBoxTextColourId));
    g.drawText(text, bounds, juce::Justification::centredRight, false);
}

void TextSlider::setUseDigitalReadout(bool shouldBeDigital)
{
    digitalReadout = shouldBeDigital;
}

void TextSlider::setNumDecimalPlaces(int numPlaces)
{
    numDecimals = numPlaces;
}

void TextSlider::setFontHeight(float newHeight)
{
    fontHeight = newHeight;
}

void TextSlider::setShadowOffset(float newOffset)
{
    offset = newOffset;
}
