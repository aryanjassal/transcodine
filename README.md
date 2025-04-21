# What is Transcodine?

> [!WARNING] 
> This program is for educational purposes and is not production-ready!

Transcodine is a command-line application for securely storing and managing
collections of files in **encrypted-at-rest** bins, with optional compression
and multi-bin support. It’s designed to give users complete control over
sensitive data while maintaining portability and performance through a
simplified archive structure. Access to stored files requires an authenticated
and unlocked agent session, making all bin contents unreadable without explicit
authorization.

## Key Features

### Encrypted-at-Rest Bins

Each bin acts like a standalone secure archive. All contents (including headers,
metadata, and file data) are encrypted before being written to disk using a
per-bin key derived from a user-defined master password.

### Simplified TAR-like Structure

Transcodine uses a lightweight custom archive format modeled after the TAR
protocol. It supports file and directory headers, optional nesting, and
extensible metadata fields — while removing legacy TAR limitations.

### Optional Compression

Bins can be optionally compressed using a block-based streaming method, reducing
disk usage without sacrificing security. Compression occurs after headers are
constructed but before encryption.

### Multi-Bin Architecture

Users can manage multiple bins, each with its own independent encryption key.
This isolation ensures that brute-forcing or compromising one bin does not
expose others.

### Password-Based Key Derivation

Users can manage multiple bins, each with its own independent encryption key.
This isolation ensures that brute-forcing or compromising one bin does not
expose others. The password is hashed and encrypted using state-of-the-art
PBKDF2-HMAC_SHA-256 hashing algorithm to ensure resistance against all the most
common attacks.

### Unlockable Agent

The system includes a secure unlock agent that holds derived keys temporarily in
memory for authenticated sessions. Until the agent is unlocked via password
entry, all bins remain cryptographically sealed.

### Token-Based Session Unlocking

Upon successful authentication, the agent writes an unlock token (based on a
re-encrypted hash) to disk. This allows subsequent commands to verify session
state without re-requesting the password — while remaining resistant to
tampering.

### Portability

Only standard ANSI C headers were used to maximise portability. While this means
that some features were compromised, high performance and safety are guaranteed.

## Security Model

- **Encryption:** Bins are encrypted using derived keys through authenticated
  encryption primitives (e.g., ChaCha20/Poly1305).
- **Key Isolation:** Each bin receives a **unique cryptographic key**, derived
  independently using unique salts.
- **No plaintext leaks:** Even metadata like file names and sizes are encrypted.
- **Unlock tokens:** Stored tokens are protected using a one-way function to
  prevent reconstruction of the session key.
- **Brute-force resistance:** PBKDF2 iteration count is configurable to balance
  security and speed, making brute-forcing unfeasible. The inclusion of salting
  also prevents rainbow-table attacks.
- **Tamper protection:** File headers and padding are validated at read time to
  detect unauthorized modifications.

To provide maximum security for the users, the most advanced security protocols
have been selected to keep the data secure. Some of the implementations include
the following protocols.

- SHA-256
- HMAC-SHA-256
- PBKDF2-HMAC-SHA-256

## Technical Notes

- This program is designed to leverage Unix behaviour and will not compile or
  run under Windows without rewrites.
- To simplify working with heap memory, a custom implementation of buffers is
  included under `lib/`. This implementation aims to model the most basic
  features of strings in C++ or `Buffer` in Node.js runtimes. However, this
  implementation focuses more heavily on heap management than features and only
  supports basic functions like appending data or clearing the buffer.
- The password is completely managed by the unlock command handler. It handles
  reading the input and hashing it. If needed, the hashing protocol can be
  trivially updated to something more advanced.
- Unability to access platform-specific headers included with wanting the safety
  of not invoking any `system` calls led to the inability to create system
  directories. As such, all the configuration will be dumped in directories
  known to exist and being writable. In this case, it defaults to `$HOME`.
- Performance, correctness, or production-readiness were not prioritised in this
  implementation. Rather, this is for educational purposes and the aim was to
  showcase advanced encryption/compression algorithms, which I'd say this does
  pretty well.
