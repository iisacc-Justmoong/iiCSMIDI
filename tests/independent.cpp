#include <MidiDocument.h>
#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>

using namespace iiCSMIDI;
namespace {
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
template<class F> bool fails(F action) { try { action(); } catch (const std::exception&) { return true; } return false; }
class FakeAuthor final : public Authorship {
    std::string value_;
public:
    FakeAuthor() = default;
    explicit FakeAuthor(std::string value) : value_(std::move(value)) {}
    std::unique_ptr<Authorship> clone() const override { return std::make_unique<FakeAuthor>(*this); }
    std::string_view dump() const noexcept override { return value_; }
    bool isEmpty() const noexcept override { return value_.empty(); }
    bool setAuthor(std::string_view profile) override {
        if (profile!="fixture") throw std::invalid_argument("bad author");
        if (!value_.empty()) return false;
        value_="fixture:1"; return true;
    }
    void recordChange() override { if (!value_.empty()) value_="fixture:2"; }
};
class FakeFactory final : public AuthorshipFactory {
public:
    std::unique_ptr<Authorship> empty() const override { return std::make_unique<FakeAuthor>(); }
    std::unique_ptr<Authorship> fromDump(std::string_view data) const override {
        if (data!="fixture:1" && data!="fixture:2") return {};
        return std::make_unique<FakeAuthor>(std::string(data));
    }
};
class AlteringFactory final : public AuthorshipFactory {
public:
    std::unique_ptr<Authorship> empty() const override { return std::make_unique<FakeAuthor>(); }
    std::unique_ptr<Authorship> fromDump(std::string_view) const override {
        return std::make_unique<FakeAuthor>("fixture:2");
    }
};
class MemoryStorage final : public MidiStorage {
public:
    std::map<std::filesystem::path,Bytes> files;
    void create(const std::filesystem::path& path,ByteView bytes) override {
        if (!files.emplace(path,Bytes(bytes.begin(),bytes.end())).second) throw std::runtime_error("exists");
    }
    Bytes read(const std::filesystem::path& path,std::size_t maximum) override {
        auto data=files.at(path); if (data.size()>maximum) throw std::runtime_error("too large"); return data;
    }
    void update(const std::filesystem::path& path,ByteView expected,ByteView replacement) override {
        auto& data=files.at(path);
        if (!std::ranges::equal(data,expected)) throw std::runtime_error("conflict");
        data.assign(replacement.begin(),replacement.end());
    }
};
}
int main() try {
    auto factory=std::make_shared<FakeFactory>();
    MidiDocument plain;
    const auto empty=plain.toBytes();
    const auto plainRoundtrip=MidiDocument::fromBytes(empty);
    check(plainRoundtrip.midiData()==empty,"plain MIDI must not require any adapter");
    check(fails([&]{plain.setFileAuthor("fixture");}),"missing author factory rejected");
    check(fails([&]{(void)MidiFile::create("missing.mid",{});}),"missing storage rejected");
    MidiDocument document(factory);
    check(document.setFileAuthor("fixture"),"author accepted");
    check(!document.setFileAuthor("fixture"),"repeated author is a no-op");
    const auto authored=document.toBytes();
    check(fails([&]{(void)MidiDocument::fromBytes(authored);}),"authored metadata requires a validator");
    auto restored=MidiDocument::fromBytes(authored,factory);
    check(restored.toBytes()==authored && restored.authorship()->dump()=="fixture:1","authored round trip");
    check(fails([&]{(void)MidiDocument::fromBytes(authored,std::make_shared<AlteringFactory>());}),
          "validator cannot silently alter imported authorship");
    auto storage=std::make_shared<MemoryStorage>();
    auto file=MidiFile::create("synthetic.mid",storage,document);
    check(fails([&]{(void)MidiFile::create("synthetic.mid",storage);}),"exclusive creation");
    auto changed=empty;
    changed[12]=std::byte{0x01}; changed[13]=std::byte{0xe1};
    check(!file.edit([&](MidiDocument& draft){draft.setMidiData(changed);return false;}),"rejected draft");
    check(file.document().midiData()==empty && file.document().authorship()->dump()=="fixture:1","rejected draft preserves state");
    check(file.edit([&](MidiDocument& draft){return draft.setMidiData(changed);}),"committed edit");
    auto read=MidiFile::open("synthetic.mid",storage,factory);
    check(read.document().midiData()==changed && read.document().authorship()->dump()=="fixture:2","storage and author commit");
    auto moved=std::move(read);
    check(moved.document().midiData()==changed && fails([&]{read.edit([](MidiDocument&){return true;});}),
          "moved-from file cannot edit");
    check(fails([&]{file.edit([&](MidiDocument&){return file.edit([](MidiDocument&){return true;});});}),"nested edit rejected");
    storage->files[file.path()]={std::byte{'x'}};
    check(fails([&]{file.edit([&](MidiDocument& draft){return draft.setMidiData(empty);});}),"external conflict rejected");
    check(file.document().midiData()==changed && storage->files[file.path()]==Bytes{std::byte{'x'}},"conflict preserves both states");
    for (std::size_t i=0;i<empty.size();++i)
        check(fails([&]{(void)MidiDocument::fromBytes(ByteView(empty).first(i));}),"truncated MIDI rejected");
    std::cout << "independent MIDI and injected storage contracts passed\n";
    return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
