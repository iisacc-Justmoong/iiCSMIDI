#include <iiCSMIDI.h>
#include <iiFileProviderPresentation.h>
#include <algorithm>
#include <iostream>
using namespace iiCSMIDI;
using namespace iiFileProvider::native;
using ii::core::Bytes;
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
int main(int argc,char** argv) {
    auto author=FileAuthor::fromIisaccAccount({{"sub","midi-author"},{"email","author@example.com"},
        {"displayName","MIDI Author"},{"userId","@midi_author"},{"societyCloudMembership","Free"}},
        "https://iisacc.com","2026-10-14T10:00:00.000Z");
    check(bool(author),"valid account");
    MidiDocument document; check(document.setFileAuthor(*author),"attach author");
    const auto valid=hex("4d546864000000060000000101e04d54726b0000001100903c6483603c0000ff01017800ff2f00");
    check(document.setMidiData(valid),"music edit");
    check(document.authorship().revision()==2,"immediate revision");
    const auto before=document.authorship().dump();
    check(!document.setMidiData(valid),"no-op");
    auto restored=MidiDocument::fromBytes(document.toBytes());
    check(restored.midiData()==valid && restored.authorship().dump()==before,"lossless authored round trip");
    check(before.find("token")==before.npos && !restored.authorship().hasActiveAuthor(),"credential-free read has no editing identity");
    check(invalid([&]{document.setMidiData(ii::core::ByteView(valid).first(valid.size()-1));}) && document.authorship().dump()==before,"malformed input preserves state");
    auto corrupt=document.toBytes(); auto raw=text(corrupt); const auto marker=raw.find("iisacc:authorship:v1:");
    corrupt[marker+21]=std::byte{'!'}; check(invalid([&]{(void)MidiDocument::fromBytes(corrupt);}),"invalid base64");
    for (unsigned format:{1U,2U}) {
        auto multiple=valid; multiple[9]=std::byte(format); multiple[11]=std::byte{2};
        const auto track=hex("4d54726b0000000800f0017e00ff2f00"); multiple.insert(multiple.end(),track.begin(),track.end());
        auto typed=MidiDocument::fromBytes(multiple); typed.setFileAuthor(*author);
        auto read=MidiDocument::fromBytes(typed.toBytes()); check(read.midiData()==multiple,"multi-track SysEx bytes preserved");
    }
    auto extended=valid; extended[7]=std::byte{8}; extended.insert(extended.begin()+14,{std::byte{'x'},std::byte{'y'}});
    extended[12]=std::byte{0xe7}; extended[13]=std::byte{40};
    auto ext=MidiDocument::fromBytes(extended); ext.setFileAuthor(*author);
    auto extRead=MidiDocument::fromBytes(ext.toBytes()); check(extRead.midiData()==extended,"SMPTE/header extensions");
    auto unknown=document.toBytes(); unknown[marker+19]=std::byte{'9'};
    check(invalid([&]{(void)MidiDocument::fromBytes(unknown);}),"future metadata version");
    for (std::size_t length=0;length<valid.size();++length)
        check(invalid([&]{(void)MidiDocument::fromBytes(ii::core::ByteView(valid).first(length));}),"every truncated file rejects");
    check(argc==2,"fixture directory required"); const auto fixtures=std::filesystem::path(argv[1]);
    const auto legacy=hex(File::read(fixtures/"legacy-qt68.smf.hex"));
    auto legacyDoc=MidiDocument::fromBytes(legacy);
    check(legacyDoc.toBytes()==legacy,"Qt 6.8 authored SMF is byte-identical");
    check(legacyDoc.authorship().dump()==File::read(fixtures/"legacy-qt68.authorship.json"),"legacy metadata bytes preserved");
    check(legacyDoc.authorship().firstEditor()->metadata().account.sub=="legacy-midi","legacy author schema");
    check(!legacyDoc.authorship().hasActiveAuthor(),"legacy data does not grant active author");
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
    auto file=MidiFile::create(dir.path()/"한글 🎵.mid",document);
    bool rejected=false;
    try { (void)MidiFile::create(file.path()); } catch (const FileError &) { rejected=true; }
    check(rejected && File::read(file.path())==text(document.toBytes()),"exclusive creation preserves existing file");
    auto changed=valid; changed[24]=std::byte{62};
    check(file.edit([&](MidiDocument& draft){draft.setMidiData(changed);return true;}),"file edit");
    auto disk=MidiFile::open(file.path()); check(disk.document().midiData()==changed && disk.document().authorship().revision()==3,"committed before return");
    check(!file.edit([&](MidiDocument& draft){draft.setMidiData(valid);return false;}),"rejected draft");
    check(file.document().midiData()==changed,"draft rejection preserves memory");
    check(invalid([&]{file.edit([&](MidiDocument&){return file.edit([](MidiDocument&){return true;});});}),"nested edit rejected");
    rejected=false;
    try { file.edit([](MidiDocument& draft)->bool { draft=MidiDocument(); throw std::runtime_error("reject draft"); }); } catch (...) { rejected=true; }
    check(rejected && file.document().authorship().revision()==3,"throwing draft preserves state");
    File::write(file.path(),"external");
    rejected=false;
    try { file.edit([&](MidiDocument& draft){draft.setMidiData(valid);return true;}); } catch (const FileError &error) { rejected=error.code()==FileCode::Conflict; }
    check(rejected && file.document().authorship().revision()==3 && File::read(file.path())=="external","external conflict preserves both versions");
    check(Presentation::waitForIdle(std::chrono::seconds(10)),"presentation idle");
    check(Presentation::load(file.path()).has_value(),"provider publishes file details and preview");
    check(File::remove(file.path()) && !std::filesystem::exists(file.path()),"provider deletion");
    std::cout<<checks<<" native SMF/storage/compatibility checks passed\n";
}
