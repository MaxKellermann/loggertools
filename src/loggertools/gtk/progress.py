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
from threading import Thread

class Cancel(Exception):
    pass

class ProgressThread(Thread):
    def __init__(self, action, dialog):
        Thread.__init__(self)
        self.setDaemon(False)
        self.__cancelled = False
        self.__action = action
        self.__dialog = dialog
        dialog.connect("response", self.__on_response)

    def __check_cancel(self):
        if self.__cancelled:
            # this exception is caught by run()
            raise Cancel
        return False

    def __on_response(self, dialog, response_id):
        self.__cancelled = True
        self.join()

    def run(self):
        try:
            try:
                self.__action(self.__check_cancel)
            except Cancel:
                # the user has clicked on the "cancel" button
                pass
        finally:
            self.__dialog.destroy()

def progress_do(title, action):
    dialog = gtk.Dialog(title, None, 0,
                        (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL))
    dialog.connect("close", lambda dialog: dialog.response(gtk.RESPONSE_CANCEL))
    thread = ProgressThread(action, dialog)
    thread.start()
    dialog.run()
