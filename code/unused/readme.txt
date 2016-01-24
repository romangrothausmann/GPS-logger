###############################################
# Willkommen bei Holgi's kleinem ATMega-DOS ! #
###############################################

################################################################################
# Nachbau und Benutzung auf eigene Gefahr !
#
# Vor dem ersten Versuch folgende Dinge kontrollieren:
# Im makefile nachsehen welcher Prozessor eingestellt ist.
# In mydefs.h den Prozessortakt richtig einstellen.
# In media.h das Medium (CF/SD/MMC) ausw�hlen. 
# In dosdefs.h alle gew�nschten DOS Routinen de-/aktivieren. 
################################################################################

Viel Spa� damit
hk@holger-klabunde.de
http://www.holger-klabunde.de/index.html

24.09.2006

################################################################################
# Warnung: DOS legt die Daten in Partitionstabelle,Bootsektor,Verzeichnissen
#          und FAT im LITTLE ENDIAN Format ab. Wer z.B. KEIL C51 benutzt
#          muss diese Daten beim lesen erst in BIG ENDIAN umrechnen.
#          Beim schreiben muss wieder ins LITTLE ENDIAN Format umgerechnet
#          werden, sonst kann DOS nichts mehr lesen.
################################################################################

Mein ATMega-DOS ist nur f�r Prozessoren ab 1kB RAM geeignet. Ein FAT-Buffer kann
nur ab 2kB RAM benutzt werden ! ATMega8 hat zwar gen�gend RAM, aber evtl. nicht
ausreichend Flash f�r ALLE Routinen !
Mit AT90Sxxxx AVR's oder ATMega8515 arbeiten diese Routinen nicht !

CompactFlash (CF) werden hier im 8 Bit Microcontroller Modus betrieben.
Die Ansteuerung hat nichts mit Festplatten zu tun. Die FAT-Routinen zum
lesen/schreiben von Dateien k�nnte man aber �bernehmen.

Neu hinzugekommen sind Routinen f�r die Ansteuerung von MultiMedia- (MMC)
und SecureDigital- (SD) Cards im SPI Modus.

Das meiste was hier zu CF gesagt wird gilt auch f�r MMC/SD.

Wer wissen m�chte wie schnell das ganze arbeitet findet in den
Unterverzeichnissen der Testprogramme jeweils eine Datei readme.txt
mit unverbindlichen Messungen ;)

Und ein ganz wichtiger Tip: Halte die Kabel kurz ! Nicht mehr als 10cm.
Bei meinen MMC war bereits bei 18cm NICHTS mehr zu machen.

CF brauchen unbedingt Pullups am Datenport. Ohne interne Pullups vom
ATMega ging fast gar nichts. Bei langen Kabeln k�nnte es erforderlich
sein externe Pullups um 10k am Datenport anzuschliessen.

Das Programm erwartet das ein CF/MMC/SD vorhanden ist ! Ich bekam schon
R�ckmeldungen das nichts angezeigt wird wenn z.B. kein MMC angeschlossen
ist, und man einige Schleifen mit Timeouts versehen sollte damit doch noch
eine Meldung kommt. Wozu ? Das macht das Programm nur langsamer. Es war nie
dazu vorgesehen OHNE CF/MMC/SD zu funktionieren ;)

CF/MMC/SD k�nnen NICHT im laufenden Programm eingesteckt oder rausgezogen werden !
Bitte die Schaltung deaktivieren bevor du das machst.

###################
# Zur Geschichte: #
###################

Ziel des Projektes war es moderne Speichermedien mit einem Microcontroller
zu lesen und zu schreiben weil ein 512kB Flash Eprom wie 29F040 manchmal
noch zu klein ist. Flash Eproms brauchen auch zu viele Pins f�r die
Ansteuerung. Die sind in der Schaltung oft nicht frei.

Au�erdem sollte eine einfache M�glichkeit her die Daten auch mit dem PC zu
lesen oder zu schreiben. Also am besten ein FAT-Filesystem. Kann man mit
jedem billigen CF/MMC/SD-Reader lesen und schreiben.

