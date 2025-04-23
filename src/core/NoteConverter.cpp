#include "NoteConverter.h"
#include <cmath>   
#include <limits>  

// Initialize the static vector of note names
const std::vector<std::string> NoteConverter::noteNames_ = {
    // Standard MIDI mapping starts note 0 as C, but it's easier to calculate
    // relative to A and then map. MIDI note numbers modulo 12 correspond to:
    // 0=C, 1=C#, 2=D, 3=D#, 4=E, 5=F, 6=F#, 7=G, 8=G#, 9=A, 10=A#, 11=B
    // So index 0 below is C, index 9 is A.
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

    // Basic validity check for frequency (very low frequencies = noise)
    if (frequencyHz <= 10.0f) { // Arbitrary low threshold
        info.isValid = false;
        info.midiNoteNumber = -1; // Indicate invalid MIDI note
        return info;
    }

    // --- Calculate Semitones from Reference ---
    // N = 12 * log2(f / f_ref)
    // This gives the number of semitones 'f' is away from A4.
    // Example: If f = 440Hz, N = 12 * log2(1) = 0
    // Example: If f = 880Hz, N = 12 * log2(2) = 12 (one octave up)
    // Example: If f = 220Hz, N = 12 * log2(0.5) = -12 (one octave down)
    double semitones_from_a4 = 12.0 * log2(frequencyHz / referenceA4_Hz_);

    // --- Calculate Nearest MIDI Note Number ---
    // MIDI standard defines A4 as note number 69.
    // Round to the nearest integer to get the closest standard MIDI note.
    int nearestMidiNote = static_cast<int>(round(semitones_from_a4)) + 69;

    // Check if the calculated MIDI note is within range (0-127)
    if (nearestMidiNote < 0 || nearestMidiNote > 127) {
        info.isValid = false;
        info.midiNoteNumber = -1;
        return info;
    }

    info.midiNoteNumber = nearestMidiNote; // Store MIDI note

    // Calculate Cents Deviation
    info.cents = static_cast<float>((semitones_from_a4 - round(semitones_from_a4)) * 100.0);


    // --- Determine Note Name and Octave ---
    // MIDI note number 0 is C-1
    // MIDI note number 60 is Middle C (C4).
    // MIDI note number 69 is A4.
    // Octave changes between B and C.
    info.octave = (nearestMidiNote / 12) - 1; // Standard octave notation where C4 is in octave 4
    int noteIndex = nearestMidiNote % 12;
    info.name = noteNames_[noteIndex];

    info.isValid = true;

    return info;
}