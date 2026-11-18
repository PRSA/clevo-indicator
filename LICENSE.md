# Licencia de adaptación — clevo-indicator

Este proyecto es una adaptación del trabajo original:

- **Repositorio original:** https://github.com/SkyLandTW/clevo-indicator
- **Autor original:** SkyLandTW
- **Licencia original:** Unlicense (dominio público)

La adaptación (modificaciones, adiciones y reescrituras realizadas por prsa)
se publica bajo la licencia de adaptación que figura a continuación.

> **Nota:** las versiones en inglés y chino simplificado son traducciones de
> cortesía. En caso de discrepancia o duda interpretativa, prevalece el texto
> en español.

---

## Español (castellano)

### LICENCIA DE ADAPTACIÓN (no comercial, atribución obligatoria)

Copyright (c) 2026 prsa (adaptador)

Se concede permiso para copiar, modificar, usar y distribuir las partes de este
código que constituyan la ADAPTACIÓN (definidas más abajo), bajo las siguientes
condiciones:

1. **Atribución obligatoria.** Toda copia, distribución o trabajo derivado que
   incluya la adaptación debe conservar de forma visible y completa los
   siguientes avisos:
   - "Adaptado a partir de clevo-indicator de SkyLandTW
     (https://github.com/SkyLandTW/clevo-indicator)"
   - "Adaptación por prsa (2026) — licenciada bajo términos de adaptación no
     comercial con atribución"

2. **Uso no comercial.** La adaptación NO podrá usarse, incorporarse o
   distribuirse como parte de productos, servicios o actividades comerciales
   (incluyendo ofertas SaaS, software embebido en hardware vendido, servicios
   de consultoría que redistribuyan el código, etc.) sin autorización escrita
   previa del adaptador (prsa).

3. **Gratuidad.** No se cobrará licencia, royalty ni contraprestación económica
   por la adaptación en sí. (Se permite cobrar por servicios de instalación,
   configuración o soporte asociados, siempre que la adaptación se entregue sin
   coste adicional y con los avisos del punto 1.)

4. **Sin garantía.** LA ADAPTACIÓN SE PROPORCIONA "TAL CUAL", SIN GARANTÍA DE
   NINGÚN TIPO, EXPRESA O IMPLÍCITA, INCLUYENDO PERO NO LIMITADO A GARANTÍAS DE
   COMERCIABILIDAD, IDONEIDAD PARA UN PROPÓSITO PARTICULAR Y NO INFRACCIÓN. EN
   NINGÚN CASO EL ADAPTADOR SERÁ RESPONSABLE POR RECLAMACIONES, DAÑOS U OTRAS
   RESPONSABILIDADES.

5. **Separabilidad.** Si alguna cláusula se declarase inválida o no exigible,
   las demás permanecerán en vigor.

6. **Ley aplicable y jurisdicción.** Esta licencia se rige por la legislación
   española. Para cualquier controversia, las partes se someten a los juzgados
   y tribunales de la residencia del adaptador.

### DEFINICIÓN DE "ADAPTACIÓN"

A efectos de esta licencia, "adaptación" comprende ÚNICAMENTE las partes de
este repositorio que difieren del código original de SkyLandTW/clevo-indicator,
incluyendo (lista no exhaustiva):

- Implementación propia del item SNI (sin libappindicator) mediante GDBus:
  registro ante StatusNotifierWatcher, propiedades Id, Category, Title, Status,
  ToolTip, IconPixmap, IconName, Menu, etc.
- Dibujo del icono de ventilador (aspas giratorias) con cairo.
- Tooltip rico (ToolTip title + description) con emisión manual de NewToolTip.
- Menú vía libdbusmenu-glib (com.canonical.dbusmenu).
- Fix del handler SIGCHLD (solo termina si muere el worker del EC).
- Ajuste MAX_FAN_RPM 4400 → 5500.
- Portabilidad a Ubuntu 26.04 (dependencias: gio-2.0, dbusmenu-glib-0.4,
  cairo; sin GTK ni appindicator).
- Ficheros de documentación añadidos: NOTAS.md, LICENSE.md.
- Cualquier otra modificación, adición o reescritura realizada por prsa sobre
  la base original.

El código original (base) conserva su licencia Unlicense (dominio público).
Las partes no modificadas del original NO están sujetas a esta licencia de
adaptación.

---

## English

### ADAPTATION LICENSE (non-commercial, attribution required)

Copyright (c) 2026 prsa (adapter)

Permission is granted to copy, modify, use and distribute the parts of this code
that constitute the ADAPTATION (defined below), subject to the following
conditions:

1. **Attribution required.** Any copy, distribution or derivative work that
   includes the adaptation must retain, visibly and completely, the following
   notices:
   - "Adapted from clevo-indicator by SkyLandTW
     (https://github.com/SkyLandTW/clevo-indicator)"
   - "Adaptation by prsa (2026) — licensed under non-commercial adaptation
     terms with attribution"

2. **Non-commercial use.** The adaptation may NOT be used, incorporated or
   distributed as part of commercial products, services or activities
   (including SaaS offerings, software embedded in sold hardware, consulting
   services that redistribute the code, etc.) without prior written
   authorization from the adapter (prsa).

3. **Free of charge.** No license fee, royalty or monetary consideration shall
   be charged for the adaptation itself. (Charging for installation,
   configuration or support services is permitted, provided the adaptation is
   delivered at no additional cost and with the notices in item 1.)

4. **No warranty.** THE ADAPTATION IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
   KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
   EVENT SHALL THE ADAPTER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY.

5. **Severability.** If any provision is held invalid or unenforceable, the
   remaining provisions shall remain in force.

6. **Governing law and jurisdiction.** This license is governed by Spanish law.
   Any dispute shall be submitted to the courts of the adapter's residence.

### DEFINITION OF "ADAPTATION"

For the purposes of this license, "adaptation" comprises ONLY the parts of this
repository that differ from the original code of SkyLandTW/clevo-indicator,
including (non-exhaustive list):

- Own implementation of the SNI item (without libappindicator) via GDBus:
  registration with StatusNotifierWatcher, properties Id, Category, Title,
  Status, ToolTip, IconPixmap, IconName, Menu, etc.
- Drawing of the fan icon (rotating blades) with cairo.
- Rich tooltip (ToolTip title + description) with manual emission of NewToolTip.
- Menu via libdbusmenu-glib (com.canonical.dbusmenu).
- SIGCHLD handler fix (only terminates if the EC worker dies).
- MAX_FAN_RPM adjustment 4400 → 5500.
- Portability to Ubuntu 26.04 (dependencies: gio-2.0, dbusmenu-glib-0.4, cairo;
  no GTK or appindicator).
- Documentation files added: NOTAS.md, LICENSE.md.
- Any other modification, addition or rewrite made by prsa on the original base.

The original (base) code retains its Unlicense (public domain). The unmodified
parts of the original are NOT subject to this adaptation license.

---

## 简体中文

### 改编许可（非商业，必须署名）

版权所有 (c) 2026 prsa（改编者）

特此授予复制、修改、使用和分发本代码中构成"改编部分"（定义见下文）的
权利，但须遵守以下条件：

1. **必须署名。** 任何包含本改编部分的副本、分发或衍生作品，都必须以清晰
   完整的方式保留以下声明：
   - "改编自 SkyLandTW 的 clevo-indicator
     （https://github.com/SkyLandTW/clevo-indicator）"
   - "由 prsa 改编（2026）——依据非商业改编条款并保留署名"

2. **非商业使用。** 未经改编者（prsa）事先书面授权，不得将本改编部分用于、
   并入或随商业产品、服务或活动（包括 SaaS 服务、随销售硬件分发的软件、
   再分发本代码的咨询服务等）一起分发。

3. **免费。** 不得就本改编部分本身收取许可费、版税或任何金钱对价。（允许
   就安装、配置或支持服务收费，前提是本改编部分不额外收费，且保留第 1 条
   中的声明。）

4. **无担保。** 本改编部分按"原样"提供，不附带任何明示或默示的担保，包括
   但不限于适销性、特定用途适用性和不侵权的担保。在任何情况下，改编者均
   不对任何索赔、损害或其他责任承担责任。

5. **可分割性。** 如果本许可的任何条款被认定为无效或不可执行，其余条款仍
   应继续有效。

6. **适用法律与管辖。** 本许可受西班牙法律管辖。任何争议均应提交改编者
   住所地的法院管辖。

### "改编部分"的定义

就本许可而言，"改编部分"仅指本仓库中与 SkyLandTW/clevo-indicator 原始代码
不同的部分，包括（非穷尽列表）：

- 通过 GDBus 自行实现 SNI 项目（不使用 libappindicator）：向
  StatusNotifierWatcher 注册，属性包括 Id、Category、Title、Status、ToolTip、
  IconPixmap、IconName、Menu 等。
- 使用 cairo 绘制风扇图标（旋转叶片）。
- 富工具提示（ToolTip title + description），并手动发送 NewToolTip。
- 通过 libdbusmenu-glib（com.canonical.dbusmenu）实现菜单。
- 修复 SIGCHLD 处理程序（仅在 EC worker 死亡时终止）。
- MAX_FAN_RPM 调整 4400 → 5500。
- 适配 Ubuntu 26.04（依赖：gio-2.0、dbusmenu-glib-0.4、cairo；不依赖 GTK
  或 appindicator）。
- 新增文档文件：NOTAS.md、LICENSE.md。
- prsa 在原始基础上进行的任何其他修改、添加或重写。

原始（基础）代码保留其 Unlicense（公有领域）。原始代码中未修改的部分不受
本改编许可约束。

---

## Licencia original (Unlicense)

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
