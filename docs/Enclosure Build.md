# Enclosure Build

The dashboard sits in a small pine wood frame built around the e-paper panel.

![Pine stock cut to rough length, ruler and saw alongside](../images/wood-1.JPG)

## Pieces

| Piece | Role             | Length × Width × Thickness | Qty |
|-------|------------------|-----------------------------|-----|
| A     | Top / bottom rail | 163 × 44 × 13 mm            | 2   |
| B     | Side rail         | 125 × 44 × 13 mm            | 2   |

Material: pine. Cuts were guided by the on-screen ruler from `docs/Screen Measurement Mode.md` (`SHOW_SCREEN_RULER`), latched on the panel and used as a 1:1 physical template.

## Scaled drawings

Dimensions in millimetres, drawings to scale within each diagram.

### Piece A — top/bottom — 163 × 44 × 13 (×2)

<svg width="271" height="74" viewBox="0 0 271 74" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <marker id="arrow-a" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">
      <path d="M0,0 L10,5 L0,10 Z" fill="#b23b2e"></path>
    </marker>
  </defs>
  <g stroke="#ddd0b8" stroke-width="0.4">
    <path d="M25,0 V74 M35,0 V74 M45,0 V74 M55,0 V74 M65,0 V74 M75,0 V74 M85,0 V74 M95,0 V74 M105,0 V74 M115,0 V74 M125,0 V74 M135,0 V74 M145,0 V74 M155,0 V74 M165,0 V74 M175,0 V74 M188,0 V74"></path>
  </g>
  <!-- plan view: 163 x 44 -->
  <rect x="25" y="20" width="163" height="44" fill="#ecd9b6" stroke="#2b2115" stroke-width="1"></rect>
  <!-- length dim -->
  <line x1="25" y1="12" x2="188" y2="12" stroke="#b23b2e" stroke-width="0.8" marker-start="url(#arrow-a)" marker-end="url(#arrow-a)"></line>
  <line x1="25" y1="8" x2="25" y2="20" stroke="#b23b2e" stroke-width="0.5"></line>
  <line x1="188" y1="8" x2="188" y2="20" stroke="#b23b2e" stroke-width="0.5"></line>
  <rect x="93" y="6" width="26" height="9" fill="#fffdf7"></rect>
  <text x="106" y="13" font-size="7" fill="#b23b2e" text-anchor="middle">163</text>
  <!-- width dim -->
  <line x1="15" y1="20" x2="15" y2="64" stroke="#b23b2e" stroke-width="0.8" marker-start="url(#arrow-a)" marker-end="url(#arrow-a)"></line>
  <line x1="10" y1="20" x2="25" y2="20" stroke="#b23b2e" stroke-width="0.5"></line>
  <line x1="10" y1="64" x2="25" y2="64" stroke="#b23b2e" stroke-width="0.5"></line>
  <rect x="7" y="37" width="16" height="9" fill="#fffdf7"></rect>
  <text x="15" y="44" font-size="7" fill="#b23b2e" text-anchor="middle">44</text>
  <!-- end view: 13 x 44 -->
  <rect x="216" y="20" width="13" height="44" fill="#c1873f" stroke="#2b2115" stroke-width="1"></rect>
  <line x1="216" y1="12" x2="229" y2="12" stroke="#b23b2e" stroke-width="0.8" marker-start="url(#arrow-a)" marker-end="url(#arrow-a)"></line>
  <line x1="216" y1="8" x2="216" y2="20" stroke="#b23b2e" stroke-width="0.5"></line>
  <line x1="229" y1="8" x2="229" y2="20" stroke="#b23b2e" stroke-width="0.5"></line>
  <rect x="211" y="5" width="24" height="9" fill="#fffdf7"></rect>
  <text x="223" y="12" font-size="7" fill="#b23b2e" text-anchor="middle">13</text>
  <text x="223" y="72" font-size="6" fill="#6b5c47" text-anchor="middle">section</text>
</svg>

### Piece B — sides — 125 × 44 × 13 (×2)

