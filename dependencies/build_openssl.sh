#! /bin/bash

#OS=$(uname | tr 'A-Z' 'a-z')
if [[ "${OSTYPE}" == "darwin"* ]]; then
  OS="darwin"
elif [[ "${OSTYPE}" == "linux-android" ]]; then
  OS="android"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
  OS="linux"
else
  echo "unknown OSTYPE = ${OSTYPE}"
  exit
fi

if [[ -d openssl ]]; then
  cd openssl || exit
else
  git clone --branch OpenSSL_1_1_1l https://gitee.com/mirrors/openssl.git
  cd openssl || exit
fi

make distclean

./config no-ui --prefix="$(pwd)"/../"${OS}" --openssldir="$(pwd)"/../"${OS}"
# perl Configure VC-WIN64A no-asm no-shared --prefix="C:\Users\haidy\CLionProjects\devent\dependencies\windows"

if [[ "${OS}" == "android" ]]; then
  make -j2
else
  make -j8
fi
make install
