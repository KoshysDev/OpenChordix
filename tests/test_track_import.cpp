#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "track/TrackChartDocument.h"
#include "track/import/GuitarProImporter.h"
#include "track/import/ImportTimingDebug.h"
#include "track/import/ImporterRegistry.h"
#include "track/import/MidiImporter.h"
#include "track/import/TrackImportApply.h"

namespace
{
    std::vector<std::uint8_t> simpleMidi()
    {
        return {
            'M', 'T', 'h', 'd', 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x60,
            'M', 'T', 'r', 'k', 0x00, 0x00, 0x00, 0x1E,
            0x00, 0xFF, 0x03, 0x04, 'L', 'e', 'a', 'd',
            0x00, 0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20,
            0x00, 0xC0, 0x1D,
            0x00, 0x90, 0x40, 0x64,
            0x60, 0x80, 0x40, 0x00,
            0x00, 0xFF, 0x2F, 0x00,
        };
    }

    void writeU8(std::vector<std::uint8_t> &bytes, std::uint8_t value)
    {
        bytes.push_back(value);
    }

    void writeI16(std::vector<std::uint8_t> &bytes, std::int16_t value)
    {
        bytes.push_back(static_cast<std::uint8_t>(value & 0xff));
        bytes.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
    }

    void writeI32(std::vector<std::uint8_t> &bytes, std::int32_t value)
    {
        bytes.push_back(static_cast<std::uint8_t>(value & 0xff));
        bytes.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
        bytes.push_back(static_cast<std::uint8_t>((value >> 16) & 0xff));
        bytes.push_back(static_cast<std::uint8_t>((value >> 24) & 0xff));
    }

    void writeU16(std::vector<std::uint8_t> &bytes, std::uint16_t value)
    {
        bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
        bytes.push_back(static_cast<std::uint8_t>((value >> 8) & 0xffU));
    }

    void writeU32(std::vector<std::uint8_t> &bytes, std::uint32_t value)
    {
        bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
        bytes.push_back(static_cast<std::uint8_t>((value >> 8) & 0xffU));
        bytes.push_back(static_cast<std::uint8_t>((value >> 16) & 0xffU));
        bytes.push_back(static_cast<std::uint8_t>((value >> 24) & 0xffU));
    }

    void writeZeros(std::vector<std::uint8_t> &bytes, std::size_t count)
    {
        bytes.insert(bytes.end(), count, 0);
    }

    void writeFixedString(std::vector<std::uint8_t> &bytes, const std::string &text, std::size_t count)
    {
        writeU8(bytes, static_cast<std::uint8_t>(text.size()));
        bytes.insert(bytes.end(), text.begin(), text.end());
        writeZeros(bytes, count - text.size());
    }

    void writeIntByteString(std::vector<std::uint8_t> &bytes, const std::string &text)
    {
        writeI32(bytes, static_cast<std::int32_t>(text.size() + 1));
        writeU8(bytes, static_cast<std::uint8_t>(text.size()));
        bytes.insert(bytes.end(), text.begin(), text.end());
    }

    void writeTrack(std::vector<std::uint8_t> &bytes, const std::string &name, int channel,
                    bool percussion, bool gp510 = false)
    {
        writeU8(bytes, 0);
        writeU8(bytes, percussion ? 0x01 : 0x00);
        writeFixedString(bytes, name, 40);
        writeI32(bytes, percussion ? 0 : 6);
        for (const int tuning : {64, 59, 55, 50, 45, 40, 0})
        {
            writeI32(bytes, tuning);
        }
        writeI32(bytes, 1);
        writeI32(bytes, channel);
        writeI32(bytes, channel);
        writeI32(bytes, 24);
        writeI32(bytes, 0);
        writeZeros(bytes, 4);
        writeI16(bytes, 1);
        writeZeros(bytes, 3);
        writeI32(bytes, 0);
        writeI32(bytes, 0);
        writeI32(bytes, -1);
        writeZeros(bytes, 12);
        writeI32(bytes, percussion ? 0 : 25);
        writeI32(bytes, 1);
        writeI32(bytes, 0);
        writeI16(bytes, 0);
        writeU8(bytes, 0);
        if (gp510)
        {
            writeZeros(bytes, 5);
            writeIntByteString(bytes, "");
            writeIntByteString(bytes, "");
        }
    }

    void writeTabMeasure(std::vector<std::uint8_t> &bytes)
    {
        writeI32(bytes, 3);

        writeU8(bytes, 0);
        writeU8(bytes, 0);
        writeU8(bytes, 0x60);
        writeU8(bytes, 0x68);
        writeU8(bytes, 1);
        writeU8(bytes, 3);
        writeU8(bytes, 0);
        writeU8(bytes, 0x0a);
        writeU8(bytes, 0x4b);
        writeU8(bytes, 0x01);
        writeU8(bytes, 0x20);
        writeU8(bytes, 1);
        writeU8(bytes, 2);
        writeU8(bytes, 0);
        writeI16(bytes, 0);

        writeU8(bytes, 0x40);
        writeU8(bytes, 2);
        writeU8(bytes, 0);
        writeU8(bytes, 0);
        writeI16(bytes, 0);

        writeU8(bytes, 0);
        writeU8(bytes, 0);
        writeU8(bytes, 0x40);
        writeU8(bytes, 0x20);
        writeU8(bytes, 1);
        writeU8(bytes, 5);
        writeU8(bytes, 0);
        writeI16(bytes, 0);

        writeI32(bytes, 0);
        writeU8(bytes, 0);
    }

    void writeEmptyMeasure(std::vector<std::uint8_t> &bytes)
    {
        writeI32(bytes, 0);
        writeI32(bytes, 0);
        writeU8(bytes, 0);
    }

