

.PHONY : all clean


all : gps_sch.pdf \
	gps-front.svg gps-front.png \
	gps-back.svg gps-back.png
#	gps_sch.svg # not working properly with 1.9.1

clean:
	-rm -v gps_sch.pdf
	-rm -v gps.ps gps.pdf
	-rm -v gps-front.pdf gps-front.svg gps-front.png
	-rm -v gps-back.pdf gps-back.svg gps-back.png



gps_sch.pdf gps_sch.svg : gps.sch
	gaf export -o $@ $<


%.ps : %.pcb
	pcb -x ps --fill-page --ps-color --fill-page --no-outline --no-align-marks --no-show-legend --psfile $@ $< # see pcb --help

%.pdf : %.ps
	ps2pdf $< $@

# %-front.png : %.pcb
# 	pcb -x png --as-shown --use-alpha --outfile $@ $< # see pcb --help

%-front.png : %.pdf
	convert -trim -rotate 90 $<[10] $@ # http://imagemagick.org/discourse-server/viewtopic.php?t=16565

%-back.png : %.pdf
	convert -trim -rotate -90 $<[09] $@ # http://imagemagick.org/discourse-server/viewtopic.php?t=16565

%-front.pdf: %.pdf
	pdftk A=$< cat A11east output $@
	inkscape $@ --export-pdf=$@ --export-area-drawing

%-back.pdf: %.pdf
	pdftk A=$< cat A10west output $@
	inkscape $@ --export-pdf=$@ --export-area-drawing

%.svg : %.pdf # http://www.imagemagick.org/discourse-server/viewtopic.php?t=6458
	inkscape $< --export-plain-svg=$@ --export-area-drawing

