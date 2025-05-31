#include "PixelScope.h"

PixelScope::PixelScope()
{
    dataMin.resize(pixelsX);
    dataMax.resize(pixelsX);

    addAndMakeVisible(background);
}

void PixelScope::paint (juce::Graphics& g)
{
    if(dataMin.empty() || dataMax.empty()) {
        return;
    }

    g.setColour(juce::Colour(0xFF2A2E0D));

    auto bounds = getLocalBounds().toFloat();
    auto width = bounds.getWidth();
    auto height = bounds.getHeight();

    for(int i = 0; i < pixelsX; ++i)
    {
        auto yPos = dataMax[i];
        do
        {
            g.fillRect(i * width / pixelsX,
                    yPos * height,
                    pixelSize, pixelSize
            );
            yPos += 1.0f / pixelsYfloat;
        } while (yPos <= dataMin[i]);
    }
}

void PixelScope::resized()
{
    background.setBounds(getLocalBounds());
}

void PixelScope::setDataAt(int index, float minValue, float maxValue)
{
    auto indexValid = juce::isPositiveAndBelow(index, pixelsX);
    jassert(indexValid);

    if(!indexValid) {
        return;
    }

    minValue = normalizeValue(minValue);
    maxValue = normalizeValue(maxValue);

    dataMin[index] = round(minValue * pixelsYfloat) / pixelsYfloat;
    dataMax[index] = round(maxValue * pixelsYfloat) / pixelsYfloat;
}

//==============================================================================

PixelScope::PixelScopeBackground::PixelScopeBackground(int numPixelsX, int numPixelsY, float pixelSize)
    : pxX(numPixelsX), pxY(numPixelsY), pxSize(pixelSize)
{

}

void PixelScope::PixelScopeBackground::paint (juce::Graphics& g)
{
    g.setColour(juce::Colours::black.withAlpha(0.125f));

    auto bounds = getLocalBounds().toFloat();
    auto width = bounds.getWidth();
    auto height = bounds.getHeight();

    for(int x = 0; x < pxX; ++x)
    {
        for(int y = 0; y < pxY; ++y)
        {
            g.fillRect( (x * width / pixelsX) + pxOffset,
                        (y * height / pixelsY) + pxOffset,
                        pxSize, pxSize
            );
        }
    }
}

void PixelScope::PixelScopeBackground::setPixelSize(float newSize)
{
    pxSize = newSize;
}

void PixelScope::PixelScopeBackground::setPixelOffset(float newOffset)
{
    pxOffset = newOffset;
}
