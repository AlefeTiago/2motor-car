/*
 PROJETO FINAL - EA871
 * ALEFE TIAGO FELICIANO RA:230530
 * ARMANDO ARRUDA SOBREIRA RA:213242
 */

#define F_CPU 16000000UL
#include <avr/io.h> //Incluo a biblioteca para uso da Macros
#include <util/delay.h>
#include<avr/interrupt.h>



volatile unsigned int conta_echo = 0; //Vari�vel para contabilizar o tempo no pulso do terminal echo
volatile unsigned int dist_echo = 0;  //Vari�vel para guardar o valor do conta_echo
volatile unsigned int timer1 = 0;     //Vari�vel para contar tempo
volatile unsigned int timer2 = 0;
volatile char flag_a0 = 0;   //Vari�vel para controlar o pulso enviado em A0
volatile char flag_delay = 0;
volatile char flag_echo = 0; //Vari�vel para habilitar a contagem do pulso do echo

// Aqui, vou declarar 3 vari�veis auxiliares que ir�o modificar o valor da string de dist�ncia para a exibi��o serial
unsigned int aux1;
unsigned int aux2;
unsigned int aux3;
//A vari�vel led_aux � uma vari�vel auxiliar que vai definir os per�odos vari�veis para que o led pisque de maneira inversamente proporcional � dist�ncia do obstaculo.
unsigned int led_aux=0;
//Aqui, declaro as strings que ser�o utilizadas nas transmiss�es seriais.
char msg_w[]="FRENTE\n";
char msg_s[]="TRAS\n";
char msg_a[]="ANTI-HORARIO\n";
char msg_d[]="HORARIO\n";
char msg_q[]="PARADO\n";
char msg_e[]="DDDcm\n";
char msg_6[]="Velocidade 60%\n";
char msg_8[]="Velocidade 80%\n";
char msg_0[]="Velocidade 100%\n";
char msg_obstaculo[]="OBSTACULO!\n";
/*Aqui, declaro e aloco alguns ponteiros para cada primeiro valor das strings supracitadas.
Dessa forma, posso iterar sobre a string dentro da rotina de transmiss�o incrementando a posi��o do ponteiro, n�o precisando 
utilizar loops */
char * msg_wv = &(msg_w[0]);
char * msg_sv = &(msg_s[0]);
char * msg_av = &(msg_a[0]);
char * msg_dv = &(msg_d[0]);
char * msg_qv = &(msg_q[0]);
char * msg_ev = &(msg_e[0]);
char * msg_6v = &(msg_6[0]);
char * msg_8v = &(msg_8[0]);
char * msg_0v = &(msg_0[0]);
char * msg_obs = &(msg_obstaculo[0]);
//Vari�vel que vai receber do teclado o comando a ser executado
volatile unsigned char comando;

