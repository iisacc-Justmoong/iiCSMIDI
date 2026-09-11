#include "MidiDocument.h"
#include <iiFileProvider.h>
#include <QtEndian>
#include <stdexcept>

namespace iiCSMIDI {
namespace {
const QByteArray Marker = "iisacc:authorship:v1:";
[[noreturn]] void invalid(const char *message) { throw std::invalid_argument(message); }
quint16 u16(const QByteArray &data, qsizetype offset) {
    return qFromBigEndian<quint16>(data.constData() + offset);
}
quint32 u32(const QByteArray &data, qsizetype offset) {
    return qFromBigEndian<quint32>(data.constData() + offset);
}
QByteArray be32(quint32 value) {
    QByteArray bytes(4, '\0'); qToBigEndian(value, bytes.data()); return bytes;
}
quint32 vlq(const QByteArray &data, qsizetype &offset, qsizetype end) {
    quint32 value = 0;
    for (int count = 0; count < 4; ++count) {
        if (offset >= end) invalid("truncated MIDI variable-length quantity");
        const auto byte = static_cast<quint8>(data[offset++]);
        value = (value << 7) | (byte & 0x7f);
        if (!(byte & 0x80)) return value;
    }
    invalid("MIDI variable-length quantity exceeds four bytes");
}
QByteArray encodeVlq(quint32 value) {
    QByteArray bytes(1, static_cast<char>(value & 0x7f));
    while ((value >>= 7)) bytes.prepend(static_cast<char>((value & 0x7f) | 0x80));
    return bytes;
}
struct Parsed { QByteArray music; iiFileProvider::Authorship authorship; };
Parsed parse(const QByteArray &bytes) {
    if (bytes.size() > MidiDocument::MaximumFileBytes || bytes.size() < 14 || bytes.first(4) != "MThd")
        invalid("not a supported Standard MIDI File");
    const auto headerSize = u32(bytes, 4);
    if (headerSize < 6 || headerSize > static_cast<quint64>(bytes.size()-8)) invalid("invalid MIDI header length");
    const auto format = u16(bytes, 8), tracks = u16(bytes, 10), division = u16(bytes, 12);
    if (format > 2 || !tracks || (format == 0 && tracks != 1) || !division) invalid("invalid MIDI header values");
    if (division & 0x8000) {
        const auto rate = static_cast<qint8>(division >> 8);
        if (!(division & 0xff) || (rate != -24 && rate != -25 && rate != -29 && rate != -30)) invalid("invalid SMPTE division");
    }
    Parsed parsed; parsed.music = bytes.first(8 + headerSize);
    qsizetype offset = 8 + headerSize;
    bool sawAuthor = false;
    for (quint32 track = 0; track < tracks; ++track) {
        if (bytes.size()-offset < 8 || bytes.mid(offset,4) != "MTrk") invalid("missing MIDI track chunk");
        const auto size = u32(bytes,offset+4); offset += 8;
        if (size > static_cast<quint64>(bytes.size()-offset)) invalid("truncated MIDI track");
        const auto start = offset, end = offset + size;
        qsizetype stripEnd = start;
        quint8 running = 0;
        bool ended = false;
        while (offset < end) {
            const auto eventStart = offset;
            const auto delta = vlq(bytes,offset,end);
            if (offset >= end) invalid("missing MIDI event");
            auto status = static_cast<quint8>(bytes[offset]);
            if (status & 0x80) ++offset;
            else { if (!running) invalid("missing MIDI running status"); status = running; }
            if (status >= 0x80 && status <= 0xef) {
                running = status;
                const int count = ((status & 0xf0) == 0xc0 || (status & 0xf0) == 0xd0) ? 1 : 2;
                if (end-offset < count) invalid("truncated MIDI channel event");
                for (int index = 0; index < count; ++index)
                    if (static_cast<quint8>(bytes[offset++]) & 0x80) invalid("invalid MIDI data byte");
            } else if (status == 0xff || status == 0xf0 || status == 0xf7) {
                running = 0;
                quint8 type = 0;
                if (status == 0xff) {
                    if (offset >= end) invalid("missing MIDI meta type");
                    type = static_cast<quint8>(bytes[offset++]);
                    if (type & 0x80) invalid("invalid MIDI meta type");
                }
                const auto length = vlq(bytes,offset,end);
                if (length > static_cast<quint64>(end-offset)) invalid("truncated MIDI meta/SysEx event");
                if (status == 0xff && type == 1 && bytes.mid(offset, length).startsWith("iisacc:authorship:")) {
                    if (!bytes.mid(offset,length).startsWith(Marker)) invalid("unsupported authorship event version");
                    if (sawAuthor || track != 0 || eventStart != start || delta != 0) invalid("misplaced or duplicate authorship event");
                    const auto text = bytes.mid(offset + Marker.size(), length - Marker.size());
                    if (text.size() > iiFileProvider::Authorship::MaximumBytes * 2) invalid("oversized authorship event");
                    auto decoded = QByteArray::fromBase64Encoding(text, QByteArray::Base64UrlEncoding | QByteArray::AbortOnBase64DecodingErrors);
                    if (!decoded || decoded.decoded.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals) != text)
                        invalid("invalid authorship base64");
                    auto authorship = iiFileProvider::Authorship::fromDump(decoded.decoded);
                    if (!authorship) invalid("invalid authorship metadata");
                    parsed.authorship = std::move(*authorship); sawAuthor = true; stripEnd = offset + length;
                }
                offset += length;
                if (status == 0xff && type == 0x2f) {
                    if (length != 0 || offset != end) invalid("invalid end-of-track event");
                    ended = true;
                }
            } else invalid("unsupported MIDI status in track");
        }
        if (!ended) invalid("MIDI track lacks end-of-track");
        parsed.music += "MTrk"; parsed.music += be32(static_cast<quint32>(end-stripEnd));
        parsed.music += bytes.mid(stripEnd,end-stripEnd);
    }
    if (offset != bytes.size()) invalid("unexpected bytes after MIDI tracks");
    return parsed;
}
QByteArray read(const QString &path) {
    return iiFileProvider::File::read(path, MidiDocument::MaximumFileBytes);
}
}
MidiDocument::MidiDocument()
    : m_midi(QByteArray::fromHex("4d546864000000060000000101e04d54726b0000000400ff2f00")) {}
