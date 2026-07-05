#import "template.typ": *

#show: doc => conf(
  titulo: "Sistema Inteligente de Control de Acceso Híbrido",
  subtitulo: "Del diseño a la implementación: una solución IoT Edge + Servidor",
  materia: "I161 — Internet de las Cosas · Trabajo Final Integrador",
  autores: (
    "Murray Agustín (03255/1)",
    "Polanis Iván Valentín (03266/5)",
    "Savenia Manuel (03302/1)",
  ),
  fecha: "Julio de 2026",
  doc,
)

// ─────────────────────────────────────────────────────────────────────────────
#heading(numbering: none, level: 1)[Resumen]

Se presenta la implementación de un sistema de control de acceso físico basado en
RFID con una arquitectura *híbrida Edge + Servidor*. La lógica de decisión se
distribuye entre un nodo perimetral (ESP32) y un servidor central (Node-RED), de
modo que el acceso local sigue operativo aun ante la caída de la red o del broker.
El nodo mantiene un *caché local* con tiempo de vida (TTL) e invalidación
proactiva, mientras que el servidor actúa como *única fuente de verdad*,
evaluando habilitación, expiración y franjas horarias sobre una base de datos
relacional. La telemetría se registra en una base de series temporales y se
visualiza en tableros; la administración se realiza en tiempo real mediante un bot
de Telegram. El sistema fue montado en hardware físico y verificado de extremo a
extremo. Este informe describe el contexto completo de la solución, la contrasta
con la propuesta original, justifica las decisiones de arquitectura y analiza las
ventajas y limitaciones de la tecnología empleada.

// ─────────────────────────────────────────────────────────────────────────────
= Contexto y objetivo

El problema abordado es el de un control de acceso físico que debe
validar credenciales RFID y accionar una cerradura. Los sistemas comerciales
tradicionales suelen adoptar uno de dos extremos: o bien concentran toda la lógica
en un controlador local aislado, o
bien delegan cada decisión en un servidor central. El objetivo de este trabajo fue construir una
solución que combine lo mejor de ambos: resiliencia local con administración y
auditoría centralizadas.

Sobre esa premisa se definieron los siguientes requisitos funcionales, todos
implementados y verificados:

- Lectura de tarjetas RFID y accionamiento de una cerradura simulada (relé + LEDs).
- Decisión de acceso sub-segundo en el caso frecuente, sin depender de la red.
- Operación degradada tolerante a fallos de red (política de acceso definida).
- Padrón de usuarios administrable, con habilitación, expiración y
  franjas horarias por usuario.
- Bloqueo global instantáneo (_lockdown_) y baja inmediata de credenciales.
- Registro y visualización de todos los eventos de acceso para auditoría.

// ─────────────────────────────────────────────────────────────────────────────
= De la propuesta a la solución

La propuesta original planteó la arquitectura, el stack y la estrategia de caché.
La implementación respetó ese diseño en su totalidad y, además, amplió varios
puntos que en la propuesta quedaban abiertos. El siguiente cuadro resume la
correspondencia:

#figure(
  caption: [Propuesta original vs. solución implementada.],
  table(
    columns: (1.1fr, 1.4fr, 1.5fr),
    align: (left, left, left),
    inset: 6pt,
    stroke: 0.5pt + gris,
    table.header(
      [*Aspecto*], [*Propuesta*], [*Solución implementada*],
    ),
    [Arquitectura], [Híbrida Edge + Servidor], [Idéntica.],
    [Caché en el edge], [Caché local con TTL e invalidación], [Implementada en LittleFS, persistente a reinicios, con purga periódica.],
    [Validación central], [Verifica estado en SQLite], [Amplía a `enabled` + `expires_at` + `schedule` (franja horaria) como reglas evaluadas en el servidor.],
    [Administración], [Bot de Telegram], [12 comandos: alta/baja, horarios, expiración, lockdown, TTL remoto, consultas y auditoría.],
    [Telemetría], [InfluxDB + Grafana], [Idéntico. Dashboard de accesos provisto por _provisioning_.],
    [Modelo de credencial], [RFID genérico], [MIFARE Classic vía RC522, usando el UID como identificador.],
  )
)

// ─────────────────────────────────────────────────────────────────────────────
= Arquitectura de la solución

== Visión general

