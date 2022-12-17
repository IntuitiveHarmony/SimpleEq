/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEqAudioProcessorEditor::SimpleEqAudioProcessorEditor (SimpleEqAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//attaching GUI sliders to the filters
peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
peakQualitySliderAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider)
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    for( auto* comp : getComps() )
    {
        addAndMakeVisible(comp);
    }
    
    //grab all parameters and listen to them
    const auto& params = audioProcessor.getParameters();
    for( auto param : params )
    {
        param->addListener(this);
    }
    
    startTimerHz(60);
    
    setSize (600, 400);
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
}

SimpleEqAudioProcessorEditor::~SimpleEqAudioProcessorEditor()
{
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    const auto& params = audioProcessor.getParameters();
    for( auto param : params )
    {
        param->removeListener(this);
    }
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
}

//==============================================================================
void SimpleEqAudioProcessorEditor::paint (juce::Graphics& g)
{
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);
    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);
    
    auto w = responseArea.getWidth();
    
    //get chain elements
    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();
    
    //get sample rate
    auto sampleRate = audioProcessor.getSampleRate();
    
    //place to store them^^
    std::vector<double> mags;
    
    mags.resize(w);
    
    for( int i = 0; i < w; ++i )
    {
        double mag = 1.f;
        //normalize to human hearing range
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);
        
        //check if the filters are bypassed
        if(! monoChain.isBypassed<ChainPositions::Peak>() )
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        //low and high have 4ea
        if( !lowcut.isBypassed<0>() )
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if( !lowcut.isBypassed<1>() )
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if( !lowcut.isBypassed<2>() )
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if( !lowcut.isBypassed<3>() )
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        
        if( !highcut.isBypassed<0>() )
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if( !highcut.isBypassed<1>() )
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if( !highcut.isBypassed<2>() )
            mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if( !highcut.isBypassed<3>() )
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        //convert mags to decibels and store it
        mags[i] = Decibels::gainToDecibels(mag);
    }
    
    //paths for the response curve
    Path responseCurve;
    
    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };
    //subpaths
    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    
    //apply to every other magnitude
    for( size_t i = 1; i < mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }
    
    //drawing the window
    g.setColour(Colours::orange);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);
    
    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
    
    
    
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
}

void SimpleEqAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    auto bounds = getLocalBounds();
    //setting some areas
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.33);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto highCutArea = bounds.removeFromRight (bounds.getWidth() * 0.5);
    
    //placing the sliders
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);


    peakFreqSlider.setBounds (bounds. removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds (bounds. removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);
    
}

void SimpleEqAudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue)
{
    //set Atomic flag
    parametersChanged.set(true);
}

void SimpleEqAudioProcessorEditor::timerCallback()
{
    if( parametersChanged.compareAndSetBool(false, true) )
    {
        //update the monochain
        auto chainSettings = getChainSettings(audioProcessor.apvts);
        auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
        
        auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
        auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
        
        updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
        updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);
        //signal a repaint
        repaint();
    }
}

std::vector<juce::Component*> SimpleEqAudioProcessorEditor::getComps()
{
    return
    {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider
    };
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
