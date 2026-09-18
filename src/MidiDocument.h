#pragma once
#include "Export.h"
#include <iiFileProvider.h>
#include <functional>

namespace iiCSMIDI {
// Standard MIDI File bytes plus eagerly serialized, credential-free authorship.
class IICSMIDI_EXPORT MidiDocument final {
public:
    static constexpr qsizetype MaximumFileBytes = 64 * 1024 * 1024;
    MidiDocument();
    [[nodiscard]] static MidiDocument fromBytes(const QByteArray &bytes);
    [[nodiscard]] QByteArray toBytes() const;
    [[nodiscard]] const QByteArray &midiData() const noexcept;
    [[nodiscard]] const iiFileProvider::Authorship &authorship() const noexcept;
    bool setMidiData(const QByteArray &bytes);
    bool setFileAuthor(const iiFileProvider::FileAuthor &author);
private:
    QByteArray m_midi;
    iiFileProvider::Authorship m_authorship;
};

// Every accepted edit commits an atomic file replacement before returning.
// Read-only snapshots contain no writable binding. False/throw rejects the draft.
class IICSMIDI_EXPORT MidiFile final {
public:
    [[nodiscard]] static MidiFile create(const QString &path, const MidiDocument &document = {});
    [[nodiscard]] static MidiFile open(const QString &path);
    MidiFile(const MidiFile &) = delete;
    MidiFile &operator=(const MidiFile &) = delete;
    MidiFile(MidiFile &&) noexcept = default;
    MidiFile &operator=(MidiFile &&) noexcept = default;
    [[nodiscard]] const MidiDocument &document() const noexcept;
    [[nodiscard]] const QString &path() const noexcept;
    bool edit(const std::function<bool(MidiDocument &)> &callback);
private:
    MidiFile() = default;
    QString m_path;
    QByteArray m_committed;
    MidiDocument m_document;
    bool m_editing = false;
};
} // namespace iiCSMIDI
