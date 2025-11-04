# PSI-Microcontroladores2-Aula07
Atividade: Semáforos de Pedestres e Veículos

## Objetivo
Desenvolver um sistema embarcado de controle de semáforos para pedestres e veículos, utilizando **threads** e **mutex**, e validar o funcionamento do código por meio de **testes utilizando o modelo V (testes unitários, de integração e de sistema)**.  
Opcionalmente, alunos podem utilizar **IA generativa** para auxiliar na elaboração de trechos de código ou na geração de planos de teste, mas **a avaliação deve se concentrar na qualidade dos testes e na correta validação do sistema**.

## Trabalho em Dupla
- A atividade deve ser realizada **em duplas**.
- Cada membro deve contribuir ativamente no desenvolvimento.
- **Commits no repositório Git** devem ser feitos de forma individual, permitindo a avaliação das contribuições de cada aluno.

## Descrição do Sistema
O sistema deve controlar semáforos de dois tipos: pedestres e carros, incluindo **modo noturno** e **botão de travessia de pedestres**.

### 1. Semáforo de Pedestres
- Contém dois LEDs: verde e vermelho.
- Comportamento:
  - Verde acende por 4 segundos.
  - Vermelho acende por 2 segundos.
- Controle deve ser feito por **duas threads independentes**, garantindo exclusão mútua (**mutex**) entre verde e vermelho.
- Deve ser utilizado o microcontrolador de um integrante.

### 2. Semáforo de Veículos
- Contém três LEDs: verde, amarelo e vermelho.
- Comportamento:
  - Verde acende por 3 segundos.
  - Amarelo acende por 1 segundo.
  - Vermelho acende por 4 segundos.
- Controle deve ser feito por **três threads independentes**, garantindo exclusão mútua (**mutex**) entre os LEDs.
- Deve ser validado que há **sincronismo** entre o semáforo de pedestres e o semáforo de veículos.
- Deve ser utilizado o microcontrolador do outro integrante.

### 3. Modo Noturno
- Um modo alternativo em que os semáforos piscam:
  - Carros: amarelo piscando a cada 2 segundos (1 segundo aceso, 1 segundo apagado).
  - Pedestres: vermelho piscando a cada 2 segundos (1 segundo aceso, 1 segundo apagado).

### 4. Botão de Travessia
- Permite que pedestres acionem o semáforo:
  - Pedestre verde é ativado.
  - Semáforo de carros é bloqueado de forma segura.
- O sistema deve suportar uma única via
- Controle de acesso deve garantir **consistência entre semáforos**, evitando conflito de LEDs.

## Requisitos de Projeto
1. Cada LED deve ser controlado por **uma thread**.
2. Threads devem utilizar **mutex** para evitar conflitos.
3. Modos de operação:
   - Normal (dia)
   - Noturno
   - Travessia acionada pelo botão
4. **Validação do código**
   - Alunos devem elaborar **planos de testes completos**, seguindo o **modelo V**, para garantir o correto funcionamento de cada parte e do sistema completo.
   - Testes devem incluir:
     - Testes unitários (cada semáforo)
     - Testes de integração (interação entre semáforos)
     - Testes de sistema (modos noturno e botão de travessia)
6. **Uso de IA generativa**
   - Sugestão de uso para auxiliar na geração de código ou testes.
   - A avaliação será baseada **na qualidade dos testes e nas evidências registradas no repositório**, não apenas na implementação do código.
7. **Controle de contribuições individuais**
   - Cada membro da dupla deve realizar **commits separados** no repositório.
   - A avaliação individual considerará o histórico de commits e a participação de cada aluno no desenvolvimento e validação do sistema.

Para testar os códigos, decidimos fazer 2 testes para cada uma das categorias. Os testes são os seguintes: 
-  Testes para o código individual — Semáforo de veículos

Teste V1

Pré-condição: modo_noturno = 0, botão não pressionado, sem fio de pedestres conectado ou ignorado.

Passos: Iniciar placa de veículos; verificar sequência normal: verde → amarelo → vermelho → repete.

Pós-condição esperada:

LED verde acende por ~3 s, depois amarelo (~1 s), depois vermelho (~4 s).





Teste V2

Pré-condição: modo_noturno = 0, botão pressionado logo após início da fase VERDE.

Passos: Durante a fase verde, pressione o botão de pedestre; observe se ele força a transição para AMARELO imediatamente. Faça o mesmo teste para quando o LED estiver amarelo ou vermelho.

Pós-condição esperada:

Ao pressionar o botão enquanto veículo está verde, LED veículos muda para amarelo quase que instantaneamente. Caso contrário, o sistema deve ignorar o acionamento.

Depois segue para vermelho conforme previsto.




Teste V3

Pré-condição: modo_noturno = 1.


Pós-condição esperada:

O semáforo deve piscar permanentemente amarelo, ignorando o botão.


Teste P1

Pré-condição: modo_noturno = 0, botão não pressionado, sem fio de veículos conectado ou ignorado.

Passos: Iniciar placa de pedestres; verificar sequência normal: verde → vermelho → repete.

