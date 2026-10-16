#include <MidiAsync.h>
#include <iostream>
#include <stdexcept>
using namespace ii::runtime;
int main() {
    Runtime runtime({2,16});
    SerialQueue receiverQueue(runtime);
    TaskScope receiver;
    OperationOwner owner;
    iiCSMIDI::MidiDocument empty;
    auto input=empty.toBytes();
    auto parsed=iiCSMIDI::decodeAsync(owner,runtime,input);
    input.clear(); // asynchronous payload owns its copy
    int delivered=0;
    if (!parsed.then(receiverQueue,receiver.sender(),[&](auto result) {
        if (!receiverQueue.is_current() || result->status!=OperationStatus::succeeded || result->value->midiData()!=empty.midiData())
            throw std::runtime_error("native MIDI reply execution context");
        ++delivered;
    })) return 1;
    if (parsed.wait_for(std::chrono::seconds(5))!=OperationWait::ready || !runtime.wait_idle() || delivered!=1) return 2;
    auto invalid=iiCSMIDI::decodeAsync(owner,runtime,{});
    if (invalid.wait_for(std::chrono::seconds(5))!=OperationWait::ready || invalid.result()->status!=OperationStatus::failed || !invalid.result()->error->cause) return 3;
    EventLoop host;
    auto cancelled=iiCSMIDI::decodeAsync(owner,host,empty.toBytes());
    cancelled.cancel();
    if (!cancelled.result() || cancelled.result()->status!=OperationStatus::cancelled) return 4;
    auto expired=iiCSMIDI::decodeAsync(owner,host,empty.toBytes(),{ii::core::Deadline::after(std::chrono::seconds(0))});
    host.poll();
    if (expired.result()->status!=OperationStatus::timed_out || expired.result()->effects!=SideEffects::none) return 5;
    if (!owner.close() || !receiver.close() || !receiverQueue.close() || !runtime.join()) return 6;
    std::cout << "native asynchronous MIDI decode and reply lifetime checks passed\n";
}
