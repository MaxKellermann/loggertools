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
import gtk, pango

import struct
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

def load_angle(x):
    assert isinstance(x, str)
    assert len(x) == 4
    x = struct.unpack('BBBB', x)
    assert(x[3] == 0 or x[3] == 1)
    value = reduce(lambda a, b: a * 60 + b, x[0:3])
    if x[3] != 0:
        value = -value
    return value

def save_angle(value):
    assert isinstance(value, int)
    assert value >= -180 * 3600
    assert value <= 180 * 3600
    if value < 0:
        sign = 1
        value = -value
    else:
        sign = 0
    return struct.pack('BBBB', value / 3600, (value / 60) % 60,
                       value % 60, sign)

class SurfacePosition:
    def __init__(self, latitude, longitude):
        assert isinstance(latitude, int)
        assert isinstance(longitude, int)
        assert latitude >= -90 * 3600
        assert latitude <= 90 * 3600
        assert longitude >= -180 * 3600
        assert longitude <= 180 * 3600

        self.latitude = latitude
        self.longitude = longitude

    def __str__(self):
        a = abs(self.latitude)
        if self.latitude < 0:
            sign = 'S'
        else:
            sign = 'N'
        latitude = '%02u.%02u.%02u %c' % (a / 3600, (a / 60) % 60,
                                    a % 60, sign)

        a = abs(self.longitude)
        if self.longitude < 0:
            sign = 'W'
        else:
            sign = 'E'
        longitude = '%03u.%02u.%02u %c' % (a / 3600, (a / 60) % 60,
                                     a % 60, sign)

        return latitude + ' ' + longitude

    def save(self):
        return save_angle(self.latitude) + save_angle(self.longitude)

def convert_angle(value):
    assert isinstance(value, int)
    assert value >= -180 * 3600
    assert value <= 180 * 3600

    import math
    return value * math.pi / (180. * 3600.)

def great_circle_distance(a, b):
    # formula from http://en.wikipedia.org/wiki/Great-circle_distance
    lat1 = convert_angle(a.latitude)
    lon1 = convert_angle(a.longitude)
    lat2 = convert_angle(b.latitude)
    lon2 = convert_angle(b.longitude)

    from math import atan2, hypot, cos, sin
    return atan2(hypot(cos(lat2) * sin(lon2 - lon1),
                       cos(lat1) * sin(lat2) -
                       sin(lat1) * cos(lat2) * cos(lon2 - lon1)),
                 (sin(lat1) * sin(lat2) +
                  cos(lat1) * cos(lat2) * cos(lon2 - lon1))) * 6372.795;

class TaskWaypoint:
    def __init__(self, name, position):
        assert isinstance(name, str) or isinstance(name, unicode)
        assert isinstance(position, SurfacePosition)

        self.name = name
        self.position = position

    def save(self):
        return dump_str(self.name, 12) + self.position.save()

class Task:
    def __init__(self):
        self.waypoints = list()

    def save(self):
        x = ''
        prefix = struct.pack('B', len(self.waypoints)) + '\0\0\0'
        for waypoint in self.waypoints:
            x += prefix
            x += waypoint.save()
        if len(x) < 480:
            x += '\0' * (480 - len(x))
        return x

class TaskListStore(gtk.ListStore):
    def __init__(self, task):
        self._task = task
        gtk.ListStore.__init__(self, str, str, str)

    def reload(self):
        self.clear()

        prev = None
        total = 0
        for i, waypoint in zip(range(len(self._task.waypoints)), self._task.waypoints):
            if i == len(self._task.waypoints) - 1:
                distance = '%u km' % total
            elif i >= 2:
                length = int(great_circle_distance(prev, waypoint.position))
                distance = '%u km' % length
                total += length
            else:
                distance = ''
            prev = waypoint.position
            self.append((waypoint.name, str(waypoint.position), distance))

        self.append((None, None, None))

    def append_waypoint(self, waypoint):
        i = len(self._task.waypoints)
        self._task.waypoints.append(waypoint)
        self.insert(i, (waypoint.name, str(waypoint.position), 'y'))

    def iter_to_index(self, iter):
        path = self.get_path(iter)
        i = path[0]
        if i == len(self._task.waypoints): return None
        return i