    std::vector<std::uint8_t> simpleGp5(bool includeDrums = false, bool gp510 = false,
                                        bool emptyLead = false)
    {
        std::vector<std::uint8_t> bytes;
        writeFixedString(bytes, gp510 ? "FICHIER GUITAR PRO v5.10" : "FICHIER GUITAR PRO v5.00", 30);
        writeIntByteString(bytes, "Fixture Song");
        writeIntByteString(bytes, "");
        writeIntByteString(bytes, "Fixture Artist");
        writeIntByteString(bytes, "Fixture Album");
        for (int index = 0; index < 5; ++index)
        {
            writeIntByteString(bytes, "");
        }
        writeI32(bytes, 0);

        writeI32(bytes, 1);
        for (int index = 0; index < 5; ++index)
        {
            writeI32(bytes, 1);
            writeI32(bytes, 0);
        }

        for (int index = 0; index < 7; ++index)
        {
            writeI32(bytes, 0);
        }
        writeI16(bytes, 0);
        if (gp510)
        {
            writeZeros(bytes, 19);
        }
        for (int index = 0; index < 10; ++index)
        {
            writeIntByteString(bytes, "");
        }
        writeIntByteString(bytes, "");
        writeI32(bytes, 120);
        if (gp510)
        {
            writeU8(bytes, 0);
        }
        writeU8(bytes, 0);
        writeI32(bytes, 0);

        for (int index = 0; index < 64; ++index)
        {
            writeI32(bytes, index == 0 ? 25 : (index == 9 ? 0 : -1));
            writeZeros(bytes, 8);
        }
        writeZeros(bytes, 19 * 2);
        writeI32(bytes, 0);
        writeI32(bytes, 1);
        writeI32(bytes, includeDrums ? 2 : 1);

        writeU8(bytes, 0x03);
        writeU8(bytes, 4);
        writeU8(bytes, 4);
        writeZeros(bytes, 4);
        writeU8(bytes, 0);
        writeU8(bytes, 0);

        writeTrack(bytes, "Lead Guitar", 1, false, gp510);
        if (includeDrums)
        {
            writeTrack(bytes, "Drums", 10, true, gp510);
        }
        writeZeros(bytes, gp510 ? 1 : 2);
        if (emptyLead)
        {
            writeEmptyMeasure(bytes);
        }
        else
        {
            writeTabMeasure(bytes);
        }
        if (includeDrums)
        {
            writeEmptyMeasure(bytes);
        }
        return bytes;
    }

    struct ZipTextEntry
    {
        std::string name;
        std::string contents;
    };

    std::uint32_t crc32(std::string_view text)
    {
        std::uint32_t crc = 0xffffffffU;
        for (const unsigned char value : text)
        {
            crc ^= value;
            for (int bit = 0; bit < 8; ++bit)
            {
                crc = (crc >> 1) ^ ((crc & 1U) != 0U ? 0xedb88320U : 0U);
            }
        }
        return ~crc;
    }

    std::vector<std::uint8_t> storedZip(const std::vector<ZipTextEntry> &entries)
    {
        std::vector<std::uint8_t> bytes;
        std::vector<std::uint32_t> offsets;
        offsets.reserve(entries.size());
        for (const ZipTextEntry &entry : entries)
        {
            offsets.push_back(static_cast<std::uint32_t>(bytes.size()));
            const std::uint32_t checksum = crc32(entry.contents);
            writeU32(bytes, 0x04034b50U);
            writeU16(bytes, 20);
            writeU16(bytes, 0);
            writeU16(bytes, 0);
            writeU16(bytes, 0);
            writeU16(bytes, 0);
            writeU32(bytes, checksum);
            writeU32(bytes, static_cast<std::uint32_t>(entry.contents.size()));
            writeU32(bytes, static_cast<std::uint32_t>(entry.contents.size()));
            writeU16(bytes, static_cast<std::uint16_t>(entry.name.size()));
            writeU16(bytes, 0);
            bytes.insert(bytes.end(), entry.name.begin(), entry.name.end());
            bytes.insert(bytes.end(), entry.contents.begin(), entry.contents.end());
        }

        const std::uint32_t centralOffset = static_cast<std::uint32_t>(bytes.size());
        for (std::size_t index = 0; index < entries.size(); ++index)
        {
            const ZipTextEntry &entry = entries[index];
            const std::uint32_t checksum = crc32(entry.contents);
            writeU32(bytes, 0x02014b50U);
            writeU16(bytes, 20);
            writeU16(bytes, 20);
            writeU16(bytes, 0);
            writeU16(bytes, 0);
            writeU16(bytes, 0);
            writeU16(bytes, 0);
            writeU32(bytes, checksum);
            writeU32(bytes, static_cast<std::uint32_t>(entry.contents.size()));
            writeU32(bytes, static_cast<std::uint32_t>(entry.contents.size()));
            writeU16(bytes, static_cast<std::uint16_t>(entry.name.size()));
            writeU16(bytes, 0);
            writeU16(bytes, 0);
            writeU16(bytes, 0);
            writeU16(bytes, 0);
            writeU32(bytes, 0);
            writeU32(bytes, offsets[index]);
            bytes.insert(bytes.end(), entry.name.begin(), entry.name.end());
        }
        const std::uint32_t centralSize = static_cast<std::uint32_t>(bytes.size()) - centralOffset;
        writeU32(bytes, 0x06054b50U);
        writeU16(bytes, 0);
        writeU16(bytes, 0);
        writeU16(bytes, static_cast<std::uint16_t>(entries.size()));
        writeU16(bytes, static_cast<std::uint16_t>(entries.size()));
        writeU32(bytes, centralSize);
        writeU32(bytes, centralOffset);
        writeU16(bytes, 0);
        return bytes;
    }

