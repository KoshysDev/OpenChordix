#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "NoteConverter.h"

TEST_CASE("NoteConverter maps A4 correctly", "[note]")
{
    NoteConverter converter(440.0f);
    NoteInfo info = converter.getNoteInfo(440.0f);

    REQUIRE(info.isValid);
    REQUIRE(info.name == "A");
    REQUIRE(info.octave == 4);
    REQUIRE(info.midiNoteNumber == 69);
    REQUIRE(info.cents == Catch::Approx(0.0f).margin(0.01f));
}

TEST_CASE("NoteConverter maps octaves and rejects low frequencies", "[note]")
{
    NoteConverter converter(440.0f);

    NoteInfo a5 = converter.getNoteInfo(880.0f);
    REQUIRE(a5.isValid);
    REQUIRE(a5.name == "A");
    REQUIRE(a5.octave == 5);
    REQUIRE(a5.midiNoteNumber == 81);

    NoteInfo low = converter.getNoteInfo(5.0f);
    REQUIRE_FALSE(low.isValid);
    REQUIRE(low.midiNoteNumber == -1);
}

TEST_CASE("NoteConverter respects custom reference pitch", "[note]")
{
    NoteConverter converter(432.0f);
    NoteInfo info = converter.getNoteInfo(432.0f);

    REQUIRE(info.isValid);
    REQUIRE(info.name == "A");
    REQUIRE(info.octave == 4);
    REQUIRE(info.midiNoteNumber == 69);
    REQUIRE(info.cents == Catch::Approx(0.0f).margin(0.01f));
}
