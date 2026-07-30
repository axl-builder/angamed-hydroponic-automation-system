# SISTEMA IoT DE MONITOREO Y CONTROL: HIDROPONÍA DE PRECISIÓN

## Documento Técnico de Arquitectura, Hardware y Especificación de Base

**Proyecto:** Angamed — Automatización Hidropónica Comercial

**Versión:** 1.0 | **Fecha:** Junio 2026

**Estado:** Documento Técnico de Base

**Desarrollo Técnico:** Equipo de Ingeniería IoT Senior

## 1. Objetivo General del Proyecto

Automatizar los procesos manuales críticos del sistema hidropónico comercial mediante tecnología IoT, desplegando hardware con telemetría inalámbrica y electrónica de grado industrial para la toma de datos en tiempo real, monitoreo remoto, control de actuadores y gestión de alertas del proceso productivo.

### 1.1 Alcance Técnico

- **Monitoreo Continuo:** Supervisión en tiempo real de parámetros críticos: pH, EC (electroconductividad), nivel de tanques, caudal de inyección, humedad de sustrato, temperatura y humedad relativa en 3 zonas del invernadero.
- **Control Automatizado:** Gestión de bombas de recirculación, riego e inyección gobernadas por lógica *Fail-Safe*.
- **Telemetría Inalámbrica:** Red WiFi desde nodos periféricos basados en chips ESP32 hacia un concentrador central (Raspberry Pi 4).
- **Visualización SCADA:** Interfaz en aplicación móvil basada en Thingsboard CE, accesible de forma remota y en tiempo real.
- **Sistema de Alertas Multicanal:** Notificaciones automatizadas mediante Telegram Bot, correo electrónico y alertas *push* ante desviaciones críticas.
- **Seguridad y Resiliencia:** Arquitectura cifrada mediante MQTT con TLS, aislamiento de red por VLAN, acceso remoto mediante VPN con Tailscale y almacenamiento local persistente ante caídas de internet.
- **Escalabilidad:** Arquitectura *Plug & Play* diseñada para incorporar nuevos nodos sin alterar la infraestructura núcleo.

### 1.2 Principios de Diseño No Negociables

| **Principio** | **Descripción** | **Impacto si se omite** |
| --- | --- | --- |
| **Security by Design** | Toda comunicación encriptada desde el primer nodo. Sin tráfico en claro. | Exposición de credenciales y control de bombas accesible desde internet de forma vulnerable. |
| **Fail-Safe First** | El sistema falla con bombas **APAGADAS** ante cualquier error de firmware, red o alimentación. | Riesgo crítico de inundación, marcha en seco de motores o sobredosificación química sin control. |
| **Plug & Play** | Los nuevos nodos se auto-registran en el concentrador vía *MQTT discovery* sin intervención manual. | Cada nodo nuevo requiere configuración manual en el broker, base de datos y paneles de visualización. |
| **Resiliencia Offline** | Los datos persisten en una base InfluxDB local ante pérdida de internet. Operación local autónoma. | Pérdida irreversible del histórico del proceso productivo ante cortes de conectividad cloud. |
| **Aislamiento Galvánico** | Aisladores obligatorios entre electrodos de pH y EC coexistentes en el mismo tanque. | Interferencia cruzada entre electrodos, generando lecturas erráticas e incorrectas garantizadas. |

## 2. Arquitectura del Sistema

### 2.1 Topología de Tres Capas

```
+--------------------------------------------------------------------------+
|                          CAPA 3 - REMOTA / CLOUD                         |
|      EMQX Cloud (Broker)  <-->  Thingsboard CE  <-->  App Móvil SCADA     |
+--------------------------------------------------------------------------+
                                    ▲
                                    │ (MQTT sobre TLS 8883 / HTTPS)
                                    ▼
+--------------------------------------------------------------------------+
|                          CAPA 2 - GATEWAY LOCAL                          |
|             Raspberry Pi 4 (Docker Compose: Mosquitto, InfluxDB)          |
+--------------------------------------------------------------------------+
                                    ▲
                                    │ (WiFi WPA2 - MQTT 1883 - VLAN IoT)
                                    ▼
+--------------------------------------------------------------------------+
|                          CAPA 1 - CAMPO / NODOS                          |
|          Nodos Periféricos (ESP32-S3 / ESP32-C3) + Sensores             |
+--------------------------------------------------------------------------+
```

| **Capa** | **Componente Principal** | **Función** | **Protocolo** |
| --- | --- | --- | --- |
| **Capa 1 — Campo** | Nodos ESP32-S3 / ESP32-C3 | Adquisición de señales analógicas/digitales, control de actuadores locales y transmisión inalámbrica. | MQTT / WiFi WPA2 |
| **Capa 2 — Local** | Concentrador Raspberry Pi 4 | Broker MQTT local, base de datos de series temporales (TSDB), motor de reglas local y *bridge* hacia la nube. | MQTT / Docker Compose |
| **Capa 3 — Remota** | EMQX Cloud + Thingsboard CE | Alojamiento de la plataforma SCADA, distribución de alertas *push*, analítica avanzada y acceso móvil. | MQTT TLS 8883 / HTTPS |

