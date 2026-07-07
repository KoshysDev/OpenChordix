/* Copyright Phillip Potter, 2019 under MIT License
 * Based upon https://github.com/juliangruber/parse-gp5 (also MIT) */
#ifndef GP_PARSER
#define GP_PARSER

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>
#include <string>
#include <sstream>

namespace gp_parser {

// Supported versions and other data
static const char *VERSIONS[] = {
	"FICHIER GUITAR PRO v5.00",
	"FICHIER GUITAR PRO v5.10"
};
static const int QUARTER_TIME = 960;
static const int QUARTER = 4;
static const int TGEFFECTBEND_MAX_POSITION_LENGTH = 12;
static const int TGEFFECTBEND_SEMITONE_LENGTH = 1;
static const int GP_BEND_SEMITONE = 25;
static const int GP_BEND_POSITION = 60;
static const int TGVELOCITIES_MIN_VELOCITY = 15;
static const int TGVELOCITIES_VELOCITY_INCREMENT = 16;

// Spacing for XML output
#define XML_SPACING "    "

// Define struct to hold lyrics data
struct Lyric {
	std::int32_t from;
	std::string lyric;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define channel parameter struct
struct ChannelParam {
	std::string key;
	std::string value;

	void addToXML(std::ostringstream& outputStream, std::int32_t identLevel) const;
};

// Define channel struct
struct Channel {
	std::int32_t id;
	std::string name;
	std::int32_t program;
	std::int8_t volume;
	std::int8_t balance;
	std::int8_t chorus;
	std::int8_t reverb;
	std::int8_t phaser;
	std::int8_t tremolo;
	std::string bank;
	bool isPercussionChannel;
	std::vector<ChannelParam> parameters;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define division struct
struct Division {
	std::int32_t enters;
	std::int32_t times;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define denominator struct
struct Denominator {
	std::int8_t value;
	Division division;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define duration struct
struct Duration {
	double value;
	bool dotted;
	bool doubleDotted;
	Division division;
};

// Define time signature struct
struct TimeSignature {
	std::int8_t numerator;
	Denominator denominator;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define color struct
struct Color {
	std::uint8_t r;
	std::uint8_t g;
	std::uint8_t b;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define measure marker struct
struct Marker {
	std::int32_t measure;
	std::string title;
	Color color;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define tempo struct
struct Tempo {
	std::int32_t value;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

struct TempoChange {
	std::int32_t tick;
	std::int32_t value;
};

// Define measure header struct
struct MeasureHeader {
	std::int32_t number;
	std::int32_t start;
	bool repeatOpen;
	std::int8_t repeatClose;	
	std::uint8_t repeatAlternative;
	std::string tripletFeel;
	Tempo tempo;
	TimeSignature timeSignature;
	Marker marker;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define tremolo point struct
struct TremoloPoint {
	std::int32_t pointPosition;
	std::int32_t pointValue;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define tremolo bar struct
struct TremoloBar {
	std::vector<TremoloPoint> points;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define bend point struct
struct BendPoint {
	std::int32_t pointPosition;
	std::int32_t pointValue;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define bend struct
struct Bend {
	std::vector<BendPoint> points;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define grace struct
struct Grace {
	std::uint8_t fret;
	std::int32_t dynamic;
	std::string transition;
	std::uint8_t duration;
	bool dead;
	bool onBeat;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define effect duration struct
struct EffectDuration {
	std::string value;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define tremolo picking struct
struct TremoloPicking {
	EffectDuration duration;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define harmonic struct
struct Harmonic {
	std::string type;
	std::int32_t data;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define trill struct
struct Trill {
	std::int8_t fret;
	EffectDuration duration;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define note effect struct
struct NoteEffect {
	bool fadeIn;
	bool vibrato;
	bool tapping;
	bool slapping;
	bool popping;
	bool deadNote;
	bool accentuatedNote;
	bool heavyAccentuatedNote;
	bool ghostNote;
	bool slide;
	bool hammer;
	bool letRing;
	bool palmMute;
	bool staccato;
	TremoloBar tremoloBar;
	TremoloPicking tremoloPicking;
	Bend bend;
	Grace grace;
	Harmonic harmonic;
	Trill trill;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define note struct
struct Note {
	std::int32_t string;
	bool tiedNote;
	std::int8_t value;
	std::int32_t velocity;
	NoteEffect effect;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define voice struct
struct Voice {
	bool empty;
	double duration;	
	std::vector<Note> notes;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define stroke struct
struct Stroke {
	std::string direction;
	std::string value;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define guitar string struct
struct GuitarString {
	std::int32_t number;
	std::int32_t value;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define chord struct
struct Chord {
	std::string name;
	std::vector<GuitarString>* strings;
	std::vector<std::int32_t> frets;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define beat text struct
struct BeatText {
	std::string value;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define beat struct
struct Beat {
	std::int32_t start;
	BeatText text;
	Stroke stroke;
	Chord chord;
	std::vector<Voice> voices;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define measure struct
struct Measure {
	MeasureHeader *header;
	std::int32_t start;
	std::int8_t keySignature;
	std::string clef;
	std::vector<Beat> beats;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define track struct
struct Track {
	std::int32_t channelId;
	std::int32_t number;
	std::string name;
	std::int32_t offset;
	Lyric lyrics;
	Color color;
	std::vector<GuitarString> strings;
	std::vector<Measure> measures;