El sistema se organiza en capas desacopladas que se comunican por MQTT. Cada
componente cumple una única responsabilidad, lo que facilita reemplazarlo o
escalarlo de forma independiente.

#figure(
  caption: [Arquitectura de la solución. El acoplamiento entre capas es únicamente por mensajes MQTT.],
  image("assets/pipeline.png", width: 100%),
)

== ¿Por qué una arquitectura híbrida?

La decisión central del diseño fue dónde vive la lógica de acceso. Se eligió
distribuirla por tres razones:

/ Disponibilidad: la apertura de una puerta es una función crítica que no puede
  depender de un enlace de red. Con el caché local, el nodo resuelve el caso
  frecuente (usuario ya conocido) sin tocar la red, y sigue funcionando ante
  cortes. El servidor deja de ser un punto único de falla para el uso cotidiano.

/ Latencia y tráfico: consultar al servidor en cada lectura añade latencia y
  carga la red innecesariamente. El patrón _cache-aside_ reduce el tráfico a solo
  los eventos nuevos (_cache miss_) y la telemetría, que se envía de forma
  asincrónica.

/ Administrabilidad y auditoría: al mismo tiempo, mantener el *padrón y las reglas
  centralizados* evita tener que actualizar cada nodo a mano. El servidor es la
  única fuente de verdad; los nodos son cachés que se pueden invalidar. Así, una
  baja o un _lockdown_ impactan al instante vía comando MQTT.

== De una puerta a varias

Aunque el sistema se montó y verificó con un único nodo (`door01`), la arquitectura
es *multi-dispositivo por diseño*: todos los nodos se conectan al mismo backend y
comparten el padrón central, sin lógica por puerta. Esto no es una aspiración, sino
una consecuencia directa de cómo están armadas las comunicaciones:

- Los tópicos siguen la convención `access/<dispositivo>/...`, y el servidor se
  suscribe con comodines (`access/+/request`, `access/+/event`, `access/+/status`),
  por lo que atiende a cualquier nodo sin conocerlo de antemano.
- El motor de reglas extrae el identificador del nodo del propio tópico y le dirige
  la respuesta de vuelta a él (`access/<dispositivo>/response`); el ruteo es dinámico.
- El padrón no está asociado a ninguna puerta: un usuario habilitado vale para todas,
  y la administración desde Telegram es única para todo el sistema.

En la práctica, sumar una segunda puerta se reduce a flashear otro ESP32 con un
identificador distinto: el backend no requiere cambios. La telemetría y el monitoreo
de estado también son transversales, de modo que un despliegue multi-puerta queda
visible en un mismo tablero.

== Elección del stack y su justificación

Cada pieza se eligió por resolver un problema concreto de la arquitectura, no por
familiaridad. A continuación se justifican las decisiones menos evidentes.

- *ESP32*: microcontrolador de bajo costo con WiFi integrado y memoria suficiente
  para el caché y la cola. El firmware corre como un loop no bloqueante, y la sincronización NTP se lanza de forma asincrónica para no colgar la
  lectura de tarjetas.
- *MQTT / Mosquitto*: se eligió un modelo _pub/sub_
  porque desacopla al nodo del servidor: publica su evento y sigue, sin bloquearse
  esperando respuesta, y tolera que el otro extremo esté momentáneamente caído.
- *Node-RED*: el _backend_ es esencialmente orquestación dirigida por eventos. El modelo de flujos
  de Node-RED mapea directo a ese patrón y trae integración nativa con MQTT, ambas
  bases y Telegram, evitando _boilerplate_ de conexión y parseo.
- *Docker*: define toda la infraestructura del servidor (broker, Node-RED, ambas
  bases y Grafana) como código, reproducible y desplegable en cualquier entorno.

=== Persistencia en el ESP

El nodo necesita guardar el caché (`cache.json`) y la cola (`queue.json`) en la flash
para que sobrevivan a reinicios. Como estos archivos se reescriben durante la
operación normal, un corte de energía a mitad de una escritura es un fallo posible, y
lo que buscábamos era que eso no corrompiera los datos.

Para el ESP32 evaluamos las dos opciones de sistema de archivos habituales: *SPIFFS* y
*LittleFS*. Nos quedamos con LittleFS por dos motivos concretos: es tolerante a cortes
de energía, si una escritura se interrumpe, conserva la versión anterior en lugar de
dejar el archivo a medias, mientras que SPIFFS no da esa garantía y además tiende a
volverse más lento e inestable a medida que se llena.

