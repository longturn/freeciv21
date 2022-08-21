Freeciv21 on the Server
***********************

In order to play Freeciv21 on the network, one machine needs to act as the *server*: the state of
the game lives on the server and players can connect to play. There are many ways to operate such a
server; this page gives an introduction to the topic.

In order to run a server, you need a computer that other players can reach over the network. If
you're planning to play at home, this can usually be any machine connected to your WiFi or local
cabled network. Otherwise, the easiest is to rent a small server from a hosting provider. We
strongly recommend that you choose a Linux-based server, as that is what we have experience with.
You will need the ability to run your own programs on the server, so SSH access is a must. Apart
from that, Freeciv21 is quite light on resources so you will hardly hit the limits of even the
cheapest options.

Whether you choose to use your own machine or to rent one, the basic principle of operating a
server is the same: you need to run a program called ``freeciv21-server`` for as long as the game
will last. This program will wait for players to connect and handle their moves in the exact same
way as in a single player game. In fact, Freeciv21 always uses a server, even when there is only
one player!