Beim Microcontroller sollte m�glichst kein externes RAM zum Einsatz
kommen. Das ist ganz gut gelungen. 0.7kB f�r ein Dateisystem ohne
FAT-Buffer und 1.2kB mit FAT-Buffer passt noch gut in einen ATMega32.
Programmspeicherbedarf mit ALLEN Funktionen bisher unter 12kB (6k Worte).

Da die Routinen in C geschrieben sind d�rfte es nicht all zu schwer
sein das ganze auch auf 8051 (dann externes RAM) oder anderen Micro-
controllern zum laufen zu bringen. PIC16 sind da leider keine gute
Wahl weil man wegen des RAM Bankings maximal 256 Byte RAM am St�ck
bekommt. Das d�rfte recht kompliziert werden. Bei PIC18 siehts besser aus.

Hier eine �bersicht welche Funktionen man zur Zeit benutzen kann
ohne das man sich erst in FAT-Dateisysteme einarbeiten muss. Von
den anderen Funktionen sollte man besser die Finger weglassen !

unsigned char GetDriveInformation(void);
unsigned char Fopen(char *name, unsigned char flag);
unsigned int Fread(unsigned char *buf, unsigned int numbytes);
unsigned int Fwrite(unsigned char *buf, unsigned int numbytes);
unsigned int Fputc(unsigned char buf);
unsigned char Fgetc(void);
unsigned long Filelength(void);
void Fflush(void);
void Fclose(void);
unsigned char Chdir(char *name);
unsigned char Mkdir(char *name);
unsigned char Remove(char *name);
unsigned char FindName(char *name);
unsigned char ReadFileRaw(char *name);
unsigned char Findfirst(void);
unsigned char Findnext(void);

Namen und Parameter sind so weit wie m�glich an die DOS Funktionen
angelehnt die man auch vom PC her kennt. Es gibt allerdings
Unterschiede und Einschr�nkungen. Siehe unten.

Zuerst einmal: Was geht nicht !
===============================
Lange Dateinamen lesen wird zum Teil unterst�tzt. Siehe Findfirst() und Findnext()
Mehrere Dateien gleichzeitig �ffnen geht NICHT.
Es kann immer nur EINE Datei ge�ffnet und bearbeitet werden.
Eine Datei kann nur zum lesen ODER schreiben ge�ffnet werden.
Man kann eine Datei nicht zum schreiben �ffnen und
alte Daten �berschreiben. Es wird IMMER hinten drangeh�ngt.
Formatieren geht nat�rlich nicht.
Verzeichnisse l�schen geht nicht.
Noch kein Fseek() Befehl vorhanden.
Es wird keine Kopie von der FAT angelegt. Scandisk meckert da !
Man kann nur mit der ersten Partition arbeiten.
Es gibt keine Laufwerksnamen wie C:
Pfadangaben in Dateinamen sind nicht erlaubt. Man kann mit Chdir()
aber schrittweise in mehrere Unterverzeichnisse wechseln.
Man kann den CF nicht aus der laufenden Schaltung rausziehen bzw.
einen CF reinstecken.

Was geht ?
==========
Das Programm unterst�tzt FAT12,FAT16 und FAT32 Dateisysteme.
Man kann Dateien lesen und neue Dateien erstellen.
Daten an vorhandene Dateien dranh�ngen. Dateien l�schen.
Man kann neue Verzeichnisse erstellen. Auch in Unterverzeichnissen.
Man kann in Verzeichnisse wechseln und alle oben genannten Funktionen
darin ausf�hren. Alle Routinen arbeiten auch mit einer fragmentierten FAT !

RAM Speicherbedarf
==================
Ohne FATBuffer ca. 0,7kB RAM Bedarf
Mit FATBuffer ca. 1,2kB RAM Bedarf

Ohne FAT-Buffer ist nur f�r reine Lesesysteme empfehlenswert.
Schreiben sollte man damit nicht. Siehe unten. Beim lesen kommt
man auch ohne FAT Buffer auf gute Geschwindigkeiten.

Wenn man lange Dateinamen von Findfirst()/Findnext() bekommen
m�chte kommen noch einmal 256 Byte RAM dazu. Man kann den Wert
_MAX_NAME in find_x.h aber auch kleiner machen, z.B. 64. Wenn
der Name l�nger ist wird er einfach hinten abgeschnitten.

