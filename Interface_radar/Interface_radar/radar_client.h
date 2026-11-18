#ifndef RADAR_CLIENT_H
#define RADAR_CLIENT_H

// extern "C" : ces fonctions sont écrites en C, mais appelées depuis du C++.
// Sans ce bloc, le C++ renomme les fonctions et l'editeur de liens ne les trouve pas.
#ifdef __cplusplus
extern "C" {
#endif

int  radar_init(const char *ip, int port);
int  radar_envoyer(const char *commande, char *reponse, int taille,int delai_ms, int nb_essais);
void radar_fermer(void);
const char *radar_message_erreur(void);
int  radar_angle(int angle);
int  radar_distance(float *cm);
int  radar_scan(int debut, int fin, int pas, int angles[], float distances[], int max);

#ifdef __cplusplus
}
#endif

#endif