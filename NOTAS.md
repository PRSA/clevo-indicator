# clevo-indicator — parches locales (LXQt / GNOME / KDE)

Fuente original: https://github.com/SkyLandTW/clevo-indicator (Unlicense)
Adaptación para sesión LXQt de prsa (Ubuntu 26.04), nov 2026.

## Requisitos de la adaptación (resumen final, sesión LXQt)

1. **Item SNI propio (sin libappindicator)** — `src/clevo-indicator.c`:
    - El programa exporta directamente la interfaz `org.kde.StatusNotifierItem`
      en `/org/ayatana/NotificationItem/clevo_indicator` (GDBus) y se registra
      ante `org.kde.StatusNotifierWatcher` con reintentos.
    - Permite propiedades completas, incluida `ToolTip` tipo `(sa(iiay)ss)`, que
      libappindicator nunca exportó;
    - `IconPixmap` ARGB32 big-endian generado por cairo (ventilador con aspas
      girando, 24×24);
    - menú vía `libdbusmenu-glib` (`com.canonical.dbusmenu`) en
      `<item>/Menu` (librería ya presente, `libdbusmenu-glib-dev`).

2. **Icono** de aspas de ventilador dibujado con cairo (anillo + 3 aspas + buje),
   girando 15° cada 500 ms mientras hay RPM. Colores 100% opacos para que el
   pixmap se vea igual en todos los hosts.

3. **Tooltip** (`ToolTip`): título = `Clevo - CPU x°C · GPU y°C · RPM (duty%)`,
   descripción = texto del programa. Se emite `NewToolTip` solo cuando cambia.
   - LXQt lo muestra al pasar el ratón (usa `ToolTip.title`).
   - KDE lo muestra con título + descripción.
   - GNOME: según versión de la extensión AppIndicator (las recientes soportan
     `ToolTip`).

4. **Fix SIGCHLD** — el handler original cerraba TODO el proceso ante cualquier
   hijo que muriera (p. ej. el ayudante efímero que GTK/ibus lanza en LXQt). Ahora
   solo termina si muere el worker del EC.

5. **Adaptación Ubuntu 26.04**: compila contra `gio-2.0`, `dbusmenu-glib-0.4` y
   `cairo` (ya no necesita GTK ni appindicator). Ubuntu ya no distribuye
   `libappindicator3`.

6. **MAX_FAN_RPM** 4400 → 5500 (RPM reales de la máquina).

## Dependencias de compilación

    sudo apt-get install pkg-config libglib2.0-dev libdbusmenu-glib-dev libcairo2-dev

## Instalación

    make
    sudo make install        # /usr/local/bin/clevo-indicator, setuid root:adm

## Autostart

`~/.config/autostart/clevo-indicator.desktop` (ya en su sitio, sin cambios).

## Comprobaciones D-Bus

    # item registrado
    dbus-send --session --print-reply=literal \
      --dest=org.kde.StatusNotifierWatcher /StatusNotifierWatcher \
      org.freedesktop.DBus.Properties.Get \
      string:org.kde.StatusNotifierWatcher string:RegisteredStatusNotifierItems

    # tooltip (sustituye :1.xxx por el nombre de bus del item)
    dbus-send --session --print-reply --dest=:1.xxx \
      /org/ayatana/NotificationItem/clevo_indicator \
      org.freedesktop.DBus.Properties.Get \
      string:org.kde.StatusNotifierItem string:ToolTip

## Otros gestores principales: ¿adaptar?

- **GNOME**: cubierto vía extensión AppIndicator (el item SNI se entrega igual);
  requiere que la extensión soporte la propiedad `ToolTip` para el tooltip rico.
- **KDE Plasma**: cubierto nativamente mediante `org.kde.StatusNotifierItem`.
- **LXQt**: cubierto con item SNI propio + emisión manual de `NewToolTip`
  (lxqt-panel < 2.4 no refrescaba el tooltip con la señal `NewTitle`; además
  libappindicator nunca emitió `NewToolTip`).

**¿Otro gestor "mayoritario"?** El siguiente, en volumen de escritorios de
escritorio con panel, es **XFCE** (y en menor medida **MATE**). Ambos usan sus
propios paneles de bandeja (`xfce4-tray`, `mate-tray`) que no consumen
`org.kde.StatusNotifierItem` de la misma forma que GNOME/KDE/LXQt. Para los
añadir se necesitaría:
- un adaptador de item SNI/DBusMenu para esos gestores, o
- aceptar que el tooltip rico sólo funciona en los tres ya soportados.

Recomendación: no añadir más gestores hoy (los tres principales ya cubiertos);
si el usuario migra a XFCE/MATE conviene evaluar esa capa extra.

## Historial de diagnóstico (resumen)

1. Problema: el icono pedía `brasero-disc-110` que no existía (RPM > 4400).
   Solución: `MAX_FAN_RPM` 5500 + symlinks `brasero-disc-101.svg … 160.svg` →
   `brasero-disc-100.svg`.
2. GNOME mostraba texto (label) junto al icono; LXQt no implementa esa parte del
   SNI → icono dibujado por cairo con las temperaturas.
3. Ubuntu 26.04 ya no tenía `libappindicator3` → portado a `libayatana-appindicator3`.
4. Fallo real de "no arranca": el handler de `SIGCHLD` mataba todo el proceso
   ante cualquier hijo muerto → ahora solo termina si muere el worker del EC.
5. Tras esos fixes: icono nuevo (aspas + tooltip) funcionaba en LXQt; KDE cubierto
   nativo; GNOME parcial.

## Estado actual

- `/usr/local/bin/clevo-indicator` (setuid) = versión parcheada, en marcha
  (padre `prsa` + worker EC root).
- Fichero fuente modificado: `/opt/workspace/clevo-indicator/src/clevo-indicator.c`.
- Código fuente parcheado guardado: `/opt/workspace/clevo-indicator` (volátil;
  copiar a sitio permanente si se quiere).
- El `.desktop` de `~/.config/autostart/` no necesita cambios: en el próximo
  inicio de sesión arrancará con la versión nueva.

## Estado final (con prefijo restaurado)

- Tooltip título: `Clevo - CPU x°C · GPU y°C · RPM (duty%)`.
- Tooltip descripción: `Clevo Fan Control — ventilador a %d RPM (duty %d%%)`.
- Propiedad `Title`: `Clevo`.

## Licencias

Todo queda documentado en `LICENSE.md` (con versiones en español, inglés y chino
simplificado):

- **Código original** (base): Unlicense (dominio público).
- **Adaptación** (modificaciones y adiciones): dual-licencia — parte base bajo
  Unlicense (dominio público); partes de adaptación bajo licencia personalizada
  de no comercial con atribución obligatoria.

## Verificación

- Item registrado y estable; `NewToolTip` emitida cuando cambian los datos.
- Menú dbusmenu: AUTO/60–100%/Quit con estados habilitados/deshabilitados.
- Fix SIGCHLD verificado.

## Notas

- El usuario indicó que el ratón sobre el icono muestra `clevo` en vez de los
  datos; se resolvió con la emisión manual de `NewToolTip` y la propiedad
  `ToolTip` propia.
- El usuario indicó que se quita el prefijo "clevo - "; se restauró el prefijo
  `Clevo - ` delante de las lecturas en el tooltip.
- El tema `ePapirus` estaba vacío (ROMPIÓ iconos GTK); se corrigió a `Papirus`
  (`~/.config/gtk-3.0/settings.ini`) y se limpiaron `~/.icons/Humanity` y
  `~/.icons/ePapirus`.