class TaskDialog(gtk.Dialog):
    def __init__(self, port):
        gtk.Dialog.__init__(self, u'Task declaration', None, 0,
                            ('Read from GP940', 1,
                             'Write to GP940', 2,
                             gtk.STOCK_CLOSE, gtk.RESPONSE_REJECT))

        self._port = port
        self._task = Task()

        self.set_default_response(gtk.RESPONSE_ACCEPT)
        self.connect("close", lambda dialog: dialog.response(gtk.RESPONSE_REJECT))
        self.connect("response", self.__on_response)

        self.vbox.pack_start(self.__create_toolbar(), expand = False)

        self.model = TaskListStore(self._task)
        self.model.reload()

        self.list = gtk.TreeView(model=self.model)
        self.list.set_enable_search(False)
        self.list.get_selection().connect('changed', self.__on_selection_changed)
        
        renderer = gtk.CellRendererText()
        renderer.set_property('editable', True)
        renderer.set_property('width-chars', 25)
        renderer.connect('edited', self.__on_cell_edited)
        renderer.set_property('ellipsize', pango.ELLIPSIZE_END)
        column = gtk.TreeViewColumn(u'Name', renderer, text = 0)
        self.list.append_column(column)

        renderer = gtk.CellRendererText()
        column = gtk.TreeViewColumn(u'Position', renderer, text = 1)
        self.list.append_column(column)

        renderer = gtk.CellRendererText()
        column = gtk.TreeViewColumn(u'Length', renderer, text = 2)
        self.list.append_column(column)

        self.list.show()
        self.vbox.add(self.list)

    def __create_toolbar(self):
        toolbar = gtk.Toolbar()
        toolbar.show()

        icon = gtk.image_new_from_stock(gtk.STOCK_GO_UP, gtk.ICON_SIZE_LARGE_TOOLBAR)
        icon.show()
        self.move_up_button = gtk.ToolButton(icon, 'Up')
        #self.move_up_button.connect('clicked', self.__on_move_up)
        self.move_up_button.set_sensitive(False)
        self.move_up_button.show()
        toolbar.insert(self.move_up_button, -1)

        icon = gtk.image_new_from_stock(gtk.STOCK_GO_DOWN, gtk.ICON_SIZE_LARGE_TOOLBAR)
        icon.show()
        self.move_down_button = gtk.ToolButton(icon, 'Down')
        #self.move_down_button.connect('clicked', self.__on_move_down)
        self.move_down_button.set_sensitive(False)
        self.move_down_button.show()
        toolbar.insert(self.move_down_button, -1)

        icon = gtk.image_new_from_stock(gtk.STOCK_DELETE, gtk.ICON_SIZE_LARGE_TOOLBAR)
        icon.show()
        self.delete_button = gtk.ToolButton(icon, u'Delete')
        self.delete_button.connect('clicked', self.__on_delete)
        self.delete_button.set_sensitive(False)
        self.delete_button.show()
        toolbar.insert(self.delete_button, -1)

        return toolbar

    def __on_cell_edited(self, cell, path_string, new_text):
        iter = self.model.get_iter_from_string(path_string)
        i = self.model.iter_to_index(iter)

        if i is None:
            waypoint = TaskWaypoint(new_text, SurfacePosition(0, 0))
            waypoint.name = new_text
            self.model.append_waypoint(waypoint)
        else:
            waypoint = self._task.waypoints[i]
            waypoint.name = new_text
            self.model.set_value(iter, 0, new_text)

    def __on_delete(self, widget):
        (model, iter) = self.list.get_selection().get_selected()
        i = model.iter_to_index(iter)
        if i is None: return
        del self._task.waypoints[i]
        model.remove(iter)

    def __on_selection_changed(self, selection):
        # enable or disable the edit/delete buttons
        sensitive = selection.count_selected_rows() == 1
        self.move_up_button.set_sensitive(sensitive)
        self.move_down_button.set_sensitive(sensitive)
        self.delete_button.set_sensitive(sensitive)

    def load(self, data):
        assert isinstance(data, str)
        assert len(data) == 480

        task = Task()

        num_waypoints = struct.unpack('B', data[24])[0]
        for i in range(num_waypoints):
            waypoint = data[24 + i * 24 : 24 + i * 24 + 24]
            position = SurfacePosition(load_angle(waypoint[16:20]),
                                       load_angle(waypoint[20:24]))
            task.waypoints.append(TaskWaypoint(waypoint[4:16].strip(),
                                               position))

        self._task = task
        self.model = TaskListStore(self._task)
        self.model.reload()
        self.list.set_model(self.model)

    def save(self):
        return self._task.save()

    def __on_response(self, dialog, response_id):
        if response_id == 1:
            self._port.write('\x13')
            self.load(self._port.read(480))
        elif response_id == 2:
            self._port.write('\x12' + self.save())
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
        TaskDialog(self._port_setup.open()).show()

if __name__ == '__main__':
    #main = MainWindow()
    #main.main()
    #PersonalDataDialog(serial.Serial('/tmp/fakezander')).show()
    #gtk.main()
    TaskDialog(serial.Serial('/tmp/fakezander')).show()
    gtk.main()
