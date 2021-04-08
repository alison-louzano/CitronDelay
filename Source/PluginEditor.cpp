#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (juce::AudioProcessor& parent, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor(parent), 
    valueTreeState (vts),
    sliderHeight_ {100}, sliderWidth_ {100}, buttonHeight_ {50}, buttonWidth_ {50}
{

    addAndMakeVisible(delayLengthSlider_);
    addSliderAndSetStyle(delayLengthSlider_);
    addLabelToSlider(delayLengthLabel_, delayLengthSlider_, "Delay Type");
    delayLengthAttachment.reset(new SliderAttachment (valueTreeState, "delayLength", delayLengthSlider_));

    addAndMakeVisible(feedbackSlider_);
    addSliderAndSetStyle(feedbackSlider_);
    addLabelToSlider(feedbackLabel_, feedbackSlider_, "Feedback");
    feedbackAttachment.reset(new SliderAttachment (valueTreeState, "feedback", feedbackSlider_));

    addAndMakeVisible(dryMixSlider_);
    addSliderAndSetStyle(dryMixSlider_);
    addLabelToSlider(dryMixLabel_, dryMixSlider_, "Dry Mix");
    dryMixAttachment.reset(new SliderAttachment (valueTreeState, "dryMix", dryMixSlider_));

    addAndMakeVisible(wetMixSlider_);
    addSliderAndSetStyle(wetMixSlider_);
    addLabelToSlider(wetMixLabel_, wetMixSlider_, "Wet Mix");
    wetMixAttachment.reset(new SliderAttachment (valueTreeState, "wetMix", wetMixSlider_));

    addAndMakeVisible(bpmSlider_);
    addSliderAndSetStyle(bpmSlider_);
    addLabelToSlider(bpmSliderLabel_, bpmSlider_, "Set BPM");
    bpmAttachment.reset(new SliderAttachment (valueTreeState, "bpm", bpmSlider_));

    addAndMakeVisible(syncButton_);                                   
    syncButton_.setClickingTogglesState(true);
    syncButton_.setButtonText("OFF");
    // note that for this button we don't use a label as for the Sliders
    // The label text width, when setted to the above the component, is equal to the component width
    // Since the width of the button is small, I draw text on the paint callback to have the text not cutted off
    syncButton_.onStateChange = [this] { updateToggleState(syncButton_); };
    syncButtonAttachment.reset(new ButtonAttachment (valueTreeState, "isSync", syncButton_));

    // add the SVG logo
    titleSVG = juce::Drawable::createFromImageData(BinaryData::delayTitle_svg, BinaryData::delayTitle_svgSize);
    addAndMakeVisible(*titleSVG);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (300, 400);
    setResizeLimits(300, 500, 800, 600);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (juce::Colours::aquamarine);

    // workaround to set button label
    auto r = getLocalBounds();
    auto titleSection = r.removeFromTop(getHeight() / 4);
    auto topSection = r.removeFromTop(getHeight() / 4);
    auto topSectionLeft = topSection.removeFromLeft(getWidth() / 2);
    g.setColour(feedbackSlider_.findColour(juce::Slider::ColourIds::rotarySliderFillColourId));
    g.setFont(15.0);
    g.drawText ("Sync Host BPM", topSectionLeft.getCentreX() - 50,
                                 bpmSlider_.getY() - bpmSliderLabel_.getHeight(),
                                 100,  bpmSliderLabel_.getHeight(), juce::Justification::horizontallyCentred);
}

void AudioPluginAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto r = getLocalBounds();

    auto titleSection = r.removeFromTop(getHeight() / 4);
    titleSVG->setTransformToFit(titleSection.reduced(30).toFloat(), juce::RectanglePlacement::centred);

    auto topSection = r.removeFromTop(getHeight() / 4);
    auto topSectionLeft = topSection.removeFromLeft(getWidth() / 2);

    syncButton_.setBounds(topSectionLeft.getCentreX() - buttonWidth_ / 2,
                          topSectionLeft.getCentreY() - buttonHeight_ / 2,
                          buttonWidth_, buttonHeight_);

    bpmSlider_.setBounds(topSection.getCentreX() - sliderWidth_ / 2, 
                              topSection.getCentreY() - sliderHeight_ / 2, 
                              sliderWidth_, sliderHeight_);

    auto midSection = r.removeFromTop(getHeight() / 4);
    auto midSectionLeft = midSection.removeFromLeft(midSection.getWidth() / 2);

    delayLengthSlider_.setBounds(midSectionLeft.getCentreX() - sliderWidth_ / 2, 
                              midSectionLeft.getCentreY() - sliderHeight_ / 2, 
                              sliderWidth_, sliderHeight_);

    feedbackSlider_.setBounds(midSection.getCentreX() - sliderWidth_ / 2, 
                              midSection.getCentreY() - sliderHeight_ / 2, 
                              sliderWidth_, sliderHeight_);

    auto botSectionLeft = r.removeFromLeft(r.getWidth() / 2);

    dryMixSlider_.setBounds(botSectionLeft.getCentreX() - sliderWidth_ / 2, 
                            botSectionLeft.getCentreY() - sliderHeight_ / 2, 
                            sliderWidth_, sliderHeight_);
    wetMixSlider_.setBounds(r.getCentreX() - sliderWidth_ / 2, 
                            r.getCentreY() - sliderHeight_ / 2, 
                            sliderWidth_, sliderHeight_);
}

//==============================================================================

void AudioPluginAudioProcessorEditor::addSliderAndSetStyle(juce::Slider& slider)
{
    // add common attributes for all sliders
    slider.setSliderStyle(juce::Slider::Rotary);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setPopupDisplayEnabled(true, true, this);
}

void AudioPluginAudioProcessorEditor::addLabelToSlider(juce::Label& label, juce::Slider& slider, const char* text)
{
    // add common attributes for all labels
    label.attachToComponent(&slider, false);
    label.setText(text, juce::NotificationType::dontSendNotification);
    label.setColour(juce::Label::ColourIds::textColourId,
                    feedbackSlider_.findColour(juce::Slider::ColourIds::rotarySliderFillColourId));
    label.setJustificationType(juce::Justification::horizontallyCentred);
    label.setFont(15.0);
}

void AudioPluginAudioProcessorEditor::updateToggleState(juce::TextButton& button)
{
    // change TextButton text when clicked
    juce::String state = button.getToggleStateValue() > 0.5f ? "ON" : "OFF";
    button.setButtonText(state);
}


