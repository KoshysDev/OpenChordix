# OpenChordix Vendoring Note

This directory contains `PhilPotter/gp_parser`, sourced from upstream commit
`0a7bf609a480fd28656ab769299bb118ac8ba3a1` and used under the MIT license
in `LICENSE`.

OpenChordix locally modifies the vendored parser to accept an in-memory byte
span, validate bounds and collection counts for untrusted imported files,
advance beat timing across rests, and correct tied-note and seven-string
safety issues. The integration adapter lives in
`src/core/track/import/GuitarProImporter.cpp`.
