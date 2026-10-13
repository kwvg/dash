RPC
---

- Clarified the `getchainlockbyheight` help: only ChainLocks embedded in a later
  active-chain block's coinbase can be returned, so the best ChainLock is
  unavailable until embedded and skipped heights cannot be returned. Use
  `getbestchainlock` for the current best ChainLock. RPC behavior is unchanged.
  (#7875)
