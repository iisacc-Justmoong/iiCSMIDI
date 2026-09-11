#include <iiCSMIDI.h>
#include <iiFileProvider.h>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>
#include <cstdlib>
using namespace iiCSMIDI;
static void check(bool value, const char* message) { if (!value) { std::cerr << message << '\n'; std::exit(1); } }
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    auto author = iiFileProvider::FileAuthor::fromIisaccAccount(
        QJsonObject{{"sub", "midi-author"}, {"email", "author@example.com"},
        {"displayName", "MIDI Author"}, {"userId", "@midi_author"},
        {"societyCloudMembership", "Free"}, {"avatarUrl", QJsonValue::Null}},
        QUrl("https://iisacc.com"), QDateTime::currentDateTimeUtc());
    check(author.has_value(), "valid account");
    const auto now = QDateTime::currentDateTimeUtc();
    iiFileProvider::AuthenticationTokenInfo info;
    info.subject="midi-author"; info.serviceOrigin=QUrl("https://iisacc.com");
    info.sessionId=QString(32,'a'); info.issuedAt=now; info.expiresAt=now.addSecs(3600);
    const auto token = iiFileProvider::AuthenticationToken::create(info,QByteArray(43,'s'));
    check(token && author->setAuthenticationToken(*token), "runtime token fixture");
    MidiDocument document;
    check(document.setFileAuthor(*author), "attach author");
    const QByteArray music = QByteArray::fromHex("4d546864000000060000000101e04d54726b0000001100903c6483603c0000ff01017800ff2f00");
    // Track contains note-on, running-status note-off and an unrelated text event.
    const QByteArray valid = music;
    check(document.setMidiData(valid), "music edit");
    check(document.authorship().revision() == 2, "one immediate author revision per change");
    const auto before = document.authorship().dump();
    check(!document.setMidiData(valid), "no-op MIDI replacement");
    auto restored = MidiDocument::fromBytes(document.toBytes());
    check(restored.midiData() == valid && restored.authorship().dump() == before, "lossless SMF and author round trip");
    check(!restored.authorship().dump().contains("token") && !restored.authorship().dump().contains(QByteArray(43,'s')), "credentials are excluded from persisted metadata");
    check(!restored.authorship().hasActiveAuthor(), "reading metadata cannot select the editing identity");
    bool rejected = false;
    try { document.setMidiData(valid.left(valid.size()-1)); } catch (const std::invalid_argument &) { rejected = true; }
    check(rejected && document.authorship().dump() == before, "malformed MIDI cannot change metadata");
    auto corrupt = document.toBytes(); corrupt[corrupt.indexOf("iisacc:authorship:v1:") + 20] = '!';
    rejected = false;
    try { (void)MidiDocument::fromBytes(corrupt); } catch (const std::invalid_argument &) { rejected = true; }
    check(rejected, "corrupt authorship fails closed");
    for (const char format : {char(1),char(2)}) {
        auto multiple = valid; multiple[9]=format; multiple[11]=2;
        multiple += QByteArray::fromHex("4d54726b0000000800f0017e00ff2f00");
        auto typed = MidiDocument::fromBytes(multiple); typed.setFileAuthor(*author);
        auto typedRead = MidiDocument::fromBytes(typed.toBytes());
        check(typedRead.midiData()==multiple, "multi-track formats preserve SysEx and event bytes");
    }
    auto extended = valid; extended[7]=8; extended.insert(14,"xy");
    extended[12]=static_cast<char>(0xe7); extended[13]=40; // -25 fps, 40 ticks/frame.
    auto extendedDoc = MidiDocument::fromBytes(extended); extendedDoc.setFileAuthor(*author);
    check(MidiDocument::fromBytes(extendedDoc.toBytes()).midiData()==extended, "header extensions and SMPTE timing survive");
    auto unknown = document.toBytes(); unknown.replace("iisacc:authorship:v1:","iisacc:authorship:v9:");
    rejected=false;
    try { (void)MidiDocument::fromBytes(unknown); } catch (const std::invalid_argument &) { rejected=true; }
    check(rejected,"future metadata versions fail closed");
    QTemporaryDir dir(QDir::currentPath()+"/authorship-XXXXXX"); check(dir.isValid(), "temporary dir");
    auto file = MidiFile::create(dir.path()+"/composition.mid", document);
    const auto readDisk = [&] { QFile input(file.path()); check(input.open(QIODevice::ReadOnly), "read disk"); return MidiDocument::fromBytes(input.readAll()); };
    auto changed = valid; changed[24] = 62;
    check(file.edit([&](MidiDocument &draft) { draft.setMidiData(changed); return true; }), "file edit");
    check(readDisk().authorship().revision() == 3 && readDisk().midiData() == changed, "disk dump before edit returns");
    check(!file.edit([&](MidiDocument &draft) { draft.setMidiData(valid); return false; }), "rejected draft");
    check(readDisk().authorship().revision() == 3 && file.document().midiData() == changed, "atomic rejection");
    QFile conflict(file.path()); check(conflict.open(QIODevice::Append), "external writer"); conflict.write("x"); conflict.close();
    rejected = false;
    try { file.edit([&](MidiDocument &draft) { draft.setMidiData(valid); return true; }); } catch (const std::runtime_error &) { rejected = true; }
    check(rejected && file.document().authorship().revision() == 3, "external changes fail without advancing live state");
    check(iiFileProvider::File::remove(file.path()), "provider deletes a MIDI document");
    check(!QFile::exists(file.path()), "MIDI file is removed");
    return 0;
}
