#pragma once
#include "Export.h"
#include <iiFileProviderNative.h>
#include <iiSDKCore/Core/Bytes/Bytes.h>
#include <functional>

namespace iiCSMIDI {
// Standard MIDI File bytes plus eagerly serialized, credential-free authorship.
class IICSMIDI_EXPORT MidiDocument final {
public:
    static constexpr std::size_t MaximumFileBytes = 64 * 1024 * 1024;
    MidiDocument();
    [[nodiscard]] static MidiDocument fromBytes(ii::core::ByteView bytes);
    [[nodiscard]] ii::core::Bytes toBytes() const;
    [[nodiscard]] const ii::core::Bytes &midiData() const & noexcept;
    const ii::core::Bytes &midiData() const && = delete;
    [[nodiscard]] const iiFileProvider::native::Authorship &authorship() const & noexcept;
    const iiFileProvider::native::Authorship &authorship() const && = delete;
    bool setMidiData(ii::core::ByteView bytes);
    bool setFileAuthor(const iiFileProvider::native::FileAuthor &author);
private:
    ii::core::Bytes m_midi;
    iiFileProvider::native::Authorship m_authorship;
};

// Every accepted edit commits an atomic file replacement before returning.
// Read-only snapshots contain no writable binding. False/throw rejects the draft.
class IICSMIDI_EXPORT MidiFile final {
public:
    [[nodiscard]] static MidiFile create(const std::filesystem::path &path, const MidiDocument &document = {});
    [[nodiscard]] static MidiFile open(const std::filesystem::path &path);
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
    ii::core::Bytes m_committed;
    MidiDocument m_document;
    bool m_editing = false;
};
} // namespace iiCSMIDI
