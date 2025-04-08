#ifndef NOTECONVERTER_H
#define NOTECONVERTER_H

#include <string>
#include <vector>

// Structure to hold the results of a conversion
struct NoteInfo {
    std::string name = "---";
    int octave = 0;         
    float frequency = 0.0f;   // Original frequency detected
    float cents = 0.0f;       // Deviation from the nearest perfect pitch in cents
    int midiNoteNumber = -1;  // MIDI note number (0-127, -1 if invalid)
    bool isValid = false; 
};

class NoteConverter {
public:
    // Constructor: default reference pitch 440HZ
    explicit NoteConverter(float referenceA4 = 440.0f);

    // Prevent copying and assignment
    NoteConverter(const NoteConverter&) = delete;
    NoteConverter& operator=(const NoteConverter&) = delete;

    // Convert a frequency (Hz) into full NoteInfo
    NoteInfo getNoteInfo(float frequencyHz) const;

private:
    float referenceA4_Hz_;

    // Store the array of note names within an octave
    static const std::vector<std::string> noteNames_;
};

#endif