### 2.2 Flujo de Datos Detallado

Fragmento de código

# 

```
graph TD
    A[Nodos Periféricos ESP32] -->|WiFi WPA2 | B[Mosquitto Broker Local]
    B --> C[Node-RED Motor de Reglas]
    C -->|Escritura Directa| D[InfluxDB 2.7 Local SSD]
    C -->|Visualización LAN| E[Grafana Dashboards]
    C -->|Bridge Cifrado TLS 8883| F[EMQX Cloud]
    F --> G[Thingsboard CE SCADA]
    G --> H[App Móvil iOS / Android]
    C -->|Alertas Críticas| I[Telegram Bot API]
```

### 2.3 Decisión de Protocolo: MQTT vs. OPC-UA

> [!NOTE]
> 
> 
> **Resolución de Arquitectura:** Se descarta el uso de OPC-UA para este despliegue. Si bien OPC-UA es el estándar industrial clásico para redes locales cerradas orientadas a PLCs, para una infraestructura descentralizada que requiere consumo eficiente y conectividad directa con aplicaciones móviles y plataformas cloud, **MQTT sobre TLS** es la opción nativa idónea: es sumamente liviano, ejecutable de forma nativa en microcontroladores ESP32 y garantiza latencias inferiores a un segundo de extremo a extremo.
> 

### 2.4 Especificación de la Red Inalámbrica de Campo

El invernadero cuenta con un área de 20 × 45 metros (espacio abierto sin obstrucciones estructurales masivas). Se requiere cobertura continua y robusta.

- **Punto de Acceso Dedicado:** Se especifica el uso de hardware de grado profesional (**Ubiquiti UniFi AP** o **TP-Link EAP**). Un único AP estratégicamente centrado cubre la totalidad de los 900 m².
- **Segmentación por VLAN:** Los nodos de campo se conectan exclusivamente a una **VLAN IoT aislada**. Tienen prohibido el enrutamiento hacia la red corporativa o administrativa.
- **Seguridad Inalámbrica:** SSID oculto exclusivo para la infraestructura con cifrado **WPA2-PSK** y clave de alta entropía (mínimo 20 caracteres combinados).
- **Direccionamiento:** IPs estáticas gestionadas mediante reserva DHCP (*MAC Binding*) en el router central para facilitar la auditoría de red y reglas de firewall.
- **Acceso de Administración:** Cero puertos abiertos en el router de cara a internet pública. Todo el mantenimiento se realiza por túnel cifrado punto a punto vía **Tailscale VPN**.

> [!WARNING]
> 
> 
> **Riesgo Ambiental Crítico:** El entorno hidropónico comercial presenta de forma constante una humedad relativa elevada ($>80\%$) junto con vapores corrosivos provenientes de las sales minerales disueltas. Todos los enclosures de los nodos deben certificar un índice de protección **IP65** como mínimo, e **IP67** en zonas propensas a salpicaduras. El punto de acceso inalámbrico debe alojarse dentro de un gabinete estanco con ventilación protegida.
> 

## 3. Nodos Periféricos — Hardware

### 3.1 Microcontroladores Seleccionados

| **Característica** | **ESP32-S3 (Tanques / Maternidad / Caudal)** | **ESP32-C3 (Clima)** |
| --- | --- | --- |
| **Arquitectura CPU** | Xtensa LX7 dual-core @ 240 MHz | RISC-V single-core @ 160 MHz |
| **Memoria RAM** | 512 KB SRAM + 8 MB PSRAM externa | 400 KB SRAM |
| **Memoria Flash** | 8 MB | 4 MB |
| **Periféricos GPIO** | 45 pines utilizables | 22 pines utilizables |
| **Módulos ADC** | 2 × SAR de 12 bits | 2 × SAR de 12 bits |
| **Conectividad** | WiFi 2.4 GHz + Bluetooth 5 (LE) | WiFi 2.4 GHz + Bluetooth 5 (LE) |
| **Consumo Deep Sleep** | ~20 µA | ~5 µA |
| **Rol Asignado** | Control de Tanques Principales y Caudalímetros | Nodos Distribuidos de Clima Invernadero |

### 3.2 Estrategia de Energy Management por Tipo de Nodo

> [!CRITICAL]
> 
> 
> **Gestión de Energía en Sensores Electroquímicos:** Los sensores de pH y EC no toleran modos de *Deep Sleep* agresivos con corte de suministro eléctrico de manera intermitente. Los electrodos químicos exigen un periodo de estabilización analógica de entre 30 y 120 segundos tras ser energizados. El encendido y apagado cíclico degrada la vida útil del electrodo y genera lecturas iniciales erróneas. Por lo tanto, estos nodos operan en **Modem Sleep**: el circuito analógico permanece permanentemente alimentado, apagándose únicamente el radio WiFi entre ventanas de transmisión de datos.
> 

