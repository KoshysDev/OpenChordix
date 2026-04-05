#ifndef NOTECONVERTER_H
#define NOTECONVERTER_H

#include <string>
#include <vector>

struct NoteInfo {
    std::string name = "---";
    int octave = 0;
    float frequency = 0.0f;
    float cents = 0.0f;
    int midiNoteNumber = -1;
    bool isValid = false;
};

class NoteConverter {
public:
    explicit NoteConverter(float referenceA4 = 440.0f);

    NoteConverter(const NoteConverter&) = delete;
    NoteConverter& operator=(const NoteConverter&) = delete;

    NoteInfo getNoteInfo(float frequencyHz) const;

private:
    float referenceA4_Hz_;

    static const std::vector<std::string> noteNames_;
};

#endif
