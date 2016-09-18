#ifndef PLUGINEDITOR_H_INCLUDED
#define PLUGINEDITOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

class StanginAudioProcessorEditor  : public AudioProcessorEditor, public Timer {
  public:
    StanginAudioProcessorEditor (StanginAudioProcessor&);
    ~StanginAudioProcessorEditor();

    void paint(Graphics&) override;
    void resized() override;
    
    void drawStrings(Graphics &g);
    void drawSlider(Graphics &g, Rectangle<int> area, String text, float value, bool active);
    void drawButton(Graphics &g, Rectangle<int> area, String text, bool on);
    
    virtual void timerCallback();
    
    virtual void mouseDown(const MouseEvent &event);
    virtual void mouseUp(const MouseEvent &event);
    virtual void mouseDoubleClick(const MouseEvent &event);
    virtual void mouseDrag(const MouseEvent &event);

  protected:
    int em; // the em size of the font to use
    Font font; // the font to use for regular text
    Font smallFont; // the font to use for smaller text
    Colour bg, fg; // the background and foreground colors to draw with
    Colour accent; // an accent color to make elements stand out
    Rectangle<int> stringArea; // the area for the string display
    Rectangle<int> sustainArea; // the area for the sustain slider
    Rectangle<int> detuneArea; // the area for the detune slider
    Rectangle<int> hammeronArea; // the area for the hammer-on toggle
    Rectangle<int> pulloffArea; // the area for the pull-off toggle
    Rectangle<int> dampOpenArea; // the area for the damp open toggle
    Rectangle<int> tapArea; // the area for the tap toggle
    // whether the user is changing slider values
    bool sustainActive = false;
    bool detuneActive = false;
    
    // get the value a slider should have for the given mouse position
    float getSliderFraction(const MouseEvent &event, const Rectangle<int> area);
    // update sustain
    void setSustainFraction(float f);
    float getSustainFraction();
    // update detune
    void setDetuneFraction(float f);
    float getDetuneFraction();
    
    // get the name of a MIDI note, optionally with octave number
    String noteName(int note, bool withOctave);

  private:
    StanginAudioProcessor& processor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StanginAudioProcessorEditor)
};

#endif  // PLUGINEDITOR_H_INCLUDED
