# GPS-logger

<img src="IMGs/front.jpg" width="200"> <img src="IMGs/back.jpg" width="135"> <img src="IMGs/schematics.png" width="215"> <img src="IMGs/pcb_front.png" width="120"> <img src="IMGs/pcb_back.png" width="132">

GPS-logger project (based on university course: "EDA, PCB and AVR-Microcontroller")


## Structure

- `PCB/`   schematics of the electronic circuits (`*.sch`) as well as the PCB layouts (`*.pcb`)
  - `PCB/gschem-sym/`   symbols for gschem (referenced from PCB/gafrc)
  - `PCB/pcb-elements/`   custum footprints for non-standard devices used in the layouts (get included in `*.pcb`)
- `code/`   c-files for the firmware
  - `code/unused/`   unused files from ATMega-DOS (from Holger Klabunde)


Software from the [gEDA project](http://www.geda-project.org/) was used to create the PCB:
- [gschem](http://wiki.geda-project.org/geda:gaf) for the schematics (`*.sch`)
- [PCB](http://pcb.geda-project.org/) for the layouts  (`*.pcb`)


## Images


fotos:

<img src="IMGs/front.jpg" width="200"> <img src="IMGs/back.jpg" width="135"> <img src="IMGs/screen_start.jpg" width="278">


schematics (also as [PDF](IMGs/schematics.pdf)):

<img src="IMGs/schematics.png" width="400">


pcb layout ([SVGs](IMGs/)):

<img src="IMGs/pcb_front.png" width="200"> <img src="IMGs/pcb_back.png" width="220">


screen shots of the 7 display-modes (satelite view and satelite list):

<img src="IMGs/screen_satelite-view_bl.jpg" width="200"> <img src="IMGs/screen_satelite-list_bl.jpg" width="200">
