#include <iiCSMIDI.h>
#include <iiFileProviderPresentation.h>
#include "FileProviderAdapter.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace iiCSMIDI;
using namespace iiFileProvider::native;
using Bytes = iiCSMIDI::Bytes;
namespace {
int checks=0;
void check(bool value,const char* message) { ++checks; if (!value) throw std::runtime_error(message); }
Bytes hex(std::string_view input) {
    Bytes out;
    auto digit=[](char c)->unsigned { if(c>='0'&&c<='9') return unsigned(c-'0'); if(c>='a'&&c<='f') return unsigned(c-'a'+10); throw std::runtime_error("fixture hex"); };
    check(input.size()%2==0,"complete hex fixture");
    for (std::size_t i=0;i<input.size();i+=2) out.push_back(std::byte((digit(input[i])<<4)|digit(input[i+1])));
    return out;
}
std::string text(const Bytes& bytes) { return {reinterpret_cast<const char*>(bytes.data()),bytes.size()}; }
template<class F> bool invalid(F work) { try { work(); } catch (const std::invalid_argument &) { return true; } return false; }
}
int main(int argc,char** argv) try {
    auto factory=std::make_shared<midi_file::NativeAuthorshipFactory>();
    auto storage=std::make_shared<midi_file::NativeStorage>();
    auto parse=[&](ByteView bytes){return MidiDocument::fromBytes(bytes,factory);};
    check(!factory->fromDump("{\"revision\":1,\"revision\":2}"),"duplicate JSON keys rejected");
    auto author=FileAuthor::fromIisaccAccount({{"sub","midi-author"},{"email","author@example.com"},
        {"displayName","MIDI Author"},{"userId","@midi_author"},{"societyCloudMembership","Free"}},
        "https://iisacc.com","2026-10-14T10:00:00.000Z");
    check(bool(author),"valid account");
    MidiDocument document(factory); check(document.setFileAuthor(author->toJsonText()),"attach author");
    const auto valid=hex("4d546864000000060000000101e04d54726b0000001100903c6483603c0000ff01017800ff2f00");
    check(document.setMidiData(valid),"music edit");
    check(midi_file::nativeAuthorship(document).revision()==2,"immediate revision");
    const auto before=midi_file::nativeAuthorship(document).dump();
    check(invalid([&]{document.setFileAuthor("{\"token\":\"secret\"}");}) &&
          midi_file::nativeAuthorship(document).dump()==before,"invalid credential-bearing profile preserves authorship");
    check(invalid([&]{(void)MidiDocument::fromBytes(document.toBytes());}),"authored MIDI requires a validator");
    check(!document.setMidiData(valid),"no-op");
    auto restored=parse(document.toBytes());
    check(restored.midiData()==valid && midi_file::nativeAuthorship(restored).dump()==before,"lossless authored round trip");
    check(before.find("token")==before.npos && !midi_file::nativeAuthorship(restored).hasActiveAuthor(),"credential-free read has no editing identity");
    check(invalid([&]{document.setMidiData(ByteView(valid).first(valid.size()-1));}) && midi_file::nativeAuthorship(document).dump()==before,"malformed input preserves state");
    auto corrupt=document.toBytes(); auto raw=text(corrupt); const auto marker=raw.find("iisacc:authorship:v1:");
    corrupt[marker+21]=std::byte{'!'}; check(invalid([&]{(void)parse(corrupt);}),"invalid base64");
    for (unsigned format:{1U,2U}) {
        auto multiple=valid; multiple[9]=std::byte(format); multiple[11]=std::byte{2};
        const auto track=hex("4d54726b0000000800f0017e00ff2f00"); multiple.insert(multiple.end(),track.begin(),track.end());
        auto typed=parse(multiple); typed.setFileAuthor(author->toJsonText());
        auto read=parse(typed.toBytes()); check(read.midiData()==multiple,"multi-track SysEx bytes preserved");
    }
    auto extended=valid; extended[7]=std::byte{8}; extended.insert(extended.begin()+14,{std::byte{'x'},std::byte{'y'}});
    extended[12]=std::byte{0xe7}; extended[13]=std::byte{40};
    auto ext=parse(extended); ext.setFileAuthor(author->toJsonText());
    auto extRead=parse(ext.toBytes()); check(extRead.midiData()==extended,"SMPTE/header extensions");
    auto unknown=document.toBytes(); unknown[marker+19]=std::byte{'9'};
    check(invalid([&]{(void)parse(unknown);}),"future metadata version");
    for (std::size_t length=0;length<valid.size();++length)
        check(invalid([&]{(void)parse(ByteView(valid).first(length));}),"every truncated file rejects");
    check(argc==2,"fixture directory required"); const auto fixtures=std::filesystem::path(argv[1]);
    const auto legacy=hex(File::read(fixtures/"legacy-qt68.smf.hex"));
    auto legacyDoc=parse(legacy);
    check(legacyDoc.toBytes()==legacy,"Qt 6.8 authored SMF is byte-identical");
    check(midi_file::nativeAuthorship(legacyDoc).dump()==File::read(fixtures/"legacy-qt68.authorship.json"),"legacy metadata bytes preserved");
    check(midi_file::nativeAuthorship(legacyDoc).firstEditor()->metadata().account.sub=="legacy-midi","legacy author schema");
    check(!midi_file::nativeAuthorship(legacyDoc).hasActiveAuthor(),"legacy data does not grant active author");
    auto moving=legacyDoc; auto moved=std::move(moving);
    check(moved.toBytes()==legacy,"moved document retains persisted bytes");
    check(invalid([&]{(void)moving.toBytes();}),"moved-from document fails without buffer access");
    check(sha256(File::read(fixtures/"qt68-journal.json"))==File::read(fixtures/"qt68-journal.sha256"),"Qt journal bytes/hash fixture preserved");
    struct VisibleDirectory final {
        std::filesystem::path value=std::filesystem::current_path()/("midi-fixture-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        VisibleDirectory() { File::createDirectories(value); }
        ~VisibleDirectory() { Presentation::cancelPending(value); std::error_code error; std::filesystem::remove_all(value,error); }
        const std::filesystem::path& path() const { return value; }
    } dir;
    auto file=MidiFile::create(dir.path()/u8"한글 🎵.mid",storage,document);
    bool rejected=false;
    try { (void)MidiFile::create(file.path(),storage); } catch (const FileError &) { rejected=true; }
    check(rejected && File::read(file.path())==text(document.toBytes()),"exclusive creation preserves existing file");
    auto changed=valid; changed[24]=std::byte{62};
    check(file.edit([&](MidiDocument& draft){draft.setMidiData(changed);return true;}),"file edit");
    auto disk=MidiFile::open(file.path(),storage,factory); check(disk.document().midiData()==changed && midi_file::nativeAuthorship(disk.document()).revision()==3,"committed before return");
    check(!file.edit([&](MidiDocument& draft){draft.setMidiData(valid);return false;}),"rejected draft");
    check(file.document().midiData()==changed,"draft rejection preserves memory");
    check(invalid([&]{file.edit([&](MidiDocument&){return file.edit([](MidiDocument&){return true;});});}),"nested edit rejected");
    rejected=false;
    try { file.edit([&](MidiDocument& draft)->bool { draft=MidiDocument(factory); throw std::runtime_error("reject draft"); }); } catch (...) { rejected=true; }
    check(rejected && midi_file::nativeAuthorship(file.document()).revision()==3,"throwing draft preserves state");
    File::write(file.path(),"external");
    rejected=false;
    try { file.edit([&](MidiDocument& draft){draft.setMidiData(valid);return true;}); } catch (const FileError &error) { rejected=error.code()==FileCode::Conflict; }
    check(rejected && midi_file::nativeAuthorship(file.document()).revision()==3 && File::read(file.path())=="external","external conflict preserves both versions");
    check(Presentation::waitForIdle(std::chrono::seconds(10)),"presentation idle");
    check(Presentation::load(file.path()).has_value(),"provider publishes file details and preview");
    check(File::remove(file.path()) && !std::filesystem::exists(file.path()),"provider deletion");
    std::cout<<checks<<" native SMF/storage/compatibility checks passed\n";
} catch (const std::exception &error) {
    std::cerr << "Authorship contract " << checks << ": " << error.what() << '\n';
    return 1;
}
