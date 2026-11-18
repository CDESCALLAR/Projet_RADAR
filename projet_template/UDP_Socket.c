/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::Network:Service
 * Copyright (c) 2004-2019 Arm Limited (or its affiliates). All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    UDP_Socket.c
 * Purpose: UDP Socket Code Template
 * Rev.:    V7.0.0
 *----------------------------------------------------------------------------*/
//! [code_UDP_Socket]

// -------------------Déclaration des bibliothèques --------------------------------------------

#include "rl_net.h"
#include "stm32f7xx.h"                  // Device header
#include "AppWizard.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cmsis_os2.h"                  // osThreadNew, osDelay


// -------------------Déclaration des variables globaux -----------------------------------------

#define SCAN_MESURES_MAX  100   // au-dela, la reponse ne tient plus dans un datagramme (1472 /12) = 122
#define SCAN_REP_MAX  1300      // 100 lignes de 12 caracteres max + "END\n"
#define FLAG_SCAN  0x0001               // Flag 0 de la tache Scan : "un SCAN est demandé"

static char scan_rep[SCAN_REP_MAX];            // texte de la reponse du SCAN

// Variables partagées entre udp_cb_func (tache réseau) et Thread_Scan.
// volatile : modifiées par une tache, lues par l'autre.
static volatile int scan_demande = 0;          // 0 = libre, 1 = SCAN demande ou en cours
static volatile int scan_debut, scan_fin, scan_pas;
static int32_t      scan_socket;               // à qui repondre : socket et adresse
static NET_ADDR     scan_dest;

int32_t udp_sock;                       // UDP socket handle
osThreadId_t ID_Scan;                   // identifiant de la tache du balayage
osThreadAttr_t configScan = {.stack_size = 2048};   // pile de 2048 octets (sprintf avec %f)


// -------------------Appel des fonctions (extérieures) ------------------------------------

extern void servo_set_angle(int deg);
extern float distance_cm(void);
//float distance_cm (void) { return 42.5f; }


// -----------------------Déclaration des fonctions ----------------------------------------------

// Envoie une réponse (chaîne terminée par \0) à l'expéditeur du datagramme reçu.
static void repondre (int32_t socket, const NET_ADDR *addr, const char *texte) {
  uint8_t *sendbuf;
  int n = 0;
  int i;

  while (texte[n] != '\0') n++;          // longueur de la réponse
  sendbuf = netUDP_GetBuffer (n);        // réserve la zone de données à envoyer
  if (sendbuf == NULL) return;           // plus de mémoire, on abandonne
  for (i = 0; i < n; i++) sendbuf[i] = (uint8_t)texte[i];
  netUDP_Send (socket, addr, sendbuf, n);
}

// Convertit une chaîne de chiffres en entier. Renvoie 0 si OK, -1 sinon.
static int lire_entier (const char *s, int *valeur) {
  
	char *fin;
	long n;
	
	if (s[0] == '\0') return -1; // rien à lire
	n = strtol(s, &fin,10);  // convertit le début de s en base 10
	if (*fin !='\0') return -1;  // il reste un caractère qui n'est pas un chiffre
	if (n<0 || n> 1000) return -1; // caractère négatif ou supérieur
	*valeur = (int) n;
  return 0;
}

// Lit "<debut> <fin> <pas>" (trois entiers separes par des espaces).
// Renvoie 0 si OK, -1 si la commande est mal formee.
static int lire_scan (const char *s, int *debut, int *fin, int *pas)
{
	char *suite;
  long a, b, c;

  a = strtol(s, &suite, 10);            // 1er nombre ; "suite" pointe juste apres
  if (suite == s) return -1;            // aucun chiffre lu
  s = suite;														// saute l'espace et lit la suite
  b = strtol(s, &suite, 10);            // 2e nombre
  if (suite == s) return -1;
  s = suite;
  c = strtol(s, &suite, 10);            // 3e nombre
  if (suite == s) return -1;
  if (*suite != '\0') return -1;        // il reste des caracteres : mal formee

  *debut = (int) a;
  *fin   = (int) b;
  *pas   = (int) c;
	
  return 0;
}
 
