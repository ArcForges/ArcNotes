# WP02.04 offline build identity

`ArcNotes --build-info --evidence /absolute/path/build.json` runs before UI/host
initialization and performs no network request. The compiled assembly metadata and
embedded version sources produce `arcforges.build-identity.v1`; runtime environment
variables cannot replace the producer identity. Normal startup remains offline.

All four owned assemblies use published Build.Policy `1.0.0-ci.20.1` for the full
source commit, build kind, build ID, pipeline URL and deterministic source commit
timestamp. Owner build targets add the actual dirty state. CI requires a clean
checkout and actual GitHub run identity. Local builds identify themselves as local;
they cannot pass CI candidate verification. The timestamp is not wall-clock build
time. Third-party assemblies retain their own metadata.

The nine-axis source catalog is `eng/version-sources.json`. AppVersion reads the
application informational version. ContractSet reads the restored Contracts
`source.json` namespace/descriptor, independently of its pinned package release
`1.0.0-ci.36.1`. PackageVersion lists exact resolved dependency coordinates from
the embedded committed application lock, including framework/RID scope. No product
format, storage, capability, policy or extension version is invented for this
foundation. Explicit absent states name their later responsible work packages.
There is no first-party native ABI in this application.

The repository tool statically reads actual app/core/test/tool PE metadata once.
Preparation writes `build-identity.json` from the reviewed Git/run/release, restored
dependency and committed source inputs without launching the AOT app. This packaged
build-input receipt is not runtime execution evidence. The app's explicit local
`--build-info` command remains available for relevant support diagnostics.
Publication checks candidate identity and legal/source integrity once. CI has three
Windows/Linux compilation targets, no macOS/UI/live execution and no routine public
archive download or runtime verification cycle.

Tests mutate each of the nine distinct source kinds, check deterministic output,
and reject missing/unknown axes, aliases, duplicate subjects, malformed versions,
unsafe paths, dirty CI identities and source/run/report tampering. Synthetic future
source declarations are mechanism fixtures, not implemented application support.
This metadata does not establish commercial readiness or later compatibility gates.