| **Tipo de Nodo** | **Modo de Suspensión** | **Consumo Promedio** | **Ciclo de Trabajo** | **Sustentación Técnica** |
| --- | --- | --- | --- | --- |
| **Tanques (pH/EC/Nivel)** | Modem Sleep | ~15 - 20 mA | Muestreo cada 5 min. TX: 10 seg. | Requiere estabilización de voltaje constante en la sonda. |
| **Maternidad (Sustrato)** | Modem Sleep | ~12 mA | Muestreo cada 5 min. | Sensor capacitivo de baja deriva temporal. |
| **Caudal (Pulsos GPIO)** | Light Sleep | ~8 mA | Conteo ininterrumpido por *interrupts*. | No puede entrar en Deep Sleep para evitar la pérdida de pulsos del sensor de flujo. |
| **Clima (Temp/HR)** | Deep Sleep | ~20 µA | Despierta cada 5 min, transmite y duerme. | Sensor digital SHT31 con lectura estable e instantánea ($<1$ seg). |

### 3.3 Subsistema de Alimentación por Batería

- **Celda de Energía:** Baterías de Litio-Polímero (LiPo) de 3.7V con capacidad mínima de 3000 mAh.
- **Gestión de Carga:** Controlador de carga **TP4056** dotado de circuito integrado de protección activa (corte por sobredescarga a $<2.5\text{V}$, sobrecarga a $>4.2\text{V}$ y protección contra cortocircuitos).
- **Regulación de Voltaje:** Regulador LDO de baja caída **AMS1117-3.3** dedicado a estabilizar la línea del microcontrolador.
- **Telemetría de Batería:** Divisor de tensión resistivo ($100\text{k}\Omega / 100\text{k}\Omega$) conectado directamente a un pin ADC configurado en el ESP32, enviando el nivel de milivoltios dentro del JSON de telemetría.
- **Módulo Solar Opcional:** Para nodos climáticos o aislados, se integra un panel fotovoltaico monocristalino de 5V / 1W acoplado al TP4056.

## 4. Sensores — Especificación y Lista de Materiales (BOM)

### 4.1 Lista de Componentes Homologados para el Mercado Local

| **Parámetro** | **Módulo de Referencia** | **Interfaz Electrónica** | **Rango Operativo** | **Cantidad** |
| --- | --- | --- | --- | --- |
| **pH** | DFRobot SEN0161-V2 (Sonda Industrial BNC) | Analógica (ADC) / I2C via Bridge | 0 – 14 pH ($\pm 0.1$) | 3 |
| **Electroconductividad** | DFRobot SEN0244 | Analógica de Precisión | 0 – 20 mS/cm | 3 |
| **Aislador Galvánico** | Texas Instruments **ISO1540** / ADUM1250 | Aislamiento Digital I2C de 2.5 kVrms | Frecuencia hasta 1 MHz | 6 |
| **Nivel de Tanque** | **JSN-SR04T** (Transductor ultrasónico IP67) | Serial UART / Trigger-Echo | 25 – 450 cm ($\pm 1\%$) | 3 |
| **Temperatura Agua** | **DS18B20** (Sonda encapsulada waterproof) | Bus 1-Wire digital | -10 a +85 °C ($\pm 0.5^\circ\text{C}$) | 3 |
| **Humedad Sustrato** | Sensor Capacitivo V2.0 (Estilo Stemma) | Voltaje Analógico Lineal | 0 – 100% VWC | 1 |
| **Caudal de Inyección** | YF-S201 (Cuerpo de nylon) | Salida de Pulsos (Efecto Hall) | 1 – 30 L/min | 3 |
| **Clima Aire (HR/T)** | Sensirion **SHT31** Breakout | Bus I2C Digital Directo | 0-100% HR / -40 a +125°C | 3 |

> [!CRITICAL]
> 
> 
> **Interferencia Electroquímica en Tanques:** Al sumergir electrodos de pH y EC simultáneamente dentro de una misma solución acuosa conductora, se cierran bucles de tierra galvánicos que corrompen las lecturas analógicas de forma caótica. **Es obligatorio interponer un aislador digital I2C (ISO1540 o equivalente) en cada línea de comunicación**. Esto interrumpe físicamente la continuidad eléctrica entre los circuitos analógicos de adquisición de datos, eliminando las corrientes parásitas y garantizando lecturas con precisión instrumental.
> 

> [!WARNING]
> 
> 
> **Restricción de Hardware sobre Sensores de Humedad y Clima:** > 1. Queda estrictamente **prohibido** el uso de sensores de humedad de sustrato de tipo resistivo (puntas metálicas expuestas), dado que la electrólisis destructiva los inutiliza en semanas. Deben usarse sondas capacitivas.
> 
> 2. Se descarta el sensor DHT22 para el monitoreo del aire debido a su alta tasa de falla en ambientes con sales en suspensión. Se adopta exclusivamente el **SHT31 con membrana protectora de PTFE**.
> 

### 4.2 Diagrama de Conexiones de Hardware — Nodo de Tanques