<svg width="233" height="74" viewBox="0 0 233 74" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <marker id="arrow-b" viewBox="0 0 10 10" refX="8" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">
      <path d="M0,0 L10,5 L0,10 Z" fill="#b23b2e"></path>
    </marker>
  </defs>
  <g stroke="#ddd0b8" stroke-width="0.4">
    <path d="M25,0 V74 M35,0 V74 M45,0 V74 M55,0 V74 M65,0 V74 M75,0 V74 M85,0 V74 M95,0 V74 M105,0 V74 M115,0 V74 M125,0 V74 M135,0 V74 M145,0 V74 M150,0 V74"></path>
  </g>
  <!-- plan view: 125 x 44 -->
  <rect x="25" y="20" width="125" height="44" fill="#ecd9b6" stroke="#2b2115" stroke-width="1"></rect>
  <line x1="25" y1="12" x2="150" y2="12" stroke="#b23b2e" stroke-width="0.8" marker-start="url(#arrow-b)" marker-end="url(#arrow-b)"></line>
  <line x1="25" y1="8" x2="25" y2="20" stroke="#b23b2e" stroke-width="0.5"></line>
  <line x1="150" y1="8" x2="150" y2="20" stroke="#b23b2e" stroke-width="0.5"></line>
  <rect x="74" y="6" width="26" height="9" fill="#fffdf7"></rect>
  <text x="87" y="13" font-size="7" fill="#b23b2e" text-anchor="middle">125</text>
  <!-- width dim -->
  <line x1="15" y1="20" x2="15" y2="64" stroke="#b23b2e" stroke-width="0.8" marker-start="url(#arrow-b)" marker-end="url(#arrow-b)"></line>
  <line x1="10" y1="20" x2="25" y2="20" stroke="#b23b2e" stroke-width="0.5"></line>
  <line x1="10" y1="64" x2="25" y2="64" stroke="#b23b2e" stroke-width="0.5"></line>
  <rect x="7" y="37" width="16" height="9" fill="#fffdf7"></rect>
  <text x="15" y="44" font-size="7" fill="#b23b2e" text-anchor="middle">44</text>
  <!-- end view: 13 x 44 -->
  <rect x="178" y="20" width="13" height="44" fill="#c1873f" stroke="#2b2115" stroke-width="1"></rect>
  <line x1="178" y1="12" x2="191" y2="12" stroke="#b23b2e" stroke-width="0.8" marker-start="url(#arrow-b)" marker-end="url(#arrow-b)"></line>
  <line x1="178" y1="8" x2="178" y2="20" stroke="#b23b2e" stroke-width="0.5"></line>
  <line x1="191" y1="8" x2="191" y2="20" stroke="#b23b2e" stroke-width="0.5"></line>
  <rect x="173" y="5" width="24" height="9" fill="#fffdf7"></rect>
  <text x="185" y="12" font-size="7" fill="#b23b2e" text-anchor="middle">13</text>
  <text x="185" y="72" font-size="6" fill="#6b5c47" text-anchor="middle">section</text>
</svg>

## Assembly

The top rail and the two side rails are glued and nailed together into a fixed U shape. A channel is carved into the inner face of each of these three pieces; the e-paper panel's edges slide into that channel from the open bottom.

The bottom rail is not glued or nailed: it stays loose so the frame can be opened. To assemble, the panel slides down into the channels of the fixed U, then the bottom rail closes underneath to hold it in place.

There is no corner joinery (no miter, no notch) between the four wood pieces themselves: the rails simply butt against each other at the corners.

## Stock

2×163 + 2×125 = 576 mm of linear pine stock at 44 mm width covers the four pieces (not accounting for saw kerf between cuts).

## Photos

The channel-cut U frame dry-fit around the panel, using `docs/Screen Measurement Mode.md`'s on-screen ruler (still latched on the panel) to check the fit before gluing:

![U-shaped frame dry-fit around the e-paper panel, screen ruler pattern latched on it](../images/wood-2.JPG)

Wiring inside the closed frame, before the bottom rail goes back on — the Waveshare driver HAT top-left, the DHT22 bottom-left, and the ESP32-S3 header on the right:

![Wiring inside the assembled frame: driver HAT, DHT22 and ESP32-S3 header](../images/inside-1.JPG)

## Open items

- Finish (paint, oil, or raw wood) not yet decided.
