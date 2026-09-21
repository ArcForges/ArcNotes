# ArcNotes

Native C# desktop entry point for the ArcForges knowledge management product. This bootstrap provides a working Hello screen connected to the deployed Cloud Native AOT service at `https://arcforges.com/api`.

The application starts offline. Enter a name and select **Say hello** to send one binary gRPC-Web request through the published Contracts client. Progress, cancellation and failures are visible; retry is an explicit user action. Names retain whitespace and Unicode, with a 1–256 UTF-16 unit limit. Authentication and product-specific workflows are not implemented in this bootstrap.

## Run and develop

Install .NET SDK **10.0.401** and the native build prerequisites in [development](docs/development.md). The solution uses C# 14, Avalonia **12.1.2**, exact central NuGet versions and per-project lock files. No Node, Python, CMake or neighboring source checkout is required.

```sh
dotnet restore ArcNotes.slnx --locked-mode
dotnet build ArcNotes.slnx -c Release --no-restore
dotnet test --project tests/ArcForges.ArcNotes.Tests/ArcForges.ArcNotes.Tests.csproj -c Release --no-build --filter-not-class '*.TransportTests'
dotnet run --project src/ArcForges.ArcNotes
dotnet run --project eng/ArcForges.Repository -- hooks
```

The UI, application state and repository tool are C#. Avalonia/Skia supply packaged native UI/rendering dependencies. The application consumes `ArcForges.Contracts.PublicApi` **1.0.0-ci.36.1** and the private build-time `ArcForges.Build.Policy` **1.0.0-ci.20.1**. It does not ship unused DesktopPlatform media engines.

## Downloads and automation

Each successful main push publishes a prerelease `v0.1.0-ci.<run>.<attempt>` to [GitHub Releases](https://github.com/ArcForges/ArcNotes/releases). PRs compile the same three Windows/Linux Native AOT targets and run offline checks but never publish a release:

| Platform | Archive |
| --- | --- |
| Windows x64 / ARM64 | Portable ZIP; extract everything and run `ArcNotes.exe` |
| Linux x64 | Portable tar.gz; extract everything and run `./ArcNotes` in an X11/XWayland desktop |

Keep all files together. These self-contained builds do not require a .NET installation. They are development distributions: Windows binaries are unsigned. Automated releases do not contain macOS builds; local macOS source support remains available without CI or release claims. No installer, app-store identity or OS trust claim is included. Linux system libraries are listed in [development](docs/development.md). Source for a release is its exact Git tag/commit.

CI performs compilation, packaging, offline unit/static checks and security scanning. It does not launch packaged applications, native UI or live Cloud requests. Runtime smoke is an explicit local-only command when needed; no post-publication asset download or runtime cycle is required. See [release mechanics](docs/releasing.md) and the [validation policy](https://github.com/ArcForges/ArcForges-Design/blob/47db6670a727317939b91245e8c0b288834acf99/docs/assurance/ci-and-local-validation-policy.md).

## Contribute and report issues

The repository check enforces the [project licence boundary](docs/licence-boundary.md)
for the application, core library, tests and C# tooling. Every build also checks
the effective MSBuild declarations and local project references.

The same tool enforces [source provenance](docs/provenance.md): complete file
classification, immutable reuse records, full notices and portable-archive checks.

Read [CONTRIBUTING](CONTRIBUTING.md), [security reporting](SECURITY.md) and the [code of conduct](CODE_OF_CONDUCT.md). ArcNotes remains **AGPL-3.0-only**; see [LICENSE](LICENSE) and [third-party notices](THIRD_PARTY_NOTICES.md).

`--build-info --evidence <absolute-path.json>` writes offline support metadata from the compiled application. See [build identity](docs/build-identity.md).