```
+-----------------------------------------------------------------------+
|                              ESP32-S3                                 |
+-----------------------------------------------------------------------+
    │
    ├─── [I2C Bus: GPIO21/SDA · GPIO22/SCL]
    │       ├─── [ISO1540 Aislador A] ──► [Sonda pH DFRobot] ──► Electrodo
    │       └─── [ISO1540 Aislador B] ──► [Sonda EC DFRobot] ──► Electrodo
    │
    ├─── [1-Wire Bus: GPIO4] ──► [Sensor DS18B20 Sonda Térmica Agua]
    │
    ├─── [UART1 Bus: TX17 / RX16] ──► [Sensor Ultrasónico JSN-SR04T]
    │
    ├─── [GPIO34 - ADC] ──► [Divisor Resistivo 100k/100k] ──► Telemetría LiPo
    │
    ├─── [GPIO6 - Entrada Pull-Up] ──► [Presostato de Marcha en Seco]
    │
    └─── [GPIO5 - Salida Digital] ──► [Optoacoplador PC817]
                                            │
                                            └──► [Relé 5V] ──► [Contactor 220V] ──► Bomba
```

### 4.3 Protocolo de Calibración de Precisión

#### 4.3.1 Calibración de pH en 2 Puntos

1. Enjuagar exhaustivamente el electrodo en agua destilada.
2. Sumergir la sonda en la solución estándar de calibración **Buffer pH 7.00**. Esperar 120 segundos a que la lectura del ADC se estabilice térmicamente. Registrar el valor analógico crudo y guardarlo en memoria NVS como el **punto de offset**.
3. Retirar, limpiar nuevamente con agua destilada e introducir en la solución **Buffer pH 4.00**. Tras 120 segundos, registrar el valor del ADC. Calcular matemáticamente la pendiente de ganancia (*slope*) del sensor.
4. Salvar ambos coeficientes de forma persistente dentro de la partición de almacenamiento cifrada **NVS** del ESP32. Planificar ciclos automáticos de recalibración cada 30 días.

#### 4.3.2 Algoritmos de Compensación Térmica

Las lecturas electroquímicas experimentan una deriva intrínseca causada por las fluctuaciones en la temperatura del agua. El firmware aplica correcciones automáticas en cada ciclo basándose en las lecturas del DS18B20:

$$\text{pH}_{\text{Compensado}} = \text{pH}_{\text{Raw}} + 0.0018 \times \left(T_{\text{Agua}} - 25.0^\circ\text{C}\right)$$

La electroconductividad (EC) se normaliza aplicando un coeficiente estándar del $2\%$ por cada grado Celsius de desviación respecto a la temperatura de referencia industrial de $25^\circ\text{C}$.

## 5. Actuadores y Control de Potencia (220V AC)

### 5.1 Cadena de Control de Señal

Para integrar de manera segura la lógica digital de baja potencia con la infraestructura eléctrica existente de tableros de potencia y contactores, se implementa la siguiente topología de aislamiento:

```
 [ESP32 GPIO 3.3V]
       │
       ▼ (~10 mA a través de R=330 Ohms)
 [Optoacoplador PC817]  <--- AISLAMIENTO GALVÁNICO DE SEGURIDAD
       │
       ▼ (Conducción del fototransistor)
 [Transistor NPN BC547] <--- DRIVER DE POTENCIA DE LA BOBINA
       │
       ▼ (Activación de bobina 5V - Diodo Flyback 1N4007 en paralelo)
 [Módulo Relé 5V]
       │
       ▼ (Cierre de contactos secos 10A / 250VAC)
 [Contactor Industrial 220VAC en Tablero] ──► [Bomba Hidropónica Monofásica]
```

> [!CRITICAL]
> 
> 
> **Peligro de Destrucción por Retorno Inductivo:** Bajo ninguna circunstancia se debe conectar un pin GPIO del ESP32 de forma directa a la bobina de activación de un relé. El consumo nominal de corriente de la bobina ($50 - 100\text{ mA}$) supera la capacidad del pin del microcontrolador ($12\text{ mA}$). Asimismo, la fuerza contraelectromotriz generada al desenergizar bobinas inductivas destruiría el silicio del MCU si no se dispone del aislamiento optoelectrónico y el diodo de libre circulación (*flyback*).
> 

### 5.2 Algoritmos e Interbloqueos de Lógica Fail-Safe

- **Lógica de Contactos Normalmente Abiertos (NO):** Absolutamente todos los relés encargados del control de motores y electroválvulas se configuran y cablean en su posición físicamente Desconectada o Normalmente Abierta. Ante cortes de energía generalizados o fallas de hardware, el circuito se abre de inmediato, apagando todas las cargas.
- **Watchdog por Hardware:** Se inicializa el temporizador de reseteo (*Hardware Watchdog*) ajustado a 30 segundos. Si el bucle principal del firmware sufre un bloqueo o congelamiento, el microcontrolador se reinicia automáticamente de forma física.
- **Timeout de Red (Heartbeat):** Si un nodo de campo pierde conectividad con el Broker MQTT por un tiempo superior a 90 segundos, ejecuta de forma autónoma una subrutina de emergencia que apaga todos sus actuadores locales asociados.
- **Interbloqueo por Marcha en Seco:** Un presostato mecánico acoplado aguas abajo de la succión actúa como entrada de interrupción de máxima prioridad. Si detecta ausencia de flujo de fluido con la bomba encendida, interrumpe el paso de corriente al relé de inmediato en milisegundos, evitando el sobrecalentamiento del motor.
- **Control de Histéresis:** Restricción por software que limita el reencendido de motores a un máximo de 3 ciclos por minuto, protegiendo los contactores contra oscilaciones espurias de los sensores.