Probleme beim schreiben von Daten
=================================
Beim schreiben darf man die Schaltung nicht einfach ausschalten bzw.
den CF rausziehen bevor nicht Fclose() aufgerufen wurde. Die Dateigr��e
stimmt dann nicht oder es gehen Cluster verloren weil sie noch nicht
in die FAT eingetragen wurden. Der Grund daf�r ist das f�r Daten und
f�r die FAT immer ein kompletter Sektor im Speicher gehalten wird.
Diese Sektorbuffer werden nur geschrieben wenn neue Daten in einem
anderen Sektor liegen. Nur so kann man Schreibzugriffe auf den CF
minimieren und seine Lebensdauer wirklich ausnutzen.

Es gibt ein paar M�glichkeiten das Problem zu umgehen. 100% sicher
sind die z.B. bei Stromausfall aber nicht. Win oder Linux gehts da
auch nicht anders :)

1. Datei gelegentlich mal mit Fclose() schlie�en und neu �ffnen
2. Fflush() in gr��eren Zeitabst�nden aufrufen. NICHT im Sekunden-
   takt. Ein Tag hat 86400 Sekunden. Eher so alle 10 Minuten.
3. Fflush() aufrufen wenn eine vorgegebene Datenmenge erreicht ist.
4. Einen Eingang vorsehen der dazu dient Fclose() aufzurufen wenn
   ein Schalter geschlossen wird. Die sicherste Methode.
   
Aber bedenke: Ohne Notstromversorgung ist der Daten-GAU fast unvermeidlich !   
Ein Watchdog Reset kann einem auch den Tag (die Daten) verderben.

//###################################################################
//###################################################################
// Beschreibung der DOS Funktionen
//###################################################################
//###################################################################

Hier die Beschreibungen zu den wichtigsten Funktionen die man benutzen
kann. Um die anderen mu� man sich nur Gedanken machen wenn man selbst
Erweiterungen programmieren m�chte.

//###################################################################
// unsigned char GetDriveInformation(void)
//###################################################################

Funktion:  Diese Funktion ist die wichtigste. Sie ermittelt alle
           Daten zum CF wie Anzahl Sektoren, Anzahl Cluster und
           was f�r ein Filesystem auf dem CF ist (FAT12,FAT16,FAT32),
           wo sich das RootDirectory befindet und vieles mehr.
           Ohne diese Informationen k�nnen weder Daten vom CF gelesen
           noch darauf geschrieben werden.

           Diese Funktion MUSS vor allen anderen Funktionen aufgerufen
           werden !           

           Bei MMC/SD-Karten mu� MMC_IO_Init(); vor GetDriveInformation();
           aufgerufen werden.
           
Parameter: keine

R�ckgabe:  F_OK wenn ein CF gefunden wurde
           F_ERROR wenn kein CF gefunden wurde.
           Dann auf keinen Fall weitermachen !
           Irgend etwas stimmt dann mit der Hardware nicht oder das
           Kabel ist einfach zu lang. Eventuell ist der CF gar nicht
           mit FAT formatiert ! Oder mit einem FAT das nicht den MS
           Vorgaben entspricht.

//###################################################################
// unsigned char Fopen(char *name, unsigned char flag)
//###################################################################

Funktion:  �ffnet eine Datei zum lesen ODER schreiben

Parameter: "name" ist der DOS 8.3 Name der zu �ffnenden Datei
           "flag" F_READ Datei zum lesen �ffnen
           "flag" F_WRITE Datei zum schreiben �ffnen

R�ckgabe:  F_OK wenn die Datei ge�ffnet werden konnte. 
           F_ERROR wenn die Datei nicht ge�ffnet werden konnte
           oder bereits eine Datei offen ist.
           
           Wenn F_WRITE benutzt wird und die Datei noch nicht
           existiert, wird versucht eine neue Datei anzulegen.
           Wenn die Datei bereits existiert wird an ihr ENDE
           vorgespult um Daten anzuh�ngen. Es ist nicht vorgesehen
           alte Daten zu �berschreiben.

           Das aktuelle Verzeichnis wird automatisch vergr��ert wenn
           keine Eintr�ge mehr frei sind. Das geht bei FAT12 und FAT16
           nicht wenn man sich im RootVerzeichnis befindet. Die Anzahl
           der Eintr�ge ist fest vom Dateisystem vorgegeben.
           
           Wer mehr als 512 Dateien anlegen will MUSS bei FAT12 und
           FAT16 in einem Unterverzeichnis arbeiten !

