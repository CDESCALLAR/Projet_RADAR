/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::Network:Service
 * Copyright (c) 2004-2019 Arm Limited (or its affiliates). All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    UDP_Socket.c
 * Purpose: UDP Socket Code Template
 * Rev.:    V7.0.0
 *----------------------------------------------------------------------------*/
//! [code_UDP_Socket]
#include "rl_net.h"
#include "stm32f7xx.h"                  // Device header
#include "AppWizard.h"
#include <stdio.h>
#include <string.h>

// Ajouté
#ifndef SERVO_H
#define SERVO_H

#define SERVO_US_MIN     1000   // impulsion à 0° (µs), à calibrer
#define SERVO_US_MAX     2000   // impulsion à 180° (µs), à calibrer
#define SERVO_ANGLE_MAX  180    // angle maximal (degrés)

void servo_set_angle(int deg);

#endif


int32_t udp_sock;                       // UDP socket handle

// PROVISOIRE : à supprimer quand on va fournit distance_cm().
// Convention à convenir avec lui : renvoie la distance en cm, ou une valeur < 0 si pas d'écho.
// Il faudra alors écrire à la place : extern float distance_cm(void);
float distance_cm (void) { return 42.5f; }


// Envoie une réponse (chaîne terminée par \0) à l'expéditeur du datagramme reçu.
static void repondre (int32_t socket, const NET_ADDR *addr, const char *texte) {
  uint8_t *sendbuf;
  int n = 0;
  int i;

  while (texte[n] != '\0') n++;          // longueur de la réponse
  sendbuf = netUDP_GetBuffer (n);        // réserve la zone de données à envoyer
  if (sendbuf == NULL) return;           // plus de mémoire : on abandonne
  for (i = 0; i < n; i++) sendbuf[i] = (uint8_t)texte[i];
  netUDP_Send (socket, addr, sendbuf, n);
}

// Convertit une chaîne de chiffres en entier. Renvoie 0 si OK, -1 sinon.
static int lire_entier (const char *s, int *valeur) {
  int n = 0;
  int i = 0;

  if (s[0] == '\0') return -1;           // rien à lire
  while (s[i] != '\0') {
    if (s[i] < '0' || s[i] > '9') return -1;   // caractère invalide
    n = n * 10 + (s[i] - '0');
    if (n > 1000) return -1;                   // trop grand : hors plage
    i++;
  }
  *valeur = n;
  return 0;
}




// Notify the user application about UDP socket events.
uint32_t udp_cb_func (int32_t socket, const  NET_ADDR *addr, const uint8_t *buf, uint32_t len) {
	
	char cmd[20];
  char rep[20];
  uint32_t i;
  int angle;
  int d10;
  float d;
	
	// 1. Copie dans une chaîne C : buf n'est pas terminé par \0, on s'appuie sur len
  if (len == 0 || len >= 20) {
    repondre (socket, addr, "ERR taille\n");
    return (0);
  }
  for (i = 0; i < len; i++) cmd[i] = (char)buf[i];
  cmd[len] = '\0';

  // 2. Retire la fin de ligne : \n (Linux, netcat) ou \r\n (Windows)
  while (len > 0 && (cmd[len - 1] == '\n' || cmd[len - 1] == '\r')) {
    len--;
    cmd[len] = '\0';
  }

  // 3. Analyse de la commande
  if (strncmp (cmd, "ANGLE ", 6) == 0) { // Compare les 6 premiers car de C
    if (lire_entier (&cmd[6], &angle) != 0 || angle > 180) {
      repondre (socket, addr, "ERR angle\n");
    } 
		else {
      servo_set_angle (angle);
      repondre (socket, addr, "OK\n");
    }
  }
  else if (strcmp (cmd, "DIST") == 0) {
    d = distance_cm ();
    if (d < 0.0f) {
      repondre (socket, addr, "DIST TIMEOUT\n");
    } else {
      d10 = (int)(d * 10.0f + 0.5f);               // distance en dixièmes de cm
      sprintf (rep, "DIST %d.%d\n", d10 / 10, d10 % 10);
      repondre (socket, addr, rep);
    }
  }
  else {
    repondre (socket, addr, "ERR commande\n");
  }
  
  // Data received
  /* Example
  if ((buf[0] == 0x01) && (len == 2)) {
    // Switch LEDs on and off
  }
  */
  return (0);
}
 
// Send UDP data to destination client.
void send_udp_data (void) {
 
  if (udp_sock > 0) {
    /* Example
    // IPv4 address: 192.168.0.1
    NET_ADDR addr = { NET_ADDR_IP4, 2000, 192, 168, 0, 1 };
    uint8_t *sendbuf;
 
    sendbuf = netUDP_GetBuffer (2);
    sendbuf[0] = 0x01;
    sendbuf[1] = 0xAA;
 
    netUDP_Send (udp_sock, &addr, sendbuf, 2);
    */
  }
}
 
// Allocate and initialize the socket.
void Init_Ethernet (void) {
 
  netInitialize ();
 
  // Initialize UDP socket and open port 5000
  udp_sock = netUDP_GetSocket (udp_cb_func);
  if (udp_sock > 0) { // est-ce que la demande a réussi 
    netUDP_Open (udp_sock, 5000);
  }
}

//! [code_UDP_Socket]