## 6. Concentrador (Gateway Local) — Infraestructura

### 6.1 Especificaciones de Hardware del Servidor Local

| **Componente** | **Requerimiento de Hardware** | **Fundamentación de Ingeniería** |
| --- | --- | --- |
| **SBC Compute** | **Raspberry Pi 4 Model B (4 GB RAM)** | Procesamiento holgado para la ejecución simultánea de contenedores Docker en producción sin degradación de memoria. |
| **Boot Drive** | MicroSD de 32 GB Clase 10 A2 | Dedicada únicamente para la inicialización y lectura de las imágenes estáticas del Sistema Operativo. |
| **Storage de Datos** | **SSD Externo USB 3.0 de 256 GB** | Obligatorio. Las bases de datos de series temporales (TSDB) destruyen las tarjetas MicroSD tradicionales en pocos meses debido al volumen continuo de escrituras. |
| **Respaldo Eléctrico** | UPS HAT con celdas de Litio 18650 | Garantiza autonomía de procesamiento y resguardo de datos ante interrupciones imprevistas del suministro eléctrico comercial. |
| **Sistema Operativo** | **Ubuntu Server 22.04 LTS (64-bit ARM)** | Kernel optimizado y desprovisto de entorno gráfico. Estabilidad extrema para ambientes de servidores de borde. |

### 6.2 Stack de Contenedores de Software (Docker Compose)

El ecosistema de aplicaciones se orquesta de forma aislada mediante contenedores Docker, garantizando portabilidad y facilidad de mantenimiento.

YAML

# 

```
version: '3.8'
services:
  mosquitto:
    image: eclipse-mosquitto:2
    ports:
      - "1883:1883"
      - "8883:8883"
    volumes:
      - ./mosquitto/config:/mosquitto/config
  influxdb:
    image: influxdb:2.7
    ports:
      - "8086:8086"
  nodered:
    image: nodered/node-red:latest
    ports:
      - "1880:1880"
  thingsboard:
    image: thingsboard/tb-postgres:latest
    ports:
      - "9090:9090"
```

### 6.3 Definición de la Topología de Temas (Topics) MQTT

El esquema de direccionamiento de mensajes jerárquicos se estructura bajo el formato global: `hidro/{subsistema}/{id_dispositivo}/{métrica}`.

| **Canal MQTT** | **Dirección del Mensaje** | **Formato del Contenido (JSON)** | **Intervalo temporal** |
| --- | --- | --- | --- |
| `hidro/discovery/{id}` | Nodo Periférico $\rightarrow$ Gateway | `{"device_id": "sn-01", "type": "tanque", "fw": "1.0"}` | Únicamente en el arranque del hardware. |
| `hidro/tanque/{id}/telemetria` | Nodo Periférico $\rightarrow$ Gateway | `{"pH": 6.2, "EC": 1.8, "nivel_pct": 84, "v_bat": 4120}` | Fijo cada 5 minutos. |
| `hidro/clima/{id}/telemetria` | Nodo Periférico $\rightarrow$ Gateway | `{"temp_aire": 24.5, "hr_pct": 72.1, "v_bat": 3950}` | Fijo cada 5 minutos. |
| `hidro/tanque/{id}/cmd` | Gateway $\rightarrow$ Nodo Periférico | `{"actuador": "bomba", "estado": "ON", "duration_s": 300}` | Bajo demanda (Asincrónico). |
| `hidro/tanque/{id}/alerta` | Nodo Periférico $\rightarrow$ Gateway | `{"error_code": "DRY_RUN", "severity": "CRITICAL"}` | Al ocurrir el evento anómalo. |

## 7. Ciberseguridad — Security by Design

### 7.1 Capas de Defensa en Profundidad

```
+--------------------------------------------------------------------------+
|  RED: Segmentación de Tráfico por VLAN IoT + VPN Tailscale Mesh          |
+--------------------------------------------------------------------------+
       │
       ▼
+--------------------------------------------------------------------------+
|  ACCESO: Autenticación por Dispositivo con Listas de Control (ACL)      |
+--------------------------------------------------------------------------+
       │
       ▼
+--------------------------------------------------------------------------+
|  DATO: Cifrado en Tránsito (TLS 1.3) + Almacenamiento en NVS Encriptado  |
+--------------------------------------------------------------------------+
```

### 7.2 Matriz de Implementación de Seguridad

| **Dimensión de Seguridad** | **Mecanismo de Control Técnico** | **Implementación Operativa** |
| --- | --- | --- |
| **Seguridad de Red** | Aislamiento estricto de Capa 2 y 3. | Reglas de Firewall en router para impedir que dispositivos IoT inicien conexiones hacia el segmento corporativo. |
| **Control de Acceso** | Autenticación robusta en Broker MQTT. | Deshabilitar por completo el acceso anónimo en Mosquitto. Cada chip ESP32 posee credenciales únicas vinculadas a su dirección MAC. |
| **Cifrado de Comunicaciones** | Protocolos criptográficos TLS 1.2 / 1.3. | Cifrado asimétrico forzado en el puerto 8883 para tráfico externo hacia la nube. Certificados de Autoridad de Certificación (CA) propios. |
| **Protección del Firmware** | Encriptación física de la memoria Flash. | Activación del fusible electrónico de cifrado por hardware AES-256 embebido en el ESP32. Las contraseñas WiFi residen cifradas en la partición NVS. |
| **Acceso Remoto Seguro** | Red Mesh Privada. | Despliegue de nodos **Tailscale**. La administración SSH o HTTP se realiza dentro del túnel virtual seguro cifrado. |
| **Actualizaciones de Firmware** | Mecanismo de Actualización Remota (OTA). | El Gestor de Arranque del ESP32 valida la firma criptográfica digital del binario antes de proceder a la sobreescritura del firmware. |

