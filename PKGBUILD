# Maintainer: Max Mertens <max.mail@dameweb.de>

pkgbase=structstore
pkgname=(structstore structstore_py)
pkgver=0.2.0
pkgrel=1
pkgdesc='Structured object storage, dynamically typed, to be shared between processes'
arch=('x86_64')
url='https://github.com/mertemba/structstore'
license=('BSD-3-Clause')
depends=()
structstore_py_depends=('python')
makedepends=('cmake' 'ninja' 'gcc' 'yaml-cpp' 'python' 'python-pip')
source=("git+https://github.com/mertemba/structstore.git")
sha256sums=('SKIP')

pkgver() {
    cd "$pkgbase"
    # cutting off 'v' prefix that presents in the git tag
    git describe --long --tags --abbrev=7 | sed 's/^v//;s/\([^-]*-g\)/r\1/;s/-/./g'
}

build() {
    cd "${srcdir}/${pkgbase}"
    venvdir="${srcdir}/venv"
    python -m venv "${venvdir}"
    source "${venvdir}/bin/activate"
    pip install -r requirements.txt
    cmake -B build -S "." \
        -G Ninja \
        -DBUILD_WITH_PYTHON=ON \
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
    venvdir="${srcdir}/venv"
    source "${venvdir}/bin/activate"
    cmake -B build -S "." \
        -DBUILD_WITH_PYTHON=OFF
    DESTDIR="$pkgdir" cmake --install build
}

package_structstore_py() {
    cd "${srcdir}/${pkgbase}"
    venvdir="${srcdir}/venv"
    source "${venvdir}/bin/activate"
    cmake -B build -S "." \
        -DBUILD_WITH_PYTHON=ON
    DESTDIR="$pkgdir" cmake --install build
}
