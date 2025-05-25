# What is Transcodine?

<!-- prettier-ignore-start -->
> [!WARNING]
> This program is for educational purposes and is not production-ready!
<!-- prettier-ignore-end -->

Transcodine is a command-line application for **securely storing and managing
collections of files** in virtual file systems which are **encrypted-at-rest**,
with optional compression and multi-bin support. It’s designed to give users
complete control over sensitive data while **maintaining portability and
performance** through a simplified archive structure. Access to stored files
requires an authenticated and unlocked agent session, making all **bin contents
unreadable without explicit authorization**.

**Key features**

- **Encrypted-at-rest bins** act as a freestanding secure archive, ensuring all
  contents (including file metadata) are all encrypted.
- **Simplified archive structure** thanks to a custom archival format designed
  for transcodine ensures minimal overhead for storing data while also
  supporting arbitrarily large files and their paths.
- **Optional compression** can compress all the tracked bins into a single file,
  further reducing the disk footprint.
- **Multi-bin architecture** ensures separation of security so a single leaked
  secret cannot compromise the entire database, allowing the secrets to be
  duplicated and organised.
- **Password-based key derivation** enforces the correct password to unlock the
  database containing the decryption keys, making authentication enforced by
  design instead of programatically.
- **Up-to-date cryptography** allows for maximum security by using tested and
  hardened cryptographic algorithms known to resist all sorts of malicious
  attacks.
- **Portable ANSI C** implementation makes an easy job of using the application
  on any Unix-based systems and requires minimal dependencies to get running on
  an embedded system.
- **Bin sharing** can be done by sharing the resultant compression file along a
  secret key. Anyone who has both the secret key and the compressed bin can
  import the bins into their own agents.

## Table of Contents

