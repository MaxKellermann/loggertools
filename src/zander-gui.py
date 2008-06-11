#!/usr/bin/python
#
#  loggertools
#  Copyright (C) 2004-2008 Max Kellermann <max@duempel.org>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; version 2 of the License.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

import pygtk
pygtk.require('2.0')
import gtk

import serial

from loggertools.gtk.form import TableForm

class PortSetupForm(TableForm):
    def __init__(self):
        TableForm.__init__(self, 2)
            
        self._port = gtk.Entry()
        self._port.set_text('/dev/ttyS0')
        self.attach_field(0, u'Port:', self._port)

        self._speed = gtk.combo_box_new_text()
        self._speed.append_text('9600')
        self._speed.append_text('19200')
        self._speed.append_text('38400')
        self._speed.append_text('57600')
        self._speed.set_active(0)
        self.attach_field(1, u'Speed:', self._speed)

    def open(self):
        return serial.Serial(0)

def dump_str(x, length):
    assert isinstance(x, str)
    x = x[0:length]
    if len(x) < length:
        x += ' ' * (length - len(x))
    return x

class PersonalDataDialog(gtk.Dialog):
    def __init__(self, port):
        gtk.Dialog.__init__(self, u'Personal data', None, 0,
                            ('Read from GP940', 1,
                             'Write to GP940', 2,
                             gtk.STOCK_CLOSE, gtk.RESPONSE_REJECT))

        self._port = port

        self.set_default_response(gtk.RESPONSE_ACCEPT)
        self.connect("close", lambda dialog: dialog.response(gtk.RESPONSE_REJECT))
        self.connect("response", self.__on_response)

        table = TableForm(5)

        self._pilot = gtk.Entry()
        table.attach_field(0, u'Pilot:', self._pilot)

        self._model = gtk.Entry()
        table.attach_field(1, u'Glider model:', self._model)

        self._class = gtk.Entry()
        table.attach_field(2, u'Competition class:', self._class)

        self._registration = gtk.Entry()
        table.attach_field(3, u'Registration:', self._registration)

        self._sign = gtk.Entry()
        table.attach_field(4, u'Competition sign:', self._sign)

        table.show_all()
        self.vbox.add(table)

    def load(self, data):
        assert isinstance(data, str)
        assert len(data) == 140

        self._pilot.set_text(data[0:40].strip(' \0'))
        self._model.set_text(data[40:80].strip(' \0'))
        self._class.set_text(data[80:120].strip(' \0'))
        self._registration.set_text(data[120:130].strip(' \0'))
        self._sign.set_text(data[130:140].strip(' \0'))

    def save(self):
        return dump_str(self._pilot.get_text(), 40) + \
               dump_str(self._model.get_text(), 40) + \
               dump_str(self._class.get_text(), 40) + \
               dump_str(self._registration.get_text(), 10) + \
               dump_str(self._sign.get_text(), 10)

    def __on_response(self, dialog, response_id):
        if response_id == 1:
            self._port.write('\x11')
            self.load(self._port.read(140))
        elif response_id == 2:
            self._port.write('\x10' + self.save())
        else:
            self.destroy()

class MainWindow:
    def __init__(self):
        self.window = gtk.Window(gtk.WINDOW_TOPLEVEL)
        self.window.set_title('loggertools/Zander')
        self.window.connect('delete_event', self._delete_event)
        self.window.connect('destroy', self._destroy)

        vbox = gtk.VBox(spacing = 4)
        vbox.show()
        self.window.add(vbox)

        menu_bar = gtk.MenuBar()
        vbox.pack_start(menu_bar, False, False, 2)

        menu = gtk.Menu()
        item = gtk.MenuItem(u'Quit')
        item.connect('activate', self._destroy)
        menu.append(item)

        item = gtk.MenuItem(u'File')
        item.set_submenu(menu)
        menu_bar.append(item)

        menu = gtk.Menu()
        item = gtk.MenuItem(u'Personal data')
        item.connect('activate', self._personal_data)
        menu.append(item)
        item = gtk.MenuItem(u'Task declaration')
        item.connect('activate', self._task)
        menu.append(item)
        item = gtk.MenuItem(u'Check battery')
        item.connect('activate', self._battery)
        menu.append(item)
        item = gtk.MenuItem(u'Basic information')
        item.connect('activate', self._info)
        menu.append(item)

        item = gtk.MenuItem(u'GP940')
        item.set_submenu(menu)
        menu_bar.append(item)

        menu = gtk.Menu()
        item = gtk.MenuItem(u'SR940')
        item.set_submenu(menu)
        menu_bar.append(item)

        menu_bar.show_all()

        self._port_setup = PortSetupForm()
        self._port_setup.show_all()
        vbox.add(self._port_setup)

        self.window.show()

    def main(self):
        gtk.main()

    def _delete_event(self, widget, event, data=None):
        return False

    def _destroy(self, widget, data=None):
        gtk.main_quit()

    def _info(self, widget):
        pass

    def _battery(self, widget):
        pass

    def _personal_data(self, widget):
        PersonalDataDialog(self._port_setup.open()).show()

    def _task(self, widget):
        pass

if __name__ == '__main__':
    main = MainWindow()
    main.main()
    #PersonalDataDialog(serial.Serial('/tmp/fakezander')).show()
    #gtk.main()
