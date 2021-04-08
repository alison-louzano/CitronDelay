#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"

//==============================================================================
class AudioPluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    AudioPluginAudioProcessorEditor (juce::AudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    //==============================================================================
    typedef juce::AudioProcessorValueTreeState::SliderAttachment SliderAttachment;
    typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;

private:

    juce::Slider delayLengthSlider_, feedbackSlider_, dryMixSlider_, wetMixSlider_, bpmSlider_;
    juce::Label delayLengthLabel_, feedbackLabel_, dryMixLabel_, wetMixLabel_, bpmSliderLabel_;

    juce::TextButton syncButton_;
    juce::Label syncButtonLabel_;

    int sliderWidth_, sliderHeight_, buttonWidth_, buttonHeight_;

    void addSliderAndSetStyle (juce::Slider& slider);
    void addLabelToSlider (juce::Label& label, juce::Slider& slider, const char* text);
    void updateToggleState (juce::TextButton& button);

    std::unique_ptr<SliderAttachment> delayLengthAttachment, feedbackAttachment, dryMixAttachment, wetMixAttachment, bpmAttachment;
    std::unique_ptr<ButtonAttachment> syncButtonAttachment;

    std::unique_ptr<juce::Drawable> titleSVG;

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    juce::AudioProcessorValueTreeState& valueTreeState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
