#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "sync.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
     delayBuffer(2,1),
     parameters (*this, nullptr, juce::Identifier ("AEDelayParameters"),{
                 std::make_unique<juce::AudioParameterInt> ("delayLength", "Delay Length", 0, 17, 2,
                                                            "",
                                                            [](int index, int) { return Synced[index].title;},
                                                            [](const juce::String& text) { return text.getIntValue();}),
                 std::make_unique<juce::AudioParameterFloat> ("feedback", "Feedback", 0.0f, 0.99f, 0.5f),
                 std::make_unique<juce::AudioParameterFloat> ("dryMix", "Dry Mix", 0.0f, 1.0f, 1.0f),
                 std::make_unique<juce::AudioParameterFloat> ("wetMix", "Wet Mix", 0.0f, 1.0f, 0.5f),
                 std::make_unique<juce::AudioParameterFloat> ("bpm", "Set BPM", juce::NormalisableRange<float> (80.0f, 150.0f, 0.1f), 120.0f),
                 std::make_unique<juce::AudioParameterBool>  ("isSync", "Is Synced", true, " bpm")})
{
    // store parameter pointers
    delayLengthParameter = parameters.getRawParameterValue("delayLength");
    feedbackParameter = parameters.getRawParameterValue("feedback");
    dryMixParameter = parameters.getRawParameterValue("dryMix");
    wetMixParameter = parameters.getRawParameterValue("wetMix");
    bpmParameter = parameters.getRawParameterValue("bpm");
    isSyncParameter = parameters.getRawParameterValue("isSync");
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1; 
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    // set a max delay time of 5 seconds
    const int maxDelayTime = 5;

    // Allocate memmory for delayBuffer
    int delayBufferLength = static_cast<int> (maxDelayTime * sampleRate); // number of samples in 1 seconds
    int numChannels = getTotalNumInputChannels();

    delayBuffer.setSize(numChannels, delayBufferLength);
    delayBuffer.clear();

    juce::ignoreUnused(samplesPerBlock);
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    int totalNumInputChannels = getTotalNumInputChannels();
    int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
    {
        buffer.clear(channel, 0, buffer.getNumSamples());
    }

    if (*isSyncParameter > 0.5f)
    {
        // case sync boolean is ON
        // get BPM from host info
        playHead = this->getPlayHead();
        if (playHead != nullptr)
        {
            playHead->getCurrentPosition(hostTimeConfig);
        }

        if (mTempo != hostTimeConfig.bpm)
        {
            mTempo = hostTimeConfig.bpm;
        }
    }
    else
    {
        // case is OFF : take BPM from Slider
        if (mTempo != *bpmParameter)
        {
            mTempo = *bpmParameter;
        }
    }
          
    // calculate delay in samples
    int delaySamples = static_cast<int> ((60. / mTempo) * getSampleRate() * Synced[(int) *delayLengthParameter].value);
        
        
    // when getting our read position, we calculate our position by counting past the top of our buffer
    // if we didn't do it this way we would have to deal with negative modulo which is confusing
    const int readPosition = (delayBuffer.getNumSamples() + writePosition - delaySamples) % delayBuffer.getNumSamples();

    // fill delay buffer
    for(int channel = 0; channel < totalNumInputChannels; channel++)
    {
        
        bufferData = buffer.getReadPointer(channel);
        delayBufferData = delayBuffer.getReadPointer(channel);
        
        
        // first we need to fill the delay buffer with the incoming block. The delay buffer basically
        // works like a queue, so we want to implement it as a FIFO structure
        // to do this we need to keep track of a writePosition to see where we can enqueue the newest block

        // Since the delayed signal is after added multiplied with feedback
        // we compensate to avoid a delayed note higher than the signal
        
        //if there is enough space for an entire block of new samples, we just add them in
        if(delayBuffer.getNumSamples() > buffer.getNumSamples() + writePosition)
        {
            //this is how you copy samples from one buffer to another in JUCE
            delayBuffer.copyFrom(channel, writePosition, bufferData, buffer.getNumSamples(), (1 - *feedbackParameter));
            
        }
        //if there is not enough space, we use the rest of the available space in the delay buffer, and then
        // we start overwriting the oldest samples by wrapping around to the beginning of the delay buffer
        // this creates our circular buffer
        else
        {
            // to get the remaing space available at the end of the delayBuffer, we get the difference in samples
            // between our total delayBuffer size and the current position we have written up to
            delayBufRemaining = delayBuffer.getNumSamples() - writePosition;
            
            delayBuffer.copyFrom(channel, writePosition, bufferData, delayBufRemaining, (1 - *feedbackParameter));        
            delayBuffer.copyFrom(channel, 0, bufferData + delayBufRemaining, buffer.getNumSamples() - delayBufRemaining), (1 - *feedbackParameter);
            
        }
        
        //retrieve from delay buffer and add back to buffer
        
        // if we can get a full block worth of samples between our read position and the end of the buffer
        // we just add them on top of the samples in the current block
        if(delayBuffer.getNumSamples() > readPosition + buffer.getNumSamples())
        {
            // addFromWithRamp adds the specified delay signal from the delay buffer to the current blocks
            // existing signal
            buffer.addFromWithRamp(channel, 0, delayBufferData + readPosition, buffer.getNumSamples(), *wetMixParameter, *wetMixParameter);
            
        }
        else
        {
            //similar process to above
            delayBufRemaining = delayBuffer.getNumSamples() - readPosition;
            buffer.addFromWithRamp(channel, 0, delayBufferData + readPosition, delayBufRemaining, *wetMixParameter, *wetMixParameter);
            buffer.addFromWithRamp(channel, delayBufRemaining, delayBufferData, buffer.getNumSamples() - delayBufRemaining, *wetMixParameter, *wetMixParameter);
            
        }
        
        
        // Finally, we want to add back the combination of the delayed and dry signal in the new block to
        // the delay buffer to create a feedback loop
        
        if(delayBuffer.getNumSamples() > writePosition + buffer.getNumSamples())
        {
            delayBuffer.addFromWithRamp(channel, writePosition, bufferData, buffer.getNumSamples(), *feedbackParameter, *feedbackParameter);
        }
        else
        {
            delayBufRemaining = delayBuffer.getNumSamples() - writePosition;
            delayBuffer.addFromWithRamp(channel, writePosition, bufferData, delayBufRemaining, *feedbackParameter, *feedbackParameter);
            delayBuffer.addFromWithRamp(channel, 0, bufferData + delayBufRemaining, buffer.getNumSamples() - delayBufRemaining, *feedbackParameter, *feedbackParameter);
        }
        
    }
    
    // apply output gain to buffer
    buffer.applyGain(*dryMixParameter);
    
    //After each block we update the write pointer position by increments of the block size
    writePosition += buffer.getNumSamples();
    
    // If we get to the end of the delay buffer, we need to wrap around to the beginning again
    writePosition %= delayBuffer.getNumSamples();
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this, parameters);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();       
}
