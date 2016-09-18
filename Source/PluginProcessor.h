#ifndef PLUGINPROCESSOR_H_INCLUDED
#define PLUGINPROCESSOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"

// string state
typedef struct {
  uint8_t openNote; // the note the string has when fret = 0
  int fret = 0; // the current fret number on the string
  uint8_t velocity = 0; // the velocity of the last pluck
  int samplesLeft = 0; // samples before the string stops sounding
  int samplesSustain = 0; // the number of samples left after last pluck
  int age = 0; // samples since the last note change
  int note = -1; // the last MIDI note number of the string
  int sample = -1; // the sample when the string state was last changed
} StringState;

// button indices
typedef enum {
  ButtonSquare = 0,
  ButtonX,
  ButtonCircle,
  ButtonTriangle,
  ButtonSelect,
  ButtonStart,
  ButtonConsole,
  ButtonShake,
  ButtonDown,
  ButtonRight,
  ButtonUp,
  ButtonLeft,
  ButtonCount // (not a real button)
} ButtonIndex;

// instrument state
typedef struct {
  StringState string[6]; // string 0 has the highest pitch
  bool button[ButtonCount]; // buttons
  int detune = 0; // number of semitones to adjust tuning on all strings
  double sustain = 1.0; // the maximum length of played notes
  bool hammeron = true; // whether to allow the note to rise while sounding
  bool pulloff = true; // whether to allow the note to fall while sounding
  bool dampOpen = true; // whether to damp the string when it becomes open
  bool tap = false; // whether to start notes when frets are pressed
  bool dirty = false; // whether any state has changed
} GuitarState;

class StanginAudioProcessor  : public AudioProcessor {
  public:
    StanginAudioProcessor();
    ~StanginAudioProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool setPreferredBusArrangement (bool isInput, int bus, const AudioChannelSet& preferredSet) override;
   
    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    GuitarState guitar;
    
    float sustainIncrement = 0.1f;
    float minSustain = 0.01f;
    float maxSustain = 10.0f;

  protected:
    int timePressingButton = 0;
    
    GuitarState updateGuitarState(GuitarState state, int sample, const uint8_t *data, int dataSize);
    GuitarState sendNotes(GuitarState oldState, GuitarState newState, MidiBuffer &output);
    GuitarState ageGuitarState(GuitarState state, int startSample, int endSample, MidiBuffer &output);
    GuitarState onButton(GuitarState state, ButtonIndex button, int sample);

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StanginAudioProcessor)
};

#endif  // PLUGINPROCESSOR_H_INCLUDED
