// Copyright (C) 2015 Foam Kernow
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include <iostream>
#include <QtGui>
#include "qt/app.h"
#include <unistd.h>

using namespace std;

int main(int argc, char **argv) {
  srand(::time(NULL));
  red_king::app red_king;
  QApplication app(argc, argv);
  red_king.init_qt();
  return app.exec();

  while (true) {
    red_king.m_model.step();

  }

}