Pós-condição esperada:

LED verde acende por ~4 e depois vermelho (~2 s).


Teste P2

Pré-condição: modo_noturno = 0, botão pressionado logo após início da fase VERMELHA.

Passos: Durante a fase vermelha, pressione o botão de pedestre; observe se ele força a transição para VERDE após 1 segundo.

Pós-condição esperada:

Ao pressionar o botão enquanto veículo está vermelho, LED veículos muda para verde após 1 segundo. Caso contrário, o sistema deve ignorar o acionamento.

Depois segue para vermelho conforme previsto.

Teste V3

Pré-condição: modo_noturno = 1.


Pós-condição esperada:

O semáforo deve piscar permanentemente vermelho, ignorando o botão.

Teste I1

Pré-condição: ambas placas (veículos + pedestres) conectadas por o fio PB1↔PB1 + GND comum. modo_noturno = 0. Veículos iniciam em verde.

Passos: Enquanto veículos estão verdes, pressione o botão na placa de veículos (ou pedestre, conforme design); observe comunicação e transição.

Pós-condição esperada:

Veículos mudam de verde para amarelo imediatamente após o botão.

Veículos então vão para vermelho, pino de sinal passa para “0”, pedestres mudam para verde.

Após tempo previsto, veículos voltam para verde e pedestres voltam para vermelho.

Teste I2

Pré-condição: mesma conexão; botão pressionado quando veículos NÃO estão verdes (por exemplo durante vermelho ou amarelo).

Passos: Pressione o botão nesse momento.

Pós-condição esperada:

Botão ignorado: veículos continuam o ciclo normal sem interrupção por botão.

Pedestres permanecem vermelhos (caso veículos não estejam em vermelho) ou comportamento normal se estiverem.

Comunicação do pino PB1 permanece conforme o ciclo normal.

-    Testes para o modo noturno

Teste N1

Pré-condição: modo_noturno = 1 em ambas placas. Placas individualizadas ou integradas.

Passos: Inicie ambas placas no modo noturno.

Pós-condição esperada:

Veículos: LED amarelo (ou combinação LEDs que representam amarelo) pisca com período de ~2 s (1 s aceso, 1 s apagado).

Pedestres: LED vermelho pisca com o mesmo padrão (1 s aceso, 1 s apagado).

Botão/desligamento normal/intersinalização são ignorados ou inativos.

printk correspondente ao modo noturno aparecem (“Modo noturno ativo…”).

Teste N2

Pré-condição: colocar o veículo (mestre) em modo noturno e o pedestre (escravo) em modo diurno

Passos: Com sistema em modo normal, mude modo_noturno para 1. Observe transição para piscar.

Pós-condição esperada:

Ao ativar modo noturno: veículos piscando amarelo e pedestres permanentemente vermelho.


Teste N3

Pré-condição: colocar o veículo (mestre) em modo diurno e o pedestre (escravo) em modo noturno

Passos: Com sistema em modo normal, mude modo_noturno para 1. Observe transição para piscar.

Pós-condição esperada:

Ao ativar modo noturno: veículos funcionando normalmente e pedestres piscando vermelho. O botão deve funcionar. 





A seguir, estão os resultados dos testes:

Teste V1: A placa está funcionando normalmente, ficando vermelha por 4 segundos, amarela por 1 e verde por 3. Logo, o teste foi um sucesso.
Teste V2: O botão funciona quando o semáforo está verde, mas não quando está vermelho ou amarelo. Logo, o teste foi um sucesso. 
Teste V3: Em modo noturno, o semáforo fica permanentemente piscando amarelo, ignorando os botões. Logo, o teste foi um sucesso.

Teste V1: A placa está funcionando normalmente, ficando vermelha por 2 segundos e verde por 4. Logo, o teste foi um sucesso.
Teste V2: O botão funciona quando o semáforo está vermelho, mas não quando está verde. Logo, o teste foi um sucesso. 
Teste V3: Em modo noturno, o semáforo fica permanentemente piscando vermleho, ignorando os botões. Logo, o teste foi um sucesso.

Teste T1: O botão interrompeu o fluxo e fez o semafóro dos veículos ir para amarelo e, após um segundo, o semáforo dos pedestres foi para verde. Logo, o teste foi um sucesso.
Teste T2: O sistema ignorou completamente o acionamento do botão quando o semáforo dos veículos estava amarelo ou vermelho. Logo, o teste foi um sucesso. 

Teste N1: A placa dos veículos permanentemente pisca amarelo e a dos pedestres permanentemente vermelho. O botão é completamente ignorado, assim como o ciclo verde-amarelo-vermelho. Logo, o teste foi um sucesso.

Teste N2: A placa dos veículos permanentemente pisca amarelo e a dos pedestres fica permanente acesa no vermelho. O botão é completamente ignorado. Assim, o teste foi um sucesso.

Teste N3: A placa dos veículos funciona normalmente enquanto a dos pedestres fica permanentemente piscando vermelho. O botão funciona para o semáforo de veículos. Logo, o teste foi um sucesso. 