- [Overview](#overview)
- [Capabilities](#capabilities)
- [Technical Details](#technical-details)
  - [Password Management](#password-management)
  - [Agent Management](#agent-management)
  - [Bin Management](#bin-management)
  - [Bin Management](#compression)
  - [Notes](#notes)
- [Credits](#credits)

## Overview

Transcodine supports a wide array of commands and features.

```bash
transcodine
├── agent
│   ├── setup
│   ├── reset
│   └── help
├── bin
│   ├── create <bin_name>
│   ├── ls
│   ├── rm <bin_name>
│   ├── save <out_path> <...bin_names>
│   ├── load <in_path>
│   ├── rename <old_name> <new_name>
│   └── help
├── file
│   ├── ls <bin_name>
│   ├── add <bin_name> <local_path> <virtual_path>
│   ├── cat <bin_name> <virtual_path>
│   ├── rm <bin_name>
│   ├── get <bin_path> <virtual_path> <local_path>
│   ├── cp <bin_path> <virtual_src_path> <virtual_dst_path>
│   ├── mv <bin_path> <virtual_src_path> <virtual_dst_path>
│   └── help
└── help
```

### Agent

When you first launch `transcodine`, the first thing you need to do is setup a
new agent. This will setup the configuration directory (default is
`~/.transcodine` but can be changed via the `TRANSCODINE_CONFIG_PATH`
environment variable) which will store all the files tracked by the program. The
setup will require you enter a new password and remember it, otherwise the data
in the agent is unrecoverable! You can reset the password using
`transcodine agent reset`, but only if you still remember your old password.

### Bin

A bin is a vault where you can safely input data and it will stay there,
encrypted. You can create multiple bins, and each bin will track its own
individual state. Note that you should not directly tamper with the bins, as
even things like their names and locations are sensitive to changes, and leave
the bin in an unrecoverable state.

FOr example, the `transcodine bin create` command will add an entry in an
encrypted database pointing to the file name. These names are used to show
tracked bins in `transcodine bin ls`. Due to this reason, if you manually add a
bin to the configuration directory, it will be ignored. That doesn't mean you
can do tha, though. It is not recommended to manually interact with the
configuration directory. If you accidentally rename a bin, then the program will
exhibit undefined behaviour, and tampering with the encrypted contents will make
the file and all their contents unrecoverable!

The `transcodine bin save` and `transcodine bin load` is a special set of
commands. It uses Huffman compression to compress multiple bins into a single
file. As the bins are encrypted beforehand, this results in the archive not
needing further encryption, but AES128 (the encryption protocol being used to
encrypt the bins) produces high-entropy result, meaning each bit is equally
likely to be present. This reduces the efficacy of Huffman compression.

To unlock the bins, you also need a secret key per bin. This is usually stored
in your internal database, so nobody can view the data in the archive -- not
even you! To work around this, an encrypted database containing the relevant
keys is stitched with the archive, and a secret key is created at the time of
archive creation. This secret key will unlock the database and allow anyone to
import all the saved bins into their agent.

### File

This sub-command operates on the files in a single bin. You can use
`transcodine file ls` to list files in a bin, or use `transcodine file cat` to
read a file. You can save the file directly using `transcodine bin get`, or copy
and move files around withg `transcodine file cp` and `transcodine file mv`. You
need to add files to the bin using `transcodine bin add` before you can use any
commands which operate on existing files, though.

## Capabilities

To provide maximum security for the users, the most advanced security protocols
have been selected to keep the data secure. The implementations include the
following protocols.

- [SHA256](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf)
- [SHA256-HMAC](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.198-1.pdf)
- [PBKDF2](https://www.rfc-editor.org/rfc/pdfrfc/rfc8018.txt.pdf)
- [AES128](https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.197-upd1.pdf)
- [AES-CTR](https://nvlpubs.nist.gov/nistpubs/legacy/sp/nistspecialpublication800-38a.pdf)

## Technical Details

### Password Management

The password is hashed using PBKDF2 hashing algorithm. This is notably different
than encryption, as encryption is designed to be reversed, hashing isn't. The
password is hashed and stored in a file on the user's system. We can only really
pin our hopes that the user's home directory will be writable at runtime and can
store per-user information, so we will be using that directory as the root of
our hashed key.

The encrypted key generated by the hashing algorithm is the root key. This root
key then unlocks a key encryption key, or KEK for short. This middle-man exists
to prevent re-encrypting either the bins or the database storing all the keys,
as updating the password would mean a complete rehash of everything that the
hash was used for as a key, otherwise all the encrypted data will be lost
forever.

### Agent Management

Previously, the agent was being kept unlocked via an unlock token stored in a
temporary directory, but that led to a security vulnerability. The unlock token
needs to be encrypted and not hashed, so it can be decrypted at runtime with a
known key, but if the unlock is achievable without entering a password to unlock
the encrypted database, then the unlock token can be easily forged to falsely
unlock a user session.

In favor of enhancing security, each command requires entering the password
instead. This allows for the password itself to be used as the input key to
derive decryption hashes,so a bad actor can obtain the password hashes and
salts, but without having access to the raw password, they will be unable to do
anything with it.

### Bin Management

Each bin contains a 40-byte global header. This header stores a public
decryption key. The paired private key is stored in an encrypted database which
is only opened by using the original password as the key. All the data after the
global header is encrypted via AES-CTR. A magic string is stored immediately
after the global header which will read `UNLOCKED` if the decryption was
successful.

To store a file, it needs a new header. The file header contains the size of the
file path and the size of the data. This allows for the file paths and data to
be dynamic and to not waste storage space in each header block for a file name
which would often undershoot the reserved space and might overshoot the allowed
space as well.

Immediately following the file header, the file path and the file data is
located. Note that the file path must be null-terminated. This does not apply to
the file data.

The file paths are stored as fully-qualified paths, meaning the files are not
grouped by directories by architecture and needs to be done manually at
parse-time. This also introduces a limitation where empty directories cannot be
made, they can only be made implicitly via file creation.

### Compression

Compression has been added to align with the assignment requirements, but it is
a feature which serves to only complicate things rather than making them more
efficient. As I am using AES128 to encrypt the bins, they are already in a state
of high entropy. This means that the data closely resembles random noise. It is
hard to compress random data, meaning the compression will not have any real
impact.

The compression could have been done on the bin itself prior to compression, and
while this would have yielded better results, it would have been more difficult
to compress and decompress the archive, with more places to go wrong. This is
why you will probably see a warning for low efficiency of huffman compression on
the file.

The compressed bins are linked with a new encrypted database to store the
relevant keys. This database can only be unlocked using the master key provided
after compression. This key can be used to import bins into an agent. Bin name
collisions and deduplication is handled automatically. This ease of sharing can
be both a blessing and a curse. All it takes is a single mistake to leak the
entire contents of a compressed archive to a malicious actor.

### Notes

- Debug logging is enabled by default. You can disable it by going into
  `include/constants.h` and removing the line containing `#define DEBUG`.
- This program is designed to leverage Unix behaviour and will not compile or
  run under Windows without rewrites.
- To simplify working with heap memory, a custom implementation of buffers is
  included under `core/`. This implementation aims to model the most basic
  features of strings in C++ or `Buffer` in Node.js runtimes. However, this
  implementation focuses more heavily on heap management than features and only
  supports basic functions like appending data or clearing the buffer.
- Performance, correctness, or production-readiness were not prioritised in this
  implementation. Rather, this is for educational purposes and the aim was to
  showcase advanced encryption/compression algorithms, which I'd say this does
  pretty well.
- The main hashing algorithm, PBDKF2, is designed to be fast and have a lower
  memory impact, however that makes it weaker to GPU cracking. Thus, for hashes
  which can be easily accessed by bad actors, using stronger hashing algorithms
  like argon2 is recommended for production environments.

## Credits

This project can be found on
[GitHub](https://github.com/aryanjassal/transcodine).

Refer to [Polykey](https://github.com/MatrixAI/Polykey) for a complete and
production-ready implementation of this concept.
