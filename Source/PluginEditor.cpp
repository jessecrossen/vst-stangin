#include "PluginProcessor.h"
#include "PluginEditor.h"

StanginAudioProcessorEditor::StanginAudioProcessorEditor (StanginAudioProcessor& p)
                            : AudioProcessorEditor (&p), processor (p) {
  // configure sizes
  em = 18;
  setSize(21 * em, 18 * em);
  // configure graphics
  font = Font((float)em, Font::FontStyleFlags::bold);
  smallFont = font.withHeight((float)((em * 2) / 3));
  bg = Colour::greyLevel(0.1);
  fg = Colour::greyLevel(1.0);
  accent = Colour::fromHSV(0.0, 1.0, 0.75, 1.0);
  // set up the menu of tunings
  initTuningMenu();
  // start updating the display
  startTimerHz(20);
}

StanginAudioProcessorEditor::~StanginAudioProcessorEditor() {
  stopTimer();
}

void StanginAudioProcessorEditor::timerCallback() {
  repaint();
}

void StanginAudioProcessorEditor::paint (Graphics& g) {
  g.fillAll(bg);
  // draw strings
  drawStrings(g);
  // draw the menu button for tunings
  drawTuningMenu(g);
  // draw sliders
  int detune = processor.guitar.detune;
  String detuneText = String::formatted("DETUNE: %s%d", 
                                        detune > 0 ? "+" : "", detune);
  drawSlider(g, detuneArea, detuneText, getDetuneFraction(), detuneActive);
  String sustainText = String::formatted("SUSTAIN: %.01f", processor.guitar.sustain);
  drawSlider(g, sustainArea, sustainText, getSustainFraction(), sustainActive);
  // draw buttons
  drawButton(g, hammeronArea, String("HAMMER ON"), processor.guitar.hammeron);
  drawButton(g, pulloffArea, String("PULL OFF"), processor.guitar.pulloff);
  drawButton(g, dampOpenArea, String("DAMP OPEN"), processor.guitar.dampOpen);
  drawButton(g, tapArea, String("TAP"), processor.guitar.tap);
}

void StanginAudioProcessorEditor::resized() {
  // set the spacing to use between segments
  int spacing = em;
  // leave a margin around the edge
  Rectangle<int> area = getLocalBounds().reduced(em);
  // position the strings at the top
  stringArea = area.withHeight(em * 9);
  area = area.withTrimmedTop(stringArea.getHeight() + spacing);
  // position the sliders
  int sliderHeight = (em * 3) / 2;
  detuneArea = area.withHeight(sliderHeight);
  area = area.withTrimmedTop(sliderHeight + (spacing / 2));
  sustainArea = area.withHeight(sliderHeight);
  area = area.withTrimmedTop(sliderHeight + spacing);
  // position the buttons
  area = area.withTrimmedTop(spacing / 2);
  int buttonSpacing = spacing / 2;
  area = area.withHeight(em);
  hammeronArea = area.withWidth(em * 5);
  area = area.withTrimmedLeft(hammeronArea.getWidth() + buttonSpacing);
  pulloffArea = area.withWidth(em * 4);
  area = area.withTrimmedLeft(pulloffArea.getWidth() + buttonSpacing);
  dampOpenArea = area.withWidth(em * 5);
  area = area.withTrimmedLeft(dampOpenArea.getWidth() + buttonSpacing);
  tapArea = area.withWidth(em * 3);
  area = area.withTrimmedLeft(tapArea.getWidth() + buttonSpacing);
}