## 8. Interfaz SCADA Móvil — Thingsboard Community Edition

### 8.1 Justificación Técnica de la Plataforma Seleccionada

Frente a entornos tradicionales cerrados de alto costo corporativo (ej. AVEVA SCADA) o soluciones básicas web de control limitado (ej. paneles nativos de Node-RED), **Thingsboard CE** provee una infraestructura open-source ideal para proyectos de IoT industrial. Cuenta con soporte nativo de pasarelas MQTT, bases de datos optimizadas con PostgreSQL, un motor de reglas visual (*Rule Engine*) potente y la capacidad de generar aplicaciones móviles híbridas nativas para iOS y Android de forma ágil y costo-eficiente.

### 8.2 Especificación de Componentes del Cuadro de Mando (*Dashboard*)

```
+-----------------------------------------------------------------------------+
| [GAUGE CIRCULAR]             [GAUGE LINEAL]             [TANQUE ANIMADO]    |
|     pH Tanque 1                  EC Tanque 1               Nivel Llenado    |
|   Valor Act: 6.2               Valor Act: 1.8 mS/cm             82%         |
| (Verde: 5.8 - 6.8)           (Verde: 1.0 - 2.5)         (Bajo si < 20%)     |
+-----------------------------------------------------------------------------+
| [GRÁFICO DE LÍNEAS HISTÓRICO]                                               |
|  2.5 mS/cm ───────────────────────────────────────────── (Umbral Alto Max)  |
|  1.8 mS/cm ───────/\───────────/\────────────/\───────── (Lectura EC)       |
|  1.0 mS/cm ──────/──\─────────/──\──────────/──\──────── (Umbral Bajo Min)  |
|            +─────+─────+─────+─────+─────+─────+─────+                      |
|           00:00 02:00 04:00 06:00 08:00 10:00 12:00                         |
+-----------------------------------------------------------------------------+
| [INTERRUPTOR CONTROL]        [TELEMETRÍA CLIMA]         [ALERTAS ACTIVAS]   |
|   BOMBA RECIRCULACIÓN         Zona Norte Invernadero     ¡NIVEL CRÍTICO T3!  |
|     [ ESTADO: ON ]            T: 24.5°C  |  HR: 68%     [ RECONOCER ALERTA ]|
+-----------------------------------------------------------------------------+
```

- **Indicador Analógico de pH (×3 Tanques):** Widget de arco radial con codificación cromática activa. Rango óptimo (Verde): `5.8` a `6.8`. Zonas de advertencia (Amarillo): `5.5 - 5.8` y `6.8 - 7.0`. Zonas de alarma fuera de control (Rojo): $<5.5$ o $>7.0$.
- **Medidor Lineal de Conductividad Eléctrica (×3 Tanques):** Barra horizontal de monitorización de sales en solución. Rango nominal: `1.0` a `2.5 mS/cm`.
- **Visualizador Volumétrico de Contenedores:** Gráfico dinámico vertical que representa el porcentaje neto de volumen de agua remanente. Alarma visual si el nivel disminuye del $20\%$.
- **Gráficos de Tendencias Temporales:** Historiador gráfico interactivo con ventanas conmutables de tiempo (1 hora, 6 horas, 24 horas, 7 días) dotado de líneas estáticas horizontales de consigna.
- **Matriz de Control de Actuadores:** Botones de alternancia manual/automática protegidos por confirmación de comando para el encendido forzado de bombas de manera excepcional.

### 8.3 Matriz Avanzada de Gestión de Alarmas y Eventos

| **Detonador del Evento** | **Nivel de Severidad** | **Canal de Salida** | **Acción de Mitigación Automatizada** |
| --- | --- | --- | --- |
| **pH fuera de rango óptimo** ($<5.5$ o $>7.0$) | **Crítica** | Telegram Bot + Alerta Móvil Push | Registra la desviación en la base de datos y reenvía recordatorios cíclicos cada 5 minutos hasta recibir un acuse de recibo técnico. |
| **Nivel Crítico de Fluido** ($<20\%$) | **Alta** | Alerta Móvil Push + Correo Electrónico | Genera la desconexión inmediata por software del relé que energiza la bomba de recirculación asociada para evitar cavitación. |
| **Evento de Marcha en Seco** | **Crítica** | Telegram Bot + Alerta Móvil Push | Desenergización fulminante del contactor principal y bloqueo lógico del actuador hasta una inspección presencial. |
| **Pérdida de Telemetría** ($>10\text{ min offline}$) | **Alta** | Alerta Móvil Push | Modifica el estado del nodo a "Inactivo" en el panel SCADA y emite una alerta de falla de enlace de comunicaciones. |
| **Voltaje de Batería Crítico** ($<3.5\text{ V}$) | **Baja** | Correo Electrónico Informativo | Añade una tarea programada en el registro de mantenimiento diario para el reemplazo preventivo de la celda. |