void setup(){
  
  cli(); //Desabilita as interrup��es globais
 
  /*Config BAUD RATE 9600 - 103 nos REGS! Para isso, o registrador de bits mais significativos se mantem em zero enquanto 
  o registrador de bits menos significativos recebe 67 em hexa que � igual � 103 em decimal!
  
  Bit 15 14 13 12 11 10 9 8
		? ? ? ? UBRRn[11:8]  UBRRnH

				UBRRn[7:0]    UBRRnL
                
   Temos:
        - - - - 0 0 0 0 
        0 1 1 0 0 1 1 1
  */ 

  UBRR0L &= 0x00;
  UBRR0L |= 0x67;
  UBRR0H |= 0x00;
    
    
  /*No registrador UCSR0A vou desativar o modo double speed e tamb�m o modo de multi-processamento*/
  /* Bit 7     6    5    4    3    2     1    0
		RXCn TXCn UDREn FEn DORn UPEn U2Xn MPCMn      UCSE0A
        -     -    -    -    -    -    0     0   

	Como os bits 7,6,5,4,3 n�o nos importam nesse momento, fiz uma opera��o and bit a bit com a mascara 0xFC para
    garantir que os bits 1 e 0 estar�o em nivel l�gico baixo.
        */
    
  UCSR0A &= 0xFC;
    
  /* Aqui, habilito a rececp��o completa e deixo o modo Registrador de Dados Vazio em baixa por enquanto,
visto que ser� ativado posteriormente

  7      6       5     4     3      2      1     0
RXCIEn TXCIEn UDRIEn RXENn TXENn UCSZn2 RXB8n TXB8n   UCSR0B

	1    -       -     1      1    -     0      0 
    
    As opera��es para garantir tal configura��o s�o: and bit a bit com a mascara 0x03 e or bit a bit com a marcara 0x98. Zerando os bits 1 e 0 e Setando os bits 7,4,3
*/    
  UCSR0B &= 0x03;
  UCSR0B |= 0x98;
    
   
   /* 
   
   UMSELn1 UMSELn0 UPMn1 UPMn0 USBSn UCSZn1 UCSZn0 UCPOLn       UCSR0C
   
   0            0     0     0     0      0     1      1 */ 
  /*Aqui, habilito o modo de USART Assincrona, 8 bits por Data Frame, 1 bit de parada e sem bits de paridade.*/
  
  UCSR0C &=0x00;
  UCSR0C |=0x06;
  
  /*Definir o canal PWM (PD3) como sa�da e o terminal ECHO (PD2) como entrada*/
  DDRD = 0X08;
  
  /*Definir os terminais A0-A5 como sa�das*/
  DDRC = 0x3F;
  
  /*Configura��o das interrup��es do terminal ECHO*/
  
  /*EIMSK - External Interrupt Mask Register*/
  /* - - - -  - - INT1 INT0 */
  /* - - - -  - -   0    1  */
  /*INT1 = 0, para desabilitar interrup��es no pino INT1 (PD3)*/
  /*INT0 = 1; para habilitar interrup��es no pino INT0 (PD2)*/
  EIMSK = 0x01;
  
  /*EICRA - External Interrupt Control Register A*/
  /* - - - -  ISC11 ISC10 ISC01 ISC00 */
  /* - - - -   -     -     0     1    */
  /*ISC01 e ISC00 = 0 1; qualquer borda em INT0 chama a interrup��o*/
  EICRA = 0x01;
  
  /*Configura��o do temporizador 0 para a contagem de tempo*/
  
  /*Usamos a configura��o CTC com TOP em OCR2A = 117 (0x75) e com um prescaler de 8*/
  /*Com essas configura��es e a frequ�ncia de clock de 16MHz temos uma faixa no temporizador de T = (8 * 118)/(16e^6) = 59us*/
  OCR0A = 0x75;
  
  /*TIMSK0 - Timer/Counter Interrupt Mask Register*/
  /* - - - -  - OCIE0B OCIE0A TOIE0*/
  /* - - - -  -    0     1      0  */
  /*OCIE0A = 1; para habilitar interrup��es em Compare Match com o valor em OCR2A*/
  TIMSK0 = 0x02;
  
  /*TCCR0B ? Timer/Counter Control Register B*/
  /*FOC0A FOC0B ? ? WGM02 CS02 CS01 CS00*/
  /*  0     0   - -   0     0    1    0 */
  /*FOC0A e FOC0B = 0 0; pois n�o uso a for�agem de compara��o*/
  /*WGM02 = 0; para modo CTC com TOP = OCR0A*/
  /*CS02, CS01 e CS00 = 0 1 0; para configurar o prescaler para 8*/
  TCCR0B = 0x02;
  
  /*TCCR0A ? Timer/Counter Control Register A*/
  /*COM0A1 COM0A0 COM0B1 COM0B0 ? ? WGM01 WGM00*/
  /*  0       0      0     0    - -   1     0  */
  /*COM2A1 e COM2A0 = 0 0; n�o usamos o terminal OC0A*/
  /*COM2B1 e COM2B0 = 0 0; n�o usamos o terminal OC0B*/
  /*WGM21 e WGM20 = 1 0; para modo CTC com TOP = OCR0A*/
  TCCR0A = 0x02;
  
  /*Configura��o do PWM*/
  
  /*O PWM usa o temporizador 2 do terminal 3 da placa (OC2B)*/
  /*Usamos a configur��o Fast PWM com TOP em 0XFF, e com um prescaler de 32*/
  /*Com essas configura��es e a frequ�ncia de clock de 16MHz temos um per�odo de Tpwm = (32 * 256)/(16e^6) = 0,512ms 
  /*Nos dando uma frequ�ncia na ordem de KHz como pedido, de Fpwm = 1.953125 kHz*/
  
  /*Como o sistema deve iniciar com um duty cycle de 60%, configuramos o OCR2B para o valor
  correspondente, as contas para cada duty cycle s�o mostradas depois*/
  OCR2B = 0X99;

  /*TCCR2B ? Timer/Counter Control Register B*/
  /*FOC2A FOC2B ? ? WGM22 CS22 CS21 CS20*/
  /*  0     0   - -   0     0    1    1 */
  /*FOC2A e FOC2B = 0 0; pois n�o uso a for�agem de compara��o*/
  /*WGM22 = 0; para modo Fast PWN com TOP = 0XFF*/
  /*CS22, CS21 e CS20 = 0 1 1; para configurar o prescaler para 32*/
  TCCR2B = 0x03;
  
  /*TCCR2A ? Timer/Counter Control Register A*/
  /*COM2A1 COM2A0 COM2B1 COM2B0 ? ? WGM21 WGM20*/
  /*  0       0      1     0    - -   1     1  */
  /*COM2A1 e COM2A0 = 0 0; pois n�o usamos o registrador OC2A*/
  /*COM2B1 e COM2B0 = 1 0; para o modo Fast PWN com Clear OC2B on Compare Match, set OC2B at BOTTOM*/
  /*WGM21 e WGM20 = 1 1; para modo Fast PWN com TOP = 0XFF*/
  TCCR2A = 0x23;
  
  sei(); //Habilita as interrup��es globais
}

