
#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

template<typename SampleType>
class Fifo
{
public:

    Fifo()
    {
    }

    Fifo(int size)
    {
        setSize(size);
    }

    void setSize(int numElements)
    {
        jassert(numElements > 0);

        abstractFifo.setTotalSize(numElements);
        internalBuffer.resize(numElements);
        reset();
    }

    void reset()
    {
        std::fill(internalBuffer.begin(), internalBuffer.end(), static_cast<SampleType>(0.0));
        abstractFifo.reset();
    }

    int size()
    {
        return abstractFifo.getTotalSize();
    }

    int getSizeToRead()
    {
        return abstractFifo.getNumReady();
    }

    void addToFifo(const juce::AudioBuffer<SampleType>& buffer, int numChannelsToRead = -1)
    {
        const auto numSamples = buffer.getNumSamples();
        if(numChannelsToRead <= 0) {
            numChannelsToRead = buffer.getNumChannels();
        }

        jassert(numChannelsToRead > 0);

        const auto scope = abstractFifo.write(juce::jmin(numSamples, abstractFifo.getFreeSpace()));

        if (scope.blockSize1 > 0)
        {
            for(int i = 0; i < scope.blockSize1; ++i)
            {
                auto value = buffer.getReadPointer(0)[i];
                if(numChannelsToRead > 1) {
                    for(int c = 1; c < numChannelsToRead; ++c) {
                        value += buffer.getReadPointer(c)[i];
                    }
                    value = value / static_cast<SampleType>(numChannelsToRead);
                }
                internalBuffer[i + scope.startIndex1] = value;
            }
        }

        if (scope.blockSize2 > 0)
        {
            for(int i = 0; i < scope.blockSize2; ++i)
            {
                auto value = buffer.getReadPointer(0)[i + scope.blockSize1];
                if(numChannelsToRead > 1) {
                    for(int c = 1; c < numChannelsToRead; ++c) {
                        value += buffer.getReadPointer(c)[i + scope.blockSize1];
                    }
                    value = value / static_cast<SampleType>(numChannelsToRead);
                }
                internalBuffer[i + scope.startIndex2] = value;
            }
        }
    }

    void zeroFifo(int numItems)
    {
        const auto scope = abstractFifo.write(juce::jmin(numItems, abstractFifo.getFreeSpace()));

        if (scope.blockSize1 > 0) {
            std::fill(internalBuffer.begin() + scope.startIndex1, internalBuffer.begin() + scope.startIndex1 + scope.blockSize1, static_cast<SampleType>(0.0));
        }

        if (scope.blockSize2 > 0) {
            std::fill(internalBuffer.begin() + scope.startIndex2, internalBuffer.begin() + scope.startIndex2 + scope.blockSize2, static_cast<SampleType>(0.0));
        }
    }

    void addToFifo(const SampleType* someData, int numItems)
    {
        const auto scope = abstractFifo.write(juce::jmin(numItems, abstractFifo.getFreeSpace()));

        if (scope.blockSize1 > 0) {
            std::memcpy(internalBuffer.data() + scope.startIndex1, someData, scope.blockSize1 * sizeof(SampleType));
        }

        if (scope.blockSize2 > 0) {
            std::memcpy(internalBuffer.data() + scope.startIndex2, someData + scope.blockSize1, scope.blockSize2 * sizeof(SampleType));
        }
    }

    void readFromFifo(SampleType* someData, int numItems)
    {
        const auto scope = abstractFifo.read(numItems);

        if (scope.blockSize1 > 0) {
            std::memcpy(someData, internalBuffer.data() + scope.startIndex1, scope.blockSize1 * sizeof(SampleType));
        }

        if (scope.blockSize2 > 0) {
            std::memcpy(someData + scope.blockSize1, internalBuffer.data() + scope.startIndex2, scope.blockSize2 * sizeof(SampleType));
        }
    }

private:

    juce::AbstractFifo abstractFifo {1};
    std::vector<SampleType> internalBuffer;
};