//###################################################################
// void Fclose(void)
//###################################################################

Funktion:  Schlie�t eine offene Datei

Parameter: keine

R�ckgabe:  keine
           
           Bei FileFlag F_WRITE wird der Inhalt des Datei-Sektorbuffers
           geschrieben. Dazu wird Fflush() aufgerufen. Falls ein
           FAT-Write-Buffer verwendet wird, wird dieser auf den CF
           geschrieben. Auch das passiert in Fflush().
           
           Der Verzeichniseintrag der Datei wird mit der zuletzt
           ermittelten Dateigr��e aufgefrischt.
           
           Eine offene Datei muss AUF JEDEN FALL mit Fclose() geschlossen
           werden. Tut man dies nicht kann das zu einem kompletten
           Datenverlust f�hren.

           Wer eine Uhr in seinem System hat k�nnte mit einer Funktion
           , z.B. getclock(); , in UpdateFileEntry() auch Uhrzeit und
           Datum des letzten Schreibzugriffes speichern.

//###################################################################
// unsigned int Fread(unsigned char *buf, unsigned int numbytes)
//###################################################################

Funktion:  Liest eine bestimmte Anzahl Bytes aus einer Datei in einen
           Puffer

Parameter: "buf" Zeiger auf den Puffer wo die Daten rein sollen
           "numbytes" Anzahl der Bytes die gelesen werden sollen

R�ckgabe:  Anzahl der Bytes die gelesen wurden
           
Beispiel: 11 Bytes lesen

          unsigned char inbuff[10];
          unsigned char by;
          unsigned int readbytes;
          
          if(Fopen("mydata.dat",F_READ)==F_OK)
           {
            readbytes=Fread(inbuff,10);
            readbytes=Fread(&by,1);     // Achte auf das & vor by !
            Fclose();
           } 

          Wenn das Dateiende erreicht ist liefert Fread() die Anzahl
          bis zum Dateiende gelesener Bytes oder einfach eine 0.
           
//###################################################################
// unsigned int Fwrite(unsigned char *buf, unsigned int numbytes)
//###################################################################

Funktion:  Schreibt eine bestimmte Anzahl Bytes aus einem Puffer in
           eine Datei

Parameter: "buf" Zeiger auf den Puffer der die Daten enth�lt
           "numbytes" Anzahl der Bytes die geschrieben werden sollen

R�ckgabe:  Anzahl der Bytes die geschrieben wurden
           
Beispiel:
          unsigned char outbuff[10];
          unsigned char by;
          unsigned int written;
          
          for(by=0; by<10; by++) outbuff[by]=by;
          
          if(Fopen("mydata.dat",F_WRITE)==F_OK)
           {
            written=Fwrite("Hello World",11);
            written=Fwrite(outbuff,10);
            written=Fwrite(&by,1);     // Achte auf das & vor by !
            Fclose();
           } 

          Wenn Fwrite() nicht die Anzahl Bytes zur�ckliefert die
          geschrieben werden sollten, dann ist der CF voll !

//###################################################################
// void Fflush(void)
//###################################################################

Parameter: keine

R�ckgabe:  keine

           Fflush() ruft man auf wenn alle gesammelten Daten sofort
           auf den CF geschrieben werden sollen OHNE die Datei zu
           schlie�en.

           Der Inhalt des Sektorbuffers wird geschrieben. Falls ein
           FAT-Write-Buffer verwendet wird, wird dieser auf den CF
           geschrieben. Der Verzeichniseintrag der Datei wird mit der
           zuletzt ermittelten Dateigr��e aufgefrischt.
           
           Wer eine Uhr in seinem System hat k�nnte mit einer Funktion
           , z.B. getclock(); , in UpdateFileEntry() auch Uhrzeit und
           Datum des letzten Schreibzugriffes speichern.
           
