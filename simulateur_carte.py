#!/usr/bin/env python3
"""
Simulateur de la carte radar, pour tester le client PC SANS la carte.

Il écoute en UDP (port 5000 par défaut) et comprend ANGLE, DIST et SCAN,
comme le sujet. L'option --perte fait "perdre" des datagrammes au hasard,
pour vérifier que le délai d'attente et le nouvel essai du client marchent.

Exemples :
    python3 simulateur_carte.py                  # port 5000, aucune perte
    python3 simulateur_carte.py --perte 0.5      # perd la moitié des requêtes
    python3 simulateur_carte.py --port 8000 --perte 1.0   # ne répond jamais
"""
import argparse
import math
import random
import socket
import time

DELAI_PAR_MESURE = 0.05      # le vrai servo met du temps à se placer (s)
MAX_MESURES = 100            # un SCAN doit tenir dans un datagramme


def distance_a(angle):
    """Distance fictive : un obstacle qui dépend de l'angle (en cm)."""
    return 60 + 40 * math.sin(math.radians(angle * 2))


def traiter(commande, etat):
    """Renvoie la réponse (texte) à une commande, comme le fait la carte."""
    texte = commande.rstrip("\r\n")
    mots = texte.split(" ")

    if mots[0] == "ANGLE" and len(mots) == 2:
        if mots[1].isdigit() and int(mots[1]) <= 180:
            etat["angle"] = int(mots[1])
            return "OK\n"
        return "ERR angle\n"

    if texte == "DIST":
        return "DIST %.1f\n" % distance_a(etat["angle"])

    if mots[0] == "SCAN" and len(mots) == 4:
        if not all(m.isdigit() for m in mots[1:]):
            return "ERR scan\n"
        debut, fin, pas = (int(m) for m in mots[1:])
        if debut > fin or fin > 180 or pas < 1:
            return "ERR scan\n"
        angles = list(range(debut, fin + 1, pas))
        if len(angles) > MAX_MESURES:
            return "ERR trop de mesures\n"
        lignes = []
        for a in angles:
            time.sleep(DELAI_PAR_MESURE)
            if a == 90:                      # un point sans écho, pour tester
                lignes.append("%d TIMEOUT" % a)
            else:
                lignes.append("%d %.1f" % (a, distance_a(a)))
        etat["angle"] = angles[-1]
        return "\n".join(lignes) + "\nEND\n"

    return "ERR commande\n"


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--port", type=int, default=5000)
    p.add_argument("--perte", type=float, default=0.0,
                   help="probabilité (0 à 1) de perdre une requête")
    a = p.parse_args()

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("0.0.0.0", a.port))
    print("Simulateur en écoute sur le port %d (perte %.0f %%)"
          % (a.port, a.perte * 100), flush=True)

    etat = {"angle": 0}
    while True:
        donnees, expediteur = s.recvfrom(1024)
        commande = donnees.decode("ascii", errors="replace")
        if random.random() < a.perte:
            print("  [perdu]   %r" % commande, flush=True)
            continue
        reponse = traiter(commande, etat)
        print("  %-18r -> %r" % (commande, reponse[:40]), flush=True)
        s.sendto(reponse.encode("ascii"), expediteur)


if __name__ == "__main__":
    main()