    std::string simpleGpif(const std::string &version = "7")
    {
        return std::string(R"(<?xml version="1.0" encoding="utf-8"?>
<GPIF>
  <GPVersion>)") + version + R"(</GPVersion>
  <Score><Title>GPIF Fixture</Title><SubTitle>Readable package</SubTitle><Artist>Fixture Artist</Artist><Album>Fixture Album</Album></Score>
  <MasterTrack>
    <Tracks>0 1</Tracks>
    <Automations><Automation><Type>Tempo</Type><Bar>0</Bar><Position>0</Position><Value>132 2</Value></Automation></Automations>
  </MasterTrack>
  <Tracks>
    <Track id="0">
      <Name>Lead Guitar</Name><InstrumentSet><Name>Electric Guitar</Name></InstrumentSet>
      <Staves><Staff><Properties><Property name="Tuning"><Pitches>40 45 50 55 59 64</Pitches></Property></Properties></Staff></Staves>
      <GeneralMidi><Program>25</Program></GeneralMidi>
    </Track>
    <Track id="1"><Name>Drums</Name><GeneralMidi table="Percussion"><Program>0</Program></GeneralMidi></Track>
  </Tracks>
  <MasterBars><MasterBar><Time>4/4</Time><Bars>0 1</Bars></MasterBar></MasterBars>
  <Bars><Bar id="0"><Voices>0</Voices></Bar><Bar id="1"><Voices>1</Voices></Bar></Bars>
  <Voices><Voice id="0"><Beats>0 1 2</Beats></Voice><Voice id="1"><Beats>3</Beats></Voice></Voices>
  <Beats>
    <Beat id="0"><Rhythm ref="q"/><Notes>0 1</Notes><Tremolo>1/4</Tremolo></Beat>
    <Beat id="1"><Rhythm ref="q"/></Beat>
    <Beat id="2"><Rhythm ref="q"/><Notes>2</Notes></Beat>
    <Beat id="3"><Rhythm ref="q"/></Beat>
  </Beats>
  <Notes>
    <Note id="0"><LetRing/><Vibrato>Slight</Vibrato><Accent>12</Accent><Properties>
      <Property name="String"><String>5</String></Property><Property name="Fret"><Fret>3</Fret></Property>
      <Property name="PalmMuted"><Enable/></Property><Property name="Bended"><Enable/></Property>
      <Property name="HopoOrigin"><Enable/></Property><Property name="Slide"><Flags>1</Flags></Property>
      <Property name="HarmonicType"><HType>Natural</HType></Property>
    </Properties></Note>
    <Note id="1"><Properties><Property name="String"><String>4</String></Property><Property name="Fret"><Fret>2</Fret></Property></Properties></Note>
    <Note id="2"><Properties><Property name="String"><String>5</String></Property><Property name="Fret"><Fret>5</Fret></Property></Properties></Note>
  </Notes>
  <Rhythms><Rhythm id="q"><NoteValue>Quarter</NoteValue></Rhythm></Rhythms>
</GPIF>)";
    }

    std::string timedGpif(const std::string &masterTrackExtra,
                          const std::string &masterBars,
                          const std::string &bars,
                          const std::string &voices,
                          const std::string &beats,
                          const std::string &notes,
                          const std::string &rhythms)
    {
        return std::string(R"(<GPIF><GPVersion>7</GPVersion><Score><Title>Timing Fixture</Title></Score>
<MasterTrack>)") + masterTrackExtra + R"(<Tracks>0</Tracks><Automations><Automation><Type>Tempo</Type><Bar>0</Bar><Position>0</Position><Value>120 2</Value></Automation></Automations></MasterTrack>
<Tracks><Track id="0"><Name>Lead Guitar</Name><InstrumentSet><Name>Electric Guitar</Name></InstrumentSet>
<Staves><Staff><Properties><Property name="Tuning"><Pitches>40 45 50 55 59 64</Pitches></Property></Properties></Staff></Staves>
<GeneralMidi><Program>25</Program></GeneralMidi></Track></Tracks>
<MasterBars>)" + masterBars + "</MasterBars><Bars>" + bars + "</Bars><Voices>" + voices +
               "</Voices><Beats>" + beats + "</Beats><Notes>" + notes + "</Notes><Rhythms>" +
               rhythms + "</Rhythms></GPIF>";
    }

    std::string tabNote(const std::string &id, int stringIndex, int fret)
    {
        return "<Note id=\"" + id + "\"><Properties><Property name=\"String\"><String>" +
               std::to_string(stringIndex) + "</String></Property><Property name=\"Fret\"><Fret>" +
               std::to_string(fret) + "</Fret></Property></Properties></Note>";
    }

    std::vector<std::uint8_t> identifiedGuitarPro(const std::string &header)
    {
        std::vector<std::uint8_t> bytes;
        writeFixedString(bytes, header, 30);
        return bytes;
    }

    std::filesystem::path writeTemporaryFixture(const std::string &name,
                                                const std::vector<std::uint8_t> &bytes)
    {
        const std::filesystem::path path = std::filesystem::temp_directory_path() / name;
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        stream.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        return path;
    }
}

