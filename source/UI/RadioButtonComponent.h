#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================

class RadioButtonComponent : public juce::Component
{
public:
    RadioButtonComponent();
    RadioButtonComponent(const juce::StringArray& itemList);
    ~RadioButtonComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void setSelectedItemIndex(int itemIndex);
    int getSelectedItemIndex() { return selectedItemIndex; }
    int getNumItems() { return numItems; }

    void setShadowDistance(float newDistance);

    void addItem(juce::StringRef displayText, juce::StringRef itemTip = "");
    void addDivider(juce::StringRef displayText) { dividerText = displayText; }

    void addListener(juce::Value::Listener*);
    void removeListener(juce::Value::Listener*);

    std::function<void()> onChildClicked;

private:

    class RadioButton : public juce::Button
    {
    public:

        RadioButton(int indexValue, juce::StringRef displayText, juce::StringRef itemTip = "");
        ~RadioButton() {};

        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
        int getIndex() { return index; }

        void setShadowDistance(float newDistance) { shadowDistance = newDistance; }

        int getLength() { return juce::roundToInt(juce::TextLayout::getStringWidth(juce::Font(juce::FontOptions()), getButtonText())); }

    private:

        int index = 0;
        float shadowDistance = 2.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RadioButton)
    };

    juce::Value selectedItemValue { -1 };
    int numItems = 0, selectedItemIndex = -1;
    juce::OwnedArray<RadioButton> items;
    juce::OwnedArray<juce::Rectangle<int>> dividers;
    juce::String dividerText;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadioButtonComponent)
};

//==============================================================================

class RadioButtonAttachment : public juce::Value::Listener
{
public:
    RadioButtonAttachment(juce::RangedAudioParameter& parameter, RadioButtonComponent& component, juce::UndoManager* undoManager = nullptr);
    ~RadioButtonAttachment() override;
    void sendInitialUpdate();

private:

    void setValue(float newValue);
    void valueChanged(juce::Value& value) override;

    RadioButtonComponent& componentRef;
    juce::RangedAudioParameter& storedParameter;
    juce::ParameterAttachment attachment;
    bool ignoreCallbacks = false;
};
