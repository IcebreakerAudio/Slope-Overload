#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class PixelScope : public juce::Component
{
public:

    PixelScope();

    void paint (juce::Graphics& g) override;
    void resized() override;

    void setDataAt(int index, float minValue, float maxValue);

    constexpr int getDataSize() { return pixelsX; }

private:

    static constexpr int pixelsX = 60;
    static constexpr int pixelsY = 18;
    static constexpr float pixelsYfloat = static_cast<float>(pixelsY);

    float pixelSize = 4.0f;

    std::vector<float> dataMin;
    std::vector<float> dataMax;

    float normalizeValue(float in)
    {
        in = (in * 0.5f) + 0.5f;
        in = 1.0f - in;
        in = juce::jlimit(0.0f, 1.0f, in);
        return in;
    }

    class PixelScopeBackground : public juce::Component
    {
    public:
        PixelScopeBackground(int numPixelsX, int numPixelsY, float pixelSize);

        void paint (juce::Graphics& g) override;
        void resized() override {};

        void setPixelSize(float newSize);
        void setPixelOffset(float newOffset);

    private:

        int pxX = 60, pxY = 19;
        float pxSize = 4.0f, pxOffset = 2.0f;
    };

    PixelScopeBackground background { pixelsX, pixelsY, pixelSize };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PixelScope)
};

