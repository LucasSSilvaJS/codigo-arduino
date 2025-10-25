# Projeto de Totem de Votação Interativo com RFID e ESP32

Este repositório contém o código-fonte para um totem de votação interativo desenvolvido com ESP32, leitores RFID MFRC522 e um display LCD I2C. O sistema permite que usuários votem em uma pergunta atual, verificando sua identidade via cartão RFID e comunicando-se com uma API web para registro de votos e exibição de resultados em tempo real.

## 1. Visão Geral do Projeto

O totem funciona como um ponto de interação onde o usuário aproxima um cartão RFID para participar de uma votação de "Sim" ou "Não". O sistema gerencia o ciclo completo de interação:

1.  **Conexão WiFi:** Estabelece conexão segura com a rede.
2.  **Espera de Cartão:** Aguarda a aproximação de um cartão em um dos leitores RFID.
3.  **Verificação/Cadastro de Usuário:** Verifica se o UID do cartão está cadastrado na API. Se não estiver, realiza o cadastro.
4.  **Obtenção da Pergunta:** Busca a pergunta ativa mais recente na API.
5.  **Votação:** Aguarda a aproximação do cartão em um dos leitores (associados a "Sim" ou "Não") para registrar o voto.
6.  **Envio do Voto:** Envia a interação para a API.
7.  **Exibição do Resultado:** Exibe o placar atualizado da votação na tela LCD.
8.  **Retorno à Tela Inicial:** Reinicia o ciclo para o próximo usuário.

## 2. Componentes Necessários

A tabela a seguir lista os componentes de hardware essenciais para a montagem do totem:

| Componente | Quantidade | Descrição |
| :--- | :--- | :--- |
| **ESP32** (ou similar) | 1 | Microcontrolador principal com capacidade WiFi. |
| **Módulo LCD 16x2 I2C** | 1 | Display para exibir mensagens e resultados. |
| **Módulo Leitor RFID MFRC522** | 2 | Um para a opção "Sim" e outro para a opção "Não". |
| **Jumpers** | Vários | Fios de conexão. |
| **Fonte de Alimentação** | 1 | Para alimentar o ESP32 e os periféricos. |

## 3. Configuração de Software

### 3.1. Bibliotecas

O projeto utiliza as seguintes bibliotecas do Arduino IDE:

*   **`WiFi.h`**: Gerenciamento da conexão WiFi.
*   **`HTTPClient.h`**: Para requisições HTTP/HTTPS à API.
*   **`Wire.h`**: Comunicação I2C (para o LCD).
*   **`LiquidCrystal_I2C.h`**: Controle do display LCD I2C.
*   **`SPI.h`**: Comunicação SPI (para os leitores RFID).
*   **`MFRC522.h`**: Gerenciamento dos leitores RFID.
*   **`ArduinoJson.h`**: Para parsear as respostas JSON da API.
*   **`WiFiClientSecure.h`**: Para requisições HTTPS seguras.

### 3.2. Variáveis de Configuração

As seguintes constantes no código (`pasted_content.txt`) devem ser configuradas de acordo com o ambiente:

| Variável | Descrição | Exemplo Padrão |
| :--- | :--- | :--- |
| `ssid` | Nome da rede WiFi. | `"SENAC-Mesh"` |
| `password` | Senha da rede WiFi. | `"09080706"` |
| `API_BASE` | URL base da API para comunicação. **Nota:** O código utiliza `client.setInsecure()` para requisições HTTPS, o que deve ser revisado para um ambiente de produção. | `"https://projeto-bigdata.onrender.com"` |
| `TOTEM_ID` | Identificador único deste totem. | `"dfff2270cd60"` |

## 4. Diagrama de Conexões (Fritzing/Esquemático)

O código utiliza dois leitores RFID MFRC522 e um display LCD I2C. O ESP32 é o microcontrolador central.

### 4.1. Conexões I2C (LCD)

O display LCD I2C utiliza os pinos padrão de I2C do ESP32:

| Componente | Pino do LCD I2C | Pino do ESP32 |
| :--- | :--- | :--- |
| **LCD I2C** | SDA | `GPIO 21` (`SDA_PIN`) |
| **LCD I2C** | SCL | `GPIO 22` (`SCL_PIN`) |

### 4.2. Conexões SPI (RFID)

Ambos os leitores RFID compartilham os pinos SPI (MISO, MOSI, SCK) e utilizam pinos de *Chip Select* (SS) e *Reset* (RST) separados para endereçamento individual.

| Pino Compartilhado | Pino do ESP32 |
| :--- | :--- |
| **MISO** | `GPIO 19` (`MISO_PIN`) |
| **MOSI** | `GPIO 23` (`MOSI_PIN`) |
| **SCK** | `GPIO 18` (`SCK_PIN`) |

| Componente | Pino de Controle | Pino do ESP32 |
| :--- | :--- | :--- |
| **RFID "Não"** | SS (Slave Select) | `GPIO 5` (`SS_PIN_NAO`) |
| **RFID "Não"** | RST (Reset) | `GPIO 4` (`RST_PIN_NAO`) |
| **RFID "Sim"** | SS (Slave Select) | `GPIO 25` (`SS_PIN_SIM`) |
| **RFID "Sim"** | RST (Reset) | `GPIO 26` (`RST_PIN_SIM`) |

### 4.3. Diagrama Visual de Componentes

O diagrama a seguir ilustra a arquitetura lógica e as principais conexões entre os componentes e a API:

