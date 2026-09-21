# Source provenance (WP00.03)

The [accepted Design profile](https://github.com/ArcForges/ArcForges-Design/blob/5322d698a1b650a52a5a139d986dd85b00b48581/docs/assurance/reference-coverage-and-provenance.md)
governs the current ArcNotes repository. The retired initialization repository is
historical lineage, not a source/build prerequisite. Desktop UI remains native
Avalonia/Skia. This change adds no browser UI or tooling runtime dependency.

Before reusing material, complete `eng/provenance/template.json` as a record in
`eng/provenance/records/<id>.json`. The ten fields identify the exact source,
commit and paths, file-level licence, attribution, target, disposition, oracle,
NOTICE and lifetime. Generated material also identifies each generator and input;
temporary material names its removal trigger and owner. The Licensing and
Provenance Owner reviews actual evidence within the closed decision table in
`eng/policy/reuse-policy.json`. A maintainer-authorized review may exercise that
role; an `approved` string alone cannot replace review. Record unresolved conflicts
under `eng/provenance/conflicts` and resolve them before accepting the material.

`eng/provenance/files.json` accounts for every tracked file, including tracked
ignored files, and every non-ignored new file during local checks. Reused targets
have exact record bindings and SHA-256 values; all other files are explicitly
classified as current first-party material. Review must also detect copied material
inserted into an existing file: an inventory check cannot establish authorship.
The original records reconcile existing legal documents honestly and do not claim
that approval preceded their historical introduction.

Used records remain immutable, even after retirement. A changed source, target,
intent or obligation requires a new revision with `supersedes`; retain its earlier
records and update only the active binding. CI reads its trusted event base and
requires the complete comparison history. A retained external file cannot silently
become first-party material. The explanatory template is never an approved record.

Run the existing C# checks and tests from `CONTRIBUTING.md`. After completing and
reviewing new records/inventory, regenerate the deterministic summary with:

```sh
dotnet run --project eng/ArcForges.Repository -- provenance-notice
dotnet run --project eng/ArcForges.Repository -- check
```

The checker rejects missing/blank fields, unclassified files, unknown/prohibited
licences, missing or changed targets, escaping/linked paths, altered history,
broken supersession and missing notices. Its C# adaptation records the Apache
Contracts source at `18a970c67c463f1971ca05773b80f31a1b1ba1a7`; full original terms
and changed-file attribution remain in the repository. This source/build tool is
absent from the Native AOT application. No sibling source is consumed during builds.

Portable staging requires committed, audited source. It retains the root licence,
six exact upstream legal documents, the generated package source summary and the
actual source-audit receipt. One publication-handoff check reads the
real ZIP/tar entries, compares full legal bytes and requires a passing receipt for
the candidate's clean source commit. Existing dependency notices remain intact;
the summary does not replace them. The repository static-check command runs once on Linux; each of the three Windows/Linux staging steps retains its source-policy receipt and required legal material.

Current CI and publication follow the [accepted validation policy](https://github.com/ArcForges/ArcForges-Design/blob/47db6670a727317939b91245e8c0b288834acf99/docs/assurance/ci-and-local-validation-policy.md). Windows/Linux compilation, offline unit/static/security checks and required provenance remain; macOS, UI/live/runtime gates and repeated public downloads are removed. These checks do not establish product completeness or commercial readiness.