// Notify the user application about UDP socket events.
uint32_t udp_cb_func (int32_t socket, const  NET_ADDR *addr, const uint8_t *buf, uint32_t len) {
	
	char cmd[20];
  char rep[20];
  uint32_t i;
  int32_t angle;
	int32_t debut, fin, pas; // pour le balayage
  float d;

  // Copie dans une chaîne C : buf n'est pas terminé par \0, on s'appuie sur len
  if (len == 0 || len >= 20) {
    repondre (socket, addr, "ERR taille\n");
    return (0);
  }
  for (i = 0; i < len; i++) cmd[i] = (char)buf[i];
  cmd[len] = '\0';

  // Retire la fin de ligne : \n 
  while (len > 0 && (cmd[len - 1] == '\n' || cmd[len - 1] == '\r')) {
    len--;
    cmd[len] = '\0';
  }

	// Vérification du drapeau
	if (scan_demande) {                               // balayage en cours : servo et capteur occupés
    repondre (socket, addr, "ERR occupe\n");
  }
	
  // Analyse de la commande ANGLE nombre   
  else if (strncmp (cmd, "ANGLE ", 6) == 0) {
    if (lire_entier (&cmd[6], &angle) != 0 || angle > 180) {
      repondre (socket, addr, "ERR angle\n");
    } else {
      servo_set_angle (angle);
      repondre (socket, addr, "OK\n");
    }
  }
	// Analyse de la commande DIST
  else if (strcmp (cmd, "DIST") == 0) {
    d = distance_cm ();
    if (d < 0.0f || d > 999.0f) {                  // pas d'écho (ou valeur très supérieure) en cm
      repondre (socket, addr, "DIST TIMEOUT\n");
    } else {
      sprintf (rep, "DIST %.1f\n", d);
      repondre (socket, addr, rep);
    }
  }
	// Analyse de la commande SCAN angle angle pas
	  else if (strncmp (cmd, "SCAN ", 5) == 0) {
    if (lire_scan (&cmd[5], &debut, &fin, &pas) != 0) {
      repondre (socket, addr, "ERR format\n");
    } else if (debut < 0 || fin > 180 || debut > fin) {
      repondre (socket, addr, "ERR angle\n");
    } else if (pas < 1 || pas > 180) {
      repondre (socket, addr, "ERR pas\n");
    } else if ((fin - debut) / pas + 1 > SCAN_MESURES_MAX) {
      repondre (socket, addr, "ERR trop de mesures\n");
		} else if (ID_Scan == NULL) {
      repondre (socket, addr, "ERR interne\n");     // la tache du balayage n'existe pas
    } else {
      scan_debut  = debut;                          // on note la demande
      scan_fin    = fin;
      scan_pas    = pas;
      scan_socket = socket;                         // ...et à qui repondre
      scan_dest   = *addr;
      scan_demande = 1;                             // ...on lève le drapeau (pour dire occupé)
			osThreadFlagsSet (ID_Scan, FLAG_SCAN);        // ...puis on reveille Thread_Scan
    }                                               // la reponse part de Thread_Scan
  }
		// Commande inconnu
  else {
    repondre (socket, addr, "ERR commande\n");
  }
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

// Tache du balayage : dort jusqu'a ce qu'un SCAN soit demande.
void Thread_Scan (void const * argument) {

  int a, n;
	float d;

  while (1) {
    osThreadFlagsWait (FLAG_SCAN, osFlagsWaitAny, osWaitForever);   // sommeil jusqu'au reveil
		
		servo_set_angle (scan_debut);                   // on va d'abord à l'angle de depart...
    osDelay (600);                 // ...en laissant le temps de traverser (600 ms pour 180° car 0,17s par 60°)

    n = 0;                                          // nombre de caracteres ecrits dans scan_rep
    for (a = scan_debut; a <= scan_fin; a += scan_pas) {
      servo_set_angle (a);                          // 1) on place le servo
      osDelay (200);                     						// 2) on attend qu'il se stabilise
      d = distance_cm ();                           // 3) on mesure

      if (d < 0.0f || d > 999.0f)                   // 4) pas d'echo : on écrit TIMEOUT
        n += sprintf (&scan_rep[n], "%d TIMEOUT\n", a);
      else                                          //    sinon la vraie distance
        n += sprintf (&scan_rep[n], "%d %.1f\n", a, d);

      osDelay (60);      														// 5) pause avant la mesure suivante
    }
    n += sprintf (&scan_rep[n], "END\n");

    repondre (scan_socket, &scan_dest, scan_rep); // UN seul datagramme
    scan_demande = 0;                               // libre pour la commande suivante
  }
}
 
// Allocate and initialize the socket.
void Init_Ethernet (void) {
 
  netInitialize ();
	
	// Creation de la tache du balayage (elle dort jusqu'à la première commande SCAN)
  ID_Scan = osThreadNew ((osThreadFunc_t) Thread_Scan, NULL, &configScan);
 
  // Initialize UDP socket and open port 5000
  udp_sock = netUDP_GetSocket (udp_cb_func);
  if (udp_sock > 0) {
    netUDP_Open (udp_sock, 5000); //Port 5000 de la carte
  }
}

//! [code_UDP_Socket]