// draw the strings display
void StanginAudioProcessorEditor::drawStrings(Graphics &g) {
  int i;
  // find the maximum width of a note name
  int w = 0;
  for (i = 0; i < 6; i++) {
    int openNote = processor.guitar.string[i].openNote + 
                   processor.guitar.detune;
    int tw = font.getStringWidth(noteName(openNote, true));
    if (tw > w) w = tw;
  }
  // precompute some coordinates
  int stringHeight = stringArea.getHeight() / 6;
  int x = stringArea.getX();
  int y = stringArea.getY();
  int mw = font.getStringWidth(String("-"));
  int pw = font.getStringWidth(String("+"));
  int mps = em / 6;
  int textLeft = x + w; // right edge of text on the left
  int textRight = stringArea.getRight() - w; // left edge of text on the right
  int stringMargin = (em / 2); // space between text and strings
  int stringLeft = textLeft + pw + stringMargin; // left edge of the string line
  int stringRight = textRight - stringMargin; // right edge of the string line
  // update the area of the tuning display
  tuningArea = Rectangle<int>(0, y, stringLeft, stringArea.getHeight());
  tuningMenuArea = Rectangle<int>(0, 0, stringLeft, y);
  // draw strings
  for (i = 0; i < 6; i++) {
    StringState &string = processor.guitar.string[i];
    // draw the tuning on the left side
    int openNote = string.openNote + processor.guitar.detune;
    String openNoteName = noteName(openNote, true);
    g.setColour(fg);
    g.setFont(font);
    g.drawFittedText(openNoteName, 
      x, y, w, stringHeight, Justification::centred, 1);
    // draw +/- symbols to show notes can be clicked to change them
    g.setColour(bg.interpolatedWith(fg, 0.25f));
    g.drawFittedText(String("-"), 
      x - mw - mps, y, mw, stringHeight, 
      Justification::verticallyCentred | Justification::left, 1);
    g.drawFittedText(String("+"), 
      x + w + mps, y, pw, stringHeight, 
      Justification::verticallyCentred | Justification::right, 1);
    // get the fraction of the string's playback time that has elapsed
    float life = 0.0f;
    if (string.samplesSustain > 0) {
      life = ((double)string.samplesLeft / (double)(string.samplesSustain));
    }
    // draw the string with a corresponding color
    g.setColour(bg.interpolatedWith(fg, 0.25f + (life * 0.75f)));
    String fretNumber = String(string.fret);
    int fretTextWidth = font.getStringWidth(fretNumber);
    int fretCenter = (stringLeft + stringRight) / 2;
    int fretLeft = fretCenter - ((fretTextWidth / 2) + 3);
    int fretRight = fretCenter + ((fretTextWidth / 2) + 3);
    int stringY = y + (stringHeight / 2);
    g.fillRect(stringLeft, stringY - 1, fretLeft - stringLeft, 3);
    g.fillRect(fretRight, stringY - 1, stringRight - fretRight, 3);
    g.drawFittedText(fretNumber, 
      fretLeft, y, fretRight - fretLeft, stringHeight, 
      Justification::centred, 1);
    // draw the fretted note on the right side
    int frettedNote = openNote + string.fret;
    String frettedNoteName = noteName(frettedNote, false);
    g.drawFittedText(frettedNoteName, 
      textRight, y, w, stringHeight, 
      Justification::verticallyCentred | Justification::left, 1);
    // advance to the next string
    y += stringHeight;
  }
}

void StanginAudioProcessorEditor::drawTuningMenu(Graphics &g) {
  Path p;
  Rectangle<float>r = tuningMenuArea.toFloat();
  float hw = (float)em / 4.0f;
  float h = (float)em / 3.0f;
  float x = r.getCentreX();
  float y = r.getBottom() - ((float)em / 6.0);
  p.startNewSubPath(x - hw, y - h);
  p.lineTo(x + hw, y - h);
  p.lineTo(x, y);
  p.closeSubPath();
  g.setColour(bg.interpolatedWith(fg, 0.25f));
  g.fillPath(p);
}

