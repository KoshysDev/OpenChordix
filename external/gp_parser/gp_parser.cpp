/* Copyright Phillip Potter, 2019 under MIT License
 * Based upon https://github.com/juliangruber/parse-gp5 (also MIT) */
#include <stdexcept>
#include <fstream>
#include <iterator>
#include <regex>
#include <algorithm>
#include <cstdio>
#include <cmath>
#include "gp_parser.h"

namespace gp_parser {

/* This constructor takes a Guitar Pro file and reads it into the internal
 * vector for further use */
Parser::Parser(const char *filePath)
{
	// Open file
	if (filePath == nullptr)
		throw std::logic_error("Null file path passed to constructor");
	std::ifstream file;
	file.open(filePath, std::ifstream::in | std::ifstream::binary);

	// Initialise vector
	fileBuffer = std::vector<char>(
		     std::istreambuf_iterator<char>(file),
		     {}
		     );

	// Close file
	file.close();

	parse();
}

Parser::Parser(std::span<const std::uint8_t> fileData)
	: fileBuffer(fileData.begin(), fileData.end())
{
	parse();
}

void Parser::parse()
{
	// Parse version and check it is supported
	readVersion();
	if (!isSupportedVersion(version))
		throw std::logic_error("Unsupported version");

	// Parse out major and minor version numbers
	std::regex majorAndMinorExp("(\\d+)\\.(\\d+)");
	major = std::stoi(std::regex_replace(
			  version,
			  majorAndMinorExp,
			  "$1",
			  std::regex_constants::format_no_copy));
	minor = std::stoi(std::regex_replace(
			  version,
			  majorAndMinorExp,
			  "$2",
			  std::regex_constants::format_no_copy));

	// Read attributes of tab file
	title = readStringByteSizeOfInteger();
	subtitle = readStringByteSizeOfInteger();
	artist = readStringByteSizeOfInteger();
	album = readStringByteSizeOfInteger();
	lyricsAuthor = readStringByteSizeOfInteger();
	musicAuthor = readStringByteSizeOfInteger();
	copyright = readStringByteSizeOfInteger();
	tab = readStringByteSizeOfInteger();
	instructions = readStringByteSizeOfInteger();
	auto commentLen = readInt();
	if (commentLen < 0 || commentLen > 100000)
		throw std::runtime_error("Invalid GP5 comment count");
	for (auto i = 0; i < commentLen; ++i)
		comments.push_back(readStringByteSizeOfInteger());

	// Read lyrics data
	lyricTrack = readInt();
	lyric = readLyrics();

	// Read page setup
	readPageSetup();

	// Read tempo value
	tempoValue = readInt();

	if (versionIndex > 0)
		skip(1);

	// Read key signature
	globalKeySignature = readKeySignature();
	
	skip(3);

	// Octave
	readByte();

	// Read channels
	channels = readChannels();

	skip(42);

	// Read measures and track count info
	measures = readInt();
	trackCount = readInt();
	if (measures < 1 || measures > 100000 || trackCount < 1 || trackCount > 512)
		throw std::runtime_error("Invalid GP5 measure or track count");

	// Read measure headers
	auto timeSignature = TimeSignature();
	timeSignature.numerator = 4;
	timeSignature.denominator.value = QUARTER;
	timeSignature.denominator.division.enters = 1;
	timeSignature.denominator.division.times = 1;
	for (auto i = 0; i < measures; ++i) {
		if (i > 0)
			skip(1);
		std::uint8_t flags = readUnsignedByte();
		auto header = MeasureHeader();
		header.number = i + 1;
		header.start = 0;
		header.tempo.value = 120;
		header.repeatOpen = (flags & 0x04) != 0;
		if ((flags & 0x01) != 0)
			timeSignature.numerator = readByte();
		if ((flags & 0x02) != 0)
			timeSignature.denominator.value = readByte();
		header.timeSignature = timeSignature;
		if ((flags & 0x08) != 0)
			header.repeatClose = (readByte() & 0xFF) - 1;
		if ((flags & 0x20) != 0) {
			header.marker.measure = header.number;
			header.marker.title = readStringByteSizeOfInteger();
			header.marker.color = readColor();
		}
		if ((flags & 0x10) != 0)
			header.repeatAlternative = readUnsignedByte();
		if ((flags & 0x40) != 0) {
			globalKeySignature = readKeySignature();
			skip(1);
		}
		if ((flags & 0x01) != 0 || (flags & 0x02) != 0)
			skip(4);
		if ((flags & 0x10) == 0)
			skip(1);
		auto tripletFeel = readByte();
		if (tripletFeel == 1)
			header.tripletFeel = "eigth";
		else if (tripletFeel == 2)
			header.tripletFeel = "sixteents";
		else
			header.tripletFeel = "none";

		// Push header to vector
		measureHeaders.push_back(header);
	}

	// Read tracks
	for (auto number = 1; number <= trackCount; ++number) {
		auto track = Track();
		readUnsignedByte();
		if (number == 1 || versionIndex == 0)
			skip(1);
		track.number = number;
		track.lyrics = number == lyricTrack ? lyric : Lyric();
		track.name = readStringByte(40);
		auto stringCount = readInt();
		if (stringCount < 0 || stringCount > 7)
			throw std::runtime_error("Invalid GP5 string count");
		for (auto i = 0; i < 7; ++i) {
			auto tuning = readInt();
			if (stringCount > i) {
				auto string = GuitarString();
				string.number = i + 1;
				string.value = tuning;
				track.strings.push_back(string);
			}
		}
		readInt();
		readChannel(track);
		readInt();
		track.offset = readInt();
		track.color = readColor();
		skip(versionIndex > 0 ? 49 : 44);
		if (versionIndex > 0) {
			readStringByteSizeOfInteger();
			readStringByteSizeOfInteger();
		}
		tracks.push_back(track);
	}
	skip(versionIndex == 0 ? 2 : 1);

	// Iterate through measures
	auto tempo = Tempo();
	tempo.value = tempoValue;
	auto start = QUARTER_TIME;
	for (auto i = 0; i < measures; ++i) {
		auto& header = measureHeaders[i];
		header.start = start;
		for (auto j = 0; j < trackCount; ++j) {
			Track& track = tracks[j];
			auto measure = Measure();
			measure.header = &header;
			measure.start = start;
			track.measures.push_back(measure);
			readMeasure(track.measures[track.measures.size() - 1], track, tempo, globalKeySignature);
			skip(1);
		}
		header.tempo = tempo;
		start += getLength(header);
	}
}

/* This reads an unsigned byte from the file buffer and increments the
 * position at the same time */
void Parser::require(std::size_t n) const
{
	if (bufferPosition > fileBuffer.size() || n > fileBuffer.size() - bufferPosition)
		throw std::runtime_error("Truncated GP5 input");
}

std::uint8_t Parser::readUnsignedByte()
{
	require(1);
	return static_cast<uint8_t>(fileBuffer[bufferPosition++]);
}

/* This reads a signed byte from the file buffer and increments the
 * position at the same time */
std::int8_t Parser::readByte()
{
	require(1);
	return static_cast<int8_t>(fileBuffer[bufferPosition++]);
}

/* This reads a signed 32-bit integer from the file buffer in little-endian
 * mode and increments the position at the same time */
std::int32_t Parser::readInt()
{
	require(4);
	auto returnVal = static_cast<int32_t>(
			    ((fileBuffer[bufferPosition + 3] & 0xFF) << 24) |
			    ((fileBuffer[bufferPosition + 2] & 0xFF) << 16) |
			    ((fileBuffer[bufferPosition + 1] & 0xFF) << 8) |
			    (fileBuffer[bufferPosition] & 0xFF)
			    );
	bufferPosition += 4;

	return returnVal;
}

/* This version of the function takes no 'len' parameter and merely forwards
 * through to the full method by setting 'len' to be equal to 'size' */
std::string Parser::readString(size_t size)
{
	return readString(size, size);
}

/* This returns a string from the file buffer, in the general case by reading
 * 'size' bytes from the file buffer then converting it to a string of 'len'
 * bytes */
std::string Parser::readString(size_t size, size_t len)
{
	// Work out number of bytes to read
	auto bytesToRead = size > 0 ? size : len;
	if (len > bytesToRead)
		throw std::runtime_error("Invalid GP5 string length");
	require(bytesToRead);

	// Read this number of bytes from the file buffer
	auto bytes = std::vector<char>(bytesToRead);
	std::copy(fileBuffer.begin() + bufferPosition,
		  fileBuffer.begin() + bufferPosition + bytesToRead,
		  bytes.begin());

	// Increment position
	bufferPosition += bytesToRead;

	// Convert to string and return
	return std::string(bytes.begin(),
			   len <= bytesToRead ?
			   (bytes.begin() + len) : (bytes.begin() + size));
}

/* This returns a string from the file buffer, but using a byte before it to
 * tell it the length of the string */
std::string Parser::readStringByte(size_t size)
{
	return readString(size, readUnsignedByte());
}

/* This returns a string from the file buffer, but using an integer before it
 * to tell it the total number of bytes to read - the initial byte that is
 * read still gives the string length */
std::string Parser::readStringByteSizeOfInteger()
{
	auto size = readInt();
	if (size < 1)
		throw std::runtime_error("Invalid GP5 sized string");
	return readStringByte(static_cast<std::size_t>(size - 1));
}

std::string Parser::readStringInteger()
{
	auto size = readInt();
	if (size < 0)
		throw std::runtime_error("Invalid GP5 string");
	return readString(static_cast<std::size_t>(size));
}

/* This just moves the position past 'n' number of bytes in the file buffer */
void Parser::skip(std::size_t n)
{
	require(n);
	bufferPosition += n;
}

/* This reads the version data from the file buffer */
void Parser::readVersion()
{
	version = readStringByte(30);
}

/* This checks if the supplied version is supported by the parser */
bool Parser::isSupportedVersion(std::string& version)
{
	auto versionsCount = sizeof(VERSIONS) / sizeof(const char *);
	for (auto i = 0; i < versionsCount; ++i) {
		if (version.compare(VERSIONS[i]) == 0) {
			versionIndex = i;
			return true;
		}
	}
	return false;
}

/* This reads lyrics data */
Lyric Parser::readLyrics()
{
	auto lyric = Lyric();
	lyric.from = readInt();
	lyric.lyric = readStringInteger();

	for (auto i = 0; i < 4; ++i) {
		readInt();
		readStringInteger();
	}

	return lyric;
}

/* This reads the page setup data */
void Parser::readPageSetup()
{
	skip(versionIndex > 0 ? 49 : 30);
	for (auto i = 0; i < 11; ++i) {
		skip(4);
		readStringByte(0);
	}
}

/* This reads the key signature */
std::int8_t Parser::readKeySignature()
{
	auto keySignature = readByte();
	if (keySignature < 0)
		keySignature = 7 - keySignature;

	return keySignature;
}

/* This reads the channel attributes data */
std::vector<Channel> Parser::readChannels()
{
	std::vector<Channel> channels;
	for (auto i = 0; i < 64; ++i) {
		auto channel = Channel();
		channel.program = readInt();
		channel.volume = readByte();
		channel.balance = readByte();
		channel.chorus = readByte();
		channel.reverb = readByte();
		channel.phaser = readByte();
		channel.tremolo = readByte();
		if (i == 9) {
			channel.bank = "default percussion bank";
			channel.isPercussionChannel = true;
		} else {
			channel.bank = "default bank";
		}
		if (channel.program < 0)
			channel.program = 0;
		channels.push_back(channel);
		skip(2);
	}

	return channels;
}

/* Read a color value */
Color Parser::readColor()
{
	auto c = Color();
	c.r = readUnsignedByte();
	c.g = readUnsignedByte();
	c.b = readUnsignedByte();
	skip(1);

	return c;
}

/* Read a channel */
void Parser::readChannel(Track& track)
{
	auto gmChannel1 = readInt() - 1;
	auto gmChannel2 = readInt() - 1;
	if (gmChannel1 >= 0 && gmChannel1 < channels.size()) {
		// Allocate temporary buffer to hold chars for conversion
		auto gmChannel1Param = ChannelParam();
		auto gmChannel2Param = ChannelParam();
		gmChannel1Param.key = "gm channel 1";
		gmChannel1Param.value = std::to_string(gmChannel1);
		gmChannel2Param.key = "gm channel 2";
		gmChannel2Param.value = std::to_string(gmChannel1 != 9 ? gmChannel2 : gmChannel1);

		// Copy channel to temporary variable
		Channel channel = channels[gmChannel1];

		// TODO: channel auxiliary, JS code below:
		/*for (let i = 0; i < channels.length; i++) {
			let channelAux = channels[i];
			for (let n = 0; n < channelAux.; i++) {

			}
		}*/

		if (channel.id == 0) {
			channel.id = channels.size() + 1;
			channel.name = "TODO";
			channel.parameters.push_back(gmChannel1Param);
			channel.parameters.push_back(gmChannel2Param);
			channels.push_back(channel);
		}
		track.channelId = channel.id;
	}
}

/* Read a measure */
void Parser::readMeasure(Measure& measure, Track& track, Tempo& tempo, std::int8_t keySignature)
{
	for (auto voice = 0; voice < 2; ++voice) {
		auto start = measure.start;
		auto beats = readInt();
		if (beats < 0 || beats > 1000000)
			throw std::runtime_error("Invalid GP5 beat count");
		for (auto k = 0; k < beats; ++k)
			start += readBeat(start, measure, track, tempo, voice);
	}

	std::vector<Beat*> emptyBeats;
	for (auto i = 0; i < measure.beats.size(); ++i) {
		auto beatPtr = &measure.beats[i];
		auto empty = true;
		for (auto v = 0; v < beatPtr->voices.size(); ++v) {
			if (beatPtr->voices[v].notes.size() != 0)
				empty = false;
		}
		if (empty)
			emptyBeats.push_back(beatPtr);
	}
	for (auto beatPtr : emptyBeats) {
		for (auto i = 0; i < measure.beats.size(); ++i) {
			if (beatPtr == &measure.beats[i]) {
				measure.beats.erase(measure.beats.begin() + i);
				break;
			}
		}
	}
	measure.clef = getClef(track);
	measure.keySignature = keySignature;
}

/* Get measure length */
std::int32_t Parser::getLength(MeasureHeader& header)
{
	return static_cast<std::int32_t>(std::round(header.timeSignature.numerator *
		getTime(denominatorToDuration(header.timeSignature.denominator))));
}

/* Adds a new measure to the beat */
Beat& Parser::getBeat(Measure& measure, std::int32_t start)
{
	for (auto& beat : measure.beats) {
		if (beat.start == start)
			return beat;
	}

	auto beat = Beat();
	beat.voices.resize(2);
	beat.start = start;
	measure.beats.push_back(beat);

	return measure.beats[measure.beats.size() - 1];
}

/* Read mix change */
void Parser::readMixChange(Tempo& tempo)
{
	readByte(); // instrument

	skip(16);
	auto volume = readByte();
	auto pan = readByte();
	auto chorus = readByte();
	auto reverb = readByte();
	auto phaser = readByte();
	auto tremolo = readByte();
	readStringByteSizeOfInteger(); // tempoName
	auto tempoValue = readInt();
	if (volume >= 0)
		readByte();
	if (pan >= 0)
		readByte();
	if (chorus >= 0)
		readByte();
	if (reverb >= 0)
		readByte();
	if (phaser >= 0)
		readByte();
	if (tremolo >= 0)
		readByte();
	if (tempoValue >= 0) {
		tempo.value = tempoValue;
		skip(1);
		if (versionIndex > 0)
			skip(1);
	}
	readByte();
	skip(1);
	if (versionIndex > 0) {
		readStringByteSizeOfInteger();
		readStringByteSizeOfInteger();
	}
}

/* Read beat effects */
void Parser::readBeatEffects(Beat& beat, NoteEffect& noteEffect)
{
	auto flags1 = readUnsignedByte();
	auto flags2 = readUnsignedByte();
	noteEffect.fadeIn = (flags1 & 0x10) != 0;
	noteEffect.vibrato = (flags1 & 0x02) != 0;
	if ((flags1 & 0x20) != 0) {
		auto effect = readUnsignedByte();
		noteEffect.tapping = effect == 1;
		noteEffect.slapping = effect == 2;
		noteEffect.popping = effect == 3;
	}
	if ((flags2 & 0x04) != 0)
		readTremoloBar(noteEffect);
	if ((flags1 & 0x40) != 0) {
		auto strokeUp = readByte();
		auto strokeDown = readByte();
		// TODO
		if (strokeUp > 0) {
			beat.stroke.direction = "stroke_up";
			beat.stroke.value = "stroke_down";
		} else if (strokeDown > 0) {
			beat.stroke.direction = "stroke_down";
			beat.stroke.value = "stroke_down";
		}
	}
	if ((flags2 & 0x02) != 0)
		readByte();
}

/* Read tremolo bar */
void Parser::readTremoloBar(NoteEffect& effect)
{
	skip(5);
	auto tremoloBar = TremoloBar();
	auto numPoints = readInt();
	if (numPoints < 0 || numPoints > 10000)
		throw std::runtime_error("Invalid GP5 tremolo point count");
	for (auto i = 0; i < numPoints; ++i) {
		auto position = readInt();
		auto value = readInt();
		readByte();

		auto point = TremoloPoint();
		point.pointPosition = static_cast<std::int32_t>(std::round(
				position * 1.0 /*'max position length'*/ / 
				1.0 /*'bend position'*/)); // TODO
		point.pointValue = static_cast<std::int32_t>(std::round(
				value / (1.0/*'GP_BEND_SEMITONE'*/
				* 0x2f))); //TODO
		tremoloBar.points.push_back(point);
	}
	if (tremoloBar.points.size() > 0)
		effect.tremoloBar = tremoloBar;
}

/* Read beat text */
void Parser::readText(Beat& beat)
{
	beat.text.value = readStringByteSizeOfInteger();
}

/* Read chord */
void Parser::readChord(std::vector<GuitarString>& strings, Beat& beat)
{
	auto chord = Chord();
	chord.strings = &strings;
	skip(17);
	chord.name = readStringByte(21);
	skip(4);
	chord.frets.resize(7);
	chord.frets[0] = readInt();
	for (auto i = 0; i < 7; ++i) {
		auto fret = readInt();
		if (i < chord.strings->size())
			chord.frets[i] = fret;
	}
	skip(32);
	if (chord.strings->size() > 0)
		beat.chord = chord;
}

/* Get duration */
double Parser::getTime(Duration duration)
{
	auto time = QUARTER_TIME * 4.0 / duration.value;
	if (duration.dotted)
		time += time / 2;
	else if (duration.doubleDotted)
		time += (time / 4) * 3;

	return time * duration.division.times / duration.division.enters;
}

/* Read duration */
double Parser::readDuration(std::uint8_t flags)
{
	auto duration = Duration();
	duration.value = pow(2, (readByte() + 4)) / 4;
	duration.dotted = (flags & 0x01) != 0;
	if ((flags & 0x20) != 0) {
		auto divisionType = readInt();
		switch (divisionType) {
		case 3:
			duration.division.enters = 3;
			duration.division.times = 2;
			break;
		case 5:
			duration.division.enters = 5;
			duration.division.times = 5;
			break;
		case 6:
			duration.division.enters = 6;
			duration.division.times = 4;
			break;
		case 7:
			duration.division.enters = 7;
			duration.division.times = 4;
			break;
		case 9:
			duration.division.enters = 9;
			duration.division.times = 8;
			break;
		case 10:
			duration.division.enters = 10;
			duration.division.times = 8;
			break;
		case 11:
			duration.division.enters = 11;
			duration.division.times = 8;
			break;
		case 12:
			duration.division.enters = 12;
			duration.division.times = 8;
			break;
		case 13:
			duration.division.enters = 13;
			duration.division.times = 8;
			break;
		}
	}
	if (duration.division.enters == 0) {
		duration.division.enters = 1;
		duration.division.times = 1;
	}

	return getTime(duration);
}

/* Read beat */
double Parser::readBeat(std::int32_t start, Measure& measure, Track& track, Tempo& tempo, std::size_t voiceIndex)
{
	auto flags = readUnsignedByte();

	auto& beat = getBeat(measure, start);
	auto& voice = beat.voices[voiceIndex];
	if ((flags & 0x40) != 0) {
		auto beatType = readUnsignedByte();
		voice.empty = (beatType & 0x02) == 0;
	}
	auto duration = readDuration(flags);
	auto effect = NoteEffect();
	if ((flags & 0x02) != 0)
		readChord(track.strings, beat);
	if ((flags & 0x04) != 0)
		readText(beat);
	if ((flags & 0x08) != 0)
		readBeatEffects(beat, effect);
	if ((flags & 0x10) != 0)
		readMixChange(tempo);
	auto stringFlags = readUnsignedByte();
	for (auto i = 6; i >= 0; --i) {
		if ((stringFlags & (1 << i)) != 0 && (6 - i) < track.strings.size()) {
			auto string = track.strings[6 - i];
			auto note = readNote(string, track, effect);
			voice.notes.push_back(note);
		}
		voice.duration = duration;
	}

	skip(1);

	auto read = readByte();
	if ((read & 0x02) != 0)
		skip(1);

	return duration;
}

/* Read note */
Note Parser::readNote(GuitarString& string, Track& track, NoteEffect& effect)
{
	auto flags = readUnsignedByte();
	auto note = Note();
	note.string = string.number;
	note.effect = effect;
	note.effect.accentuatedNote = (flags & 0x40) != 0;
	note.effect.heavyAccentuatedNote = (flags & 0x02) != 0;
	note.effect.ghostNote = (flags & 0x04) != 0;
	if ((flags & 0x20) != 0) {
		auto noteType = readUnsignedByte();
		note.tiedNote = noteType == 0x02;
		note.effect.deadNote = noteType == 0x03;
	}
	if ((flags & 0x10) != 0) {
		note.velocity = TGVELOCITIES_MIN_VELOCITY +
		(TGVELOCITIES_VELOCITY_INCREMENT * readByte()) -
		TGVELOCITIES_VELOCITY_INCREMENT; // TODO
	}
	if ((flags & 0x20) != 0) {
		auto fret = readByte();
		auto value = note.tiedNote
			? getTiedNoteValue(string.number, track)
			: fret;
		note.value = value >= 0 && value < 100
			? value
			: 0;
	}
	if ((flags & 0x80) != 0)
		skip(2);
	if ((flags & 0x01) != 0)
		skip(8);
	skip(1);
	if ((flags & 0x08) != 0)
		readNoteEffects(note.effect);

	return note;
}

/* Get tied note value */
std::int8_t Parser::getTiedNoteValue(std::int32_t string, Track& track)
{
	auto measureCount = track.measures.size();
	if (measureCount > 0) {
		for (auto m = static_cast<std::int64_t>(measureCount) - 1; m >= 0; --m) {
			auto& measure = track.measures[static_cast<std::size_t>(m)];
			for (auto b = static_cast<std::int64_t>(measure.beats.size()) - 1; b >= 0; --b) {
				auto& beat = measure.beats[b];
				for (auto v = 0; v < beat.voices.size(); ++v) {
					auto& voice = beat.voices[v];
					if (!voice.empty) {
						for (auto n = 0; n < voice.notes.size(); ++n) {
							auto& note = voice.notes[n];
							if (note.string == string)
								return note.value;
						}
					}
				}
			}
		}
	}

	return 0;
}

/* Read effects for note */
void Parser::readNoteEffects(NoteEffect& noteEffect)
{
	auto flags1 = readUnsignedByte();
	auto flags2 = readUnsignedByte();
	if ((flags1 & 0x01) != 0)
		readBend(noteEffect);
	if ((flags1 & 0x10) != 0)
		readGrace(noteEffect);
	if ((flags2 & 0x04) != 0)
		readTremoloPicking(noteEffect);
	if ((flags2 & 0x08) != 0) {
		noteEffect.slide = true;
		readByte();
	}
	if ((flags2 & 0x10) != 0)
		readArtificialHarmonic(noteEffect);
	if ((flags2 & 0x20) != 0)
		readTrill(noteEffect);
	noteEffect.hammer = (flags1 & 0x02) != 0;
	noteEffect.letRing = (flags1 & 0x08) != 0;
	noteEffect.vibrato = (flags2 & 0x40) != 0;
	noteEffect.palmMute = (flags2 & 0x02) != 0;
	noteEffect.staccato = (flags2 & 0x01) != 0;
}

/* Read bend */
void Parser::readBend(NoteEffect& effect)
{
	skip(5);
	auto bend = Bend();
	auto numPoints = readInt();
	if (numPoints < 0 || numPoints > 10000)
		throw std::runtime_error("Invalid GP5 bend point count");
	for (auto i = 0; i < numPoints; ++i) {
		auto bendPosition = readInt();
		auto bendValue = readInt();
		readByte();
		auto p = BendPoint();
		p.pointPosition = std::round(bendPosition *
				TGEFFECTBEND_MAX_POSITION_LENGTH /
				static_cast<double>(GP_BEND_POSITION));
		p.pointValue = std::round(bendValue *
				TGEFFECTBEND_SEMITONE_LENGTH /
				static_cast<double>(GP_BEND_SEMITONE));
		bend.points.push_back(p);
	}
	if (bend.points.size() > 0)
		effect.bend = bend;
}

/* Read grace */
void Parser::readGrace(NoteEffect& effect)
{
	auto fret = readUnsignedByte();
	auto dynamic = readUnsignedByte();
	auto transition = readByte();
	auto duration = readUnsignedByte();
	auto flags = readUnsignedByte();
	auto grace = Grace();
	grace.fret = fret;
	grace.dynamic = (TGVELOCITIES_MIN_VELOCITY +
			(TGVELOCITIES_VELOCITY_INCREMENT * dynamic)) -
			TGVELOCITIES_VELOCITY_INCREMENT;
	grace.duration = duration;
	grace.dead = (flags & 0x01) != 0;
	grace.onBeat = (flags & 0x02) != 0;
	if (transition == 0)
		grace.transition = "none";
	else if (transition == 1)
		grace.transition = "slide";
	else if (transition == 2)
		grace.transition = "bend";
	else if (transition == 3)
		grace.transition = "hammer";
	effect.grace = grace;
}

/* Read tremolo picking */
void Parser::readTremoloPicking(NoteEffect& effect)
{
	auto value = readUnsignedByte();
	auto tp = TremoloPicking();
	if (value == 1) {
		tp.duration.value = "eigth";
		effect.tremoloPicking = tp;
	} else if (value == 2) {
		tp.duration.value = "sixteenth";
		effect.tremoloPicking = tp;
	} else if (value == 3) {
		tp.duration.value = "thirty_second";
		effect.tremoloPicking = tp;
	}
}

/* Read artificial harmonic */
void Parser::readArtificialHarmonic(NoteEffect& effect)
{
	auto type = readByte();
	auto harmonic = Harmonic();
	if (type == 1) {
		harmonic.type = "natural";
		effect.harmonic = harmonic;
	} else if (type == 2) {
		skip(3);
		harmonic.type = "artificial";
		effect.harmonic = harmonic;
	} else if (type == 3) {
		skip(1);
		harmonic.type = "tapped";
		effect.harmonic = harmonic;
	} else if (type == 4) {
		harmonic.type = "pinch";
		effect.harmonic = harmonic;
	} else if (type == 5) {
		harmonic.type = "semi";
		effect.harmonic = harmonic;
	}
}

/* Read trill */
void Parser::readTrill(NoteEffect& effect)
{
	auto fret = readByte();
	auto period = readByte();
	auto trill = Trill();
	trill.fret = fret;
	if (period == 1) {
		trill.duration.value = "sixteenth";
		effect.trill = trill;
	} else if (period == 2) {
		trill.duration.value = "thirty_second";
		effect.trill = trill;
	} else if (period == 3) {
		trill.duration.value = "sixty_fourth";
		effect.trill = trill;
	}
}

/* Tests if the channel corresponding to the supplied id is a
 * drum channel */
bool Parser::isPercussionChannel(std::int32_t channelId)
{
	for (auto& channel : channels) {
		if (channel.id == channelId)
			return channel.isPercussionChannel;
	}

	return false;
}

/* Get clef */
std::string Parser::getClef(Track& track)
{
	if (!isPercussionChannel(track.channelId)) {
		for (auto& string : track.strings) {
			if (string.value <= 34)
				return "CLEF_BASS";
		}
	}	

	return "CLEF_TREBLE";
}

/* This generates the same state as the XML blob, but in object
 * form that can be manipulated by the caller */
TabFile Parser::getTabFile()
{
	return TabFile(major, minor, title, subtitle, artist, album,
		       lyricsAuthor, musicAuthor, copyright, tab,
		       instructions, comments, lyric, tempoValue,
		       globalKeySignature, channels, measures,
		       trackCount, measureHeaders, tracks);
}

/* Tells us how many digits there are in a base 10 number */
std::int32_t numOfDigits(std::int32_t num)
{
	auto digits = 0;
	for (auto order = 1; num / order != 0; order *= 10)
		++digits;

	return digits;
}

/* Converts a denominator struct to a duration struct */
Duration denominatorToDuration(Denominator& denominator)
{
	auto duration = Duration();
	duration.value = denominator.value;
	duration.division = denominator.division;

	return duration;
}

}
