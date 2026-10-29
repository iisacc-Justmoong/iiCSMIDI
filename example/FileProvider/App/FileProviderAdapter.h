#pragma once
#include <MidiDocument.h>
#include <iiFileProviderNative.h>
#include <iiJson/Json.h>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace midi_file {
class NativeAuthorship final : public iiCSMIDI::Authorship {
public:
    NativeAuthorship() = default;
    explicit NativeAuthorship(iiFileProvider::native::Authorship value) : value_(std::move(value)) {}
    std::unique_ptr<iiCSMIDI::Authorship> clone() const override { return std::make_unique<NativeAuthorship>(*this); }
    std::string_view dump() const noexcept override { return value_.dump(); }
    bool isEmpty() const noexcept override { return value_.isEmpty(); }
    bool setAuthor(std::string_view profile) override {
        auto author = iiFileProvider::native::FileAuthor::fromJsonText(profile);
        if (!author) throw std::invalid_argument("invalid credential-free author profile");
        return value_.setAuthor(*author);
    }
    void recordChange() override { value_.recordChange(); }
    const iiFileProvider::native::Authorship& native() const noexcept { return value_; }
private:
    iiFileProvider::native::Authorship value_;
};
class NativeAuthorshipFactory final : public iiCSMIDI::AuthorshipFactory {
public:
    std::unique_ptr<iiCSMIDI::Authorship> empty() const override { return std::make_unique<NativeAuthorship>(); }
    std::unique_ptr<iiCSMIDI::Authorship> fromDump(std::string_view data) const override {
        ii::json::Limits limits; limits.input_bytes=iiCSMIDI::MidiDocument::MaximumAuthorshipBytes;
        if (!ii::json::parse(data,limits)) return {};
        auto value=iiFileProvider::native::Authorship::fromDump(data);
        return value ? std::make_unique<NativeAuthorship>(std::move(*value)) : nullptr;
    }
};
class NativeStorage final : public iiCSMIDI::MidiStorage {
    static std::string_view text(iiCSMIDI::ByteView bytes) {
        return {reinterpret_cast<const char*>(bytes.data()),bytes.size()};
    }
public:
    void create(const std::filesystem::path& path, iiCSMIDI::ByteView bytes) override {
        iiFileProvider::native::File::create(path,text(bytes));
    }
    iiCSMIDI::Bytes read(const std::filesystem::path& path, std::size_t maximum) override {
        const auto data=iiFileProvider::native::File::read(path,maximum);
        iiCSMIDI::Bytes result(data.size());
        if (!data.empty()) std::memcpy(result.data(),data.data(),data.size());
        return result;
    }
    void update(const std::filesystem::path& path, iiCSMIDI::ByteView expected,
                iiCSMIDI::ByteView replacement) override {
        iiFileProvider::native::File::update(path,text(expected),text(replacement));
    }
};
inline const iiFileProvider::native::Authorship& nativeAuthorship(const iiCSMIDI::MidiDocument& document) {
    const auto* state=dynamic_cast<const NativeAuthorship*>(document.authorship());
    if (!state) throw std::invalid_argument("MIDI document has no native authorship state");
    return state->native();
}
} // namespace midi_file
