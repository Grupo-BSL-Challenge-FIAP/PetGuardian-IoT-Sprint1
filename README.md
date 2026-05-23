<div style="display: flex; align-items: center; gap: 20px;">
  <img src="" alt="Logo da Pet Guardian"
  style="width: 100px; height: 100px;">

  <h1>PetGuardian - Monitoramento IoT de Pets</h1>
</div>

Sistema de monitoramento de saúde em tempo real para pets, desenvolvido com ESP32 e sensores de temperatura e frequência cardíaca. Os dados são exibidos em um display LCD e enviados para uma API REST.

---

## Tecnologias utilizadas

**Hardware**
- ESP32
- Sensor de temperatura e umidade DHT22 (simula a temperatura corporal/ambiente do pet)
- Display LCD 20x4
- Buzzer (para alertas sonoros emergenciais, é ativado apenas em situações críticas.)
- 2× Potenciômetros (simulam atividade física e frequência cardíaca)

**Software e bibliotecas**
- `LiquidCrystal_I2C` - controle do display LCD via I2C
- `DHTesp` - leitura do sensor DHT22
- `WiFi.h` - conexão Wi-Fi
- `HTTPClient.h` - envio de dados para a API

**Ambiente de Simulação**
- [Wokwi](https://wokwi.com) - simulador de circuitos para ESP32/Arduino

---

## Como usar

### Simulação no Wokwi:
1. Acesse o projeto no Wokwi: `[https://wokwi.com/projects/464756501156900865]`
![Screenshot do projeto no Wokwi]()
2. Clique em **Play** para iniciar a simulação
3. Ajuste os potenciômetros para simular diferentes níveis de atividade e frequência cardíaca
4. Observe as leituras no display LCD e no Serial Monitor

### Execução em hardware real
1. Monte o circuito conforme o `diagram.json` do repositório
2. Instale as dependências no Arduino IDE:
   - `LiquidCrystal_I2C`
   - `DHTesp`
3. Ajuste a constante `API_URL` no código com o endereço real da sua API
4. Faça o upload do `sketch.ino` para o ESP32

---
