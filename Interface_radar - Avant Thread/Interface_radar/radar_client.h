#ifndef RADAR_CLIENT_H
#define RADAR_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

int radar_init(const char *ip, int port);
int radar_envoyer(const char *commande, char *reponse, int taille);
int radar_angle(int angle);
int radar_distance(float *cm);
int radar_scan(int debut, int fin, int pas, int angles[], float distances[], int max);
void radar_fermer(void);

#ifdef __cplusplus
}
#endif

#endif