TEST_CASE("Importer registry detects MIDI and Guitar Pro chart sources", "[track][import]")
{
    using namespace openchordix::track::imports;

    REQUIRE(ImporterRegistry::formatForPath("lead.mid").has_value());
    CHECK(*ImporterRegistry::formatForPath("lead.MIDI") == ImportFormat::Midi);
    CHECK(*ImporterRegistry::formatForPath("arrangement.gp3") == ImportFormat::GuitarPro);
    CHECK(*ImporterRegistry::formatForPath("arrangement.gp4") == ImportFormat::GuitarPro);
    CHECK(*ImporterRegistry::formatForPath("arrangement.gp5") == ImportFormat::GuitarPro);
    CHECK(*ImporterRegistry::formatForPath("arrangement.gpx") == ImportFormat::GuitarPro);
    CHECK(*ImporterRegistry::formatForPath("arrangement.gp") == ImportFormat::GuitarPro);
}

TEST_CASE("Importer registry reports unsupported extensions cleanly", "[track][import]")
{
    openchordix::track::imports::ImporterRegistry registry;
    const auto result = registry.importFile("arrangement.txt");

    REQUIRE_FALSE(result);
    CHECK(result.error().code == openchordix::track::imports::ImportErrorCode::UnsupportedFormat);
    CHECK_FALSE(result.error().message.empty());
}

TEST_CASE("Guitar Pro 5 importer reads tablature timing chords rests and techniques", "[track][import]")
{
    using namespace openchordix::track::imports;

    const auto result = GuitarProImporter().importBytes(simpleGp5(), "fixture.gp5");
    REQUIRE(result);
    const ImportedSong &song = result.value();
    CHECK(song.detectedFormat == "Guitar Pro 5.00");
    CHECK(song.title == "Fixture Song");
    CHECK(song.artist == "Fixture Artist");
    CHECK(song.album == "Fixture Album");
    CHECK(song.ticksPerBeat == 960);
    REQUIRE(song.measures.size() == 1);
    CHECK(song.measures.front().durationTicks == 3840);
    REQUIRE(song.parts.size() == 1);
    const ImportedPart &part = song.parts.front();
    CHECK(part.type == ImportedPartType::Guitar);
    CHECK(part.suggestedStringCount == 6);
    REQUIRE(part.suggestedTuning.size() == 6);
    CHECK(part.suggestedTuning.front() == "E4");
    REQUIRE(part.notes.size() == 3);
    CHECK(part.notes[0].tick == 0);
    CHECK(part.notes[1].tick == 0);
    CHECK(part.notes[0].duration == 960);
    CHECK(*part.notes[0].stringIndex == 0);
    CHECK(*part.notes[0].fret == 3);
    CHECK(*part.notes[1].stringIndex == 1);
    CHECK(*part.notes[1].fret == 2);
    CHECK(part.notes[2].tick == 1920);
    CHECK(*part.notes[2].fret == 5);
    CHECK(part.notes[0].technique.hammerOn);
    CHECK(part.notes[0].technique.letRing);
    CHECK(part.notes[0].technique.palmMute);
    CHECK(part.notes[0].technique.vibrato);
    CHECK(part.notes[0].technique.slideType == "slide");
}

TEST_CASE("Importer registry routes GP5 while unsupported Guitar Pro variants report clearly", "[track][import]")
{
    using namespace openchordix::track::imports;

    const std::filesystem::path path = writeTemporaryFixture("openchordix_fixture.gp5", simpleGp5());
    const auto gp5 = ImporterRegistry().importFile(path);
    std::filesystem::remove(path);
    REQUIRE(gp5);
    CHECK(gp5.value().format == ImportFormat::GuitarPro);

    const std::filesystem::path genericGpPath = writeTemporaryFixture("openchordix_fixture.gp", simpleGp5());
    const auto genericGp = ImporterRegistry().importFile(genericGpPath);
    std::filesystem::remove(genericGpPath);
    REQUIRE(genericGp);
    CHECK(genericGp.value().detectedFormat == "Guitar Pro 5.00");

    const std::filesystem::path gp3Path =
        writeTemporaryFixture("openchordix_unsupported.gp3", identifiedGuitarPro("FICHIER GUITAR PRO v3.00"));
    const auto gp3 = ImporterRegistry().importFile(gp3Path);
    std::filesystem::remove(gp3Path);
    REQUIRE_FALSE(gp3);
    CHECK(gp3.error().code == ImportErrorCode::UnsupportedFeature);
    CHECK(gp3.error().detectedFormat == "Guitar Pro 3");
    const std::filesystem::path gp4Path =
        writeTemporaryFixture("openchordix_unsupported.gp4", identifiedGuitarPro("FICHIER GUITAR PRO v4.00"));
    const auto gp4 = ImporterRegistry().importFile(gp4Path);
    std::filesystem::remove(gp4Path);
    REQUIRE_FALSE(gp4);
    CHECK(gp4.error().code == ImportErrorCode::UnsupportedFeature);
    const std::filesystem::path gpxPath =
        writeTemporaryFixture("openchordix_unsupported.gpx", {'B', 'C', 'F', 'Z'});
    const auto gpx = ImporterRegistry().importFile(gpxPath);
    std::filesystem::remove(gpxPath);
    REQUIRE_FALSE(gpx);
    CHECK(gpx.error().code == ImportErrorCode::UnsupportedFeature);
    CHECK(gpx.error().message.find("GPX") != std::string::npos);
    CHECK(gpx.error().detectedFormat == "Guitar Pro 6 (GPX)");

}

TEST_CASE("Guitar Pro parser accepts GP5.10 content through the normalized DTO adapter", "[track][import]")
{
    using namespace openchordix::track::imports;

    const auto result = GuitarProImporter().importBytes(simpleGp5(false, true), "fixture.gp5");
    REQUIRE(result);
    CHECK(result.value().detectedFormat == "Guitar Pro 5.10");
    REQUIRE(result.value().parts.size() == 1);
    CHECK(result.value().parts.front().notes.size() == 3);
    CHECK(*result.value().parts.front().notes.front().fret == 3);
}

