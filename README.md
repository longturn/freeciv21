Freeciv21
=========

[![License: GPL v3+](https://img.shields.io/badge/License-GPLv3%2B-blue)](https://www.gnu.org/licenses/gpl-3.0.en.html)
[![Build](https://github.com/longturn/freeciv21/actions/workflows/build.yaml/badge.svg)](https://github.com/longturn/freeciv21/actions/workflows/build.yaml)
[![CodeFactor](https://www.codefactor.io/repository/github/longturn/freeciv21/badge)](https://www.codefactor.io/repository/github/longturn/freeciv21)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/5963b2222b88430b8ba0055e70d50ab5)](https://app.codacy.com/gh/longturn/freeciv21?utm_source=github.com&utm_medium=referral&utm_content=longturn/freeciv21&utm_campaign=Badge_Grade_Settings)
[![Coverity](https://scan.coverity.com/projects/21964/badge.svg)](https://scan.coverity.com/projects/longturn-freeciv21)
[![quality badge](https://img.shields.io/static/v1?label=SUPER&message=HOT&color=green)](http://www.emergencykitten.com/)

Freeciv21 is an empire-building strategy game inspired by the history of human civilization. It takes its roots in the well-known FOSS game Freeciv and extends it for more fun, with a revived focus on competitive multiplayer environments.

![Screenshot](https://github.com/longturn/freeciv21/raw/master/data/screenshot.png)

The documentation is found on our [documentation website](https://longturn.readthedocs.io/). Some older documents are found in the [doc](doc) directory.

Freeciv21 is maintained by folks from [longturn.net](https://longturn.net). We welcome merge requests, bug reports and simple suggestions! Get in touch on [Discord](https://discord.gg/98krqGm).

Installation
------------

### Windows

We provide Windows installers in the **Assets** section of every [release](https://github.com/longturn/freeciv21/releases) (make sure to download the **.exe** file). After downloading, run the installer and [follow the usual steps](https://longturn.readthedocs.io/en/latest/General/windows-install.html).

### Linux

Linux users need to compile the code. Ubuntu 20.04 or higher is supported. See [this link](https://longturn.readthedocs.io/en/latest/General/install.html) for the detailed procedure.

#### Install dependencies

You may need to adjust this command for your package manager. You need CMake 3.16 or higher, Qt 5.10 or higher, and Lua 5.3 or 5.4. You need to do this only once.
```sh
sudo apt install git cmake ninja-build g++ python3 gettext qt5-default \
  libkf5archive-dev liblua5.3-dev libsqlite3-dev libsdl2-mixer-dev
```

#### Get the code

Use this command the first time you download Freeciv21:
```sh
git clone https://github.com/longturn/freeciv21.git
cd freeciv21
```

Afterwards, you can refresh the code with:
```sh
cd freeciv21
git pull --ff-only
```

#### Compile

Freeciv21 uses a standard CMake workflow. We recommend building with Ninja:
```sh
cmake . -B build -G Ninja -DCMAKE_INSTALL_PREFIX=$HOME/freeciv21
cmake --build build
```

#### (Optional) Install

This will install the files in the directory `$HOME/freeciv21` specified above:
```sh
cmake --build build --target install
```
