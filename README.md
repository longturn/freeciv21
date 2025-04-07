Freeciv21 - Develop Your Civilization from Humble Roots to a Global Empire
==========================================================================

[![License: GPL v3+](https://img.shields.io/badge/License-GPLv3%2B-blue)](https://www.gnu.org/licenses/gpl-3.0.en.html)
[![Build](https://github.com/longturn/freeciv21/actions/workflows/build.yaml/badge.svg)](https://github.com/longturn/freeciv21/actions/workflows/build.yaml)
[![CodeFactor](https://www.codefactor.io/repository/github/longturn/freeciv21/badge)](https://www.codefactor.io/repository/github/longturn/freeciv21)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/5963b2222b88430b8ba0055e70d50ab5)](https://app.codacy.com/gh/longturn/freeciv21?utm_source=github.com&utm_medium=referral&utm_content=longturn/freeciv21&utm_campaign=Badge_Grade_Settings)
[![Coverity](https://scan.coverity.com/projects/21964/badge.svg)](https://scan.coverity.com/projects/longturn-freeciv21)
[![quality badge](https://img.shields.io/static/v1?label=SUPER&message=HOT&color=green)](http://www.emergencykitten.com/)

![Screenshot](https://github.com/longturn/freeciv21/raw/master/dist/readme-screenshot.png)

------------

<p><img src="https://github.com/longturn/freeciv21/raw/master/data/icons/128x128/freeciv21-client.png" align="left"/>
<!-- This description is in each of the metainfo files, about.rst, freeciv21-server.rst, and snapcraft.yaml -->
Freeciv21 is a free open source turn-based empire-building 4x strategy game, in which each player becomes the leader of a civilization. You compete against several opponents to build cities and use them to support a military and an economy. Players strive to complete an empire that survives all encounters with its neighbors to emerge victorious. Play begins at the dawn of history in 4,000 BCE.</p>
<p>Freeciv21 takes its roots in the well-known FOSS game Freeciv and extends it for more fun, with a revived focus on competitive multiplayer environments. Players can choose from over 500 nations and can play against the computer or other people in an active online community.</p>
<p>The code is maintained by the team over at <a href="https://longturn.net/">Longturn.net</a> and is based on the QT framework. The game supports both hex and square tiles and is easily modified to create custom rules.
<br clear="left"/></p>

Get started by reviewing our [about page](https://longturn.readthedocs.io/en/latest/Getting/about.html) and get in touch on [Discord](https://discord.gg/98krqGm). The #General channel is a great place to start.
The documentation is found on our [documentation website](https://longturn.readthedocs.io/).

We welcome pull requests, bug reports and simple suggestions! Check out our [contributing docs](https://longturn.readthedocs.io/en/latest/Contributing/index.html) or talk to us on Discord.

Installation
------------

### Snap, Flathub and AUR Editions

<p><a href='https://snapcraft.io/freeciv21'><img width='182' height='56' alt='Get Freeciv21 on Snapcraft' src='https://snapcraft.io/static/images/badges/en/snap-store-white.svg'/></a>&nbsp;<a href='https://flathub.org/apps/net.longturn.freeciv21'><img width='182' height='56' alt='Get Freeciv21 on Flathub' src='https://dl.flathub.org/assets/badges/flathub-badge-i-en.svg'/></a>&nbsp;<a href='https://aur.archlinux.org/packages/freeciv21'><img width='182' height'56' alt="AUR version" src="https://img.shields.io/aur/version/freeciv21?style=for-the-badge&logo=archlinux&label=AUR"></p>

### Windows and macOS

We provide Windows and macOS installers in the **Assets** section of every [release](https://github.com/longturn/freeciv21/releases). Make sure to download the **.exe** or **.dmg** file depending on platform. After downloading, run the installer. The Windows install is [documented here](https://longturn.readthedocs.io/en/latest/Getting/windows-install.html).

### Linux

Linux users running on Debian (or one of the many variants such as Ubuntu) can download a package from the **Assets** section of every [release](https://github.com/longturn/freeciv21/releases) (make sure to download the **.deb** file). After downloading, run the installer:
```sh
sudo apt install ./freeciv21_*_amd64.deb
```

Other Linux users will need to compile the code. Ubuntu 22.04 or higher is supported. See [this link](https://longturn.readthedocs.io/en/latest/Getting/compile.html) for the detailed procedure and a list of supported distributions. See below for a quick set of instructions for Debian based distributions.

#### Install dependencies

You may need to adjust this command for your package manager. You need CMake 3.21 or higher, Qt (base and SVG) 6.6 or higher, and Lua 5.3 or 5.4. You need to do this only once.
```sh
sudo apt install git cmake ninja-build g++ python3 gettext qt6-base-dev \
  qt6-svg-dev libkf6archive-dev liblua5.3-dev libsqlite3-dev libsdl2-mixer-dev
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

#### Using Nix

Alternatively, Freeciv21 can be used with Nix. For this, enable Nix flakes and run
```sh
nix run github:longturn/freeciv21#;
```

### FreeBSD

Freeciv21 is available in the FreeBSD ports tree. You can install the binary
package using this command:
```sh
pkg install freeciv21
```

Although it is recommended to use the binary package, you can also build
Freeciv21 using the ports tree:
```sh
make -C /usr/ports/games/freeciv21 install clean
```

### OpenBSD

Freeciv21 is available in the OpenBSD ports tree. You can install the binary
package using this command (as root):
```sh
pkg_add freeciv21
```

Although it is recommended to use the binary package, you can also build
Freeciv21 using the ports tree:
```sh
make -C /usr/ports/games/freeciv21 install clean
```
