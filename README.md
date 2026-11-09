# Mini-projet : Radar à ultrasons piloté par réseau

# Contexte et objectifs
   
Vous allez construire un petit « radar » : un capteur à ultrasons monté sur un servomoteur,
piloté par un microcontrôleur que l'on commande à distance depuis un PC, à travers un
routeur.
La difficulté du projet est réseau autant qu'embarquée : la carte est branchée côté LAN
du routeur, alors que le PC est côté WAN. Le PC ne peut donc pas joindre la carte
directement ; c'est à vous de configurer le routeur pour la rendre accessible depuis
l'extérieur.
Ce module de 4 séances de 4 h, suivies d'une 5e séance de restitution devant un jury,
réunit ce que vous avez vu en langage C, en microcontrôleurs et en réseau.
Objectifs pédagogiques
À la fin du projet, vous saurez :
piloter un servomoteur (signal PWM) et mesurer une distance avec un capteur à
ultrasons (mesure d'une durée d'impulsion) depuis un microcontrôleur STM32, en C,
avec Keil µVision et le Middleware Ethernet ;
écrire sur la carte un serveur UDP qui interprète des commandes textuelles ;
écrire en C, sur PC, une application graphique (Qt) qui joue le rôle de client réseau
avec l'API sockets (UDP) ;
configurer un routeur : réservation DHCP statique et redirection de port (NAT) ;
diagnostiquer une panne réseau avec les outils usuels (ping, netcat, Wireshark) ;
tester, intégrer et documenter un système complet, en équipe.

# Résultat attendu

En démonstration finale, depuis le PC, vous envoyez une commande pour orienter le
capteur à un angle donné, puis une commande de balayage (par exemple de 0° à 180° par
pas de 10°). Votre application affiche les distances mesurées pour chaque angle.