Sobre esos archivos, optamos por guardarlos en JSON en lugar de un formato binario
más compacto. Un binario sería algo más liviano, pero a esta escala ese ahorro no era algo que
consideráramos crítico. Preferimos priorizar un único formato JSON de punta a punta:
los mensajes MQTT, el _backend_ en Node-RED y el estado en el nodo hablan todos el
mismo lenguaje, lo que simplifica el desarrollo.

=== Elección de las bases de datos

El sistema maneja dos formas de dato radicalmente distintas, y forzarlas en un mismo
motor perjudicaría a ambas.

/ Padrón (estado): son pocos usuarios, mutables (altas, bajas, cambios de horario)
  y consultados puntualmente por UID. Exige integridad relacional y consistencia
  transaccional para que una baja o un _lockdown_ no dejen el padrón en un estado
  intermedio. Para eso usamos *SQLite*: relacional, embebida, ACID, en un solo archivo.

/ Accesos (telemetría): es un flujo _append-only_ que crece sin límite, timestampeado
  y consultado por ventanas de tiempo para los tableros. Para eso usamos *InfluxDB*:
  alta tasa de escritura, políticas de retención y agregación por _buckets_ temporales
  nativa.

¿No alcanzaba con SQLite para todo? Técnicamente se podría agregar una tabla de
eventos, pero (1) mezclaría un _log_ que crece de forma monótona con el estado mutable
sobre un motor de un solo escritor, generando contención y un archivo que crece sin
retención; (2) obligaría a implementar a mano el _bucketing_ y _downsampling_ temporal
que una base de series temporales da gratis; y (3) se perdería la integración directa
InfluxDB–Grafana. Asignar cada workload al motor pensado para él mantiene el padrón
pequeño y transaccional, y la telemetría rápida de escribir y de agregar.

// ─────────────────────────────────────────────────────────────────────────────
= Funcionamiento de la solución

== Caché y máquina de estados

Ante cada lectura de tarjeta, el firmware ejecuta una máquina de estados que
prioriza la respuesta local y define explícitamente el comportamiento ante fallos:

#figure(
  caption: [Flujo de decisión del ESP32 ante una lectura.],
  image("assets/work_flow.png", width: 50%),
)

Detalles de implementación relevantes:

- *Persistencia*: el caché (`cache.json`) y la cola de eventos (`queue.json`) viven
  en LittleFS, por lo que el estado sobrevive a reinicios y cortes.
- *TTL y expiración*: cada entrada del caché guarda un instante de expiración. El
  TTL por defecto es 24 h y es configurable de forma remota (`/ttl`). Una tarea
  periódica purga las entradas vencidas.
- *Sincronización horaria*: el nodo sincroniza la hora por NTP en segundo plano.
  Mientras no hay hora válida usa el _uptime_, y las entradas viejas quedan
  automáticamente marcadas como expiradas al comparar contra el tiempo Unix.
- *Cola offline*: si al reportar un evento no hay red, este se encola y se drena
  automáticamente al reconectar, garantizando que ningún acceso quede sin
  registrar.
- *Invalidación proactiva*: el servidor puede ordenar borrar un UID (o todo el
  caché) por MQTT, de modo que una baja impacta de inmediato sin esperar al
  vencimiento del TTL.

== Node-RED

Todo el _backend_ vive en Node-RED, construido como un conjunto de flujos: cadenas
de nodos por los que circula un mensaje, desde que entra hasta
que se responde o se persiste. En lugar de un servicio monolítico, la lógica se
reparte en cinco flujos según su responsabilidad, lo que mantiene cada camino corto y
aislado:

/ Init & Schema: al arrancar, crea el esquema de la base (tablas `users` y `audit`)
  si no existe.
/ Cache Miss Handler: atiende los `request` de los nodos y ejecuta el motor de reglas.
/ Event Recorder: recibe los eventos de acceso y los vuelca a InfluxDB.
/ Status Monitor: escucha los tópicos de estado y detecta nodos caídos vía su
  _Last Will_.
/ Telegram Bot: recibe comandos, valida al administrador y ejecuta las operaciones
  sobre el padrón.

