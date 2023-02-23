New RPC Argument
--------
- `sendrawtransaction` has a new, optional argument, `maxburnamount` with a default value of `0`. Any transaction containing an unspendable output with a value greater than `maxburnamount` will not be submitted. At present, the outputs deemed unspendable are those with scripts that begin with an `OP_RETURN` code (known as 'datacarriers'), scripts that exceed the maximum script size, and scripts that contain invalid opcodes. Asset lock transactions are exempt from this check: their `OP_RETURN` output carries the amount locked for Dash Platform and is not a burn.

