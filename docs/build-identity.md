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

Preparation executes the actual Native AOT candidate offline and compares its
report with independently obtained Git/run/release and restored dependency inputs.
The report is included in each portable archive. Archive verification reads it
and rejects even rehashed metadata changes. Existing native UI/live Cloud checks
remain required on all five native CI hosts. The `verify-assemblies` command reads
the actual PE metadata of the app, core, tests and repository tool after compilation.

Tests mutate each of the nine distinct source kinds, check deterministic output,
and reject missing/unknown axes, aliases, duplicate subjects, malformed versions,
unsafe paths, dirty CI identities and source/run/report tampering. Synthetic future
source declarations are mechanism fixtures, not implemented application support.
This metadata does not establish commercial readiness or later compatibility gates.
