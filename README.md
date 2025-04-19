# Transcodine

A transformation and encoding pipeline designed to better encode documents for
enhanced security.

## What is it?

Transcodine is an application designed for dynamic and enhanced encryption and
compression. There are a number of preset encryption and compression protocols
available. Custom pipelines can be designed which use the available transformers
dynamically to design a custom encryption/compression protocol.

This custom encryption/compression protocol (abbreviated to encom) can be
configured via the configuration file, allowing for enhanced security. The
configuration file for encon protocols and transformers will be encrypted at
rest for maximum security.

Due to the availability of dynamic pipelines, specialised presets can be defined
for different use cases. For example, images can be compressed with a different
protocol, and text documents with another.
