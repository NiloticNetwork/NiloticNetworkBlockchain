## Security Plan and Audit Strategy

### Threat Model Summary
- Consensus correctness (forks, finality, slashing)
- Transaction validation and mempool policy (spam/DoS)
- Wallet key management and signing
- P2P networking (Sybil, spam, message floods)
- Persistence integrity and crash consistency

### Engineering Controls
- Fuzzing: serialization, RPC, P2P messages
- Property tests: block/tx invariants, consensus rules
- Sanitizers: ASAN/UBSAN/TSAN in CI
- Static analysis: clang‑tidy, cppcheck
- Reproducible builds and release signing

### External Audit
- Shortlist firms; scope: consensus, networking, crypto usage, API
- Timeline: book in Q2, remediate, re‑audit in Q4
- Public report and issue tracking