void dutycycle60(){
  /*Como o TOP � em 0XFF (255), para calcular o valor de OCR2B necess�rio para um certo duty cycle fazemos: dutycycle = (OCR2B + 1)/256*/
  /*Para um duty cycle = 0.6, obtemos (arredondando) um OCR2B = 153, 0X99 em hexadecimal*/
  OCR2B = 0x99;
}
  
void dutycycle80(){
  /*Como o TOP � em 0XFF (255), para calcular o valor de OCR2B necess�rio para um certo duty cycle fazemos: dutycycle = (OCR2B + 1)/256*/
  /*Para um duty cycle = 0.8, obtemos (arredondando) um OCR2B = 204, 0XCC em hexadecimal*/
  OCR2B = 0xCC;
}

void dutycycle100(){
  /*Como o TOP � em 0XFF (255), para calcular o valor de OCR2B necess�rio para um certo duty cycle fazemos: dutycycle = (OCR2B + 1)/256*/
  /*Para um duty cycle = 1, obtemos (arredondando) um OCR2B = 255, 0XFF em hexadecimal*/
  OCR2B = 0xFF;
}

ISR (TIMER0_COMPA_vect){
  /*Cada vez que essa rotina � chamada pela interrup��o de Compare Match com o OCR2A, passaram 59us*/
  /*Como visto no roteiro, 118us no terminal ECHO equivale � 2cm de dist�ncia, logo 59us equivale � 1cm*/
  /*Assim, quando fizermos a contagem do tempo do pulso atrav�s da vari�vel conta_echo, teremos o valor j� medido em cm, sem necessidade de fazer a opera��o*/
  /*Caso a flag para contar o pulso esteja setada*/
if (flag_echo == 1)
    conta_echo++;  //Incrementa o valor da contagem de tempo do pulso no terminal ECHO
  
  timer1++;      //Incrementa o timer1
  timer2++;     /*Aqui, vou incrementar a vari�vel timer2 at� que a mesma chegue em 16950 incrementos, pois assim, saberei quando se passou o tempo para uma nova transmiss�o*/
  led_aux++;    /*Aqui, incremento uma vari�vel que ser� utilizada futuramente para criar periodos variados para o led piscar*/
  
  
  //---------------------------------------------------------------------//
  
  /*A cada 1s 16950 faixas de 59us (16950*59us = 1.0001) seu ativo a transmiss�o, visto que ao final 
  de cada transmiss�o isso � desativado*/
  
if (timer2 == 16950){
  timer2=0; // Zero o timer para que uma nova contagem de 1s aconte�a. 
  UCSR0B |= 0x20; //Ativo as interrup��es de transmiss�o.
  }

  /*Se a flag do pulso em A0 estiver setada, o pulso foi mandado*/
if (flag_a0 = 1){
    PORTC &= 0xFE;  //Como ja passaram 59us, posso zerar o terminal A0 encerrando o pulso
    flag_a0 = 0;
  }
      
  /*Ap�s 200ms, que s�o 3390 faixas de 59us (3390*59us = 0.20001s)*/
if (timer1 == 3390){
    timer1 = 0;     //Reinicia a contagem
    flag_a0 = 1;    //Seta a flag indicando que o pulso foi mandado em A0
    PORTC |= 0x01;  //Seta o terminal A0 para enviar o pulso
  }
      }

