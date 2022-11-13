Freeciv21
=========

[![License: GPL v3+](https://img.shields.io/badge/License-GPLv3%2B-blue)](https://www.gnu.org/licenses/gpl-3.0.en.html)
[![Build](https://github.com/longturn/freeciv21/actions/workflows/build.yaml/badge.svg)](https://github.com/longturn/freeciv21/actions/workflows/build.yaml)
[![CodeFactor](https://www.codefactor.io/repository/github/longturn/freeciv21/badge)](https://www.codefactor.io/repository/github/longturn/freeciv21)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/5963b2222b88430b8ba0055e70d50ab5)](https://app.codacy.com/gh/longturn/freeciv21?utm_source=github.com&utm_medium=referral&utm_content=longturn/freeciv21&utm_campaign=Badge_Grade_Settings)
[![Coverity](https://scan.coverity.com/projects/21964/badge.svg)](https://scan.coverity.com/projects/longturn-freeciv21)
[![quality badge](https://img.shields.io/static/v1?label=SUPER&message=HOT&color=green)](http://www.emergencykitten.com/)

![Logo](https://github.com/longturn/freeciv21/raw/master/data/freeciv21-client.png)

Freeciv21 is an empire-building strategy game inspired by the history of human civilization. It takes its roots in the well-known FOSS game Freeciv and extends it for more fun, with a revived focus on competitive multiplayer environments.

![Screenshot](https://github.com/longturn/freeciv21/raw/master/data/screenshot.png)

The documentation is found on our [documentation website](https://longturn.readthedocs.io/).

The WebAssembly build of Freeciv21 is playable at [freecivweb.com](https://freecivweb.com).

Freeciv21 is maintained by folks from [longturn.net](https://longturn.net). We welcome pull requests, bug reports and simple suggestions! Get in touch on [Discord](https://discord.gg/98krqGm). The #General channel is a great place to start.

Installation
------------

### Windows and macOS

We provide Windows and macOS installers in the **Assets** section of every [release](https://github.com/longturn/freeciv21/releases). Make sure to download the **.exe** or **.dmg** file depending on platform. After downloading, run the installer. The Windows install is [documented here](https://longturn.readthedocs.io/en/latest/Getting/windows-install.html).

### Linux

Linux users running on Debian (or one of the many variants such as Ubuntu) can download a package from the **Assets** section of every [release](https://github.com/longturn/freeciv21/releases) (make sure to download the **.deb** file). After downloading, run the installer:
```sh
sudo apt install ./freeciv21_*_amd64.deb
```

Other Linux users will need to compile the code. Ubuntu 22.04 or higher is supported. See [this link](https://longturn.readthedocs.io/en/latest/Getting/compile.html) for the detailed procedure and a list of supported distributions. See below for a quick set of instructions for Debian based distributions.

#### Install dependencies

You may need to adjust this command for your package manager. You need CMake 3.16 or higher, Qt (base and SVG) 5.15 or higher, and Lua 5.3 or 5.4. You need to do this only once.
```sh
sudo apt install git cmake ninja-build g++ python3 gettext qtbase5-dev \
  libqt5svg5-dev libkf5archive-dev liblua5.3-dev libsqlite3-dev libsdl2-mixer-dev
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
