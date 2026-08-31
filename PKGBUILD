# Maintainer: kido0001 mesrouzer@gmail.com
pkgname=klok
pkgver=0.2.0
pkgrel=1
pkgdesc="A lightweight terminal clock with resizable windows, pomodoro, weather, and a dynamic theme"
arch=('x86_64' 'aarch64')
url="https://github.com/kido0001/klok"
license=('BSD 3-Clause')
depends=('ncurses' 'curl')
makedepends=('gcc' 'make')
source=("$pkgname-$pkgver.tar.gz::https://github.com/kido0001/klok/archive/refs/tags/v$pkgver.tar.gz")
sha256sums=('SKIP')  # replace by running `updpkgsums` once the tag exists (see PACKAGING.md)

build() {
    cd "$pkgname-$pkgver"
    make WITH_WEATHER=1
}

package() {
    cd "$pkgname-$pkgver"
    make DESTDIR="$pkgdir" PREFIX=/usr install
    install -Dm644 README.md "$pkgdir/usr/share/doc/$pkgname/README.md"
    install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