/*Rotina de servi�o de interrup��o no terminal ECHO (INT0)*/
ISR (INT0_vect){
  /*Caso o o valor do bit 2 de PIND esteja em n�vel alto ap�s a chamada de interrup��o, foi uma borda de subida*/
  if ((PIND & 0X04) == 0x04){
    conta_echo = 0;           //Reinicia o contador
    flag_echo = 1;            //Seta a flag para contar o tempo do pulso ECHO
  }
  /*Caso o o valor do bit 2 de PIND esteja em n�vel baixo ap�s a chamada de interrup��o, foi uma borda de descida*/
  if ((PIND & 0X04) == 0x00){
    dist_echo = conta_echo;  //Guarda o valor do contador
    flag_echo = 0;           //Zera a flag para contar o tempo do pulso ECHO
 	
    /*
 Nas proximas linhas, vou preparar a string para enviar a distancia do echo pelo monitor serial:
 * Separo unidade dezena e centena fazendo:
 *      unidade=dist_echo%10;
        dezena=(dist_echo/10)%10;
        centena=((dist_echo/10)/10)%10;
 * e fa�o um update na string da mensagem com esses valores na posi��o [0],[1],[2]. Lembrando que 
 * devemos colocar na string os caracteres correspondentes da tabela ASCII, ent�o fa�o um offset de 0x30 (codigo ASCII corresponde para [0,9]).
 */
    
    aux1=dist_echo%10;
    aux2=(dist_echo/10)%10;
    aux3=((dist_echo/10)/10)%10;
    
    msg_e[0]=aux3+(0x30);
    msg_e[1]=aux2+(0x30);
    msg_e[2]=aux1+(0x30);

  }
}


//----------------------------------------Essa parte do c�digo se trata do vetor de recep��o da USART ------------------
ISR(USART_RX_vect){
	comando = UDR0; //Recebo o valor digitado na entrada serial
  	timer2=0; // Zero o auxiliador de contagem de 1s para garantir que depois de transmitido uma mensagem teremos 1s de espera
  	UCSR0B |= 0x20; /* Habilito a transmiss�o para garantir que se um comando � pedido enquanto estamos em um periodo de espera, a transmiss�o da mensagem do comando � feita de fato
                     * ou seja, n�o precisamos esperar o 1s para transmitir */
}

ISR(USART_UDRE_vect){
  
 /*A estrutura dos proximos modulos(um modulo para cada string) � bastante semelhante (na verdade � logicamente igual)! 
  A ideia �: verificamos qual comando est� armazenado na variavel, vejo se o valor para onde estou apontando � o fim da frase e tomo algum caminho:
  *         - Se � o fim da frase, desabilito as interrup��es de tranmiss�o, volto o ponteiro do vetor da mensagem para a primeira posi��o do vetor e fecho o bloco condicional.
  *         - Se n�o � fim da frase, continuo varrendo a string a cada vez que a usart � chamada, sempre incrementando a posi��o do ponteiro. 
  * Criei um vetor para cada mensagem para evitar que comandos disputem um vetor, gerando transmiss�es incompletas na USART.
  */
  
 if (comando == 'w'){
 	if(dist_echo >= 10){
 		if (*msg_wv != '\0'){  
 			UDR0 = *msg_wv;
 			msg_wv++; }
	 	else{
 			msg_wv = &(msg_w[0]);
 			UCSR0B &= ~(0x20);}}
 	else{
 		if (*msg_obs != '\0'){  
 			UDR0 = *msg_obs;  /* A l�gica adicional aqui no primeiro bloco � avaliar se a dist�ncia do obstaculo � maior que 10cm, se �, a mensagem exibida quando pressionado
                             o comando w deve ser 'Frente! Se for menor, quando w � pressionado a mensagem exibida deve ser 'Obstaculo!'*/ 
  			msg_obs++; }
        else{
            msg_obs = &(msg_obstaculo[0]);
            UCSR0B &= ~(0x20);}}} 
    
if (comando == 's'){  
    if (*msg_sv != '\0'){  
    	UDR0 = *msg_sv; 
    	msg_sv++; }
	else{
    	msg_sv = &(msg_s[0]);
    	UCSR0B &= ~(0x20);}}
  
 /* Os proximos blocos possuem a mesma logica supracitada, logo, n�o carecem de comentarios adicionais. A unica mudan�a � o vetor de mensagem e o ponteiro que 
  aponta pra esse vetor. Lembrando que na logica implementada, cada vetor possui um ponteiro*/
if (comando == 'a'){
    if (*msg_av != '\0'){  
   		UDR0 = *msg_av;
    	msg_av++; }
	else{
    	msg_av = &(msg_a[0]);
    	UCSR0B &= ~(0x20);}}
  
if (comando == 'd'){
    if (*msg_dv != '\0'){  
    	UDR0 = *msg_dv;
    	msg_dv++; }
	else{
    	msg_dv = &(msg_d[0]);
    	UCSR0B &= ~(0x20);}}  
  
if (comando == 'q'){
    if (*msg_qv != '\0'){  
   	 	UDR0 = *msg_qv;
    	msg_qv++; }
	else{
    	msg_qv = &(msg_q[0]);
    	UCSR0B &= ~(0x20);}} 
  
if (comando == 'e'){
    if (*msg_ev != '\0'){  
    	UDR0 = *msg_ev;
      	msg_ev++; }    
else{
    	msg_ev = &(msg_e[0]);
    	UCSR0B &= ~(0x20);}}   
  
if (comando == '6'){
    if (*msg_6v != '\0'){  
    	UDR0 = *msg_6v;
    	msg_6v++; }
	else{
    	msg_6v = &(msg_6[0]);
    	UCSR0B &= ~(0x20);}}   
if (comando == '8'){
    if (*msg_8v != '\0'){  
    	UDR0 = *msg_8v;
    	msg_8v++; }
	else{
    	msg_8v = &(msg_8[0]);
    	UCSR0B &= ~(0x20);}}
if (comando == '0'){
    if (*msg_0v != '\0'){  
    	UDR0 = *msg_0v;
    	msg_0v++; }
	else{
    	msg_0v = &(msg_0[0]);
    	UCSR0B &= ~(0x20);}}}