## 9. Arquitectura del Firmware

### 9.1 Entorno y Herramientas de Desarrollo

- **Estructura Base:** Framework de desarrollo Arduino compilado sobre las capas nativas de **ESP-IDF**. El entorno de desarrollo y compilación seleccionado es **PlatformIO** integrado en VS Code, gestionando dependencias y librerías por configuración de entorno.
- **Librerías Núcleo:** `PubSubClient` (Cliente MQTT de baja huella), `ArduinoJson v7.x` (Serialización eficiente de strings), `WiFiClientSecure` (Manejo de pilas TLS/SSL), `Preferences` (Lectura de memoria persistente NVS), `DallasTemperature` (Bus de datos 1-Wire).

### 9.2 Lógica de Operación del Firmware — Pseudocódigo del Nodo de Tanque

C++

# 

```
// ============================================================================
// CONFIGURACIÓN INICIAL (Ejecución única en el arranque del hardware)
// ============================================================================
void setup(){
    inicializar_hardware_watchdog(30_segundos);
    inicializar_buses_comunicacion(); // Configura pines I2C, UART y 1-Wire

    // Recupera credenciales desde memoria segura NVS
    String wifi_ssid  = Preferences.getString("wifi_ssid");
    String wifi_pass  = Preferences.getString("wifi_pass");
    String mqtt_user  = Preferences.getString("mqtt_user");
    String mqtt_token = Preferences.getString("mqtt_token");

    establecer_enlace_wifi(wifi_ssid, wifi_pass);
    conectar_broker_mqtt(mqtt_user, mqtt_token);

    // Suscripción al tópico de comandos remotos entrantes
    mqttClient.subscribe("hidro/tanque/01/cmd");

    // Ejecuta protocolo de autoregistro en el gateway local
    publicar_descriptor_discovery();

    // Carga de la última tabla de calibración analógica de electrodos
    cargar_coeficientes_calibracion_nvs();

    bomba_recirculacion_set(OFF); // Garantiza estado inicial seguro
}

// ============================================================================
// BUCLE PRINCIPAL DE OPERACIÓN (Ciclo continuo)
// ============================================================================
void loop(){
    alimentar_hardware_watchdog();
    verificar_consistencia_conexiones(); // Habilita reconexión automática de red

    unsigned long timestamp_actual = millis();

    // Execución cíclica planificada del bloque de telemetría (Cada 5 minutos)
    if (timestamp_actual - t_ultimo_muestreo >= 300000) {
        // Energización controlada de la etapa analógica aislada
        digitalWrite(PIN_CONTROL_ENERGIA_SENSORES, HIGH);
        delay(2000); // Demora requerida para la estabilización de los electrodos

        // Adquisición de señales de campo
        float temp_agua = sensor_temperatura_ds18b20.read();
        float ph_raw    = analogRead(PIN_ADC_PH);
        float ec_raw    = analogRead(PIN_ADC_EC);
        float nivel_cm  = sensor_ultrasonico_jsn.read_distance();

        // Procesamiento matemático de las variables físicas
        float ph_neto    = evaluar_curva_ph(ph_raw, temp_agua);
        float ec_neto    = evaluar_curva_ec(ec_raw, temp_agua);
        float porcentaje = mapear_volumen_tanque(nivel_cm);

        // Serialización estructurada del mensaje en formato JSON
        JsonDocument payload;
        payload["ts"]        = obtener_epoch_time();
        payload["pH"]        = ph_neto;
        payload["EC"]        = ec_neto;
        payload["nivel_pct"] = porcentaje;
        payload["temp_agua"] = temp_agua;
        payload["rssi"]      = WiFi.RSSI();

        String json_string;
        serializeJson(payload, json_string);

        mqttClient.publish("hidro/tanque/01/telemetria", json_string);

        // Desenergiza la etapa de acondicionamiento para ahorro de potencia
        digitalWrite(PIN_CONTROL_ENERGIA_SENSORES, LOW);
        t_ultimo_muestreo = timestamp_actual;
    }

    // Monitoreo continuo de Seguridad: Validación de Interrupción por Fail-Safe
    if (timestamp_actual - t_ultimo_heartbeat_broker > 90000) {
        bomba_recirculacion_set(OFF); // Forzado inmediato por pérdida del Gateway
        publicar_alerta_local("FALLA_ENLACE_BROKER_TIMEOUT");
    }

    // Configura el módem en bajo consumo de radio durante el intervalo inactivo
    WiFi.setSleep(WIFI_PS_MIN_MODEM);
    delay(10);
}
```

## 10. Plan de Trabajo e Implementación Cronológica

```
Semanas:     01   02   03   04   05   06   07   08   09   10   11   12
Infra:       [========]
Nodo Piloto:          [========]
Escalado:                      [============]
QA & Prod:                                  [========]
```