![Diagrama Visual de Componentes](https://private-us-east-1.manuscdn.com/sessionFile/vQ2kE4u9BQafEJpiXOd9aE/sandbox/vkci1brTdlSuQKDD0m7S6m-images_1761404240028_na1fn_L2hvbWUvdWJ1bnR1L2RpYWdyYW1hX2NvbXBvbmVudGVz.png?Policy=eyJTdGF0ZW1lbnQiOlt7IlJlc291cmNlIjoiaHR0cHM6Ly9wcml2YXRlLXVzLWVhc3QtMS5tYW51c2Nkbi5jb20vc2Vzc2lvbkZpbGUvdlEya0U0dTlCUWFmRUpwaVhPZDlhRS9zYW5kYm94L3ZrY2kxYnJUZGxTdVFLREQwbTdTNm0taW1hZ2VzXzE3NjE0MDQyNDAwMjhfbmExZm5fTDJodmJXVXZkV0oxYm5SMUwyUnBZV2R5WVcxaFgyTnZiWEJ2Ym1WdWRHVnoucG5nIiwiQ29uZGl0aW9uIjp7IkRhdGVMZXNzVGhhbiI6eyJBV1M6RXBvY2hUaW1lIjoxNzk4NzYxNjAwfX19XX0_&Key-Pair-Id=K2HSFNDJXOU9YS&Signature=qg~WG0QXyLi7I1Xsl~a1axJuX5ZuZ4ktP4QFDjavG1I8uk89ID2P0ywbFcSeMPsGrp0y-bw1xzjkooZM7Sobh3I2A1MOzAYjawLP7ERj6cd5Qm9he7ipkyTUxYi1kE3QNN7phQtqzDcp0CRL9JuFm4DRie8TXktsCpdNRQlK-VzvOS01~f1VRiVNP8Uw1q7CJg-YgS-2PtElQhfOg4KjItdoXytTngtP~8Z8amLqho7iJ207zTwT9RHt1f8nbplpz5umKvfJQNoA88EBEpKTv~065vlxWkyMcgKpSB-wjh6u7SzysgNKkzeMJG6v4lMt20Y-BXXvdCkjy7xGJI5z4A__)

## 5. Fluxo de Operação (Máquina de Estados)

O código é estruturado como uma **Máquina de Estados** para gerenciar o fluxo de interação do usuário. A variável `estado` define a ação atual do totem.

| Estado | Descrição | Ações Principais | Transições |
| :--- | :--- | :--- | :--- |
| `ESPERA_CARTAO` | Tela inicial, aguardando a aproximação de um cartão. | Exibe "Bem-vindo! Aproxime o VEM". Lê o UID do cartão. | Se `UID` lido, transiciona para `VERIFICANDO_USUARIO`. |
| `VERIFICANDO_USUARIO` | Verifica se o `UID` lido é um usuário existente na API. | Requisição GET para `/usuarios/{vem_hash}`. | Se `200 OK`, transiciona para `PERGUNTA`. Se não existe, transiciona para `CADASTRANDO`. |
| `CADASTRANDO` | Tenta cadastrar o novo usuário na API. | Requisição POST para `/usuarios/{vem_hash}`. | Se sucesso, transiciona para `PERGUNTA`. Se falha, `tratarErro("Cadastro")` e retorna a `ESPERA_CARTAO`. |
| `PERGUNTA` | Busca a pergunta ativa na API e a exibe no LCD. | Requisição GET para `/perguntas/ultima`. | Se sucesso, transiciona para `AGUARDANDO_VOTO`. Se falha, `tratarErro("Carregar Pergunta")`. |
| `AGUARDANDO_VOTO` | Aguarda o usuário aproximar o cartão no leitor "Sim" ou "Não". | Lê o cartão dos dois leitores. | Se `voto` (Sim ou Não) lido, envia a interação para a API e transiciona para `RESULTADO`. Se falha no envio, `tratarErro("Enviar Voto")`. |
| `RESULTADO` | Busca o placar atualizado da votação e o exibe no LCD. | Requisição GET para `/interacoes/score/{pergunta_id}`. | Após exibição, transiciona para `ESPERA_CARTAO`. Se falha, `tratarErro("Score")`. |

## 6. Comunicação com a API

O código interage com a API web através dos seguintes endpoints (assumindo `API_BASE` como o prefixo):

| Função | Método HTTP | Endpoint | Descrição |
| :--- | :--- | :--- | :--- |
| `usuarioExiste()` | `GET` | `/usuarios/{vem_hash}` | Verifica a existência do usuário. |
| `cadastrarUsuario()` | `POST` | `/usuarios/{vem_hash}` | Cadastra um novo usuário. |
| `obterUltimaPergunta()` | `GET` | `/perguntas/ultima` | Obtém o texto e o ID da pergunta atual. |
| `enviarInteracao()` | `POST` | `/interacoes/?vem_hash={...}&pergunta_id={...}&totem_id={...}&resposta={...}` | Envia o voto do usuário. |
| `mostrarResultadoReal()` | `GET` | `/interacoes/score/{pergunta_id}` | Obtém o placar atualizado (percentuais de Sim e Não). |

**Atenção:** O uso de `client.setInsecure()` desativa a verificação do certificado SSL/TLS. Em um ambiente de produção, é altamente recomendado implementar a validação de certificados para garantir a segurança da comunicação HTTPS.

