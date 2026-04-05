#include "NoteConverter.h"
#include <cmath>

const std::vector<std::string> NoteConverter::noteNames_ = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

NoteConverter::NoteConverter(float referenceA4)
    : referenceA4_Hz_(referenceA4)
{
    if (referenceA4_Hz_ <= 0) {
         referenceA4_Hz_ = 440.0f;
    }
}

NoteInfo NoteConverter::getNoteInfo(float frequencyHz) const {
    NoteInfo info;
    info.frequency = frequencyHz;

    if (frequencyHz <= 10.0f) {
        info.isValid = false;
        info.midiNoteNumber = -1;
        return info;
    }

    double semitones_from_a4 = 12.0 * log2(frequencyHz / referenceA4_Hz_);

    int nearestMidiNote = static_cast<int>(round(semitones_from_a4)) + 69;

    if (nearestMidiNote < 0 || nearestMidiNote > 127) {
        info.isValid = false;
        info.midiNoteNumber = -1;
        return info;
    }

    info.midiNoteNumber = nearestMidiNote;

    info.cents = static_cast<float>((semitones_from_a4 - round(semitones_from_a4)) * 100.0);
    info.octave = (nearestMidiNote / 12) - 1;
    int noteIndex = nearestMidiNote % 12;
    info.name = noteNames_[noteIndex];

    info.isValid = true;

    return info;
}