### Fase 1: Despliegue de Infraestructura Núcleo y Red (Semanas 1–3)

- Instalación limpia de Ubuntu Server en la unidad Raspberry Pi 4. Endurecimiento del sistema operativo: configuración del Firewall UFW, políticas restrictivas y despliegue del cliente de red mesh Tailscale.
- Inicialización y montaje de los contenedores Docker a través del script unificado de Docker Compose.
- Configuración del broker Mosquitto implementando esquemas obligatorios de listas de control de acceso (ACLs) basadas en tokens únicos.
- Diseño y ruteo de la topología lógica de la VLAN IoT dentro del equipamiento de red del invernadero.

### Fase 2: Desarrollo y Validación del Nodo Piloto de Tanque (Semanas 4–6)

- Montaje físico de los circuitos impresos y componentes del nodo de control de tanques dentro de un enclosure hermético certificado IP67.
- Escritura del firmware inicial con foco en la precisión analógica. Calibración controlada de laboratorios mediante soluciones patrones estables de pH y EC.
- Pruebas de estrés eléctrico en tableros, verificando que las conmutaciones no introduzcan ruido en los ADC.
- Validación empírica de la desconexión rápida por fallas simuladas de la red inalámbrica o cuelgues inducidos del microcontrolador.

### Fase 3: Escalado Industrial y Lógica de Alertas Integrada (Semanas 7–10)

- Fabricación e instalación en serie de los nodos correspondientes para el Tanque 2, Tanque 3, estación de Maternidad y los colectores de medición de caudal.
- Validación exhaustiva del mecanismo Plug & Play: inserción de hardware en caliente y confirmación de alta autónoma en el servidor SCADA.
- Programación y vinculación del API del bot de mensajería cifrada de Telegram y las reglas de ruteo prioritario en Node-RED.

### Fase 4: Puesta en Producción, Monitoreo de Deriva y OTA (Semanas 11–12)

- Habilitación del entorno seguro de despliegue inalámbrico de actualizaciones (Firmware Over-The-Air - OTA).
- Monitoreo riguroso de la deriva analógica de los electrodos de pH/EC expuestos a condiciones reales de operación continua durante 30 días para refinar las ecuaciones de software.
- Traspaso operacional al personal mediante manuales de mantenimiento simplificados de campo.

## 11. Matriz de Análisis y Mitigación de Riesgos Técnicos

| **Identificación del Riesgo** | **Probabilidad** | **Impacto** | **Estrategia de Ingeniería para Mitigación** |
| --- | --- | --- | --- |
| **Deriva excesiva en sensores electroquímicos de pH** | Alta | Alto | Inclusión de subrutinas automáticas en Node-RED que contrastan los promedios móviles frente a umbrales empíricos históricos, notificando al operario la necesidad de una limpieza física mensual obligatoria. |
| **Corrosión galvánica acelerada por ambiente de alta salinidad** | Alta | Medio | Hermetización total mediante resinas epóxicas en las soldaduras expuestas. Empleo estricto de conectores de acople rápido estancos IP67 y dosificación de vaselina siliconada de grado industrial en terminales de cobre. |
| **Degradación física de memoria MicroSD por ciclos repetitivos** | Media | Alto | Configuración estricta de las variables del stack Docker para desviar la totalidad de logs del sistema y las transacciones de bases de datos de series temporales hacia la unidad externa SSD USB 3.0. |
| **Caídas imprevistas de conectividad WAN a Internet pública** | Media | Bajo | El sistema opera localmente de forma 100% autónoma. InfluxDB resguarda los registros de telemetría de forma local en el SSD y Node-RED mantiene el control de lazo cerrado local. Al retornar el enlace cloud, se sincroniza el buffer acumulado de forma automática. |
| **Daño catastrófico por funcionamiento de motores en vacío (Sin Agua)** | Media | Alto | Instalación redundante: protección lógica por software evaluando lecturas del ultrasónico junto con la desconexión física instantánea por hardware gobernada por el presostato mecánico en serie. |

## 12. Plan de Acción Inmediato para el Equipo de Ingeniería

- [ ]  **Logística de Red:** Confirmar la disponibilidad y compatibilidad de las especificaciones de hardware del Router/Switch base en las instalaciones comerciales para proceder a la subdivisión y configuración de la **VLAN IoT aislada**.
- [ ]  **Adquisiciones de Control:** Emitir orden de compra prioritaria para el kit del Gateway local: Raspberry Pi 4 (4GB RAM), Fuente Oficial 15W, Gabinete con disipador pasivo de aluminio y la unidad de estado sólido SSD de 256GB de alta velocidad de escritura.
- [ ]  **Adquisiciones de Campo:** Gestionar la importación o compra local de los módulos de precisión para el Nodo Piloto 1 (Chips ESP32-S3 DevKit, Aisladores Digitales ISO1540 de Texas Instruments y Sondas industriales DFRobot con conector BNC blindado).
- [ ]  **Estructura de Software:** Inicializar el entorno de desarrollo unificado en PlatformIO, crear el árbol de directorios del proyecto y realizar el *commit* base en el repositorio Git corporativo.