// draw a slider
void StanginAudioProcessorEditor::drawSlider(Graphics &g, Rectangle<int> area, 
                                             String text, float value, 
                                             bool active) {
  int thumbRadius = em / 3;
  int thumbDiameter = thumbRadius * 2;
  // clamp the value
  if (value < 0.0f) value = 0.0f;
  if (value > 1.0f) value = 1.0f;
  // make a muted color
  Colour muted = bg.interpolatedWith(fg, 0.33f);
  // draw the text
  g.setColour(muted);
  g.setFont(smallFont);
  int textHeight = (int)smallFont.getHeight();
  g.drawFittedText(text, area, 
      Justification::left | Justification::top, 1);
  // draw the slider bar
  Rectangle<int> sliderArea = area.withHeight(area.getHeight() - textHeight);
  sliderArea.setY(sliderArea.getY() + textHeight);
  int sliderY = sliderArea.getBottom() - (thumbRadius + 2);
  g.fillRect(area.getX(), sliderY - 1, sliderArea.getWidth(), 3);
  // draw the slider thumb
  int sliderX = area.getX() + thumbRadius +
    (int)((float)(area.getWidth() - thumbDiameter) * value);
  g.setColour(bg);
  g.fillEllipse(sliderX - (thumbRadius + 2), sliderY - (thumbRadius + 2), 
                thumbDiameter + 4, thumbDiameter + 4);
  g.setColour(fg);
  g.fillEllipse(sliderX - thumbRadius, sliderY - thumbRadius, 
                thumbDiameter, thumbDiameter);
  g.setColour(active ? accent : bg);
  g.fillEllipse(sliderX - (thumbRadius - 3), sliderY - (thumbRadius - 3), 
                thumbDiameter - 6, thumbDiameter - 6);
}

// draw a button
void StanginAudioProcessorEditor::drawButton(Graphics &g, Rectangle<int> area, String text, bool on) {
  g.setColour(on ? accent : bg.interpolatedWith(fg, 0.1));
  g.fillRect(area);
  g.setColour(fg);
  g.setFont(smallFont);
  g.drawFittedText(text, area, Justification::centred, 1);
}

// reposition sliders on mouse down
void StanginAudioProcessorEditor::mouseDown(const MouseEvent &event) {
  Point<int> position = event.position.toInt();
  if (sustainArea.contains(position)) {
    setSustainFraction(getSliderFraction(event, sustainArea));
    sustainActive = true;
  }
  else if (detuneArea.contains(position)) {
    setDetuneFraction(getSliderFraction(event, detuneArea));
    detuneActive = true;
  }
}
void StanginAudioProcessorEditor::mouseUp(const MouseEvent &event) {
  sustainActive = false;
  detuneActive = false;
  // toggle buttons on click
  Point<int> position = event.getMouseDownPosition().toInt();
  if (hammeronArea.contains(position)) {
    processor.guitar.hammeron = ! processor.guitar.hammeron;
  }
  else if (pulloffArea.contains(position)) {
    processor.guitar.pulloff = ! processor.guitar.pulloff;
  }
  else if (dampOpenArea.contains(position)) {
    processor.guitar.dampOpen = ! processor.guitar.dampOpen;
  }
  else if (tapArea.contains(position)) {
    processor.guitar.tap = ! processor.guitar.tap;
  }
  // update string tuning on click
  else if (tuningArea.contains(position)) {
    int stringHeight = tuningArea.getHeight() / 6;
    int i = (position.y - tuningArea.getY()) / stringHeight;
    if (i < 0) i = 0;
    if (i > 5) i = 5;
    StringState &string = processor.guitar.string[i];
    if (position.x >= tuningArea.getCentreX()) {
      string.openNote += 1;
    }
    else {
      string.openNote -= 1;
    }
  }
  // show the list of tunings on click
  else if (tuningMenuArea.contains(position)) {
    LookAndFeel_V3 look;
    look.setColour(PopupMenu::ColourIds::backgroundColourId, bg);
    look.setColour(PopupMenu::ColourIds::textColourId, fg);
    look.setColour(PopupMenu::ColourIds::highlightedBackgroundColourId, accent);
    look.setColour(PopupMenu::ColourIds::highlightedTextColourId, fg);
    tuningMenu.setLookAndFeel(&look);
    int idx = tuningMenu.showAt(localAreaToGlobal(tuningMenuArea)) - 1;
    if ((idx >= 0) && (idx < tunings.size())) {
      for (int i = 0; i < 6; i++) {
        processor.guitar.string[i].openNote = tunings[idx].string[i];
      }
    }
  }
}
// drag sliders
void StanginAudioProcessorEditor::mouseDrag(const MouseEvent &event) {
  Point<int> position = event.getMouseDownPosition().toInt();
  if (sustainArea.contains(position)) {
    setSustainFraction(getSliderFraction(event, sustainArea));
  }
  else if (detuneArea.contains(position)) {
    setDetuneFraction(getSliderFraction(event, detuneArea));
  }
}
// reset on double-click of sliders
void StanginAudioProcessorEditor::mouseDoubleClick(const MouseEvent &event) {
  Point<int> position = event.getMouseDownPosition().toInt();
  if (sustainArea.contains(position)) {
    setSustainFraction(0.1);
  }
  else if (detuneArea.contains(position)) {
    setDetuneFraction(0.5);
  }
}