TEST_CASE("Guitar Pro zip routing rejects non-score and malformed archives cleanly", "[track][import]")
{
    using namespace openchordix::track::imports;

    const auto nonGpZip = GuitarProImporter().importBytes(storedZip({{"readme.txt", "Not a score"}}), "data.gp");
    REQUIRE_FALSE(nonGpZip);
    CHECK(nonGpZip.error().code == ImportErrorCode::UnsupportedFormat);
    CHECK(nonGpZip.error().detectedFormat == "ZIP archive");

    const auto emptyZip = GuitarProImporter().importBytes(storedZip({}), "empty.gp");
    REQUIRE_FALSE(emptyZip);
    CHECK(emptyZip.error().code == ImportErrorCode::UnsupportedFormat);
    CHECK(emptyZip.error().message.find("score.gpif") != std::string::npos);

    const auto missingScore = GuitarProImporter().importBytes(
        storedZip({{"Content/Preferences.json", "{}"}}), "missing-score.gp");
    REQUIRE_FALSE(missingScore);
    CHECK(missingScore.error().code == ImportErrorCode::UnsupportedFormat);

    const std::vector<std::uint8_t> malformedZip{'P', 'K', 0x03, 0x04, 0x00};
    const auto malformed = GuitarProImporter().importBytes(malformedZip, "broken.gp");
    REQUIRE_FALSE(malformed);
    CHECK(malformed.error().code == ImportErrorCode::MalformedData);
    CHECK(malformed.error().detectedFormat == "ZIP archive");

    const auto invalidXml = GuitarProImporter().importBytes(
        storedZip({{"Content/score.gpif", "<GPIF>"}}), "invalid.gp");
    REQUIRE_FALSE(invalidXml);
    CHECK(invalidXml.error().code == ImportErrorCode::MalformedData);
}

TEST_CASE("Guitar Pro 7 zip GPIF importer reads metadata tablature chords rests and part defaults", "[track][import]")
{
    using namespace openchordix::track::imports;

    const auto bytes = storedZip({{"VERSION", "7"}, {"Content/score.gpif", simpleGpif()}});
    const auto result = GuitarProImporter().importBytes(bytes, "fixture.gp");
    REQUIRE(result);

    const std::filesystem::path path = writeTemporaryFixture("openchordix_gpif_fixture.gp", bytes);
    const auto routed = ImporterRegistry().importFile(path);
    std::filesystem::remove(path);
    REQUIRE(routed);
    CHECK(routed.value().detectedFormat == "Guitar Pro 7 (zip package)");

    const ImportedSong &song = result.value();
    CHECK(song.detectedFormat == "Guitar Pro 7 (zip package)");
    CHECK(song.title == "GPIF Fixture");
    CHECK(song.subtitle == "Readable package");
    CHECK(song.artist == "Fixture Artist");
    CHECK(song.album == "Fixture Album");
    REQUIRE(song.tempos.size() == 1);
    CHECK(song.tempos.front().beatsPerMinute == Catch::Approx(132.0));
    REQUIRE(song.measures.size() == 1);
    CHECK(song.measures.front().durationTicks == 3840);
    REQUIRE(song.parts.size() == 2);

    const ImportedPart &lead = song.parts[0];
    CHECK(lead.name == "Lead Guitar");
    CHECK(lead.type == ImportedPartType::Guitar);
    CHECK(lead.importByDefault);
    CHECK(lead.detectedStringCount == 6);
    REQUIRE(lead.suggestedTuning.size() == 6);
    CHECK(lead.suggestedTuning.front() == "E4");
    REQUIRE(lead.notes.size() == 3);
    CHECK(lead.notes[0].tick == 0);
    CHECK(lead.notes[1].tick == 0);
    CHECK(lead.notes[2].tick == 1920);
    CHECK(lead.notes[0].duration == 960);
    CHECK(*lead.notes[0].stringIndex == 0);
    CHECK(*lead.notes[0].fret == 3);
    CHECK(*lead.notes[1].stringIndex == 1);
    CHECK(*lead.notes[1].fret == 2);
    CHECK(lead.notes[0].technique.hammerOn);
    CHECK(lead.notes[0].technique.slideType == "shift");
    CHECK(lead.notes[0].technique.bend);
    CHECK(lead.notes[0].technique.vibrato);
    CHECK(lead.notes[0].technique.palmMute);
    CHECK(lead.notes[0].technique.letRing);
    CHECK(lead.notes[0].technique.harmonicType == "natural");
    CHECK(lead.notes[0].technique.accent);
    CHECK(lead.notes[0].technique.heavyAccent);
    CHECK(lead.notes[0].technique.tremoloPicking);

    const ImportedPart &drums = song.parts[1];
    CHECK(drums.type == ImportedPartType::Percussion);
    CHECK_FALSE(drums.importByDefault);

    const auto gp8 = GuitarProImporter().importBytes(
        storedZip({{"Content/score.gpif", simpleGpif("8.1.1")}}), "fixture.gp");
    REQUIRE(gp8);
    CHECK(gp8.value().detectedFormat == "Guitar Pro 8.1.1 (zip package)");
}

