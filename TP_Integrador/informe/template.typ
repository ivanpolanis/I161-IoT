// Plantilla del informe — TP Integrador IoT
// Estilos, portada y utilidades (diagramas, cuadros, placeholders de figura)

#let acento = rgb("#1f4e79")
#let acento2 = rgb("#2e7d32")
#let gris = rgb("#5b5b5b")
#let grisclaro = rgb("#f2f2f2")

// ─── Configuración global del documento ──────────────────────────────────────
#let conf(titulo: "", subtitulo: "", autores: (), materia: "", fecha: "", doc) = {
  set page(
    paper: "a4",
    margin: (x: 2.2cm, y: 2.2cm),
    numbering: "1 / 1",
    number-align: center,
  )
  set text(size: 10.5pt, lang: "es", hyphenate: true)
  set par(justify: true, leading: 0.62em, spacing: 0.9em)

  set heading(numbering: "1.1")
  show heading.where(level: 1): it => {
    set text(size: 13pt, fill: acento, weight: "bold")
    block(above: 1.3em, below: 0.7em)[#it]
  }
  show heading.where(level: 2): it => {
    set text(size: 11pt, fill: black, weight: "bold")
    block(above: 1.0em, below: 0.5em)[#it]
  }

  show raw: set text(size: 9pt)
  set list(indent: 0.6em, spacing: 0.6em)
  set enum(indent: 0.6em, spacing: 0.6em)

  // ── Portada ──
  block(width: 100%)[
    #align(center)[
      #v(0.4cm)
      #text(size: 9.5pt, fill: gris, tracking: 2pt)[#upper(materia)]
      #v(0.9cm)
      #line(length: 40%, stroke: 0.8pt + acento)
      #v(0.5cm)
      #text(size: 21pt, weight: "bold", fill: acento)[#titulo]
      #v(0.25cm)
      #text(size: 12.5pt, fill: gris)[#subtitulo]
      #v(0.5cm)
      #line(length: 40%, stroke: 0.8pt + acento)
      #v(0.9cm)
      #grid(
        columns: (auto,),
        row-gutter: 0.35em,
        ..autores.map(a => text(size: 11pt)[#a])
      )
      #v(0.5cm)
      #text(size: 10pt, fill: gris)[#fecha]
    ]
  ]
  v(0.6cm)
  line(length: 100%, stroke: 0.5pt + gris)
  v(0.2cm)

  doc
}

// ─── Nodo para diagramas ──────────────────────────────────────────────────────
#let nodo(cuerpo, relleno: grisclaro, borde: acento, ancho: auto) = box(
  fill: relleno,
  stroke: 0.8pt + borde,
  radius: 4pt,
  inset: (x: 8pt, y: 6pt),
  width: ancho,
)[#align(center)[#text(size: 8.5pt)[#cuerpo]]]

// Flecha con etiqueta
#let flecha(etq: none) = align(center)[
  #if etq != none [ #text(size: 7pt, fill: gris)[#etq] \ ]
  #text(size: 12pt, fill: acento)[$arrow.b$]
]

// ─── Placeholder para captura/foto que el usuario completará ──────────────────
#let placeholder(titulo, alto: 4cm) = figure(
  kind: image,
  supplement: "Figura",
  box(
    width: 100%,
    height: alto,
    stroke: (paint: gris, thickness: 0.9pt, dash: "dashed"),
    radius: 4pt,
    fill: rgb("#fafafa"),
    inset: 10pt,
  )[
    #align(center + horizon)[
      #text(size: 9pt, fill: gris)[📷 *Espacio reservado para captura*]
      #v(0.2em)
      #text(size: 8.5pt, fill: gris, style: "italic")[#titulo]
    ]
  ],
  caption: titulo,
)

// ─── Caja de nota/destacado ───────────────────────────────────────────────────
#let nota(cuerpo, color: acento) = block(
  width: 100%,
  fill: color.lighten(90%),
  stroke: (left: 3pt + color),
  radius: 2pt,
  inset: (x: 10pt, y: 8pt),
)[#text(size: 9.5pt)[#cuerpo]]