MidiDocument MidiDocument::fromBytes(const QByteArray &bytes) {
    auto parsed = parse(bytes); MidiDocument document;
    document.m_midi = std::move(parsed.music); document.m_authorship = std::move(parsed.authorship);
    return document;
}
const QByteArray &MidiDocument::midiData() const noexcept { return m_midi; }
const iiFileProvider::Authorship &MidiDocument::authorship() const noexcept { return m_authorship; }
bool MidiDocument::setMidiData(const QByteArray &bytes) {
    auto parsed = parse(bytes);
    if (parsed.music == m_midi) return false;
    auto authorship = m_authorship; authorship.recordChange();
    // Validate the final embedded size before committing either part.
    auto next = *this; next.m_midi = std::move(parsed.music); next.m_authorship = std::move(authorship);
    (void)next.toBytes(); *this = std::move(next); return true;
}
bool MidiDocument::setFileAuthor(const iiFileProvider::FileAuthor &author) {
    auto next = *this; const bool changed = next.m_authorship.setAuthor(author);
    (void)next.toBytes(); *this = std::move(next); return changed;
}
QByteArray MidiDocument::toBytes() const {
    if (m_authorship.isEmpty()) return m_midi;
    const auto text = Marker + m_authorship.dump().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    const auto event = QByteArray::fromHex("00ff01") + encodeVlq(static_cast<quint32>(text.size())) + text;
    if (m_midi.size()+event.size() > MaximumFileBytes) throw std::length_error("authored MIDI file exceeds size limit");
    const auto track = 8 + u32(m_midi,4);
    return m_midi.first(track+4) + be32(u32(m_midi,track+4) + static_cast<quint32>(event.size())) + event + m_midi.mid(track+8);
}
MidiFile MidiFile::create(const QString &path, const MidiDocument &document) {
    MidiFile result; result.m_path = iiFileProvider::File::absolutePath(path); result.m_document = document;
    result.m_committed = document.toBytes();
    iiFileProvider::File::create(result.m_path, result.m_committed);
    return result;
}
MidiFile MidiFile::open(const QString &path) {
    MidiFile result; result.m_path = iiFileProvider::File::absolutePath(path); result.m_committed = read(result.m_path);
    result.m_document = MidiDocument::fromBytes(result.m_committed); return result;
}
const MidiDocument &MidiFile::document() const noexcept { return m_document; }
const QString &MidiFile::path() const noexcept { return m_path; }
bool MidiFile::edit(const std::function<bool(MidiDocument &)> &callback) {
    if (!callback || m_editing) throw std::invalid_argument("empty or nested MIDI edit");
    struct Guard { bool &value; Guard(bool &v) : value(v) { value = true; } ~Guard() { value = false; } } guard(m_editing);
    auto draft = m_document;
    if (!callback(draft)) return false;
    auto bytes = draft.toBytes();
    if (bytes == m_committed) { m_document = std::move(draft); return false; }
    iiFileProvider::File::update(m_path, m_committed, bytes);
    m_document = std::move(draft); m_committed = std::move(bytes); return true;
}
} // namespace iiCSMIDI