TEST_CASE("GPIF measure lengths follow each time signature at 960 ticks per beat", "[track][import][timing]")
{
    using namespace openchordix::track::imports;

    const std::string xml = timedGpif(
        "",
        "<MasterBar><Time>4/4</Time><Bars>0</Bars></MasterBar>"
        "<MasterBar><Time>5/4</Time><Bars>1</Bars></MasterBar>"
        "<MasterBar><Time>6/8</Time><Bars>2</Bars></MasterBar>",
        "<Bar id=\"0\"><Voices>-1</Voices></Bar><Bar id=\"1\"><Voices>-1</Voices></Bar><Bar id=\"2\"><Voices>-1</Voices></Bar>",
        "", "", "", "<Rhythm id=\"q\"><NoteValue>Quarter</NoteValue></Rhythm>");
    const auto imported = GuitarProImporter().importBytes(storedZip({{"Content/score.gpif", xml}}), "meter.gp");
    REQUIRE(imported);
    REQUIRE(imported.value().measures.size() == 3);
    CHECK(imported.value().measures[0].durationTicks == 3840);
    CHECK(imported.value().measures[1].durationTicks == 4800);
    CHECK(imported.value().measures[2].durationTicks == 2880);
    CHECK(imported.value().measures[1].startTick == 3840);
    CHECK(imported.value().measures[2].startTick == 8640);
}

TEST_CASE("GPIF rhythmic beats use exact durations and independent voice cursors", "[track][import][timing]")
{
    using namespace openchordix::track::imports;

    const std::string xml = timedGpif(
        "",
        "<MasterBar><Time>4/4</Time><Bars>0</Bars></MasterBar>",
        "<Bar id=\"0\"><Voices>0 1</Voices></Bar>",
        "<Voice id=\"0\"><Beats>0 1 2 3 4</Beats></Voice><Voice id=\"1\"><Beats>5</Beats></Voice>",
        "<Beat id=\"0\"><Rhythm ref=\"q\"/><Notes>0 1</Notes></Beat>"
        "<Beat id=\"1\"><Rhythm ref=\"e\"/></Beat>"
        "<Beat id=\"2\"><Rhythm ref=\"de\"/><Notes>2</Notes></Beat>"
        "<Beat id=\"3\"><Rhythm ref=\"te\"/><Notes>3</Notes></Beat>"
        "<Beat id=\"4\"><Rhythm ref=\"s\"/><Notes>4</Notes></Beat>"
        "<Beat id=\"5\"><Rhythm ref=\"q\"/><Notes>5</Notes></Beat>",
        tabNote("0", 5, 1) + tabNote("1", 4, 2) + tabNote("2", 5, 3) +
            tabNote("3", 5, 4) + tabNote("4", 5, 5) + tabNote("5", 5, 9),
        "<Rhythm id=\"q\"><NoteValue>Quarter</NoteValue></Rhythm>"
        "<Rhythm id=\"e\"><NoteValue>Eighth</NoteValue></Rhythm>"
        "<Rhythm id=\"de\"><NoteValue>Eighth</NoteValue><AugmentationDot count=\"1\"/></Rhythm>"
        "<Rhythm id=\"te\"><NoteValue>Eighth</NoteValue><PrimaryTuplet num=\"3\" den=\"2\"/></Rhythm>"
        "<Rhythm id=\"s\"><NoteValue>16th</NoteValue></Rhythm>");
    const auto imported = GuitarProImporter().importBytes(storedZip({{"Content/score.gpif", xml}}), "rhythm.gp");
    REQUIRE(imported);
    const auto &notes = imported.value().parts.front().notes;
    REQUIRE(notes.size() == 6);
    CHECK(notes[0].tick == 0);
    CHECK(notes[1].tick == 0);
    CHECK(notes[0].duration == 960);
    CHECK(notes[2].tick == 1440);
    CHECK(notes[2].duration == 720);
    CHECK(notes[3].tick == 2160);
    CHECK(notes[3].duration == 320);
    CHECK(notes[4].tick == 2480);
    CHECK(notes[4].duration == 240);
    CHECK(notes[5].tick == 0);
    CHECK(*notes[5].fret == 9);
}

TEST_CASE("GPIF empty introductions remain full bars while anacrusis uses rhythmic pickup length", "[track][import][timing]")
{
    using namespace openchordix::track::imports;

    const std::string commonBars =
        "<Bar id=\"0\"><Voices>0</Voices></Bar><Bar id=\"1\"><Voices>1</Voices></Bar>";
    const std::string commonVoices =
        "<Voice id=\"0\"><Beats>0</Beats></Voice><Voice id=\"1\"><Beats>1</Beats></Voice>";
    const std::string emptyIntroBeats =
        "<Beat id=\"0\"><Rhythm ref=\"e\"/></Beat>"
        "<Beat id=\"1\"><Rhythm ref=\"q\"/><Notes>1</Notes></Beat>";
    const std::string pickupBeats =
        "<Beat id=\"0\"><Rhythm ref=\"e\"/><Notes>0</Notes></Beat>"
        "<Beat id=\"1\"><Rhythm ref=\"q\"/><Notes>1</Notes></Beat>";
    const std::string commonNotes = tabNote("0", 5, 1) + tabNote("1", 5, 5);
    const std::string commonRhythms =
        "<Rhythm id=\"e\"><NoteValue>Eighth</NoteValue></Rhythm>"
        "<Rhythm id=\"q\"><NoteValue>Quarter</NoteValue></Rhythm>";
    const std::string measures =
        "<MasterBar><Time>4/4</Time><Bars>0</Bars></MasterBar>"
        "<MasterBar><Time>4/4</Time><Bars>1</Bars></MasterBar>";

    const auto ordinary = GuitarProImporter().importBytes(
        storedZip({{"Content/score.gpif", timedGpif("", measures, commonBars, commonVoices, emptyIntroBeats, commonNotes, commonRhythms)}}),
        "intro.gp");
    REQUIRE(ordinary);
    REQUIRE(ordinary.value().parts.front().notes.size() == 1);
    CHECK(ordinary.value().parts.front().notes.front().tick == 3840);
    CHECK_FALSE(ordinary.value().measures.front().pickup);

    const auto pickup = GuitarProImporter().importBytes(
        storedZip({{"Content/score.gpif", timedGpif("<Anacrusis/>", measures, commonBars, commonVoices, pickupBeats, commonNotes, commonRhythms)}}),
        "pickup.gp");
    REQUIRE(pickup);
    CHECK(pickup.value().measures.front().pickup);
    CHECK(pickup.value().measures.front().durationTicks == 480);
    CHECK(pickup.value().measures[1].startTick == 480);
    CHECK(pickup.value().parts.front().notes[1].tick == 480);

    const auto debug = buildTimingDebugSummary(pickup.value(), 16, 32);
    REQUIRE(debug.parts.size() == 1);
    CHECK(debug.parts.front().firstNoteTick == 0);
    CHECK(debug.parts.front().notes[1].measureIndex == 1);
    CHECK(debug.measures.front().pickup);
}

