loggertools fuer Windows README.txt
===================================

$Id$


Was ist Cenfis?
---------------

Cenfis ist ein exzellentes GPS-gestuetztes Navigationsgeraet fuer
Segelflugzeuge.

 http://www.holltronic.de/


Was ist cenfistool?
-------------------

cenfistool ist ein Programm, mit dem sich ein Firmware-Update beim
Cenfis machen laesst. Es laeuft auf einem Windows-Computer, und
kommuniziert ueber ein serielles Kabel mit dem Cenfis, und stellt eine
Alternative zum Programm "XTS" von Holltronic dar.

Hubert Oberhollenzer (der Entwickler von Cenfis), war so nett und hat
mir den Quellcode von XTS zur Verfuegung gestellt. Dort konnte ich das
Protokoll entnehmen.


Wo bekommt man cenfistool?
--------------------------

cenfistool soll Bestandteil von "loggertools" werden, einer Sammlung
von Open-Source-Programmen zur Steuerung von Loggern und
GPS-Navigationsgeraeten. Das Projekt ist unter der folgenden URL
erreichbar:

 http://max.kellermann.name/projects/loggertools/


Benutzung
---------

Das Protokoll hat sich im Laufe der Zeit leicht veraendert, daher
sollte man als erstes die Software updaten (aktueller Stand:
3.7c). Diese bekommt man auf der Holltronic-Homepage. Anschliessend
kann man Waypoints und Airspaces updaten.

1. cenfistool.exe starten, COM-Port auswaehlen
2. die gewuenschte .BHF-Datei auf das cenfistool-Fenster ziehen; der
   Dateiname erscheint nun im Textfeld
3. "Senden" anklicken
4. das Cenfis-Geraet einschalten, waehrenddessen am Cenfis-Geraet die
   dunkelgruene und die rote Taste gleichzeitig gedrueckt halten
5. mit der schwarzen Taste mehrmals bestaetigen (siehe Display am
   Cenfis-Geraet)
6. cenfistool laedt jetzt die Datei in das Cenfis-Geraet

Zum Hochladen der Waypoints und Airspaces geht man genauso vor, nur
bei Punkt 4 schaltet man stattdessen das Geraet normal ein und geht
ueber den roten Knopf in das Update-Menue, Rest siehe Cenfis-Handbuch.


Rechtliches
-----------

Copyright 2004-2005 Max Kellermann (max@duempel.org)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
