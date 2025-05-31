#include "RadioButtonComponent.h"

//==============================================================================
RadioButtonComponent::RadioButtonComponent()
{
}

RadioButtonComponent::RadioButtonComponent(const juce::StringArray& itemList)
{
    for(auto& i : itemList) {
        addItem(i);
    }
}

RadioButtonComponent::~RadioButtonComponent()
{
}

void RadioButtonComponent::paint (juce::Graphics& g)
{
    if(dividers.isEmpty()) {
        return;
    }

    g.setColour(getLookAndFeel().findColour(juce::TextButton::ColourIds::buttonColourId));
    g.setFont(getLocalBounds().toFloat().getHeight() * 1.1f);

    for(auto& d : dividers) {
        g.drawFittedText(dividerText, *d, juce::Justification::centred, 1, 1.0f);
    }
}

void RadioButtonComponent::resized()
{
    if(numItems < 1) {
        return;
    }

    auto bounds = getLocalBounds();
    auto totalTextLength = 0;

    for (auto i : items) {
        totalTextLength += i->getLength();
    }

    auto dividerWidth = juce::roundToInt(juce::TextLayout::getStringWidth(juce::Font(juce::FontOptions()), dividerText));
    if(dividerText.isNotEmpty())
    {
        dividers.clear();
        totalTextLength += dividerWidth * (numItems - 1);
    }

    DBG("Total Length:");
    DBG(totalTextLength);

    auto width = bounds.getWidth();

    for(int i = 0; i < numItems; ++i)
    {
        items[i]->setBounds(bounds.removeFromLeft((width * items[i]->getLength()) / totalTextLength));
        if(dividerText.isNotEmpty() && i != numItems - 1)
        {
            dividers.add(new juce::Rectangle<int>(bounds.removeFromLeft((width * dividerWidth) / totalTextLength)));
        }
    }

}

void RadioButtonComponent::setShadowDistance(float newDistance)
{
    for(auto& i : items)
    {
        i->setShadowDistance(newDistance);
    }
}

void RadioButtonComponent::setSelectedItemIndex(int itemIndex)
{
    jassert(juce::isPositiveAndBelow(itemIndex, numItems));
    itemIndex = juce::jlimit(0, numItems, itemIndex);

    for (auto i : items)
    {
        i->setToggleState(i->getIndex() == itemIndex, juce::dontSendNotification);
    }

    selectedItemIndex = itemIndex;
    selectedItemValue.setValue(selectedItemIndex);
}

void RadioButtonComponent::addItem(juce::StringRef displayText, juce::StringRef itemTip)
{
    auto i = items.add(new RadioButton(numItems, displayText, itemTip));
    addAndMakeVisible(i);
    ++numItems;
}

void RadioButtonComponent::addListener(juce::Value::Listener* l)
{
    selectedItemValue.addListener(l);
}

void RadioButtonComponent::removeListener(juce::Value::Listener* l)
{
    selectedItemValue.removeListener(l);
}

//==============================================================================

RadioButtonComponent::RadioButton::RadioButton(int indexValue, juce::StringRef displayText, juce::StringRef itemTip)
    : juce::Button(""),
      index(indexValue)
{
    setButtonText(displayText);
    onClick = [&]()
        {
            auto p = getParentComponent();
            jassert(p != nullptr);
            auto c = dynamic_cast<RadioButtonComponent*>(p);
            jassert(c != nullptr);
            c->setSelectedItemIndex(index);
            if (c->onChildClicked != nullptr) {
                c->onChildClicked();
            }
        };
    setTooltip(itemTip);
}

void RadioButtonComponent::RadioButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    auto bounds = getLocalBounds();

    juce::TextLayout layout;
    g.setFont(float(bounds.getHeight()) * 1.25f);

    g.setColour(getLookAndFeel().findColour(juce::TextButton::ColourIds::buttonColourId));
    g.drawText(getButtonText(), bounds.withX(shadowDistance).withY(shadowDistance), juce::Justification::centred, false);

    const auto state = getToggleState();
    if(state)
    {
        g.setColour(getLookAndFeel().findColour(juce::TextButton::ColourIds::textColourOnId));
        g.drawText(getButtonText(), bounds, juce::Justification::centred, false);
    }
}

//==============================================================================

RadioButtonAttachment::RadioButtonAttachment(juce::RangedAudioParameter& param, RadioButtonComponent& component, juce::UndoManager* undoManager)
    : componentRef(component),
      storedParameter(param),
      attachment(param, [this](float f) { setValue(f); }, undoManager)
{
    sendInitialUpdate();
    componentRef.addListener(this);
}

RadioButtonAttachment::~RadioButtonAttachment()
{
    componentRef.removeListener(this);
}

void RadioButtonAttachment::sendInitialUpdate()
{
    attachment.sendInitialUpdate();
}

void RadioButtonAttachment::setValue(float newValue)
{
    const auto normValue = storedParameter.convertTo0to1(newValue);
    const auto index = juce::roundToInt(normValue * (float)(componentRef.getNumItems() - 1));

    if (index == componentRef.getSelectedItemIndex())
        return;

    componentRef.setSelectedItemIndex(index);
}

void RadioButtonAttachment::valueChanged(juce::Value& value)
{
    if (!ignoreCallbacks) {
        attachment.setValueAsCompleteGesture(value.getValue());
    }
}
