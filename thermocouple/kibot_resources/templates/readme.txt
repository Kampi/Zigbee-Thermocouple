<p align="center" width="100%">
  <img alt="Logo" width="33%" src="Images/Logos/Logo.png">
</p>

<h1 align="center">Thermocouple</h1>

<p align="center" width="100%">
  <a href="${GIT_URL}/actions/workflows/pcb.yaml">
    <img alt="CI Badge" src="${GIT_URL}/actions/workflows/pcb.yaml/badge.svg">
  </a>
</p>

***

<p align="center">
  <img alt="3D Top Angled" src="${png_3d_viewer_angled_top_outpath}" width="45%">
&nbsp; &nbsp; &nbsp; &nbsp;
  <img alt="3D Bottom Angled" src="${png_3d_viewer_angled_bottom_outpath}" width="45%">
</p>

***

## SPECIFICATIONS

| Parameter | Value |
| --- | --- |
| Dimensions | ${bb_w_mm} Ã— ${bb_h_mm} mm |

***

## PROJECT DIRECTORY STRUCTURE

    .
    â”œâ”€ kibot_resources       # External resources for KiBot
    â”‚  â”œâ”€ colors             # Color theme for KiCad
    â”‚  â”œâ”€ fonts              # Fonts used in the project
    â”‚  â”œâ”€ scripts            # External scripts used with KiBot
    â”‚  â””â”€ templates          # Templates for KiBot generated reports
    â”œâ”€ kibot_yaml            # KiBot YAML config files
    â”œâ”€ logos                 # Logos
    â”œâ”€ meta                  # Project metadata
    â”œâ”€ templates             # Title block templates
    â”œâ”€ *.kicad_sch           # KiCad schematic files
    â”œâ”€ *.kicad_pcb           # KiCad PCB layout
    â”œâ”€ *.kicad_pro           # KiCad project file
    â”œâ”€ LICENSE               # Project license
    â”œâ”€ README                # Project README
    â””â”€ kibot_launch.sh       # KiBot automation script

## OUTPUT DIRECTORY STRUCTURE

    â”œâ”€ 3D                    # 3D export of the PCB
    â”œâ”€ HTML                  # HTML files for generated webpage
    â”œâ”€ Images                # Pictures and renders
    â”œâ”€ KiRI                  # KiRI (PCB diff viewer) files
    â”œâ”€ Logos                 # Logos
    â”œâ”€ Manufacturing         # Assembly and fabrication documents
    â”‚  â”œâ”€ Assembly           # Assembly documents (BoM, pos, notes)
    â”‚  â””â”€ Fabrication        # Fabrication documents (ZIP, notes)
    â”‚     â”œâ”€ Drill Tables    # CSV drill tables
    â”‚     â””â”€ Gerbers         # Gerbers
    â”œâ”€ Netlist               # PCB netlist
    â”œâ”€ Reports               # Reports for ERC/DRC
    â”œâ”€ Schematic             # PDF of schematic
    â”œâ”€ Testing
    â”‚  â””â”€ Testpoints         # Testpoints tables
    â””â”€ Variants              # Outputs for assembly variants, if available