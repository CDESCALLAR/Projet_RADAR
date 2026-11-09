#include "stm32f7xx.h"                  // Device header
#include "servo.h"

extern void configuration_overclock(void);
extern int Init_GUIThread (void);
extern void Init_Ethernet (void);


#ifdef _RTE_
#include "RTE_Components.h"             // Component selection
#endif
#ifdef RTE_CMSIS_RTOS2                  // when RTE component CMSIS RTOS2 is used
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2
#endif

/*----------------------------------------------------------------------------
 *      Tâche de test (vide) : à compléter par les étudiants
 *---------------------------------------------------------------------------*/
osThreadId_t tid_Test;

void InitPeriherique (void)
{
	RCC->AHB1ENR |= (1<<5) | (1<<8);	//Activation des GPIO F,I
	RCC->APB1ENR |= (1<<29);	//Activation DAC
	RCC->APB2ENR |= (1<<10);	//Activation ADC3
	RCC->APB2ENR |= (1<<17); // Activation TIM10
	RCC->APB2ENR |= (1<<18); // Activation TIM11
	
	GPIOF->MODER |= (2<<12);		//PF6 en AF mode
	GPIOF->MODER |= (2<<14);		//PF7 en AF mode
	GPIOI->MODER |= (1<<4); //Port PI2 en sortie
	GPIOF->MODER |= (3<<20);		//PF10 en analog mode
	GPIOF->AFR[0] |= (3<<24);	//PF6 in AF3 mode (TIM10_CH1)
	
	ADC3->SQR3 |= (1<<3);	//Choix de la voie IN8 de SQR3
	ADC3->CR2 |= (1<<0); //Enable ADC
	
	TIM10->CCMR1 |= (6<<4);	//TIM10_CH1 en PWM mode 1
	TIM10->CCER |= (1<<0);	//connecte OC1 à TIM10_CH1
	
	TIM10->PSC = 0;	//prescaler
	TIM10->ARR = 15999;	//pwm periode
	
	DAC->CR |= (1<<0); 	//Activation output channel 1(PA4)

}

/*----------------------------------------------------------------------------
 *      Servomoteur : constantes de calibration et fonction de commande
 *---------------------------------------------------------------------------*/
#define SERVO_US_MIN     1000   // largeur d'impulsion (en µs) pour 0°   
#define SERVO_US_MAX     2000   // largeur d'impulsion (en µs) pour 180° 
#define SERVO_ANGLE_MAX  180    // angle maximal accepté (en degrés)

// volatile : la variable est partagée entre plusieurs tâches du RTOS.
volatile int servo_angle_courant = 0;


void servo_set_angle(int deg)
{
	if (deg < 0) deg = 0;
	if (deg > SERVO_ANGLE_MAX) deg = SERVO_ANGLE_MAX;
	servo_angle_courant = deg;
	// Convertit l'angle en largeur d'impulsion (interpolation linéaire) :
	//   largeur = MIN + deg * (MAX - MIN) / 180
	// Le timer compte en µs (1 tick = 1 µs), donc CCR1 = largeur en µs.
	// On multiplie avant de diviser pour garder la précision en calcul entier.
	
	TIM10->CCR1 = SERVO_US_MIN + (uint32_t)deg * (SERVO_US_MAX - SERVO_US_MIN) / SERVO_ANGLE_MAX;
}

static void Thread_Test (void *argument)
{
  (void)argument;

  // Initialisations propres à la tâche (si besoin) ici
	static void Thread_Test (void *argument)
{
  (void)argument;
  while (1)
  {
    // Balayage de test désactivé : le servo est commandé par le serveur UDP
    osDelay(100);
  }
}

  while (1)
  {
    // Votre boucle de test ici (mesure de distance, balayage du servo, etc.)

    osDelay(100);   // période de la boucle : 100 ms
  }
}

static int Init_TestThread (void)
{
  tid_Test = osThreadNew(Thread_Test, NULL, NULL);
  if (tid_Test == NULL) {
    return -1;
  }
  return 0;
}


int main(void)
{
  configuration_overclock(); //pour pouvoir utiliser l'écran LCD

	/**********************************
	Début Initialisation périphérique
	**********************************/

	//Initialiser vos périphériques ici

	/**********************************
	Fin Initialisation périphérique
	**********************************/

	/**********************************
	Utilisation RTOS (Keil RTX5)
	**********************************/
	#ifdef RTE_CMSIS_RTOS2
	  /* Initialize CMSIS-RTOS2 */
	  osKernelInitialize ();
	
		InitPeriherique();

	  /* Create thread functions that start executing */
		Init_Ethernet();
		Init_GUIThread();
		Init_TestThread();   // tâche de test (boucle avec osDelay)

	  /* Start thread execution */
	  osKernelStart();
	#endif

  /* Infinite loop  (ne s'exécute pas avec le RTOS!!!)*/
  while (1)
  {
  }
}