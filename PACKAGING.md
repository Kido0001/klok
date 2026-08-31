# Packaging klok for Arch / AUR

This gets you to `paru -S klok` (or `yay -S klok`) working for *anyone*,
not just you. Do these once, then repeat the short "new release" section
each time you tag a new version.

## 0. One-time setup

1. **Create a public GitHub repo** named `klok` and push your code:
   ```sh
   cd klok
   git init          # if you haven't already
   git add .
   git commit -m "klok v0.2.0"
   git branch -M main
   git remote add origin https://github.com/YOUR_GITHUB_USER/klok.git
   git push -u origin main
   ```

2. **Tag a release** matching `pkgver` in the PKGBUILD:
   ```sh
   git tag v0.2.0
   git push --tags
   ```
   This is what the AUR package actually downloads and builds — the
   PKGBUILD points at `https://github.com/.../archive/refs/tags/v0.2.0.tar.gz`.

3. **Edit `PKGBUILD`**: replace `YOUR_GITHUB_USER`, your name/email, and
   `license=('MIT')` with whatever's actually in your `LICENSE` file
   (use the correct [SPDX identifier](https://spdx.org/licenses/), e.g.
   `GPL-3.0-only`, `BSD-2-Clause`, etc.).

4. **Fill in the checksum.** Install `pacman-contrib` (`sudo pacman -S
   pacman-contrib`), then from the folder containing `PKGBUILD`:
   ```sh
   updpkgsums
   ```
   This replaces `sha256sums=('SKIP')` with the real hash of the tagged
   tarball.

5. **Test-build it locally** before publishing anything:
   ```sh
   makepkg -si
   ```
   `-s` pulls build deps automatically, `-i` installs the result via
   `pacman` when it succeeds. Run `klok` afterward to sanity-check it,
   then `sudo pacman -R klok` to remove the test install if you want a
   clean slate before publishing.

6. **Generate `.SRCINFO`** (the AUR's metadata file, auto-derived from
   your PKGBUILD — always regenerate this, never hand-edit it):
   ```sh
   makepkg --printsrcinfo > .SRCINFO
   ```

7. **Create an AUR account** at <https://aur.archlinux.org/register>,
   then add an SSH public key under Account Settings (same kind of key
   you'd use for GitHub — `ssh-keygen -t ed25519` if you don't have one).

8. **Push to the AUR.** The remote repo is created automatically on
   first push — you don't need to "create" the klok package anywhere
   first:
   ```sh
   git clone ssh://aur@aur.archlinux.org/klok.git aur-klok
   cp PKGBUILD .SRCINFO aur-klok/
   cd aur-klok
   git add PKGBUILD .SRCINFO
   git commit -m "Initial import: klok 0.2.0"
   git push
   ```

That's it — `klok` now exists on the AUR. Anyone with `paru` or `yay` can
run `paru -S klok` and it'll clone your GitHub tag, compile it, and
install it via pacman, exactly like an official package. `pacman -Qi
klok`, `pacman -R klok`, updates, etc. all work normally afterward —
pacman itself just never *searches* the AUR on its own, which is why the
first install has to go through an AUR helper (or a manual `makepkg -si`).

## New release checklist

Each time you tag a new klok version:

1. Bump `KLOK_VERSION` in `src/klok.h`, commit, tag (`git tag vX.Y.Z`),
   push both to GitHub.
2. In the AUR repo clone: bump `pkgver` (and reset `pkgrel=1`) in
   `PKGBUILD`, run `updpkgsums`, regenerate `.SRCINFO`, commit, push.
   - If you only need to fix packaging (not klok itself) between
     releases, bump `pkgrel` instead (e.g. `1` -> `2`) and leave `pkgver`
     alone.

## Notes

- **Weather is compiled in by default** in this PKGBUILD
  (`WITH_WEATHER=1`), so `curl` is a hard dependency and `-w`/the `w` key
  work out of the box for everyone who installs via the AUR — no build
  flags for users to know about.
- **`pacman -S klok` directly (no AUR helper)** would require klok to
  get into the official `extra` repository, which happens through Arch's
  Trusted User process for established, widely-used packages — not
  something to aim for with a v1. The AUR path above is the standard,
  right-sized way to distribute a personal project like this.
- If you'd rather skip AUR entirely, you can also host your own simple
  pacman repo (a folder of `.pkg.tar.zst` files + a `repo-add`-generated
  database, served over HTTP) and have users add it to
  `/etc/pacman.conf` — this *does* make plain `pacman -S klok` work, but
  it's more infrastructure to maintain than the AUR for not much benefit
  at this stage.
- Security note: the AUR had a real supply-chain incident in June 2026
  (malicious PKGBUILDs from compromised/abandoned packages). Keep your
  PKGBUILD's `build()`/`package()` fully readable at a glance — no piped
  curl-to-shell, no fetching scripts from third-party URLs at build
  time — so it stays trustworthy for people auditing before they install.
