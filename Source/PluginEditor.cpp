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
  int textLeft = x + w; // right edge of text on the left
  int textRight = stringArea.getRight() - w; // left edge of text on the right
  int stringMargin = (em / 2); // space between text and strings
  int stringLeft = textLeft + stringMargin; // left edge of the string line
  int stringRight = textRight - stringMargin; // right edge of the string line
  // draw strings
  for (i = 0; i < 6; i++) {
    StringState &string = processor.guitar.string[i];
    // draw the tuning on the left side
    int openNote = string.openNote + processor.guitar.detune;
    String openNoteName = noteName(openNote, true);
    g.setColour(fg);
    g.setFont(font);
    g.drawFittedText(openNoteName, 
      x, y, w, stringHeight, 
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