//###################################################################
// unsigned long Filelength(void)
//###################################################################

Parameter: keine

R�ckgabe:  Die Dateigr��e.

           Ist als Macro in DOS.H definiert und entspricht FileSize.
           FileSize hat nur dann einen g�ltigen Wert wenn eine Datei
           mit Fopen() ge�ffnet wurde !

//###################################################################
// unsigned char Chdir(char *name)
//###################################################################

Funktion:  In ein Unter- oder ein h�heres Verzeichnis wechseln

Parameter: Der DOS 8.3 Verzeichnisname z.B. Chdir("tmp");

R�ckgabe:  F_OK wenn das Verzeichnis gefunden wurde,
           F_ERROR wenn das Verzeichnis nicht gefunden wurde.
           
           Man kann immer nur eine Verzeichnistiefe zur Zeit wechseln.
           Wenn man in "tmp/test" wechseln m�chte mu� Chdir()
           zweimal aufgerufen werden:
           
           Chdir("tmp");
           Chdir("test");
           
           Wenn man eine Verzeichnistiefe zur�ck m�chte ruft
           man einfach Chdir(".."); auf.

           Chdir("/"); wechselt ins RootVerzeichnis.
           
           Chdir() arbeitet NICHT wenn eine Datei offen ist !
           
//###################################################################
// unsigned char Mkdir(char *name)
//###################################################################

Funktion:  Ein neues Verzeichnis erzeugen

Parameter: Der DOS 8.3 Verzeichnisname z.B. Mkdir("tmp");

R�ckgabe:  F_OK wenn das Verzeichnis erstellt werden konnte oder
           bereits existiert.
           F_ERROR wenn das Verzeichnis nicht erstellt werden konnte.
           
           Mkdir() arbeitet NICHT wenn eine Datei offen ist !
           
//###################################################################
// unsigned char Remove(char *name)
//###################################################################

Funktion:  L�scht eine Datei

Parameter: Der DOS 8.3 Dateiname z.B. Remove("tmp");

R�ckgabe:  F_OK wenn die Datei gel�scht wurde (oder gar nicht existierte ;),
           F_ERROR wenn die Datei nicht gel�scht wurde.
           
           Remove() arbeitet NICHT wenn eine Datei offen ist !
           Remove() l�scht keine Verzeichnisse.

           Remove() bearbeitet die Eintr�ge von langen Dateinamen
           nicht wenn die Datei einen langen Dateinamen hatte ! Nur
           der Eintrag f�r den DOS-Namen wird entfernt. Es k�nnte sein
           das es dann Probleme beim lesen des CF mit dem PC gibt.

           Remove() ist eine Killer-Routine. Sie scheint zu
           funktionieren, falls es aber noch Bugs gibt k�nnten da eine
           Menge Daten verschwinden !
           
//###################################################################
// unsigned char Findfirst(void)
//###################################################################

Funktion:  Findet den ersten Datei- oder Verzeichniseintrag im
           aktuellen Verzeichnis.

Parameter: Keine

