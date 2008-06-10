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

import gtk

class TableForm(gtk.Table):
    def __init__(self, num_rows):
        gtk.Table.__init__(self, num_rows, 2)
        self.set_border_width(5)
        self.set_row_spacings(5)
        self.set_col_spacings(10)
        self._size_group = gtk.SizeGroup(gtk.SIZE_GROUP_HORIZONTAL)

    def attach_field(self, row, label, field):
        label = gtk.Label(label)
        label.set_use_underline(True)
        label.set_alignment(0, 1)
        label.set_mnemonic_widget(field)

        self.attach(label, 0, 1, row, row + 1, 0, 0, 0, 0)
        self._size_group.add_widget(field)
        self.attach(field, 1, 2, row, row + 1, gtk.EXPAND | gtk.FILL, 0, 0, 0)
