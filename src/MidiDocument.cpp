#include "MidiDocument.h"
#include <iiJson/Json.h>
#include <algorithm>
#include <stdexcept>

namespace iiCSMIDI {
namespace {
using ii::core::ByteView;
using ii::core::Bytes;
using namespace iiFileProvider::native;
constexpr std::string_view Marker="iisacc:authorship:v1:";
std::string_view text(ByteView bytes) { return {reinterpret_cast<const char*>(bytes.data()),bytes.size()}; }
Bytes owned(std::string_view bytes) { const auto view=ii::core::byte_view(bytes); return {view.begin(),view.end()}; }
[[noreturn]] void invalid(const char* message) { throw std::invalid_argument(message); }
std::uint16_t u16(std::string_view bytes,std::size_t offset) {
    return std::uint16_t((std::uint16_t(static_cast<unsigned char>(bytes[offset]))<<8)|static_cast<unsigned char>(bytes[offset+1]));
}
std::uint32_t u32(std::string_view bytes,std::size_t offset) {
    return (std::uint32_t(u16(bytes,offset))<<16)|u16(bytes,offset+2);
}
std::string be32(std::uint32_t value) {
    std::string out(4,'\0');
    for (int i=3;i>=0;--i) { out[std::size_t(i)]=char(value&255); value>>=8; }
    return out;
}
std::uint32_t vlq(std::string_view data,std::size_t& offset,std::size_t end) {
    std::uint32_t value=0;
    for (int count=0;count<4;++count) {
        if (offset>=end) invalid("truncated MIDI variable-length quantity");
        const auto byte=static_cast<unsigned char>(data[offset++]); value=(value<<7)|(byte&127);
        if (!(byte&128)) return value;
    }
    invalid("MIDI variable-length quantity exceeds four bytes");
}
std::string encodeVlq(std::uint32_t value) {
    std::string out(1,char(value&127));
    while ((value>>=7)) out.insert(out.begin(),char((value&127)|128));
    return out;
}
std::string base64(std::string_view bytes) {
    constexpr char digits[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string out; unsigned buffer=0,bits=0;
    for (unsigned char byte:bytes) {
        buffer=(buffer<<8)|byte; bits+=8;
        while (bits>=6) { bits-=6; out.push_back(digits[(buffer>>bits)&63]); }
    }
    if (bits) out.push_back(digits[(buffer<<(6-bits))&63]);
    return out;
}
std::string unbase64(std::string_view encoded) {
    if (encoded.size()>Authorship::MaximumBytes*2) invalid("oversized authorship event");
    constexpr std::string_view digits="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string out; unsigned buffer=0,bits=0;
    for (char byte:encoded) {
        const auto digit=digits.find(byte); if (digit==digits.npos) invalid("invalid authorship base64");
        buffer=(buffer<<6)|unsigned(digit); bits+=6;
        if (bits>=8) { bits-=8; out.push_back(char((buffer>>bits)&255)); }
    }
    if (base64(out)!=encoded) invalid("noncanonical authorship base64");
    return out;
}
struct Parsed final { std::string music; Authorship authorship; };
Parsed parse(std::string_view bytes) {
    if (bytes.size()>MidiDocument::MaximumFileBytes || bytes.size()<14 || !bytes.starts_with("MThd"))
        invalid("not a supported Standard MIDI File");
    const auto headerSize=u32(bytes,4);
    if (headerSize<6 || headerSize>bytes.size()-8) invalid("invalid MIDI header length");
    const auto format=u16(bytes,8),tracks=u16(bytes,10),division=u16(bytes,12);
    if (format>2 || !tracks || (format==0 && tracks!=1) || !division) invalid("invalid MIDI header values");
    if (division&0x8000) {
        const auto rate=int(division>>8)-256;
        if (!(division&255) || (rate!=-24 && rate!=-25 && rate!=-29 && rate!=-30)) invalid("invalid SMPTE division");
    }
    Parsed parsed; parsed.music=bytes.substr(0,8+headerSize);
    std::size_t offset=8+headerSize; bool sawAuthor=false;
    for (std::uint32_t track=0;track<tracks;++track) {
        if (bytes.size()-offset<8 || bytes.substr(offset,4)!="MTrk") invalid("missing MIDI track chunk");
        const auto size=u32(bytes,offset+4); offset+=8;
        if (size>bytes.size()-offset) invalid("truncated MIDI track");
        const auto start=offset,end=offset+size; auto stripEnd=start;
        unsigned char running=0; bool ended=false;
        while (offset<end) {
            const auto eventStart=offset,delta=std::size_t(vlq(bytes,offset,end));
            if (offset>=end) invalid("missing MIDI event");
            auto status=static_cast<unsigned char>(bytes[offset]);
            if (status&128) ++offset;
            else { if (!running) invalid("missing MIDI running status"); status=running; }
            if (status>=0x80 && status<=0xef) {
                running=status;
                const unsigned count=((status&0xf0)==0xc0 || (status&0xf0)==0xd0) ? 1 : 2;
                if (end-offset<count) invalid("truncated MIDI channel event");
                for (unsigned index=0;index<count;++index)
                    if (static_cast<unsigned char>(bytes[offset++])&128) invalid("invalid MIDI data byte");
            } else if (status==0xff || status==0xf0 || status==0xf7) {
                running=0; unsigned char type=0;
                if (status==0xff) {
                    if (offset>=end) invalid("missing MIDI meta type");
                    type=static_cast<unsigned char>(bytes[offset++]);
                    if (type&128) invalid("invalid MIDI meta type");
                }
                const auto length=vlq(bytes,offset,end);
                if (length>end-offset) invalid("truncated MIDI meta/SysEx event");
                const auto payload=bytes.substr(offset,length);
                if (status==0xff && type==1 && payload.starts_with("iisacc:authorship:")) {
                    if (!payload.starts_with(Marker)) invalid("unsupported authorship event version");
                    if (sawAuthor || track!=0 || eventStart!=start || delta!=0) invalid("misplaced or duplicate authorship event");
                    const auto json=unbase64(payload.substr(Marker.size()));
                    ii::json::Limits limits; limits.input_bytes=Authorship::MaximumBytes;
                    if (!ii::json::parse(json,limits)) invalid("invalid authorship JSON boundary");
                    auto authorship=Authorship::fromDump(json);
                    if (!authorship) invalid("invalid authorship metadata");
                    parsed.authorship=std::move(*authorship); sawAuthor=true; stripEnd=offset+length;
                }
                offset+=length;
                if (status==0xff && type==0x2f) {
                    if (length!=0 || offset!=end) invalid("invalid end-of-track event");
                    ended=true;
                }
            } else invalid("unsupported MIDI status in track");
        }
        if (!ended) invalid("MIDI track lacks end-of-track");
        parsed.music+="MTrk"; parsed.music+=be32(std::uint32_t(end-stripEnd)); parsed.music+=bytes.substr(stripEnd,end-stripEnd);
    }
    if (offset!=bytes.size()) invalid("unexpected bytes after MIDI tracks");
    return parsed;
}
}
MidiDocument::MidiDocument() {
    constexpr unsigned char empty[]{0x4d,0x54,0x68,0x64,0,0,0,6,0,0,0,1,1,0xe0,0x4d,0x54,0x72,0x6b,0,0,0,4,0,0xff,0x2f,0};
    for (auto byte:empty) m_midi.push_back(std::byte{byte});
}
MidiDocument MidiDocument::fromBytes(ByteView bytes) {
    auto parsed=parse(text(bytes)); MidiDocument document;
    document.m_midi=owned(parsed.music); document.m_authorship=std::move(parsed.authorship); return document;
}
const Bytes& MidiDocument::midiData() const & noexcept { return m_midi; }
const Authorship& MidiDocument::authorship() const & noexcept { return m_authorship; }
bool MidiDocument::setMidiData(ByteView bytes) {
    auto parsed=parse(text(bytes));
    if (text(m_midi)==parsed.music) return false;
    auto next=*this; next.m_midi=owned(parsed.music); next.m_authorship.recordChange();
    (void)next.toBytes(); *this=std::move(next); return true;
}
bool MidiDocument::setFileAuthor(const FileAuthor& author) {
    auto next=*this; const bool changed=next.m_authorship.setAuthor(author);
    (void)next.toBytes(); *this=std::move(next); return changed;
}
Bytes MidiDocument::toBytes() const {
    if (m_midi.size()<14) invalid("cannot serialize a moved-from MIDI document");
    if (m_authorship.isEmpty()) return m_midi;
    const auto payload=std::string(Marker)+base64(m_authorship.dump());
    const auto event=std::string("\0\xff\x01",3)+encodeVlq(std::uint32_t(payload.size()))+payload;
    if (m_midi.size()+event.size()>MaximumFileBytes) throw std::length_error("authored MIDI file exceeds size limit");
    const auto midi=text(m_midi); const auto track=8+u32(midi,4);
    return owned(std::string(midi.substr(0,track+4))+be32(u32(midi,track+4)+std::uint32_t(event.size()))+event+std::string(midi.substr(track+8)));
}
MidiFile MidiFile::create(const std::filesystem::path& path,const MidiDocument& document) {
    MidiFile result; result.m_path=std::filesystem::absolute(path); result.m_document=document; result.m_committed=document.toBytes();
    File::create(result.m_path,text(result.m_committed)); return result;
}
MidiFile MidiFile::open(const std::filesystem::path& path) {
    MidiFile result; result.m_path=std::filesystem::absolute(path);
    result.m_committed=owned(File::read(result.m_path,MidiDocument::MaximumFileBytes));
    result.m_document=MidiDocument::fromBytes(result.m_committed); return result;
}
const MidiDocument& MidiFile::document() const & noexcept { return m_document; }
const std::filesystem::path& MidiFile::path() const & noexcept { return m_path; }
bool MidiFile::edit(const std::function<bool(MidiDocument&)>& callback) {
    if (!callback || m_editing) throw std::invalid_argument("empty or nested MIDI edit");
    struct Guard final { bool& value; explicit Guard(bool& flag):value(flag){value=true;} ~Guard(){value=false;} } guard(m_editing);
    auto draft=m_document;
    if (!callback(draft)) return false;
    auto bytes=draft.toBytes();
    if (bytes==m_committed) { m_document=std::move(draft); return false; }
    File::update(m_path,text(m_committed),text(bytes));
    m_document=std::move(draft); m_committed=std::move(bytes); return true;
}
} // namespace iiCSMIDI
