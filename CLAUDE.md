# node-svmq

Native System V message queues for Node.js (NAN addon), published as `@keletes/svmq`.

## Commands
- `yarn install` builds the addon (`install` script: `node-gyp rebuild`); needs python, make, a C++ compiler.
- `yarn test` (= `yarn verify`) runs the jasmine suite against real System V queues (Linux or macOS).

## Package
- Ships `src/`, `binding.gyp`, `lib/`, `index.js`, `index.d.ts` (see `files`); consumers compile at install. Never ship prebuilt binaries.
- Consumers: `@keletes:registry=https://npm.pkg.github.com` in `.npmrc` plus a `read:packages` token.

## Releases: trunk-based, Conventional Commits
- PR titles follow Conventional Commits (`pr-title.yml`). On every push to `master`, `tag.yml` runs semantic-release: it creates the `vX.Y.Z` tag and GitHub Release and publishes to GitHub Packages. No release commit is pushed back; `package.json` keeps the placeholder version `0.0.0-development`.
- Tag and release use the GitHub App token; the publish uses the job's `GITHUB_TOKEN` (`packages: write`).
- Not every merge releases: only unreleased `feat`/`fix` (or breaking) commits.

## Open work
Mark open work as `TODO(YYYY-MM-DD):` at the code, or in `docs/todo.md`.
