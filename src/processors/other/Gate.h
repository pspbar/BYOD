#pragma once

#include "../BaseProcessor.h"

class Gate : public BaseProcessor
{
public:
    explicit Gate (UndoManager* um);
    ~Gate() override;

    ProcessorType getProcessorType() const override { return Other; }
    static ParamLayout createParameterLayout();

    void prepare (double sampleRate, int samplesPerBlock) override;
    void processAudio (AudioBuffer<float>& buffer) override;

private:
    chowdsp::FloatParameter* threshDBParam = nullptr;
    chowdsp::FloatParameter* attackMsParam = nullptr;
    chowdsp::FloatParameter* holdMsParam = nullptr;
    chowdsp::FloatParameter* releaseMsParam = nullptr;
    chowdsp::FloatParameter* makeupDBParam = nullptr;

    AudioBuffer<float> levelBuffer;
    dsp::Gain<float> makeupGain;

    class GateEnvelope;
    chowdsp::LocalPointer<GateEnvelope, 1280> gateEnvelope;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Gate)
};