float StanginAudioProcessorEditor::getSliderFraction(const MouseEvent &event, const Rectangle<int> area) {
  return((float)(event.x - area.getX()) / (float)area.getWidth());
}

// update sustain
void StanginAudioProcessorEditor::setSustainFraction(float f) {
  if (f < 0.0f) f = 0.0f;
  if (f > 1.0f) f = 1.0f;
  processor.guitar.sustain = processor.minSustain + 
    (f * (processor.maxSustain - processor.minSustain));
}
float StanginAudioProcessorEditor::getSustainFraction() {
  return((processor.guitar.sustain - processor.minSustain) / 
         (processor.maxSustain - processor.minSustain));
}

// update detune
void StanginAudioProcessorEditor::setDetuneFraction(float f) {
  if (f < 0.0f) f = 0.0f;
  if (f > 1.0f) f = 1.0f;
  processor.guitar.detune = (int)(f * 120.0f) - 60;
}
float StanginAudioProcessorEditor::getDetuneFraction() {
  return((float)(processor.guitar.detune + 60) / 120.0f);
}

// get a string naming a MIDI note number
String StanginAudioProcessorEditor::noteName(int note, bool withOctave) {
  if ((note < 0) || (note > 127)) return(String("-"));
  String octave("");
  if (withOctave) {
    octave = String::formatted("%d", (note / 12) + 1);
    if (note < 12) octave = String("-");
  }
  switch (note % 12) {
    case 0:  return(String("C") + octave);
    case 1:  return(String("C#") + octave);
    case 2:  return(String("D") + octave);
    case 3:  return(String("Eb") + octave);
    case 4:  return(String("E") + octave);
    case 5:  return(String("F") + octave);
    case 6:  return(String("F#") + octave);
    case 7:  return(String("G") + octave);
    case 8:  return(String("Ab") + octave);
    case 9:  return(String("A") + octave);
    case 10: return(String("Bb") + octave);
    case 11: return(String("B") + octave);
    default: return(String("?"));
  }
}