/*Aqui, encerro o bloco de transmiss�o da USART e inicio o programa principal*/




int main(void){
  _delay_ms(1); //Delay para otimizar a simula��o no TinkerCad
  setup(); //Chamo o Setup para aplicar as configura��es cabidas a cada registrador
  comando='q'; // Inicio tamb�m com um comando de Parado.
 
while(1){
   /*
   A proposta para o Led piscar � a seguinte:
   Pensando inicialmente em um cen�rio de oscila��o hipot�tico
    * Sabemos que o frqu�ncia � inversamente proporcional � dist�ncia. Logo, sabemos que o per�odo � diretamente proporcional � dist�ncia!
    * Sendo k uma constante positiva, temos que o per�odo � diretamente proporcional � distancia * k 
    Como queremos que o led pisque com uma frequ�ncia inversamente proporcional � dist�ncia, podemos avaliar com uma variavel auxiliadora per�odos variados 
    * cujo tamanho � diretamente proporcional � dist�ncia. A dist�ncia em quest�o n�s ja temos "dist_echo" e devemos escolher uma constante k de maneira 
    * que o piscar seja vis�vel. Por tentativa e erro, escolhemos k=90.
    * Dessa forma, a l�gica para que o led pisque �: 
    *           --Sempre que a rotina de interrup��o CompaVect � chamada, eu incremento a vari�vel led_aux. Ja na main, eu avalio se esse valor ja chegou em dist_echo*90
    * definindo assim um per�odo que varia conforme dist_echo varia. 
    *           --Visto que tal valor foi atingido, se o led estava aceso apagamos, se estava apagado, acendemos. E zeramos a variavel de contagem para o proximo periodo.
    */
if(led_aux >= (dist_echo*90)){
    led_aux = 0;
    if ((PINC & 0x20) == 0x20){
    	PORTC &= 0xDF; 
    }
    else{
   	 PORTC |= 0x20;
    }  
  }
  /*
   Nessa etapa da main, verifico qual letra est� armazenada na vari�vel que recebe o comando e opero a ponte H para tal execu��o!
   * Para w:
   *        -PORTC |= 0x0A;
            PORTC &= ~(0x14);
   * 
   * Para s:
   *         PORTC |= 0x14;
             PORTC &= ~(0x0A);
   * Para a:
   *        PORTC |= 0x12;
            PORTC &= ~(0x0C) 
   * para d:
   *        PORTC |= 0x0D;
            PORTC &= ~(0x12)
   * para q:
   *        PORTC &= ~(0x1E)
   *
   * Lembrando que w tem a restri��o da dist�ncia do echo! Se menor que 10, o carro deve parar!
   *       
   */
if (comando == 'w'){
     if(dist_echo >= 10){
      		PORTC |= 0x0A;
      		PORTC &= ~(0x14);}
     else{
      		PORTC &= ~(0x1E);}}
if (comando == 's'){
      PORTC |= 0x14;
      PORTC &= ~(0x0A);}
if (comando == 'a'){
      PORTC |= 0x12;
      PORTC &= ~(0x0C);}
if (comando == 'd'){
      PORTC |= 0x0D;
      PORTC &= ~(0x12);}
if (comando == 'q'){
      PORTC &= ~(0x1E);}
if (comando == '6'){
      dutycycle60();}   
if (comando == '8'){
      dutycycle80();}
if (comando == '0'){
      dutycycle100();}}
return 0;
}