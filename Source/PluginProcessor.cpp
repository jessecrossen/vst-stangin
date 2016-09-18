#include "PluginProcessor.h"
#include "PluginEditor.h"

StanginAudioProcessor::StanginAudioProcessor() {
  int i;
  guitar.string[0].openNote = 0x40;
  guitar.string[1].openNote = 0x3B;
  guitar.string[2].openNote = 0x37;
  guitar.string[3].openNote = 0x32;
  guitar.string[4].openNote = 0x2D;
  guitar.string[5].openNote = 0x28;
  for (i = 0; i < 6; i++) {
    guitar.button[i] = false;
  }
}

StanginAudioProcessor::~StanginAudioProcessor() {
}

// FILTER *********************************************************************

void StanginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void StanginAudioProcessor::releaseResources() {
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

void StanginAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& input) {
  GuitarState newState;
  // buffer for sysex data
  const uint8_t *data;
  int dataSize;
  // buffer for outgoing messages
  MidiBuffer output;
  // read incoming messages
  MidiBuffer::Iterator i(input);
  MidiMessage msg(0xF0);
  int lastSample = 0;
  int sample;
  while (i.getNextEvent(msg, sample)) {
    if (! msg.isSysEx()) continue;
    data = msg.getSysExData();
    dataSize = msg.getSysExDataSize();
    if (dataSize < 4) continue;
    guitar = ageGuitarState(guitar, lastSample, sample, output);
    newState = updateGuitarState(guitar, sample, data, dataSize);
    if (newState.dirty) {
      guitar = sendNotes(guitar, newState, output);
      AudioProcessorEditor *editor = getActiveEditor();
    }
    lastSample = sample;
  }
  guitar = ageGuitarState(guitar, lastSample, buffer.getNumSamples() - 1, output);
  // swap in the output buffer
  input.swapWith(output);
}

// update the state of the guitar from sysex data
GuitarState StanginAudioProcessor::updateGuitarState(GuitarState state, int sample, const uint8_t *data, int dataSize) {
  uint8_t type, fret, byte;
  // get the shortest number of samples between plays of the same note
  int minAge = (int)(0.050 * getSampleRate());
  // see what type of event we're handling
  type = data[3];
  // get the current string
  uint8_t i = (dataSize >= 5) ? (data[4] - 1) % 6 : 0;
  StringState &string = state.string[i];
  // keepalive events, ignore
  if (type == 0x09) { }
  // changes to the fret state
  else if ((type == 0x01) && (dataSize >= 6)) {
    fret = data[5];
    // offset fret numbers relative to the base note of each string
    switch (i) {
      case 0: fret -= 0x40; break;
      case 1: fret -= 0x3B; break;
      case 2: fret -= 0x37; break;
      case 3: fret -= 0x32; break;
      case 4: fret -= 0x2D; break;
      case 5: fret -= 0x28; break;
      default: return(state);
    }
    // if the fret changes to open, stop the note
    if ((state.dampOpen) && (string.fret > 0) && (fret == 0) && 
        (string.age >= minAge)) {
      string.samplesLeft = 0;
    }
    // enable tap mode
    else if ((state.tap) && (string.fret != fret)) {
      string.velocity = 127;
      string.samplesLeft = string.samplesSustain = 
        (int)(state.sustain * getSampleRate());
    }
    // enable/disable hammer-on
    if ((! state.hammeron) && (fret > string.fret)) {
      string.samplesLeft = 0;
    }
    // enable/disable pull-off
    else if ((! state.pulloff) && (fret < string.fret)) {
      string.samplesLeft = 0;
    }
    // update the string
    if ((string.fret != fret) || (string.age >= minAge)) {
      string.fret = fret;
      string.sample = sample;
    }
    state.dirty = true;
  }
  // picking events
  else if ((type == 0x05) && (dataSize >= 6)) {
    string.velocity = data[5];
    if (string.age >= minAge) {
      string.sample = sample;
      string.samplesLeft = string.samplesSustain = 
        (int)(state.sustain * getSampleRate() * 127.0) / string.velocity;
    }
    state.dirty = true;
  }
  // button events
  else if ((type == 0x08) && (dataSize >= 7)) {
    GuitarState oldState = state;
    byte = data[4];
    state.button[ButtonSquare]   = byte & 0x01;
    state.button[ButtonX]        = byte & 0x02;
    state.button[ButtonCircle]   = byte & 0x04;
    state.button[ButtonTriangle] = byte & 0x08;
    byte = data[5];
    state.button[ButtonSelect]   = byte & 0x01;
    state.button[ButtonStart]    = byte & 0x02;
    state.button[ButtonConsole]  = byte & 0x10;
    byte = data[6];
    state.button[ButtonShake]    = byte & 0x40;
    byte &= 0x0F;
    state.button[ButtonDown]     = (byte == 0x0);
    state.button[ButtonRight]    = (byte == 0x2);
    state.button[ButtonUp]       = (byte == 0x4);
    state.button[ButtonLeft]     = (byte == 0x6);
    state.dirty = true;
    // handle changes to button state
    for (int button = 0; button < ButtonCount; button++) {
      if (state.button[button] != oldState.button[button]) {
        state = onButton(state, (ButtonIndex)button, sample);
      }
    }
  }
  // report unhandled sysex events
  else {
    DBG(String::formatted("UNHANDLED SYSEX: %s", String::toHexString(data, dataSize)));
  }
  return(state);
}