// initialize the tuning menu
void StanginAudioProcessorEditor::initTuningMenu() {
  addTuning("Standard",      "E",  "A",  "D",  "G",  "B",  "E");
  addTuning("Dad-Gad",  "D",  "A",  "D",  "G",  "A",  "D");
  addTuning("Dad-Dad",  "D",  "A",  "D",  "D",  "A",  "D");
  tuningMenu.addSeparator();
  addTuning("Open A",        "E",  "A",  "C#", "E",  "A",  "E");
  addTuning("Open B",        "B",  "F#", "B",  "F#", "B",  "D#");
  addTuning("Open C",        "C",  "G",  "C",  "G",  "C",  "E");
  addTuning("Open D",        "D",  "A",  "D",  "F#", "A",  "D");
  addTuning("Open E",        "E",  "B",  "E",  "G#", "B",  "E");
  addTuning("Open F",        "F",  "A",  "C",  "F",  "C",  "F");
  addTuning("Open G",        "D",  "G",  "D",  "G",  "B",  "D");
  tuningMenu.addSeparator();
  addTuning("Cross-note A",  "E",  "A",  "E",  "A",  "C",  "E");
  addTuning("Cross-note C",  "C",  "G",  "C",  "G",  "C",  "E");
  addTuning("Cross-note D",  "D",  "A",  "D",  "F",  "A",  "D");
  addTuning("Cross-note E",  "E",  "B",  "E",  "G",  "B",  "E");
  addTuning("Cross-note G",  "D",  "G",  "D",  "G",  "Bb", "D");
  tuningMenu.addSeparator();
  addTuning("Major Thirds",  "E",  "G#", "C",  "E",  "G#", "C");
  addTuning("All Fourths",   "E",  "A",  "D",  "G",  "C",  "F");
  addTuning("Aug Fourths",   "C",  "F#", "C",  "F#", "C",  "F#");
  addTuning("All Fifths",    "C",  "G",  "D",  "A",  "E",  "G");
  tuningMenu.addSeparator();
  addTuning("Drop D",        "D",  "A",  "D",  "G",  "B",  "E");
  addTuning("Double Drop D", "D",  "A",  "D",  "G",  "B",  "D");  
}

void StanginAudioProcessorEditor::addTuning(const char *name, 
                              const char *s0, const char *s1, const char *s2, 
                              const char *s3, const char *s4, const char *s5) {
  int i = tunings.size();
  Tuning newTuning;
  newTuning.string[5] = nextNote(35, s0);
  newTuning.string[4] = nextNote(newTuning.string[5], s1);
  newTuning.string[3] = nextNote(newTuning.string[4], s2);
  newTuning.string[2] = nextNote(newTuning.string[3], s3);
  newTuning.string[1] = nextNote(newTuning.string[2], s4);
  newTuning.string[0] = nextNote(newTuning.string[1], s5);
  tunings.add(newTuning);
  String label = String::formatted("%s-%s-%s-%s-%s-%s", s0, s1, s2, s3, s4, s5);
  String nameString(name);
  if (! nameString.isEmpty()) {
    label += String::formatted(" (%s)", name);
  }
  tuningMenu.addItem(i + 1, label);
}

uint8_t StanginAudioProcessorEditor::nextNote(uint8_t note, const char *pitchClass) {
  uint8_t pitchOffset = getOffsetForPitchClass(pitchClass);
  while (note % 12 != pitchOffset) note++;
  return(note);
}
uint8_t StanginAudioProcessorEditor::getOffsetForPitchClass(const char *pitchClass) {
  String s(pitchClass);
  if (s.equalsIgnoreCase("C")) return(0);
  if (s.equalsIgnoreCase("C#")) return(1);
  if (s.equalsIgnoreCase("Db")) return(1);
  if (s.equalsIgnoreCase("D")) return(2);
  if (s.equalsIgnoreCase("D#")) return(3);
  if (s.equalsIgnoreCase("Eb")) return(3);
  if (s.equalsIgnoreCase("E")) return(4);
  if (s.equalsIgnoreCase("F")) return(5);
  if (s.equalsIgnoreCase("F#")) return(6);
  if (s.equalsIgnoreCase("Gb")) return(6);
  if (s.equalsIgnoreCase("G")) return(7);
  if (s.equalsIgnoreCase("G#")) return(8);
  if (s.equalsIgnoreCase("Ab")) return(8);
  if (s.equalsIgnoreCase("A")) return(9);
  if (s.equalsIgnoreCase("A#")) return(10);
  if (s.equalsIgnoreCase("Bb")) return(10);
  if (s.equalsIgnoreCase("B")) return(11);
  return(0);
}
