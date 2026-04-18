# Security

If you believe you have found a security vulnerability in **argsbarg**, please report it responsibly.

## How to report

- **Preferred:** use [GitHub Security Advisories](https://github.com/bdombro/cpp-argsbarg/security/advisories/new) for this repository (private disclosure to maintainers).
- If you cannot use GitHub: open a **non-public** channel you already use with the maintainer, or email the repository owner via their GitHub profile contact options.

Please include:

- A short description of the issue and its impact
- Steps to reproduce (minimal reproducer or failing test case)
- Affected version(s) or commit hash, if known

## Scope

This is a small header-only CLI parsing library. Typical concerns include untrusted input handling (argv), completion script generation, and dependency supply chain (e.g. test-only FetchContent deps). Production attack surface depends on how applications embed the library.

## Response

Maintainers will acknowledge receipt as soon as practical and coordinate a fix and release timeline where appropriate.
