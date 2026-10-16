# Legacy compatibility evidence

Captured on 2026-10-16 using the existing iiCSMIDI 0.2.1 and Qt 6.8.3 packages,
before editing the native replacement. Capture source and build logs remain in
build/legacy-capture (a build artifact, not a runtime dependency).

- legacy-qt68.smf.hex: authored minimal format-0 SMF, hexadecimal bytes.
- legacy-qt68.authorship.json: exact persisted schema-3 metadata including a
  Korean/astral display name; no credentials or active editing identity.
- qt68-journal.json: representative NamespaceJournal-shaped QJsonDocument::Compact
  output, including nested objects, string counters, Unicode and escaped text.
- qt68-journal.sha256: Qt QCryptographicHash SHA-256 of those exact compact bytes.

Authorship input was a synthetic account (legacy-midi / legacy@example.com),
with capturedAt fixed to 2026-10-14T10:00:00.000Z. The edit timestamp was captured
once and is intentionally frozen. The native test compares authored SMF and
metadata bytes exactly and verifies the journal fixture's hash. The journal is
not a production database export. No universal Qt/iiJson serialization equivalence
is claimed and NamespaceJournal's live serializer was not changed.
