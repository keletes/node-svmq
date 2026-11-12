# node-svmq

Native System V message queues for Node.js (N-API addon, `node-addon-api`), published as `@keletes/svmq`.

## Commands
- `yarn install` runs `node-gyp-build`: it compiles the addon (python, make, a C++ compiler) only when `prebuilds/` has no matching binary; the load path in `lib/queue.js` is also `node-gyp-build`.
- `yarn prebuild` builds the current platform into `prebuilds/<platform>-<arch>/` (`prebuildify --napi`, git-ignored).
- `yarn test` runs the jasmine suite against real System V queues (Linux or macOS). `yarn test:types` type-checks `test/types.ts` against `index.d.ts`. `yarn verify` runs both (CI `test` check).

## Package
- Ships `prebuilds/` (linux-x64, linux-arm64 glibc, darwin-arm64, darwin-x64), `src/`, `binding.gyp` (fallback compile), `lib/`, `index.js`, `index.d.ts` (see `files`). N-API: one binary per platform for all Node majors; consumers need no build tools on those platforms.
- `index.d.ts` is hand-written (`export =` class + namespace); change it together with `lib/queue.js` and keep `test/types.ts` compiling.
- Consumers: `@keletes:registry=https://npm.pkg.github.com` in `.npmrc` plus a `read:packages` token.

## Releases: trunk-based, Conventional Commits
- PR titles follow Conventional Commits (`pr-title.yml`). On every push to `master`, `tag.yml` runs semantic-release: it creates the `vX.Y.Z` tag and GitHub Release and publishes to GitHub Packages. No release commit is pushed back; `package.json` keeps the placeholder version `0.0.0-development`.
- Tag and release use the GitHub App token; the publish uses the job's `GITHUB_TOKEN` (`packages: write`).
- `tag.yml` first builds the four prebuilds in a matrix and uploads them as artifacts; the release job downloads them into `prebuilds/` before semantic-release publishes. A local `npm pack` only contains your own platform.
- Not every merge releases: only unreleased `feat`/`fix` (or breaking) commits.

## Open work
Mark open work as `TODO(YYYY-MM-DD):` at the code, or in `docs/todo.md`.