TEST_CASE("Guitar Pro importer rejects malformed data and disables percussion tracks by default", "[track][import]")
{
    using namespace openchordix::track::imports;

    GuitarProImporter importer;
    const auto malformed = importer.importBytes({});
    REQUIRE_FALSE(malformed);
    CHECK(malformed.error().code == ImportErrorCode::MalformedData);
    const auto truncatedResult = importer.importBytes(
        identifiedGuitarPro("FICHIER GUITAR PRO v5.00"), "truncated.gp5");
    REQUIRE_FALSE(truncatedResult);
    CHECK(truncatedResult.error().code == ImportErrorCode::MalformedData);

    const auto result = importer.importBytes(simpleGp5(true), "fixture.gp5");
    REQUIRE(result);
    REQUIRE(result.value().parts.size() == 2);
    CHECK(result.value().parts[0].importByDefault);
    CHECK(result.value().parts[1].type == ImportedPartType::Percussion);
    CHECK(result.value().parts[1].detectedStringCount == 0);
    CHECK_FALSE(result.value().parts[1].importByDefault);

    const auto emptyFretted = importer.importBytes(simpleGp5(false, false, true), "empty.gp5");
    REQUIRE(emptyFretted);
    CHECK(emptyFretted.value().parts.front().type == ImportedPartType::Guitar);
    CHECK_FALSE(emptyFretted.value().parts.front().importByDefault);
}

TEST_CASE("MIDI importer reads tempo tracks and completed notes", "[track][import]")
{
    using namespace openchordix::track::imports;

    MidiImporter importer;
    const auto result = importer.importBytes(simpleMidi(), "lead.mid");

    REQUIRE(result);
    const ImportedSong &song = result.value();
    CHECK(song.ticksPerBeat == 96);
    REQUIRE(song.tempos.size() == 1);
    CHECK(song.tempos.front().beatsPerMinute == Catch::Approx(120.0));
    REQUIRE(song.parts.size() == 1);
    CHECK(song.parts.front().name == "Lead");
    REQUIRE(song.parts.front().notes.size() == 1);
    CHECK(song.parts.front().notes.front().tick == 0);
    CHECK(song.parts.front().notes.front().duration == 96);
    CHECK(song.parts.front().notes.front().midiPitch == 64);
    CHECK_FALSE(song.parts.front().notes.front().stringIndex.has_value());
    CHECK(song.durationSeconds == Catch::Approx(0.5));
}

TEST_CASE("MIDI importer rejects empty and malformed data without producing a chart", "[track][import]")
{
    using namespace openchordix::track::imports;

    MidiImporter importer;
    CHECK_FALSE(importer.importBytes({}));
    const std::vector<std::uint8_t> truncated{'M', 'T', 'h', 'd', 0x00};
    const auto malformed = importer.importBytes(truncated);
    REQUIRE_FALSE(malformed);
    CHECK(malformed.error().code == ImportErrorCode::MalformedData);
}

TEST_CASE("Applying imported parts is additive unless replacement is requested", "[track][import]")
{
    using namespace openchordix::track::imports;

    TrackChartDocument chart;
    chart.notes().push_back(TrackTabNote{.part = "Existing", .tick = 12, .duration = 12, .stringIndex = 1, .fret = 3});

    const auto imported = MidiImporter().importBytes(simpleMidi(), "lead.mid");
    REQUIRE(imported);
    const std::vector<ImportPartSelection> selections{
        {.sourceIndex = 0, .enabled = true, .name = "Imported Lead", .stringCount = 6, .tuning = {"e", "B", "G", "D", "A", "E"}},
    };

    const ImportApplySummary summary = applyImportedParts(chart, imported.value(), selections);
    CHECK(summary.addedNotes == 1);
    REQUIRE(chart.notes().size() == 2);
    CHECK(chart.notes().front().part == "Existing");
    CHECK(chart.notes().back().part == "Imported Lead");
    CHECK(chart.notes().back().duration == 48);
    CHECK(chart.notes().back().stringIndex == 0);
    CHECK(chart.notes().back().fret == 0);
}

