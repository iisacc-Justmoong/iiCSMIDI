#pragma once
#include "Export.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace iiCSMIDI {
using Bytes = std::vector<std::byte>;
using ByteView = std::span<const std::byte>;

// The host owns author policy and serialization. Implementations must clone
// independent state, validate imported metadata and reject credential-bearing
// profiles according to the host's policy. The SDK never interprets credentials.
// dump() remains valid until this state is mutated or destroyed. A factory must
// preserve imported dump bytes exactly for lossless MIDI round trips.
class IICSMIDI_EXPORT Authorship {
public:
    virtual ~Authorship() = default;
    [[nodiscard]] virtual std::unique_ptr<Authorship> clone() const = 0;
    [[nodiscard]] virtual std::string_view dump() const noexcept = 0;
    [[nodiscard]] virtual bool isEmpty() const noexcept = 0;
    virtual bool setAuthor(std::string_view profile) = 0;
    virtual void recordChange() = 0;
};
class IICSMIDI_EXPORT AuthorshipFactory {
public:
    virtual ~AuthorshipFactory() = default;
    [[nodiscard]] virtual std::unique_ptr<Authorship> empty() const = 0;
    [[nodiscard]] virtual std::unique_ptr<Authorship> fromDump(std::string_view dump) const = 0;
};

// The caller supplies exclusive creation and optimistic, atomic replacement.
// A failed operation must leave existing data untouched; exceptions propagate.
class IICSMIDI_EXPORT MidiStorage {
public:
    virtual ~MidiStorage() = default;
    virtual void create(const std::filesystem::path&, ByteView bytes) = 0;
    [[nodiscard]] virtual Bytes read(const std::filesystem::path&, std::size_t maximumBytes) = 0;
    virtual void update(const std::filesystem::path&, ByteView expected, ByteView replacement) = 0;
};

class IICSMIDI_EXPORT MidiDocument final {
public:
    static constexpr std::size_t MaximumFileBytes = 64 * 1024 * 1024;
    static constexpr std::size_t MaximumAuthorshipBytes = 2 * 1024 * 1024;
    explicit MidiDocument(std::shared_ptr<const AuthorshipFactory> factory = {});
    MidiDocument(const MidiDocument&);
    MidiDocument& operator=(const MidiDocument&);
    MidiDocument(MidiDocument&&) noexcept = default;
    MidiDocument& operator=(MidiDocument&&) noexcept = default;
    ~MidiDocument() = default;
    [[nodiscard]] static MidiDocument fromBytes(ByteView bytes,
        std::shared_ptr<const AuthorshipFactory> factory = {});
    [[nodiscard]] Bytes toBytes() const;
    [[nodiscard]] const Bytes &midiData() const & noexcept;
    const Bytes &midiData() const && = delete;
    [[nodiscard]] const Authorship* authorship() const & noexcept;
    const Authorship* authorship() const && = delete;
    bool setMidiData(ByteView bytes);
    bool setFileAuthor(std::string_view profile);
private:
    Bytes m_midi;
    std::shared_ptr<const AuthorshipFactory> m_factory;
    std::unique_ptr<Authorship> m_authorship;
};

// Each accepted edit commits through the caller's storage before returning.
class IICSMIDI_EXPORT MidiFile final {
public:
    [[nodiscard]] static MidiFile create(const std::filesystem::path &path,
        std::shared_ptr<MidiStorage> storage, const MidiDocument &document = MidiDocument{});
    [[nodiscard]] static MidiFile open(const std::filesystem::path &path,
        std::shared_ptr<MidiStorage> storage, std::shared_ptr<const AuthorshipFactory> factory = {});
    MidiFile(const MidiFile &) = delete;
    MidiFile &operator=(const MidiFile &) = delete;
    MidiFile(MidiFile &&) noexcept = default;
    MidiFile &operator=(MidiFile &&) noexcept = default;
    [[nodiscard]] const MidiDocument &document() const & noexcept;
    const MidiDocument &document() const && = delete;
    [[nodiscard]] const std::filesystem::path &path() const & noexcept;
    const std::filesystem::path &path() const && = delete;
    bool edit(const std::function<bool(MidiDocument &)> &callback);
private:
    MidiFile() = default;
    std::filesystem::path m_path;
    std::shared_ptr<MidiStorage> m_storage;
    Bytes m_committed;
    MidiDocument m_document;
    bool m_editing = false;
};
} // namespace iiCSMIDI