El motor de reglas, en el flujo _Cache Miss Handler_. El _cache miss_
llega como un `request` MQTT; un nodo consulta el padrón en SQLite y un único nodo
función (`Evaluar acceso`) decide evaluando, en este orden: _lockdown_ global →
usuario existe → `enabled` → `expires_at` (no vencido) → `schedule` (día y franja
horaria permitidos). Solo si todas las condiciones se cumplen responde `allowed=true`
junto con un TTL.

El padrón (`users`) almacena por usuario: `uid`, `name`, `enabled`, `expires_at`,
`schedule` (JSON con días y horario) y marcas de tiempo. Una tabla `audit` registra
toda acción administrativa (quién, qué y cuándo).

== Registro, visualización y administración

Cada evento de acceso publicado por el nodo se persiste en InfluxDB y alimenta un
dashboard de Grafana con el historial y las métricas de auditoría.

En paralelo, el bot de Telegram es la interfaz de administración del sistema: le
da al operador el control completo del padrón y del estado de la puerta desde el
teléfono, sin necesidad de acceder al servidor. A través de comandos simples permite
dar de alta y de baja usuarios, habilitarlos o deshabilitarlos, configurar franjas
horarias y fechas de expiración, activar el _lockdown_ global, ajustar el TTL del
caché de forma remota y consultar los últimos accesos o los datos de un usuario. Toda
acción queda registrada en la tabla de auditoría junto con el administrador que la
ejecutó. Además de la gestión, el bot funciona como canal de alertas en tiempo
real: notifica ante un acceso denegado o cuando un nodo se da por caído. Como medida de seguridad, valida que el emisor sea un
administrador autorizado antes de ejecutar cualquier comando.

#figure(
  caption: [Comandos del bot de Telegram y su uso.],
  table(
    columns: (auto, 1fr),
    align: (left, left),
    inset: 5pt,
    stroke: 0.5pt + gris,
    table.header([*Comando*], [*Descripción y uso*]),
    [`/alta <uid> <nombre>`], [Da de alta un usuario en el padrón.],
    [`/baja <uid>`], [Elimina un usuario e invalida su caché en el nodo.],
    [`/habilitar <uid>`], [Rehabilita un usuario previamente deshabilitado.],
    [`/deshabilitar <uid>`], [Deshabilita temporalmente un usuario sin borrarlo.],
    [`/usuario <uid>`], [Muestra los datos de un usuario.],
    [`/usuarios`], [Lista todos los usuarios del padrón.],
    [`/horario <uid> <días> <desde> <hasta>`], [Fija la franja horaria permitida. Ej.: `/horario A1B2 1,2,3,4,5 08:00 18:00` (lun–vie 08–18 h; 0=dom … 6=sáb).],
    [`/expira <uid> <ISO8601\|none>`], [Fija o quita la fecha de expiración. Ej.: `/expira A1B2 2026-12-31T23:59:59Z`.],
    [`/ultimos [n]`], [Muestra los últimos _n_ eventos de acceso (10 por defecto).],
    [`/lockdown on\|off`], [Activa o desactiva el bloqueo global de accesos.],
    [`/ttl <segundos>`], [Cambia de forma remota el TTL del caché del nodo.],
    [`/ayuda`], [Lista todos los comandos disponibles.],
  )
)

#figure(
  image("assets/grafana.png"),
  caption: [Dashboard de accesos en Grafana (eventos _granted_/_denied_ e histórico).]
)

#grid(
  columns: (1fr, 1fr),
  column-gutter: 10pt,
  figure(
  image("assets/telegram1.png"),
  caption: [Alta en el bot de telegram]
  ),
  figure(
    image("assets/telegram2.png"),
    caption: [Ultimos en el bot de telegram]
  ))

== Comunicaciones y seguridad de la mensajería

Todos los tópicos siguen la convención `access/<dispositivo>/...` (por ejemplo,
`access/door01/...`), de modo que agregar una segunda puerta es simplemente sumar un
nuevo prefijo sin tocar la estructura. Los tópicos se agrupan por sentido de la
comunicación:

#figure(
  caption: [Tópicos MQTT del sistema (para el nodo `door01`).],
  table(
    columns: (auto, auto, 1.2fr),
    align: (left, left, left),
    inset: 5pt,
    stroke: 0.5pt + gris,
    table.header([*Tópico*], [*Sentido*], [*Uso*]),
    [`.../event`], [nodo → servidor], [Evento de acceso resuelto localmente (telemetría y auditoría).],
    [`.../request`], [nodo → servidor], [Consulta al servidor ante un _cache miss_.],
    [`.../response`], [servidor → nodo], [Decisión del motor de reglas (`allowed` + TTL).],
    [`.../command/invalidate`], [servidor → nodo], [Invalida un UID o todo el caché.],
    [`.../command/lockdown`], [servidor → nodo], [Activa o desactiva el bloqueo global.],
    [`.../command/config`], [servidor → nodo], [Reconfigura parámetros del nodo (p. ej. TTL).],
    [`.../status`], [nodo → todos], [Estado del nodo (_online/offline_).],
  )
)

El broker (Mosquitto) exige autenticación y aplica una ACL por tópico que otorga a cada rol el mínimo
privilegio. El usuario del ESP32 solo puede publicar en sus tópicos de
evento/request/status y leer respuestas y comandos (`command/#`); Node-RED tiene
acceso de lectura/escritura a todo el espacio `access/#`; y un usuario `admin`
separado se reserva para diagnóstico. La ACL usa el comodín `access/+/...`, de modo
que las reglas valen para cualquier puerta sin reescribirse.

En cuanto a las garantías de entrega, los comandos y respuestas viajan con *QoS 1*
(entrega garantizada, al menos una vez), mientras que el tópico `status` es
_retained_ y está declarado como _Last Will_ del nodo: si el ESP32 se desconecta
abruptamente, el broker publica automáticamente su estado _offline_, y cualquier
suscriptor nuevo conoce de inmediato si la puerta está en línea. El broker además
persiste su estado en disco.

// ─────────────────────────────────────────────────────────────────────────────
= Ventajas y limitaciones de la tecnología

== Ventajas

- *Resiliencia*: gracias al caché y a la cola offline, el sistema tolera cortes de
  red y de energía sin perder funcionalidad crítica ni registros.
- *Baja latencia y bajo tráfico*: la ruta rápida (_cache hit_) resuelve localmente
  en milisegundos; la red solo se usa para novedades y telemetría.
- *Administración centralizada*: altas, bajas, horarios, expiración,
  _lockdown_ y hasta el TTL se gestionan de forma remota sin tocar el firmware.
- *Auditoría completa*: cada acceso y cada acción administrativa quedan
  registrados y son visualizables.
- *Desacoplamiento y portabilidad*: el patrón _pub/sub_ y la contenerización
  permiten evolucionar cada pieza por separado y desplegar el _backend_ en
  cualquier entorno.

== Limitaciones

/ Seguridad del medio RFID: el sistema utiliza tarjetas
  *MIFARE Classic* leídas con un módulo RC522. Este estándar está
  criptográficamente comprometido desde 2008: su cifrado propietario _Crypto1_
  fue quebrado y hoy una tarjeta se clona en segundos con hardware accesible. Además, en nuestra implementación se usa únicamente el UID como credencial, sin
  autenticación de sectores; y el UID es trivialmente falsificable con _magic
  cards_ de UID reescribible.

/ Seguridad del transporte: el MQTT del prototipo viaja sin TLS, por lo que las
  credenciales y los mensajes son observables en la red local. En producción
  correspondería habilitar `mqtts` (TLS) en Mosquitto.

/ Escalabilidad de SQLite: es ideal para un único servidor y esta escala, pero su
  modelo de un solo escritor limita despliegues con muchos nodos concurrentes;
  ahí convendría PostgreSQL.

// ─────────────────────────────────────────────────────────────────────────────
= Conclusiones

Se cumplió el objetivo planteado en la propuesta: construir un control de acceso
que sea resiliente y administrable. La arquitectura
híbrida demostró ser acertada —la ruta rápida por caché entrega la respuesta
inmediata que exige una puerta, mientras que el servidor concentra las reglas, la
auditoría y la administración— y el sistema se validó de extremo a extremo sobre
hardware real.

El desarrollo también amplió la propuesta donde esta quedaba abierta,
transformando la validación central en un motor de reglas con habilitación,
expiración y franjas horarias, y dotando al operador de una interfaz de
administración completa por Telegram. La contenerización del _backend_ hace que,
aunque el desarrollo se realizó de manera local, toda la infraestructura pueda
trasladarse y desplegarse en la nube sin inconvenientes, habilitando escenarios
multi-puerta y acceso remoto a los tableros.