// send note events to reflect a change in state and return the new state
GuitarState StanginAudioProcessor::sendNotes(GuitarState oldState, GuitarState newState, MidiBuffer &output) {
  int i, channel, note;
  // handle changes to string state
  for (i = 0; i < 6; i++) {
    StringState &oldString = oldState.string[i];
    StringState &newString = newState.string[i];
    if (newString.sample < 0) continue;
    channel = i + 1;
    // update the string's note
    note = newString.openNote + newString.fret + newState.detune;
    // bounds check
    if ((note >= 0) && (note <= 127)) {
      newString.note = note;
      // stop the string's current note if it's playing
      if (oldString.samplesLeft > 0) {
        output.addEvent(MidiMessage::noteOff(channel, oldString.note, newString.velocity), 
                        newString.sample);
      }
      // start the string's new note
      if (newString.samplesLeft > 0) {
        output.addEvent(MidiMessage::noteOn(channel, newString.note, newString.velocity), 
                        newString.sample);
        if (newString.note != oldString.note) newString.age = 0;
      }
    }
    newString.sample = -1;
  }
  newState.dirty = false;
  return(newState);
}

// update the guitar state and send events to reflect the passing of time
GuitarState StanginAudioProcessor::ageGuitarState(GuitarState state, int startSample, int endSample, MidiBuffer &output) {
  int i, channel;
  // get the time elapsed since the last event
  int elapsed = endSample - startSample;
  if (elapsed < 0) elapsed = 0;
  // age strings
  for (i = 0; i < 6; i++) {
    StringState &string = state.string[i];
    string.age += elapsed;
    if (string.samplesLeft <= 0) continue;
    if (string.samplesLeft <= elapsed) {
      if (string.note >= 0) {
        channel = i + 1;
        output.addEvent(MidiMessage::noteOff(channel, string.note, string.velocity), 
                        startSample + string.samplesLeft);
      }
      string.samplesLeft = 0;
    }
    else {
      string.samplesLeft -= elapsed;
    }
  }
  // age button presses
  timePressingButton += elapsed;
  int buttonRate = (int)(getSampleRate() * 0.05f);
  if (timePressingButton >= buttonRate) {
    if ((state.button[ButtonTriangle]) && (state.sustain > minSustain)) {
      state.sustain -= sustainIncrement;
      if (state.sustain < minSustain) state.sustain = minSustain;
    }
    else if (state.button[ButtonX]) {
      if (state.sustain <= minSustain) state.sustain = 0.0f;
      state.sustain += sustainIncrement;
    }
    timePressingButton = 0;
  }
  return(state);
}

GuitarState StanginAudioProcessor::onButton(GuitarState oldState, ButtonIndex button, int sample) {
  int i;
  GuitarState newState = oldState;
  bool pressed = newState.button[button];
  // require the button to be held a bit before it starts repeating
  if (pressed) timePressingButton = - (int)(0.1f * getSampleRate());
  switch (button) {
    case ButtonSquare:
      break;
    case ButtonX:
      if (pressed) {
        if (newState.sustain <= minSustain) newState.sustain = 0.0f;
        newState.sustain += sustainIncrement;
      }
      break;
    case ButtonCircle:
      break;
    case ButtonTriangle:
      if ((pressed) && (newState.sustain > minSustain)) {
        newState.sustain -= sustainIncrement;
        if (newState.sustain < minSustain) newState.sustain = minSustain;
      }
      break;
    case ButtonSelect:
      if (pressed) newState.detune = 0;
      break;
    case ButtonStart:
      break;
    case ButtonConsole:
      // damp all strings
      if (pressed) {
        for (i = 0; i < 6; i++) {
          newState.string[i].samplesLeft = 0;
          newState.string[i].sample = sample;
        }
      }
      newState.dirty = true;
      break;
    case ButtonShake:
      break;
    case ButtonDown:
      if (pressed) newState.detune -= 1;
      break;
    case ButtonRight:
      if (pressed) newState.detune += 12;
      break;
    case ButtonUp:
      if (pressed) newState.detune += 1;
      break;
    case ButtonLeft:
      if (pressed) newState.detune -= 12;
      break;
  }
  // adjust detune
  if (newState.detune != oldState.detune) {
    for (i = 0; i < 6; i++) {
      if (newState.string[i].samplesLeft > 0) {
        newState.string[i].sample = sample;
      }
    }
  }
  return(newState);
}

// STATE **********************************************************************

void StanginAudioProcessor::getStateInformation (MemoryBlock& destData) {
  destData.replaceWith((void *)(&guitar), sizeof(GuitarState));
}

void StanginAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
  if (sizeInBytes == sizeof(GuitarState)) {
    guitar = *((GuitarState *)data);
  }
}

// SETUP ***********************************************************************

const String StanginAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool StanginAudioProcessor::acceptsMidi() const { return true; }

bool StanginAudioProcessor::producesMidi() const { return true; }

double StanginAudioProcessor::getTailLengthSeconds() const { return(0.0); }

int StanginAudioProcessor::getNumPrograms() {
  return(1);
}

int StanginAudioProcessor::getCurrentProgram() {
  return(0);
}

void StanginAudioProcessor::setCurrentProgram (int index) {
}

const String StanginAudioProcessor::getProgramName (int index) {
  return String();
}

void StanginAudioProcessor::changeProgramName (int index, const String& newName) {
}

bool StanginAudioProcessor::setPreferredBusArrangement (bool isInput, int bus, const AudioChannelSet& preferredSet) {
  const int numChannels = preferredSet.size();
  if (numChannels != 0) return false;
  return AudioProcessor::setPreferredBusArrangement (isInput, bus, preferredSet);
}

bool StanginAudioProcessor::hasEditor() const { return true; }

AudioProcessorEditor* StanginAudioProcessor::createEditor() {
    return new StanginAudioProcessorEditor (*this);
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new StanginAudioProcessor();
}