R�ckgabe:  0 wenn kein Verzeichnis und keine Datei gefunden wurde.
           1 wenn ein Verzeichnis oder eine Datei gefunden wurde.

           Findfirst() und Findnext() benutzen eine globale Struktur ffblk
           
           struct FindFile
            {
 	     unsigned char ff_attr;	// Attribut Datei oder Verzeichnis
 	     unsigned long ff_fsize;	// Dateigr��e, 0 bei Verzeichnissen
	     char ff_name[13];		// 8.3 DOS Dateiname mit '.' und \0 am Ende
          #ifdef USE_FINDLONG
             char ff_longname[_MAX_NAME];	// The very long filename.
          #endif
 	     unsigned int newposition;	// Finger weg !
 	     unsigned int lastposition;	// Position der gefundenen Datei/Verzeichnis
 					// Z�hlt auch ".", ".." mit ! Also Vorsicht.
            };

           struct FindFile ffblk;
           
           Findfirst() und Findnext() braucht man wenn man die Datei- oder
           Verzeichnisnamen auf dem Laufwerk nicht kennt. Mit diesen beiden
           Befehlen kann man DIR/LS Befehle bauen oder auch nach bestimmten
           Dateitypen suchen ( z.B. MP3 ;). Man kann den von den Dateien
           belegten Speicherplatz bestimmen indem man ffblk.ff_fsize aufaddiert.
           Den Dateinamen den Findfirst(),Findnext() zur�ckgeben kann man
           benutzen um mit Fopen(ffblk.ff_name); Dateien zu �ffnen. Verzeichnis-
           namen kann man an Chdir() �bergeben. Und wer m�chte kann mit
           Remove() alle Dateien auf dem Laufwerk l�schen. Die M�glichkeiten
           dieser beiden Funktionen sind also sehr vielf�ltig.
           
           Grundger�st f�r Findfirst(), Findnext()

           if(Findfirst()!=0)
            {
             do
              {
               //ffblk enth�lt jetzt die Daten des n�chsten gefundenen Eintrages

               if(ffblk.ff_attr==ATTR_FILE)
                {
                 //Bearbeite die Datei
                 //ffblk.ff_name[] enth�lt den DOS 8.3 Dateinamen
                 //ffblk.ff_longname[] enth�lt den langen Dateinamen
                }
              
               if(ffblk.ff_attr==ATTR_DIRECTORY)
                {
                 //Bearbeite das Verzeichnis
                 //ffblk.ff_name[] enth�lt den DOS 8.3 Verzeichnisnamen
                 //ffblk.ff_longname[] enth�lt den langen Verzeichnisnamen
                }

              }while(Findnext()!=0);
            }
           else
            {
             //Keine Datei/Verzeichnis gefunden
            }
            
           Wenn Chdir() aufgerufen wurde mu� auch Findfirst() noch
           einmal aufgerufen werden !

	   Wenn USE_FINDLONG definiert ist liefern Findfirst(), Findnext()
	   auch den langen Dateinamen in ffblk.ff_longname[] zur�ck. Falls
	   kein langer Dateiname zum DOS 8.3 Dateinamen existiert ist
	   ffblk.ff_longname[0] gleich Null .
	   
	   Man kann damit auch Dateien zum lesen oder schreiben �ffnen
	   wenn man nur den langen Dateinamen kennt. Man sucht mit Hilfe
	   von Findfirst(), Findnext() nach diesem Namen und �bergibt
	   dann den DOS 8.3 Namen : Fopen(ffblk.ff_name,flags).

           Man kann noch keine NEUEN Dateien mit langen Dateinamen auf dem
           Laufwerk erzeugen ! Sie m�ssen bereits existieren.

           Lange Dateinamen brauchen 256 Byte RAM zus�tzlich ! Falls der
           Speicher knapp wird kann man die L�nge in find_x.h einschr�nken.
           
           Zum Beispiel: #define _MAX_NAME 64
           
           Ist der lange Dateiname l�nger als 63 Zeichen wird er dann aber
           hinten abgeschnitten !
           
//###################################################################
// unsigned char Findnext(void)
//###################################################################

Funktion:  Findet den n�chsten Datei- oder Verzeichniseintrag im
           aktuellen Verzeichnis. Vor Findnext() MUSS einmal Findfirst()
           aufgerufen werden !

Parameter: Keine

R�ckgabe:  0 wenn kein Verzeichnis und keine Datei gefunden wurde.
           1 wenn ein Verzeichnis oder eine Datei gefunden wurde.

           Findnext() sucht bei jedem Aufruf immer vom Anfang des
           aktuellen Verzeichnisses nach dem n�chsten Eintrag.
           
           Auswertung von Findnext() siehe Findfirst().
           
//###################################################################
// unsigned char FindName(char *name)
//###################################################################

Funktion:  Sucht im aktuellen Verzeichnis nach einem Datei- oder
           Verzeichnisnamen.

Parameter: Der DOS 8.3 Datei/Verzeichnisname z.B. FindName("tmp");

