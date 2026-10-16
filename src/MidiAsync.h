#pragma once
#include "MidiDocument.h"
#include <iiSDKCore/Runtime/Operation.h>

namespace iiCSMIDI {
// Owns input bytes across asynchronous admission. The parser is CPU work, so it
// uses existing Runtime workers; no detached or per-document thread is created.
template<ii::runtime::TaskExecutor Executor>
[[nodiscard]] ii::runtime::Operation<MidiDocument> decodeAsync(
    const ii::runtime::OperationOwner& owner,Executor& executor,ii::core::Bytes bytes,
    ii::runtime::OperationOptions options={}) {
    return owner.submit<MidiDocument>(executor,[bytes=std::move(bytes)](ii::runtime::WorkContext& context) -> ii::core::Result<MidiDocument> {
        if (auto ready=context.checkpoint();!ready) return std::unexpected(ready.error());
        auto result=MidiDocument::fromBytes(bytes);
        if (auto ready=context.checkpoint();!ready) return std::unexpected(ready.error());
        return result;
    },options);
}
} // namespace iiCSMIDI
