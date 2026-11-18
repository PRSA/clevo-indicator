# clevo-indicator — indicador del ventilador para portátiles Clevo

Adaptación de **[clevo-indicator](https://github.com/SkyLandTW/clevo-indicator)**,
el indicador de bandeja de **SkyLandTW** para Ubuntu, publicado bajo
[Unlicense](https://unlicense.org/) (dominio público).

| | |
|---|---|
| Proyecto original | <https://github.com/SkyLandTW/clevo-indicator> |
| Autor original | SkyLandTW (Unlicense) |
| Esta adaptación | <https://github.com/PRSA/clevo-indicator> |
| Autor de la adaptación | prsa (2026) |
| Detalle de los cambios | [`NOTAS.md`](NOTAS.md) |

El programa muestra en el área de notificación la temperatura de CPU y GPU y las
RPM del ventilador, y permite fijar manualmente el duty del ventilador
(40–100 %). Conserva el mecanismo del original: módulo `ec_sys` y
`/sys/kernel/debug/ec/ec0/io` para leer y escribir la EC (Embedded Controller).

![Captura del proyecto original](http://i.imgur.com/ucwWxLq.png)

> La captura pertenece al proyecto original. En esta adaptación el icono se
> dibuja con cairo (aspas girando) y el tooltip es más completo; ver la tabla
> siguiente.

## Qué cambia respecto al original

| Área | Original | Esta adaptación |
|---|---|---|
| Item de bandeja | `libappindicator3` | `org.kde.StatusNotifierItem` exportado con GDBus en `/org/ayatana/NotificationItem/clevo_indicator` |
| Icono | iconos del tema (`brasero-disc-*`) | dibujado con cairo (anillo + 3 aspas + buje), gira 15° cada 500 ms mientras hay RPM |
| Tooltip | limitado | propiedad `ToolTip` `(sa(iiay)ss)` + emisión de `NewToolTip` solo al cambiar |
| Menú | appindicator | `com.canonical.dbusmenu` con `libdbusmenu-glib` (AUTO, 60–100 %, Quit) |
| `SIGCHLD` | cualquier hijo muerto terminaba el proceso | solo termina si muere el worker de la EC |
| `MAX_FAN_RPM` | 4400 | 5500 (RPM reales de la máquina) |
| Dependencias | GTK3 + appindicator | `gio-2.0`, `dbusmenu-glib-0.4`, `cairo` |

Motivos: Ubuntu 26.04 ya no distribuye `libappindicator3`, y en LXQt el icono y
el tooltip del original no se mostraban bien. El diagnóstico completo está en
[`NOTAS.md`](NOTAS.md).

## Requisitos

```shell
sudo apt-get install build-essential pkg-config \
     libglib2.0-dev libdbusmenu-glib-dev libcairo2-dev
```

## Compilación e instalación

```shell
git clone https://github.com/PRSA/clevo-indicator.git
cd clevo-indicator
make
sudo make install     # /usr/local/bin/clevo-indicator, setuid root:adm
```

El binario debe ser `setuid root` para poder escribir en la EC, pero tiene que
ejecutarse desde la sesión del usuario para publicar el item en el bus de
sesión. `make install` ya aplica `-m 4750 -g adm`.

## Uso

```shell
clevo-indicator        # indicador en el área de notificación
clevo-indicator 60     # fija el duty del ventilador al 60 % (40–100)
clevo-indicator -?     # ayuda
```

Sin `DISPLAY` (por ejemplo por SSH) solo vuelca las lecturas. Los cambios de
duty tardan 1–2 s en notarse; se comprueban en el icono y en el ruido del
ventilador.

> **Aviso:** no manipules ni consultes los puertos de E/S de la EC mientras el
> programa está en marcha. Es una advertencia que ya venía del proyecto
> original y sigue vigente.

## Escritorio

- **LXQt**: soportado (item SNI propio y `NewToolTip` manual).
- **KDE Plasma**: soportado de forma nativa.
- **GNOME**: funciona con la extensión AppIndicator; el tooltip depende de la
  versión de la extensión.
- **XFCE / MATE**: no soportados (usarían su propio panel de bandeja).

## Comprobación rápida (D-Bus)

```shell
# ¿está registrado el item?
dbus-send --session --print-reply=literal \
  --dest=org.kde.StatusNotifierWatcher /StatusNotifierWatcher \
  org.freedesktop.DBus.Properties.Get \
  string:org.kde.StatusNotifierWatcher string:RegisteredStatusNotifierItems
```

Hay más comprobaciones (tooltip, menú, autostart) en [`NOTAS.md`](NOTAS.md).

## Estructura

```
src/clevo-indicator.c   # fuente único: EC + StatusNotifierItem + cairo + dbusmenu
Makefile                # pkg-config: gio-2.0 dbusmenu-glib-0.4 cairo
NOTAS.md                # cambios de la adaptación y diagnóstico
LICENSE.md              # licencia de la adaptación (ES / EN / ZH)
```

## Licencia y atribución

- **Código original:** [SkyLandTW/clevo-indicator](https://github.com/SkyLandTW/clevo-indicator),
  autor **SkyLandTW**, licencia **Unlicense** (dominio público).
- **Adaptación:** modificaciones y adiciones de **prsa** (2026), según
  [`LICENSE.md`](LICENSE.md): las partes derivadas del original siguen en
  dominio público (Unlicense); las partes nuevas son de uso no comercial con
  atribución obligatoria.

El historial de git de este repositorio conserva los commits originales de
SkyLandTW. Si reutilizas el trabajo, mantén la referencia al repositorio
original.