R�ckgabe:  FULL_MATCH wenn die Datei/das Verzeichnis gefunden wurde.
           NO_MATCH wenn die Datei/das Verzeichnis nicht gefunden wurde.

           FindName() wird von Fopen(), Chdir(), Ckdir(),
           ReadFileRaw() und Remove() benutzt.           
           
           Theoretisch kann man auch mit Fopen() herausbekommen ob eine
           Datei existiert. Sie wird dann aber auch ge�ffnet.
           
           FindName() arbeitet NICHT wenn eine Datei offen ist !

Beispiel:  Suche nach der Datei "mydata1.dat". Wenn sie existiert dann
           "mydata2.dat" �ffnen. Wenn sie nicht existiert dann
           "mydata1.dat" erzeugen.
           
           if(FindName("mydata1.dat")==FULL_MATCH)
            {
             Fopen("mydata2.dat",F_WRITE);
            }
           else
            {
             Fopen("mydata1.dat",F_WRITE);
            } 
           
//###################################################################
// unsigned char ReadFileRaw(char *name)
//###################################################################

Funktion:  Liest eine Datei im RAW Modus

Parameter: Der DOS 8.3 Dateiname z.B. ReadFileRaw("mydata.dat");

R�ckgabe:  F_OK wenn die Datei gefunden und gelesen wurde,
           F_ERROR wenn die Datei nicht gefunden wurde.
           
           ReadFileRaw() ist wesentlich schneller als z.B. Fread().
           Es wird immer ein kompletter Sektor gelesen der dann
           irgendwie ausgewertet werden kann. Fread() ist aber
           flexibler weil man kleinere H�ppchen lesen kann.
           
           Fopen() darf f�r ReadFileRaw() nicht aufgerufen werden !
           Die Routine zum verarbeiten der Daten mu� in readraw.c
           eingetragen werden !

//###################################################################
//###################################################################
// Ende Beschreibung der DOS Funktionen
//###################################################################
//###################################################################

!!!!!!!!!!!!!!!!!!!!!!!!!!
!! Einsparm�glichkeiten !!
!!!!!!!!!!!!!!!!!!!!!!!!!!

Man kann ein bi�chen Speicherplatz sparen indem nicht ben�tigte
Programmteile beim compilieren weggelassen werden. So kann man ein
reines Lesesystem, ein reines Schreibsystem oder ein Schreib-/Lesesystem
erzeugen. Man kann auch ein System erzeugen das nur mit FAT16 arbeitet.
Mit oder ohne FAT Buffer. Holgi's kleiner FAT Baukasten sozusagen ;)

In den Unterverzeichnissen mit den Testprogrammen liegt jeweils eine
Datei namens "mydefs.h". Dort kann man bestimmte Programmteile ausschalten. 

Beispiel f�r alle Programmteile erzeugen, einfach keines von den #undef
aktivieren:

// spare program memory by deciding if we want to read, write or both
//#undef DOS_READ	//undefine this if you don't want to read files with Fread()
//#undef DOS_WRITE	//undefine this if you don't want to write files with Fwrite()
                        //deleting files is also deactivated
//#undef DOS_DELETE	//undefine this if you don't want to delete files
//#undef DOS_READ_RAW	//undefine this if you don't want to read files with ReadFileRaw()

// spare program memory by deciding if we want to use FAT12, FAT16, FAT32.
// comment out FAT types not used. NOT recommended if you don't know the
// FAT type of your drive !
//#undef USE_FAT12	//undefine this if you don't want to use FAT12
//#undef USE_FAT16	//undefine this if you don't want to use FAT16
//#undef USE_FAT32	//undefine this if you don't want to use FAT32

//#undef USE_FATBUFFER	//undefine this if you don't want to use a FAT Buffer


Beispiel f�r ein reines Schreibsystem das nur mit FAT16 arbeitet und
keinen FAT Buffer verwendet, l�schen von Dateien m�glich:

// spare program memory by deciding if we want to read, write or both
#undef DOS_READ	//undefine this if you don't want to read files with Fread()
//#undef DOS_WRITE	//undefine this if you don't want to write files with Fwrite()
                        //deleting files is also deactivated
//#undef DOS_DELETE	//undefine this if you don't want to delete files
#undef DOS_READ_RAW	//undefine this if you don't want to read files with ReadFileRaw()

