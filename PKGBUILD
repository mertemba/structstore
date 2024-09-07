# Maintainer: Max Mertens <max.mail@dameweb.de>

pkgbase=structstore
pkgname=(structstore structstore_py)
pkgver=0.1.9.r18.gbe22f23
pkgrel=1
pkgdesc='Structured object storage, dynamically typed, to be shared between processes'
arch=('x86_64')
url='https://github.com/mertemba/structstore'
license=('BSD-3-Clause')
depends=()
structstore_py_depends=('python')
makedepends=('cmake' 'ninja' 'gcc' 'yaml-cpp' 'python' 'python-poetry')
source=("git+https://github.com/mertemba/structstore.git")
sha256sums=('SKIP')

pkgver() {
    cd "$pkgbase"
    # cutting off 'v' prefix that presents in the git tag
    git describe --long --tags --abbrev=7 | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g'
}

build() {
    cd "${srcdir}/${pkgbase}"
    poetry config virtualenvs.in-project true
    poetry lock --no-update
    poetry install --no-root
    poetry env info --path
    source $(poetry env info --path)/bin/activate
    cmake -B build -S "." \
        -G Ninja \
        -DSTRUCTSTORE_WITH_PYTHON=ON \
        -DSTRUCTSTORE_WITH_PY_BUILD_CMAKE=OFF \
        -DCMAKE_BUILD_TYPE:STRING='None' \
        -DCMAKE_INSTALL_PREFIX:PATH='/usr' \
        -Wno-dev
    cmake --build build
}

check() {
    ctest --test-dir build --output-on-failure
}

package_structstore() {
    cd "${srcdir}/${pkgbase}"
    source $(poetry env info --path)/bin/activate
    cmake -B build -S "." \
        -DSTRUCTSTORE_WITH_PYTHON=OFF
    DESTDIR="$pkgdir" cmake --install build
}

package_structstore_py() {
    cd "${srcdir}/${pkgbase}"
    source $(poetry env info --path)/bin/activate
    cmake -B build -S "." \
        -DSTRUCTSTORE_WITH_PYTHON=ON
    DESTDIR="$pkgdir" cmake --install build
}
