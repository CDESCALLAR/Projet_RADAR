#ifndef RADAR_CLIENT_H
#define RADAR_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

int  radar_init(const char *ip, int port);
int  radar_envoyer(const char *commande, char *reponse, int taille);
void radar_fermer(void);

#ifdef __cplusplus
}
#endif

#endif