TEST_CASE("Applying Guitar Pro parts preserves tablature and only replace mode clears existing notes", "[track][import]")
{
    using namespace openchordix::track::imports;

    const auto imported = GuitarProImporter().importBytes(simpleGp5(), "fixture.gp5");
    REQUIRE(imported);
    const std::vector<ImportPartSelection> selections{
        {.sourceIndex = 0, .enabled = true, .name = "GP Lead", .stringCount = 6, .tuning = {"E4", "B3", "G3", "D3", "A2", "E2"}},
    };

    TrackChartDocument additive;
    additive.notes().push_back(TrackTabNote{.part = "Existing", .tick = 12, .duration = 12, .stringIndex = 1, .fret = 3});
    applyImportedParts(additive, imported.value(), selections);
    REQUIRE(additive.notes().size() == 4);
    const auto importedNote = std::find_if(additive.notes().begin(), additive.notes().end(),
                                           [](const TrackTabNote &note)
                                           { return note.part == "GP Lead" && note.stringIndex == 0 && note.fret == 3; });
    REQUIRE(importedNote != additive.notes().end());
    CHECK(importedNote->hammerOn);
    CHECK(importedNote->palmMute);
    CHECK(importedNote->slideType == "slide");

    TrackChartDocument replaced;
    replaced.notes().push_back(TrackTabNote{.part = "Existing", .tick = 12, .duration = 12, .stringIndex = 1, .fret = 3});
    applyImportedParts(replaced, imported.value(), selections, ImportMergeMode::ReplaceAllNotes);
    CHECK(replaced.notes().size() == 3);
    CHECK(std::none_of(replaced.notes().begin(), replaced.notes().end(),
                       [](const TrackTabNote &note)
                       { return note.part == "Existing"; }));
}

TEST_CASE("Applying GPIF package parts remains additive unless replacement is selected", "[track][import]")
{
    using namespace openchordix::track::imports;

    const auto imported = GuitarProImporter().importBytes(
        storedZip({{"Content/score.gpif", simpleGpif()}}), "fixture.gp");
    REQUIRE(imported);
    const std::vector<ImportPartSelection> selections{
        {.sourceIndex = 0, .enabled = true, .name = "GPIF Lead", .stringCount = 6, .tuning = {"E4", "B3", "G3", "D3", "A2", "E2"}},
    };

    TrackChartDocument additive;
    additive.notes().push_back(TrackTabNote{.part = "Existing", .tick = 12, .duration = 12, .stringIndex = 1, .fret = 3});
    applyImportedParts(additive, imported.value(), selections);
    REQUIRE(additive.notes().size() == 4);
    CHECK(std::any_of(additive.notes().begin(), additive.notes().end(),
                      [](const TrackTabNote &note)
                      { return note.part == "Existing"; }));

    TrackChartDocument replaced;
    replaced.notes().push_back(TrackTabNote{.part = "Existing", .tick = 12, .duration = 12, .stringIndex = 1, .fret = 3});
    applyImportedParts(replaced, imported.value(), selections, ImportMergeMode::ReplaceAllNotes);
    CHECK(replaced.notes().size() == 3);
    CHECK(std::none_of(replaced.notes().begin(), replaced.notes().end(),
                       [](const TrackTabNote &note)
                       { return note.part == "Existing"; }));
}

TEST_CASE("Guitar Pro import placement offsets are explicit and independent of preview state", "[track][import][timing]")
{
    using namespace openchordix::track::imports;

    const auto imported = GuitarProImporter().importBytes(
        storedZip({{"Content/score.gpif", simpleGpif()}}), "fixture.gp");
    REQUIRE(imported);
    const std::vector<ImportPartSelection> selections{
        {.sourceIndex = 0, .enabled = true, .name = "Lead", .stringCount = 6, .tuning = {"E4", "B3", "G3", "D3", "A2", "E2"}},
    };

    TrackChartDocument atBeginning;
    atBeginning.setPreviewStartSeconds(42.0);
    atBeginning.notes().push_back(TrackTabNote{.part = "Existing", .tick = 240, .duration = 48, .stringIndex = 1, .fret = 3});
    const auto beginning = applyImportedParts(atBeginning, imported.value(), selections);
    CHECK(beginning.placementOffsetTicks == 0);
    CHECK(std::any_of(atBeginning.notes().begin(), atBeginning.notes().end(),
                      [](const TrackTabNote &note)
                      { return note.part == "Lead" && note.tick == 0; }));

    TrackChartDocument atCursor;
    const auto cursor = applyImportedParts(
        atCursor, imported.value(), selections, ImportMergeMode::Additive,
        {.mode = ImportPlacement::CurrentCursor, .currentCursorTick = 144});
    CHECK(cursor.placementOffsetTicks == 144);
    CHECK(atCursor.notes().front().tick == 144);

    TrackChartDocument appended;
    appended.notes().push_back(TrackTabNote{.part = "Existing", .tick = 48, .duration = 24, .stringIndex = 1, .fret = 3});
    const auto append = applyImportedParts(
        appended, imported.value(), selections, ImportMergeMode::Additive,
        {.mode = ImportPlacement::AppendAfterExisting});
    CHECK(append.placementOffsetTicks == 72);
    CHECK(std::any_of(appended.notes().begin(), appended.notes().end(),
                      [](const TrackTabNote &note)
                      { return note.part == "Lead" && note.tick == 72; }));

    TrackChartDocument replaced;
    replaced.notes().push_back(TrackTabNote{.part = "Existing", .tick = 48, .duration = 24, .stringIndex = 1, .fret = 3});
    const auto custom = applyImportedParts(
        replaced, imported.value(), selections, ImportMergeMode::ReplaceAllNotes,
        {.mode = ImportPlacement::CustomTickOffset, .customTickOffset = 24});
    CHECK(custom.placementOffsetTicks == 24);
    CHECK(std::none_of(replaced.notes().begin(), replaced.notes().end(),
                       [](const TrackTabNote &note)
                       { return note.part == "Existing"; }));
    CHECK(replaced.notes().front().tick == 24);
    REQUIRE_FALSE(replaced.measures().empty());
    CHECK(replaced.measures().front().startTick == 24);
}
