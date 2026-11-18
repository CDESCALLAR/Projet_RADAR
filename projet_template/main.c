#include "stm32f7xx.h"                  // Device header
//#include "servo.h" 

extern void configuration_overclock(void);
extern int Init_GUIThread (void);
extern void Init_Ethernet (void);

#ifdef _RTE_
#include "RTE_Components.h"             // Component selection
#endif
#ifdef RTE_CMSIS_RTOS2                  // when RTE component CMSIS RTOS2 is used
#include "cmsis_os2.h"                  // ::CMSIS:RTOS2
#endif

#define Servo_Min 1000	//largeur d'impusion pour 0° en ticks (1tick= 0,5 us)
#define Servo_Max 5000	//largeur d'impusion pour 180° en ticks
#define Impulse_Trig 20	//impulsion capteur ultrason

/*----------------------------------------------------------------------------
 *      Tâche de test (vide) : à compléter par les étudiants
 *---------------------------------------------------------------------------*/
osThreadId_t tid_Test;

float distance_cm(void){
	static float duree,distance;
	TIM11->CR1 |= (1<<0);	//commence le compteur du TRIG
	if((GPIOG->IDR & (1<<6))==1){//si front montant
		TIM11->CNT =0;
		TIM11->CR1 |= (1<<0);
	}
	else{
		duree = TIM11->CNT;
		distance = duree/58.0;
	}
	return distance;
}

void EXTI9_5_IRQHandler(void){
	EXTI->PR &= ~(1<<6);
	distance_cm();
}

void InitPeriherique (void)
{
	RCC->AHB1ENR |= (1<<5) | (1<<6) | (1<<8);	//Activation des GPIO F,G,I
	RCC->APB1ENR |= (1<<29);	//Activation DAC
	RCC->APB2ENR |= (1<<10);	//Activation ADC3
	RCC->APB2ENR |= (1<<17); // Activation TIM10
	RCC->APB2ENR |= (1<<18); // Activation TIM11
	
	GPIOF->MODER |= (2<<12);		//PF6 en AF mode
	GPIOF->MODER |= (2<<14);		//PF7 en AF mode
	GPIOI->MODER |= (1<<4); //Port PI2 en sortie
	GPIOG->MODER &= ~(1<<6); //Port GP6 en entrée
	GPIOF->MODER |= (3<<20);		//PF10 en analog mode
	GPIOF->AFR[0] |= (3<<24);	//PF6 in AF3 mode (TIM10_CH1)
	GPIOF->AFR[0] |= (3<<28);	//PF7 in AF3 mode (TIM11_CH1)
	
	ADC3->SQR3 |= (1<<3);	//Choix de la voie IN8 de SQR3
	ADC3->CR2 |= (1<<0); //Enable ADC
	
	TIM10->CCMR1 |= (6<<4);	//TIM10_CH1 en PWM mode 1
	TIM10->CCER |= (1<<0);	//connecte OC1 à TIM10_CH1
	TIM11->CCMR1 |= (6<<4);	//TIM11_CH1 en PWM mode 1
	TIM11->CCER |= (1<<0);	//connecte OC1 à TIM11_CH1
	
	// Echauffement 3 seance1
	/*
	TIM10->PSC = 0;	//prescaler
	TIM10->ARR = 15999;	//pwm periode 1ms
	*/
	
	// Pour servomoteur, PWM 50Hz
	TIM10->PSC = 99;	//prescaler
	TIM10->ARR = 39999;	//pwm periode 20ms
	TIM10->CR1 |= (1<<0);	//commence le comptage
	
	//Pour TRIG du capteur ultrason
	TIM11->PSC = 299;	//prescaler
	TIM11->ARR = 39999;	//pwm periode 60ms
	TIM11->CCR1 = Impulse_Trig;	// temps haut 10us
	//TIM11->CR1 |= (1<<0);	//commence le compteur du TRIG
	
	DAC->CR |= (1<<0); 	//Activation output channel 1(PA4)
	
	RCC->APB2ENR |= (1<<14);//allumer SYSCFG
	
	SYSCFG->EXTICR[1] = (6<<8);//dire à EXTI6 que c'est PG6
	EXTI->IMR |= (1<<6);//démasquer EXTI6 dans IMR
	
	EXTI->RTSR |= (1<<6);
	EXTI->FTSR |= (1<<6);//activer fronts montant et descandant pour EXTI6
	
	NVIC_EnableIRQ(EXTI9_5_IRQn);// démasquer dans le NVIC
}


void servo_set_angle(int deg)
{
	if(deg<0) deg = 0;
	else if(deg>180) deg =180;
	TIM10->CCR1 = Servo_Min+(deg*(Servo_Max-Servo_Min)/180);	//pwm rapport cyclique
	TIM10->CR1 |= (1<<0);	//commence le comptage
}

static void Thread_Test (void *argument)
{
  (void)argument;
	int i;
  // Initialisations propres à la tâche (si besoin) ici

  while (1)
  {
    // Votre boucle de test ici (mesure de distance, balayage du servo, etc.)
//		if(i<=180) i+=15;
//		else i=0;
//		
//		servo_set_angle(i);
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
		//SystemClock_Config();
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
  {}
}

