Desenvolvimento do sistema de acionamento de um carrinho dotado de dois
motores, tração diferencial e um sensor de distância para evitar colisões, conforme ilustrado na Figura 1. Os
comandos de controle do sistema são enviados através da interface serial. De acordo com o sentido com
que os motores giram, definido pela polaridade da tensão aplicada em seus terminais, o carrinho pode ir
para frente, para trás, girar no sentido horário ou anti-horário. É possível também controlar a frequência de
rotação dos motores através de sinais PWM de modo a modificar a velocidade de deslocamento do
carrinho.

![alt text](https://github.com/AlefeTiago/2motor-car/blob/main/Carro.PNG)

Uma vez recebido um comando pela UART, o sistema deve executar as ações necessárias e enviar uma
mensagem de confirmação. A mensagem deve ser reenviada periodicamente a intervalos de um segundo
até que um novo comando seja recebido. A Tabela 1 mostra os comandos que devem ser implementados e
as correspondentes mensagens de confirmação. Comandos inválidos devem ser ignorados e não alteram o
comportamento do sistema.

![alt text](https://github.com/AlefeTiago/2motor-car/blob/main/Montagem.PNG)
![alt text](https://github.com/AlefeTiago/2motor-car/blob/main/Comandos.PNG)

Cada motor é acionado através de uma ponte H (L293D), que fornece a potência necessária para alimentá-
lo e permite o controle do sentido e frequência de rotação do eixo. No nosso circuito, os movimentos

previstos nos comandos são obtidos a partir da combinação dos níveis lógicos aplicados nos terminais A1,
A2, A3 e A4 da placa de desenvolvimento, conforme mostrado na Tabela 2.
A velocidade de deslocamento é controlada por meio do duty cycle do sinal PWM gerado no terminal 3 da
placa de desenvolvimento pelo temporizador 2. A frequência do sinal PWM deve ser alta o suficiente, da
ordem de kHz, para que não haja oscilações na rotação do motor. Escolha um valor de frequência e um
modo de operação do modulador apropriados para se obter as especificações do sistema.

![alt text](https://github.com/AlefeTiago/2motor-car/blob/main/Acionamento.PNG)

O sensor ultrassônico HC-SR04 é usado para medir continuamente a distância entre o carrinho e obstáculos
frontais de modo a evitar colisões. Ao receber um pulso de no mínimo 10μs de duração no terminal TRIG
(conectado ao terminal A0 da placa de desenvolvimento), o sensor dispara um uma onda ultrassônica
modulada que se propaga à sua frente. Ao colidir com um obstáculo, parte da onda retorna ao sensor que
capta o eco e gera um pulso em seu terminal ECHO (conectado ao terminal 2 da placa de desenvolvimento)
de largura igual ao tempo de trânsito da onda, ou seja, com duração igual ao intervalo transcorrido entre a
emissão da onda e a recepção do eco. A Figura 3 mostra um diagrama de tempo que ilustra o processo.

![alt text](https://github.com/AlefeTiago/2motor-car/blob/main/Echo.PNG)

A relação entre a duração T do pulso gerado no terminal ECHO e a distância entre o sensor e o obstáculo é
dada pela equação

distância = velocidade do som x T/2
em que o valor da velocidade do som no ar é de aproximadamente 343m/s.
Portanto, para estimar a distância usando essa equação, é necessário medir a duração T do pulso gerado no
terminal ECHO após a realização de um disparo. Podemos detectar os instantes em que as bordas de
subida e de descida do pulso acontecem por meio de interrupções externas associadas a esses eventos e

medir o tempo entre eles usando um temporizador. De acordo com o manual do sensor, a duração do
pulso pode assumir valores na faixa de 118μs a 23529μs, que corresponde a distâncias de 2cm a 4m.
Para prevenir colisões, o sistema deve medir periodicamente, a cada 200ms, a distância entre o carrinho e
obstáculos frontais. Caso a distância seja menor ou igual a 10cm, o carrinho deve parar e ser impedido de
se movimentar para a frente, ignorando o comando ’w’. Nessa situação, o sistema passa a enviar a
mensagem “OBSTACULO!\n” ao invés da mensagem do comando ‘w’. O sistema responde normalmente
aos demais comandos.
Para sinalizar a proximidade de obstáculos, o LED conectado ao terminal A5 deve piscar com frequência
inversamente proporcional à distância medida pelo sensor, ou seja, quanto mais perto de um obstáculo,
mais rápido o LED pisca. Defina uma relação entre frequência e distância que possibilite uma boa
percepção visual da proximidade a partir do LED.

As operações de transmissão e de recepção via interface serial devem ser implementadas utilizando a
estratégia de interrupção para lidar com os eventos assíncronos à execução do programa. Para simplificar o
sistema, não usaremos o buffer circular nesse projeto. As especificações de configuração são:
- Velocidade de transmissão normal (i.e., modo double-speed desativado);
- Modo de transmissão multi-processador desabilitado;
- Número de bits de dados por frame igual a 8;
- Modo assíncrono de funcionamento da USART;
- Sem bits de paridade;
- Uso de um bit de parada;
- Baud rate igual a 9.600 bps.

Em suma, o sistema é composto das seguintes partes principais:

- Recepção e execução de comandos;
- Envio de mensagens: a mensagem associada ao comando em execução deve ser enviada periodicamente a
cada 1s;
- Acionamento dos motores de acordo com o sentido de rotação especificado no comando em execução;
- Ajuste da frequência de rotação dos motores de acordo com o comando em execução;
- Atualização da medida da distância entre o carrinho e os obstáculos frontais a cada 200ms;
- Acionamento do LED indicador de distância de modo que a frequência de cintilação aumente com a
proximidade do obstáculo.
