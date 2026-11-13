// Sounds of the game, computed in code: sound effects and the background music. Each
// function returns the contents of a WAV file, which the audio service loads like a file
// from disk.
export module yozakura.sound;

import std;

export namespace yozakura
{

/// Returns the bell played when a spirit is caught.
std::vector<std::byte> chime_wav();

/// Returns the rising run of notes played when a golden spirit is caught.
std::vector<std::byte> golden_chime_wav();

/// Returns the low sound played when a spirit is lost.
std::vector<std::byte> miss_wav();

/// Returns the background music: a slow melody that can be repeated without a break.
std::vector<std::byte> music_wav();

} // namespace yozakura

namespace yozakura
{

constexpr std::uint32_t sample_rate = 22050;
constexpr float two_pi = 2.0f * std::numbers::pi_v<float>;

// Frequencies of the notes in hertz. The melody uses a five-note Japanese scale on D
// (D, E flat, G, A, B flat), which gives it its sound.
namespace note
{

constexpr float g2 = 98.0f;
constexpr float a2 = 110.0f;
constexpr float b_flat2 = 116.54f;
constexpr float d3 = 146.83f;
constexpr float g3 = 196.0f;
constexpr float d4 = 293.66f;
constexpr float e_flat4 = 311.13f;
constexpr float g4 = 392.0f;
constexpr float a4 = 440.0f;
constexpr float b_flat4 = 466.16f;
constexpr float d5 = 587.33f;
constexpr float e_flat5 = 622.25f;
constexpr float e6 = 1318.5f;
constexpr float g_sharp6 = 1661.2f;
constexpr float b6 = 1975.5f;
constexpr float e7 = 2637.0f;
constexpr float rest = 0.0f;

} // namespace note

template <std::unsigned_integral T>
void append_number(std::vector<std::byte>& bytes, T value)
{
    // WAV files store numbers with the lowest byte first.
    for (std::size_t byte = 0; byte < sizeof(T); ++byte)
    {
        bytes.push_back(static_cast<std::byte>((value >> (8 * byte)) & 0xFF));
    }
}

void append_text(std::vector<std::byte>& bytes, std::string_view text)
{
    bytes.append_range(std::as_bytes(std::span{text}));
}

// Packs samples from -1 to 1 into a WAV file with one channel and 16 bits per sample.
std::vector<std::byte> to_wav(std::span<const float> samples)
{
    constexpr std::uint16_t channel_count = 1;
    constexpr std::uint16_t bytes_per_sample = 2;
    constexpr std::uint32_t format_chunk_size = 16;
    constexpr std::uint16_t pcm_format = 1;
    // Loudest value of a sample, a little below the limit of 16 bits.
    constexpr float full_scale = 32000.0f;

    const auto data_size = static_cast<std::uint32_t>(samples.size() * bytes_per_sample);
    // Everything after the size field itself: "WAVE", two chunk headers and their contents.
    const std::uint32_t riff_size = 4 + 8 + format_chunk_size + 8 + data_size;

    std::vector<std::byte> bytes;
    append_text(bytes, "RIFF");
    append_number(bytes, riff_size);
    append_text(bytes, "WAVE");
    append_text(bytes, "fmt ");
    append_number(bytes, format_chunk_size);
    append_number(bytes, pcm_format);
    append_number(bytes, channel_count);
    append_number(bytes, sample_rate);
    append_number(bytes, std::uint32_t{sample_rate * channel_count * bytes_per_sample});
    append_number(bytes, static_cast<std::uint16_t>(channel_count * bytes_per_sample));
    append_number(bytes, static_cast<std::uint16_t>(bytes_per_sample * 8));
    append_text(bytes, "data");
    append_number(bytes, data_size);
    for (const float sample : samples)
    {
        const auto value = static_cast<std::int16_t>(std::clamp(sample, -1.0f, 1.0f) * full_scale);
        append_number(bytes, static_cast<std::uint16_t>(value));
    }
    return bytes;
}

std::vector<float> silence(float seconds)
{
    return std::vector<float>(static_cast<std::size_t>(seconds * static_cast<float>(sample_rate)),
                              0.0f);
}

// Adds a plucked note to the samples: a tone with two overtones that starts at once and
// fades out, like a string or a small bell. A note that is still sounding at the end of the
// samples continues at their start, so music can be repeated without a click.
void add_pluck(std::vector<float>& samples, float start, float frequency, float volume,
               float fade_rate)
{
    // Overtones at two and three times the frequency make the tone less plain.
    constexpr float second_overtone = 0.4f;
    constexpr float third_overtone = 0.15f;
    // A short rise at the start instead of a jump, which would be heard as a click.
    constexpr float attack = 0.004f;
    // The note ends when it is this much quieter than at its start.
    constexpr float silence_level = 0.002f;

    if (frequency == note::rest || samples.empty())
    {
        return;
    }
    const float duration = -std::log(silence_level) / fade_rate;
    const auto first = static_cast<std::size_t>(start * static_cast<float>(sample_rate));
    const auto count = static_cast<std::size_t>(duration * static_cast<float>(sample_rate));
    for (std::size_t index = 0; index < count; ++index)
    {
        const float time = static_cast<float>(index) / static_cast<float>(sample_rate);
        const float angle = two_pi * frequency * time;
        const float tone = std::sin(angle) + second_overtone * std::sin(2.0f * angle) +
                           third_overtone * std::sin(3.0f * angle);
        const float envelope = std::min(time / attack, 1.0f) * std::exp(-fade_rate * time);
        samples[(first + index) % samples.size()] += tone * envelope * volume;
    }
}

std::vector<std::byte> chime_wav()
{
    constexpr float volume = 0.25f;
    constexpr float fade_rate = 12.0f;
    constexpr float second_note_delay = 0.06f;

    std::vector<float> samples = silence(0.5f);
    add_pluck(samples, 0.0f, note::e6, volume, fade_rate);
    add_pluck(samples, second_note_delay, note::b6, volume, fade_rate);
    return to_wav(samples);
}

std::vector<std::byte> golden_chime_wav()
{
    constexpr std::array notes{note::e6, note::g_sharp6, note::b6, note::e7};
    constexpr float volume = 0.22f;
    constexpr float fade_rate = 9.0f;
    constexpr float note_delay = 0.07f;

    std::vector<float> samples = silence(0.8f);
    for (std::size_t index = 0; index < notes.size(); ++index)
    {
        add_pluck(samples, note_delay * static_cast<float>(index), notes[index], volume, fade_rate);
    }
    return to_wav(samples);
}

std::vector<std::byte> miss_wav()
{
    constexpr float volume = 0.4f;
    constexpr float fade_rate = 7.0f;
    constexpr float second_note_delay = 0.12f;

    std::vector<float> samples = silence(0.7f);
    add_pluck(samples, 0.0f, note::g3, volume, fade_rate);
    add_pluck(samples, second_note_delay, note::d3, volume, fade_rate);
    return to_wav(samples);
}

std::vector<std::byte> music_wav()
{
    struct MelodyNote
    {
        float frequency = 0.0f;
        float beats = 0.0f;
    };
    // Two phrases of sixteen beats; the second answers the first and ends on the home note.
    constexpr std::array melody{
        MelodyNote{note::a4, 1.0f},      MelodyNote{note::b_flat4, 1.0f},
        MelodyNote{note::a4, 1.0f},      MelodyNote{note::g4, 1.0f},
        MelodyNote{note::d4, 2.0f},      MelodyNote{note::e_flat4, 1.0f},
        MelodyNote{note::g4, 1.0f},      MelodyNote{note::a4, 2.0f},
        MelodyNote{note::d5, 1.0f},      MelodyNote{note::b_flat4, 1.0f},
        MelodyNote{note::a4, 3.0f},      MelodyNote{note::rest, 1.0f},
        MelodyNote{note::d5, 1.0f},      MelodyNote{note::e_flat5, 1.0f},
        MelodyNote{note::d5, 1.0f},      MelodyNote{note::b_flat4, 1.0f},
        MelodyNote{note::a4, 2.0f},      MelodyNote{note::g4, 1.0f},
        MelodyNote{note::e_flat4, 1.0f}, MelodyNote{note::d4, 2.0f},
        MelodyNote{note::g4, 1.0f},      MelodyNote{note::a4, 1.0f},
        MelodyNote{note::d4, 3.0f},      MelodyNote{note::rest, 1.0f},
    };
    // One low note at the start of every bar of four beats.
    constexpr std::array bass{note::d3,      note::g2, note::d3, note::a2,
                              note::b_flat2, note::g2, note::a2, note::d3};
    constexpr float beats_per_bar = 4.0f;
    constexpr float seconds_per_beat = 0.8f;
    constexpr float melody_volume = 0.2f;
    constexpr float melody_fade_rate = 2.2f;
    constexpr float bass_volume = 0.16f;
    constexpr float bass_fade_rate = 1.1f;

    std::vector<float> samples =
        silence(static_cast<float>(bass.size()) * beats_per_bar * seconds_per_beat);

    float beat = 0.0f;
    for (const MelodyNote& melody_note : melody)
    {
        add_pluck(samples, beat * seconds_per_beat, melody_note.frequency, melody_volume,
                  melody_fade_rate);
        beat += melody_note.beats;
    }
    for (std::size_t bar = 0; bar < bass.size(); ++bar)
    {
        add_pluck(samples, static_cast<float>(bar) * beats_per_bar * seconds_per_beat, bass[bar],
                  bass_volume, bass_fade_rate);
    }
    return to_wav(samples);
}

} // namespace yozakura