// spare program memory by deciding if we want to use FAT12, FAT16, FAT32.
// comment out FAT types not used. NOT recommended if you don't know the
// FAT type of your drive !
#undef USE_FAT12	//undefine this if you don't want to use FAT12
//#undef USE_FAT16	//undefine this if you don't want to use FAT16
#undef USE_FAT32	//undefine this if you don't want to use FAT32

#undef USE_FATBUFFER	//undefine this if you don't want to use a FAT Buffer

Wenn man z.B. kein FAT32 benutzt werden alle Variablen die mit Clustern
zu tun haben nicht als "unsigned long" sondern als "unsigned int" deklariert.
Das bringt nicht viel, aber in Sonderf�llen k�nnte das die letzte Rettung sein ;)

Beispiele (inklusive ein paar LCD-Routinen):

DOSfreadTest1 mit allen Optionen:
Programmspeicher 9340 Bytes
   text	   data	    bss	    dec	    hex	filename
   9274	     66	   1695	  11035	   2b1b	dosread1.elf

DOSfreadTest1 mit allen Optionen, aber kein FAT-Buffer:
Programmspeicher 9154 Bytes
   text	   data	    bss	    dec	    hex	filename
   9088	     66	   1178	  10332	   285c	dosread1.elf

DOSfreadTest1 mit allen Optionen, aber nur FAT16, und kein FAT-Buffer:
Programmspeicher 7196 Bytes
   text	   data	    bss	    dec	    hex	filename
   7130	     66	   1164	   8360	   20a8	dosread1.elf

DOSfreadTest1 nur FAT16, kein Schreiben m�glich, ohne ReadFileRaw(), ohne FAT-Buffer:
Programmspeicher 4690 Bytes
   text	   data	    bss	    dec	    hex	filename
   4624	     66	   1164	   5854	   16de	dosread1.elf


#######################################################################################
# Was muss ich im Programm �ndern um z.B. eine Festplatte zu benutzen ?               #
#######################################################################################

Die DOS Routinen sind so geschrieben das man nur drei Funktionen braucht um
dos.c, fat.c, dir.c und readraw.c ohne �nderungen an unterschiedliche Hardware
anzupassen zu k�nnen (solange 512 Byte pro Sector benutzt werden !!): 

ReadSector(a,b);   // Lowlevel Routinen zum schreiben
WriteSector(a,b);  // Lowlevel Routinen zum lesen
IdentifyMedia();   // Ermittelt die Daten zum Laufwerk:
                   // Position vom FAT, FAT Typ
                   // Position des Rootdirectory
                   // maximale Anzahl Sektoren und Cluster

Per Macro kann man dann dem Compiler mitteilen welche Funktionen er benutzen soll:

CompactFlash: compact.h

#define ReadSector(a,b) 	CFReadSector((a),(b))
#define WriteSector(a,b) 	CFWriteSector((a),(b))
#define IdentifyMedia()		CFIdentify()
 
MMC/SD: mmc_spi.h

#define ReadSector(a,b) 	MMCReadSector((a),(b))
#define WriteSector(a,b) 	MMCWriteSector((a),(b))
#define IdentifyMedia()		MMCIdentify()

Denkbar w�ren also auch hdd.c und hdd.h f�r Festplatten oder RAM/ROM Laufwerke
mit FAT Dateisystem z.B. in einem SPI DataFlash von Atmel.

#################################################################################
# Was muss ich im Programm �ndern um CF ODER MMC/SD zu benutzen ?               #
#################################################################################

In media.h w�hlt man aus f�r welche Hardware Code erzeugt werden soll:

F�r MMC/SD
//#define COMPACTFLASH_CARD
#define MMC_CARD_SPI //SD_CARD_SPI too !

F�r CompactFlash
#define COMPACTFLASH_CARD
//#define MMC_CARD_SPI //SD_CARD_SPI too !

auf KEINEN Fall beide definieren !

Siehe aber auch die Code-Beispiele der Testprogramme um zu sehen was man in
main() f�r die jeweilige Hardware eintragen mu� !!!

Schade das man beim ATMega32 mit 16MHz das SPI Modul auf max. 8MHz takten kann.
Moderne MMC k�nnen bis zu 25MHz !