	void addToXML(std::ostringstream& outputStream, std::int32_t indentLevel) const;
};

// Define struct to return overall tab - it only contains references to real values
// inside Parser object, so that they can be modified.
struct TabFile {
	// State of tab file
	std::int32_t& major;
	std::int32_t& minor;
	std::string& title;
	std::string& subtitle;
	std::string& artist;
	std::string& album;
	std::string& lyricsAuthor;
	std::string& musicAuthor;
	std::string& copyright;
	std::string& tab;
	std::string& instructions;
	std::vector<std::string>& comments;
	Lyric& lyric;
	std::int32_t& tempoValue;
	std::int8_t& globalKeySignature;
	std::vector<Channel>& channels;
	std::int32_t& measures;
	std::int32_t& trackCount;
	std::vector<MeasureHeader>& measureHeaders;
	std::vector<Track>& tracks;
	std::vector<TempoChange>& tempoChanges;

	// Constructor to set references
	TabFile(std::int32_t& major, std::int32_t& minor, std::string& title,
		std::string& subtitle, std::string& artist, std::string& album,
		std::string& lyricsAuthor, std::string& musicAuthor, std::string& copyright,
		std::string& tab, std::string& instructions, std::vector<std::string>& comments,
		Lyric& lyric, std::int32_t& tempoValue, std::int8_t& globalKeySignature,
		std::vector<Channel>& channels, std::int32_t& measures, std::int32_t& trackCount,
		std::vector<MeasureHeader>& measureHeaders, std::vector<Track>& tracks,
		std::vector<TempoChange>& tempoChanges)
		: major(major), minor(minor), title(title), subtitle(subtitle),
		  artist(artist), album(album), lyricsAuthor(lyricsAuthor), musicAuthor(musicAuthor),
		  copyright(copyright), tab(tab), instructions(instructions), comments(comments),
		  lyric(lyric), tempoValue(tempoValue), globalKeySignature(globalKeySignature),
		  channels(channels), measures(measures), trackCount(trackCount),
		  measureHeaders(measureHeaders), tracks(tracks), tempoChanges(tempoChanges) {}
};

class Parser {
public:
	Parser(const char *filePath);
	Parser(std::span<const std::uint8_t> fileData);
	std::string getXML() const;
	TabFile getTabFile();
private:
	// Private member properties
	std::vector<char> fileBuffer;
	std::size_t bufferPosition = 0;
	std::string version;
	std::size_t versionIndex;
	std::int32_t major;
	std::int32_t minor;
	std::string title;
	std::string subtitle;
	std::string artist;
	std::string album;
	std::string lyricsAuthor;
	std::string musicAuthor;
	std::string copyright;
	std::string tab;
	std::string instructions;
	std::vector<std::string> comments;
	std::int32_t lyricTrack;
	Lyric lyric;
	std::int32_t tempoValue;
	std::int8_t globalKeySignature;
	std::vector<Channel> channels;
	std::int32_t measures;
	std::int32_t trackCount;
	std::vector<MeasureHeader> measureHeaders;
	std::vector<Track> tracks;
	std::vector<TempoChange> tempoChanges;

	// Private member functions for reading low-level file data
	void parse();
	void require(std::size_t n) const;
	std::uint8_t readUnsignedByte();
	std::int8_t readByte();
	std::int32_t readInt();
	std::string readString(std::size_t size);
	std::string readString(std::size_t size, std::size_t len);
	std::string readStringByte(std::size_t size);
	std::string readStringByteSizeOfInteger();
	std::string readStringInteger();
	void skip(std::size_t n);

	// Private member functions for parsing higher-level file data
	void readVersion();
	bool isSupportedVersion(std::string& version);
	Lyric readLyrics();
	void readPageSetup();
	std::int8_t readKeySignature();
	std::vector<Channel> readChannels();
	Color readColor();
	void readChannel(Track& track);
	void readMeasure(Measure& measure, Track& track, Tempo& tempo, std::int8_t keySignature);
	std::int32_t getLength(MeasureHeader& header);
	Beat& getBeat(Measure& measure, std::int32_t start);
	void readMixChange(Tempo& tempo, std::int32_t tick);
	void readBeatEffects(Beat& beat, NoteEffect& noteEffect);
	void readTremoloBar(NoteEffect& effect);
	void readText(Beat& beat);
	void readChord(std::vector<GuitarString>& strings, Beat& beat);
	double getTime(Duration duration);
	double readDuration(std::uint8_t flags);
	double readBeat(std::int32_t start, Measure& measure, Track& track, Tempo& tempo, std::size_t voiceIndex);
	Note readNote(GuitarString& string, Track& track, NoteEffect& effect);
	std::int8_t getTiedNoteValue(std::int32_t string, Track& track);
	void readNoteEffects(NoteEffect& noteEffect);
	void readBend(NoteEffect& effect);
	void readGrace(NoteEffect& effect);
	void readTremoloPicking(NoteEffect& effect);
	void readArtificialHarmonic(NoteEffect& effect);
	void readTrill(NoteEffect& effect);
	bool isPercussionChannel(std::int32_t channelId);
	std::string getClef(Track& track);
};

std::int32_t numOfDigits(std::int32_t num);
Duration denominatorToDuration(Denominator& denominator);
void addSpacingToXML(std::ostringstream& outputStream, std::int32_t indentLevel);

}

#endif
